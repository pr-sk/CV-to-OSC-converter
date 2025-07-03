#include "PluginManager.h"
#include <iostream>
#include <filesystem>
#include <algorithm>

PluginManager::PluginManager() 
    : hotLoadingEnabled_(false), running_(false) {
}

PluginManager::~PluginManager() {
    unloadAllPlugins();
    
    if (hotLoadingThread_.joinable()) {
        running_ = false;
        hotLoadingThread_.join();
    }
}

bool PluginManager::scanPluginDirectory(const std::string& directory) {
    if (!std::filesystem::exists(directory)) {
        lastError_ = "Plugin directory does not exist: " + directory;
        return false;
    }
    
    // Add to plugin directories if not already present
    if (std::find(pluginDirectories_.begin(), pluginDirectories_.end(), directory) == pluginDirectories_.end()) {
        pluginDirectories_.push_back(directory);
    }
    
    bool foundAny = false;
    
    try {
        for (const auto& entry : std::filesystem::directory_iterator(directory)) {
            if (entry.is_regular_file()) {
                std::string filename = entry.path().string();
                
                // Check for shared library extensions
                if (filename.size() > 6 && 
                   (filename.substr(filename.size() - 6) == ".dylib" ||
                    filename.substr(filename.size() - 3) == ".so" ||
                    filename.substr(filename.size() - 4) == ".dll")) {
                    if (validatePlugin(filename)) {
                        std::cout << "Found valid plugin: " << filename << std::endl;
                        foundAny = true;
                    }
                }
            }
        }
    } catch (const std::exception& e) {
        lastError_ = "Error scanning plugin directory: " + std::string(e.what());
        return false;
    }
    
    return foundAny;
}

bool PluginManager::loadPlugin(const std::string& filename) {
    std::lock_guard<std::mutex> lock(pluginsMutex_);
    
    if (!std::filesystem::exists(filename)) {
        lastError_ = "Plugin file does not exist: " + filename;
        return false;
    }
    
    LoadedPlugin plugin;
    if (!loadPluginFromFile(filename, plugin)) {
        return false;
    }
    
    // Check if plugin with same name is already loaded
    if (loadedPlugins_.find(plugin.info.name) != loadedPlugins_.end()) {
        lastError_ = "Plugin with name '" + plugin.info.name + "' is already loaded";
        unloadPluginHandle(plugin);
        return false;
    }
    
    loadedPlugins_[plugin.info.name] = std::move(plugin);
    std::cout << "Plugin loaded successfully: " << plugin.info.name << std::endl;
    
    return true;
}

bool PluginManager::unloadPlugin(const std::string& pluginName) {
    std::lock_guard<std::mutex> lock(pluginsMutex_);
    
    auto it = loadedPlugins_.find(pluginName);
    if (it == loadedPlugins_.end()) {
        return true; // Not loaded
    }
    
    // Remove from processing chain if present
    auto chainIt = std::find(processingChain_.begin(), processingChain_.end(), pluginName);
    if (chainIt != processingChain_.end()) {
        processingChain_.erase(chainIt);
    }
    
    // Shutdown and unload plugin
    if (it->second.plugin) {
        it->second.plugin->shutdown();
    }
    
    unloadPluginHandle(it->second);
    loadedPlugins_.erase(it);
    
    std::cout << "Plugin unloaded: " << pluginName << std::endl;
    return true;
}

void PluginManager::unloadAllPlugins() {
    std::lock_guard<std::mutex> lock(pluginsMutex_);
    
    for (auto& [name, plugin] : loadedPlugins_) {
        if (plugin.plugin) {
            plugin.plugin->shutdown();
        }
        unloadPluginHandle(plugin);
    }
    
    loadedPlugins_.clear();
    processingChain_.clear();
    
    std::cout << "All plugins unloaded" << std::endl;
}

std::vector<PluginInfo> PluginManager::getLoadedPlugins() const {
    std::lock_guard<std::mutex> lock(pluginsMutex_);
    
    std::vector<PluginInfo> plugins;
    plugins.reserve(loadedPlugins_.size());
    
    for (const auto& [name, plugin] : loadedPlugins_) {
        plugins.push_back(plugin.info);
    }
    
    return plugins;
}

std::vector<PluginInfo> PluginManager::getEnabledPlugins() const {
    std::lock_guard<std::mutex> lock(pluginsMutex_);
    
    std::vector<PluginInfo> plugins;
    
    for (const auto& [name, plugin] : loadedPlugins_) {
        if (plugin.enabled) {
            plugins.push_back(plugin.info);
        }
    }
    
    return plugins;
}

bool PluginManager::enablePlugin(const std::string& pluginName) {
    std::lock_guard<std::mutex> lock(pluginsMutex_);
    
    auto it = loadedPlugins_.find(pluginName);
    if (it == loadedPlugins_.end()) {
        lastError_ = "Plugin not loaded: " + pluginName;
        return false;
    }
    
    if (!it->second.plugin) {
        lastError_ = "Plugin instance is null: " + pluginName;
        return false;
    }
    
    if (!it->second.enabled) {
        if (it->second.plugin->initialize()) {
            it->second.enabled = true;
            it->second.plugin->setEnabled(true);
            it->second.info.enabled = true;
            
            std::cout << "Plugin enabled: " << pluginName << std::endl;
            return true;
        } else {
            lastError_ = "Failed to initialize plugin: " + it->second.plugin->getLastError();
            return false;
        }
    }
    
    return true; // Already enabled
}

bool PluginManager::disablePlugin(const std::string& pluginName) {
    std::lock_guard<std::mutex> lock(pluginsMutex_);
    
    auto it = loadedPlugins_.find(pluginName);
    if (it == loadedPlugins_.end()) {
        return true; // Not loaded
    }
    
    if (it->second.enabled && it->second.plugin) {
        it->second.plugin->setEnabled(false);
        it->second.enabled = false;
        it->second.info.enabled = false;
        
        // Remove from processing chain
        auto chainIt = std::find(processingChain_.begin(), processingChain_.end(), pluginName);
        if (chainIt != processingChain_.end()) {
            processingChain_.erase(chainIt);
        }
        
        std::cout << "Plugin disabled: " << pluginName << std::endl;
    }
    
    return true;
}

bool PluginManager::isPluginLoaded(const std::string& pluginName) const {
    std::lock_guard<std::mutex> lock(pluginsMutex_);
    return loadedPlugins_.find(pluginName) != loadedPlugins_.end();
}

bool PluginManager::isPluginEnabled(const std::string& pluginName) const {
    std::lock_guard<std::mutex> lock(pluginsMutex_);
    
    auto it = loadedPlugins_.find(pluginName);
    return it != loadedPlugins_.end() && it->second.enabled;
}

IPlugin* PluginManager::getPlugin(const std::string& pluginName) {
    std::lock_guard<std::mutex> lock(pluginsMutex_);
    
    auto it = loadedPlugins_.find(pluginName);
    if (it != loadedPlugins_.end() && it->second.enabled) {
        return it->second.plugin.get();
    }
    
    return nullptr;
}

ISignalProcessor* PluginManager::getSignalProcessor(const std::string& pluginName) {
    IPlugin* plugin = getPlugin(pluginName);
    return dynamic_cast<ISignalProcessor*>(plugin);
}

ICVMapper* PluginManager::getCVMapper(const std::string& pluginName) {
    IPlugin* plugin = getPlugin(pluginName);
    return dynamic_cast<ICVMapper*>(plugin);
}

bool PluginManager::addToProcessingChain(const std::string& pluginName, int position) {
    std::lock_guard<std::mutex> lock(pluginsMutex_);
    
    // Check if plugin is loaded and enabled
    auto it = loadedPlugins_.find(pluginName);
    if (it == loadedPlugins_.end() || !it->second.enabled) {
        lastError_ = "Plugin not loaded or not enabled: " + pluginName;
        return false;
    }
    
    // Check if it's a signal processor
    ISignalProcessor* processor = dynamic_cast<ISignalProcessor*>(it->second.plugin.get());
    if (!processor) {
        lastError_ = "Plugin is not a signal processor: " + pluginName;
        return false;
    }
    
    // Remove if already in chain
    auto chainIt = std::find(processingChain_.begin(), processingChain_.end(), pluginName);
    if (chainIt != processingChain_.end()) {
        processingChain_.erase(chainIt);
    }
    
    // Add at specified position
    if (position < 0 || position >= static_cast<int>(processingChain_.size())) {
        processingChain_.push_back(pluginName);
    } else {
        processingChain_.insert(processingChain_.begin() + position, pluginName);
    }
    
    std::cout << "Plugin added to processing chain: " << pluginName << std::endl;
    return true;
}

bool PluginManager::removeFromProcessingChain(const std::string& pluginName) {
    std::lock_guard<std::mutex> lock(pluginsMutex_);
    
    auto it = std::find(processingChain_.begin(), processingChain_.end(), pluginName);
    if (it != processingChain_.end()) {
        processingChain_.erase(it);
        std::cout << "Plugin removed from processing chain: " << pluginName << std::endl;
        return true;
    }
    
    return false;
}

std::vector<std::string> PluginManager::getProcessingChain() const {
    std::lock_guard<std::mutex> lock(pluginsMutex_);
    return processingChain_;
}

std::vector<float> PluginManager::processSignalChain(const std::vector<float>& input) {
    std::lock_guard<std::mutex> lock(pluginsMutex_);
    
    std::vector<float> output = input;
    
    for (const std::string& pluginName : processingChain_) {
        auto it = loadedPlugins_.find(pluginName);
        if (it != loadedPlugins_.end() && it->second.enabled) {
            ISignalProcessor* processor = dynamic_cast<ISignalProcessor*>(it->second.plugin.get());
            if (processor) {
                output = processor->processSignal(output);
            }
        }
    }
    
    return output;
}

void PluginManager::processSignalChainInPlace(std::vector<float>& signal) {
    std::lock_guard<std::mutex> lock(pluginsMutex_);
    
    for (const std::string& pluginName : processingChain_) {
        auto it = loadedPlugins_.find(pluginName);
        if (it != loadedPlugins_.end() && it->second.enabled) {
            ISignalProcessor* processor = dynamic_cast<ISignalProcessor*>(it->second.plugin.get());
            if (processor) {
                processor->processSignalInPlace(signal);
            }
        }
    }
}

void PluginManager::enableHotLoading(bool enable) {
    hotLoadingEnabled_ = enable;
    
    if (enable && !hotLoadingThread_.joinable()) {
        running_ = true;
        hotLoadingThread_ = std::thread(&PluginManager::hotLoadingLoop, this);
    } else if (!enable && hotLoadingThread_.joinable()) {
        running_ = false;
        hotLoadingThread_.join();
    }
}

bool PluginManager::validatePlugin(const std::string& filename) const {
    void* handle = dlopen(filename.c_str(), RTLD_LAZY);
    if (!handle) {
        return false;
    }
    
    bool valid = validatePluginSymbols(handle);
    
    if (valid) {
        int apiVersion = getPluginAPIVersion(handle);
        valid = isAPICompatible(apiVersion);
    }
    
    dlclose(handle);
    return valid;
}

bool PluginManager::isAPICompatible(int pluginApiVersion) const {
    return pluginApiVersion == PLUGIN_API_VERSION;
}

// Private methods
bool PluginManager::loadPluginFromFile(const std::string& filename, LoadedPlugin& plugin) {
    plugin.handle = dlopen(filename.c_str(), RTLD_LAZY);
    if (!plugin.handle) {
        lastError_ = "Failed to load plugin library: " + std::string(dlerror());
        return false;
    }
    
    // Validate plugin symbols
    if (!validatePluginSymbols(plugin.handle)) {
        lastError_ = "Plugin missing required symbols";
        dlclose(plugin.handle);
        plugin.handle = nullptr;
        return false;
    }
    
    // Check API compatibility
    int apiVersion = getPluginAPIVersion(plugin.handle);
    if (!isAPICompatible(apiVersion)) {
        lastError_ = "Plugin API version incompatible. Expected: " + 
                    std::to_string(PLUGIN_API_VERSION) + ", Got: " + std::to_string(apiVersion);
        dlclose(plugin.handle);
        plugin.handle = nullptr;
        return false;
    }
    
    // Get plugin info
    GetPluginInfoFunc getPluginInfoFunc = (GetPluginInfoFunc)dlsym(plugin.handle, "getPluginInfo");
    if (getPluginInfoFunc) {
        plugin.info = getPluginInfoFunc();
        plugin.info.filename = filename;
    }
    
    // Create plugin instance
    CreatePluginFunc createPluginFunc = (CreatePluginFunc)dlsym(plugin.handle, "createPlugin");
    if (createPluginFunc) {
        plugin.plugin.reset(createPluginFunc());
        if (plugin.plugin) {
            plugin.enabled = false;
            plugin.lastModified = getFileModificationTime(filename);
            return true;
        }
    }
    
    lastError_ = "Failed to create plugin instance";
    dlclose(plugin.handle);
    plugin.handle = nullptr;
    return false;
}

void PluginManager::unloadPluginHandle(LoadedPlugin& plugin) {
    if (plugin.plugin && plugin.handle) {
        DestroyPluginFunc destroyPluginFunc = (DestroyPluginFunc)dlsym(plugin.handle, "destroyPlugin");
        if (destroyPluginFunc) {
            destroyPluginFunc(plugin.plugin.release());
        } else {
            plugin.plugin.reset();
        }
    }
    
    if (plugin.handle) {
        dlclose(plugin.handle);
        plugin.handle = nullptr;
    }
}

bool PluginManager::validatePluginSymbols(void* handle) const {
    // Check for required symbols
    if (!dlsym(handle, "createPlugin")) return false;
    if (!dlsym(handle, "destroyPlugin")) return false;
    if (!dlsym(handle, "getAPIVersion")) return false;
    if (!dlsym(handle, "getPluginInfo")) return false;
    
    return true;
}

int PluginManager::getPluginAPIVersion(void* handle) const {
    GetAPIVersionFunc getAPIVersionFunc = (GetAPIVersionFunc)dlsym(handle, "getAPIVersion");
    if (getAPIVersionFunc) {
        return getAPIVersionFunc();
    }
    return 0;
}

std::filesystem::file_time_type PluginManager::getFileModificationTime(const std::string& filename) {
    try {
        return std::filesystem::last_write_time(filename);
    } catch (const std::exception&) {
        return std::filesystem::file_time_type{};
    }
}

void PluginManager::hotLoadingLoop() {
    while (running_) {
        try {
            checkForUpdates();
        } catch (const std::exception& e) {
            std::cerr << "Error in hot loading loop: " << e.what() << std::endl;
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(1000)); // Check every second
    }
}

void PluginManager::checkForUpdates() {
    std::lock_guard<std::mutex> lock(pluginsMutex_);
    
    for (auto& [name, plugin] : loadedPlugins_) {
        try {
            auto currentTime = getFileModificationTime(plugin.info.filename);
            if (currentTime > plugin.lastModified) {
                std::cout << "Plugin file updated, reloading: " << name << std::endl;
                
                // Save current state
                bool wasEnabled = plugin.enabled;
                auto config = plugin.config;
                
                // Unload current plugin
                if (plugin.plugin) {
                    plugin.plugin->shutdown();
                }
                unloadPluginHandle(plugin);
                
                // Reload plugin
                if (loadPluginFromFile(plugin.info.filename, plugin)) {
                    plugin.config = config;
                    
                    if (wasEnabled) {
                        if (plugin.plugin->initialize()) {
                            plugin.plugin->configure(config);
                            plugin.plugin->setEnabled(true);
                            plugin.enabled = true;
                        }
                    }
                    
                    std::cout << "Plugin reloaded successfully: " << name << std::endl;
                } else {
                    std::cerr << "Failed to reload plugin: " << name << std::endl;
                }
            }
        } catch (const std::exception& e) {
            std::cerr << "Error checking plugin for updates: " << name << " - " << e.what() << std::endl;
        }
    }
}
