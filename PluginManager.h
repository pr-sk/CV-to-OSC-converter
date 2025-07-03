#pragma once

#include <memory>
#include <vector>
#include <string>
#include <map>
#include <functional>
#include <thread>
#include <atomic>
#include <mutex>
#include <chrono>
#include <filesystem>
#include <dlfcn.h>

// Plugin API version
#define PLUGIN_API_VERSION 1

/**
 * @brief Plugin types
 */
enum class PluginType {
    SIGNAL_PROCESSOR,
    CV_MAPPER,
    DEVICE_DRIVER,
    GUI_EXTENSION,
    UNKNOWN
};

/**
 * @brief Plugin information structure
 */
struct PluginInfo {
    std::string name;
    std::string version;
    std::string author;
    std::string description;
    PluginType type;
    int apiVersion;
    std::string filename;
    bool enabled;
    
    PluginInfo() : type(PluginType::UNKNOWN), apiVersion(0), enabled(false) {}
};

/**
 * @brief Base plugin interface
 */
class IPlugin {
public:
    virtual ~IPlugin() = default;
    
    // Plugin lifecycle
    virtual bool initialize() = 0;
    virtual void shutdown() = 0;
    virtual PluginInfo getInfo() const = 0;
    
    // Configuration
    virtual bool configure(const std::map<std::string, std::string>& config) = 0;
    virtual std::map<std::string, std::string> getConfiguration() const = 0;
    
    // Status
    virtual bool isEnabled() const = 0;
    virtual void setEnabled(bool enabled) = 0;
    virtual std::string getLastError() const = 0;
};

/**
 * @brief Signal processor plugin interface
 */
class ISignalProcessor : public IPlugin {
public:
    virtual ~ISignalProcessor() = default;
    
    // Signal processing
    virtual std::vector<float> processSignal(const std::vector<float>& input) = 0;
    virtual bool processSignalInPlace(std::vector<float>& signal) = 0;
    
    // Real-time processing
    virtual float processSample(float sample) = 0;
    virtual void processSamples(float* samples, int count) = 0;
    
    // Parameters
    virtual void setParameter(const std::string& name, float value) = 0;
    virtual float getParameter(const std::string& name) const = 0;
    virtual std::vector<std::string> getParameterNames() const = 0;
    
    // Presets
    virtual bool loadPreset(const std::string& presetName) = 0;
    virtual bool savePreset(const std::string& presetName) = 0;
    virtual std::vector<std::string> getAvailablePresets() const = 0;
};

/**
 * @brief CV mapper plugin interface
 */
class ICVMapper : public IPlugin {
public:
    virtual ~ICVMapper() = default;
    
    // Mapping functions
    virtual float mapCV(float inputCV, int channel) = 0;
    virtual std::vector<float> mapCVBatch(const std::vector<float>& inputCV) = 0;
    
    // Mapping configuration
    virtual bool setMappingFunction(int channel, const std::string& function) = 0;
    virtual std::string getMappingFunction(int channel) const = 0;
    
    // Range configuration
    virtual void setInputRange(int channel, float min, float max) = 0;
    virtual void setOutputRange(int channel, float min, float max) = 0;
    virtual std::pair<float, float> getInputRange(int channel) const = 0;
    virtual std::pair<float, float> getOutputRange(int channel) const = 0;
    
    // Calibration
    virtual bool calibrateChannel(int channel, const std::vector<std::pair<float, float>>& points) = 0;
    virtual void resetCalibration(int channel) = 0;
};

/**
 * @brief Plugin manager class
 */
class PluginManager {
public:
    PluginManager();
    ~PluginManager();
    
    // Plugin discovery and loading
    bool scanPluginDirectory(const std::string& directory);
    bool loadPlugin(const std::string& filename);
    bool unloadPlugin(const std::string& pluginName);
    void unloadAllPlugins();
    
    // Plugin management
    std::vector<PluginInfo> getAvailablePlugins() const;
    std::vector<PluginInfo> getLoadedPlugins() const;
    std::vector<PluginInfo> getEnabledPlugins() const;
    
    bool enablePlugin(const std::string& pluginName);
    bool disablePlugin(const std::string& pluginName);
    bool isPluginLoaded(const std::string& pluginName) const;
    bool isPluginEnabled(const std::string& pluginName) const;
    
    // Plugin access
    IPlugin* getPlugin(const std::string& pluginName);
    ISignalProcessor* getSignalProcessor(const std::string& pluginName);
    ICVMapper* getCVMapper(const std::string& pluginName);
    
    // Plugin configuration
    bool configurePlugin(const std::string& pluginName, const std::map<std::string, std::string>& config);
    std::map<std::string, std::string> getPluginConfiguration(const std::string& pluginName) const;
    
    // Hot loading
    void enableHotLoading(bool enable);
    bool isHotLoadingEnabled() const { return hotLoadingEnabled_; }
    void checkForUpdates();
    
    // Plugin chain for signal processing
    bool addToProcessingChain(const std::string& pluginName, int position = -1);
    bool removeFromProcessingChain(const std::string& pluginName);
    std::vector<std::string> getProcessingChain() const;
    bool reorderProcessingChain(const std::vector<std::string>& newOrder);
    
    // Process signal through plugin chain
    std::vector<float> processSignalChain(const std::vector<float>& input);
    void processSignalChainInPlace(std::vector<float>& signal);
    
    // Error handling
    std::string getLastError() const { return lastError_; }
    
    // Plugin validation
    bool validatePlugin(const std::string& filename) const;
    bool isAPICompatible(int pluginApiVersion) const;
    
private:
    struct LoadedPlugin {
        void* handle;
        std::unique_ptr<IPlugin> plugin;
        PluginInfo info;
        std::map<std::string, std::string> config;
        bool enabled;
        std::filesystem::file_time_type lastModified;
    };
    
    std::map<std::string, LoadedPlugin> loadedPlugins_;
    std::vector<std::string> processingChain_;
    std::vector<std::string> pluginDirectories_;
    
    bool hotLoadingEnabled_;
    std::thread hotLoadingThread_;
    std::atomic<bool> running_;
    
    std::string lastError_;
    mutable std::mutex pluginsMutex_;
    
    // Internal methods
    void hotLoadingLoop();
    bool loadPluginFromFile(const std::string& filename, LoadedPlugin& plugin);
    void unloadPluginHandle(LoadedPlugin& plugin);
    std::filesystem::file_time_type getFileModificationTime(const std::string& filename);
    
    // Plugin validation helpers
    bool validatePluginSymbols(void* handle) const;
    int getPluginAPIVersion(void* handle) const;
    
    // Plugin factory functions (to be implemented by plugins)
    typedef IPlugin* (*CreatePluginFunc)();
    typedef void (*DestroyPluginFunc)(IPlugin*);
    typedef int (*GetAPIVersionFunc)();
    typedef PluginInfo (*GetPluginInfoFunc)();
};

// Plugin factory macros for plugin developers
#define DECLARE_PLUGIN(ClassName) \
    extern "C" { \
        IPlugin* createPlugin() { return new ClassName(); } \
        void destroyPlugin(IPlugin* plugin) { delete plugin; } \
        int getAPIVersion() { return PLUGIN_API_VERSION; } \
        PluginInfo getPluginInfo() { \
            ClassName temp; \
            return temp.getInfo(); \
        } \
    }

#define PLUGIN_EXPORT extern "C"
