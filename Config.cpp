#include "Config.h"
#include "ErrorHandler.h"

Config::Config() {
    // Create a default profile
    profiles["default"] = ConfigProfile();
}

bool Config::loadFromFile(const std::string& filename) {
    try {
        std::ifstream file(filename);
        if (!file.is_open()) {
            ErrorHandler::getInstance().logInfo("Config file not found, creating default", filename);
            return saveToFile(filename);
        }
        
        nlohmann::json j;
        file >> j;
        
        activeProfileName = j.value("active_profile", "default");
        
        if (j.contains("profiles") && j["profiles"].is_object()) {
            profiles.clear();
            for (auto it = j["profiles"].begin(); it != j["profiles"].end(); ++it) {
                ConfigProfile profile;
                const auto& profileJson = it.value();
                
                profile.oscHost = profileJson.value("osc_host", "127.0.0.1");
                profile.oscPort = profileJson.value("osc_port", "9000");
                profile.audioDevice = profileJson.value("audio_device", "");
                profile.updateIntervalMs = profileJson.value("update_interval_ms", 10);
                
                if (profileJson.contains("cv_ranges") && profileJson["cv_ranges"].is_array()) {
                    profile.cvRanges.clear();
                    for (const auto& range : profileJson["cv_ranges"]) {
                        profile.cvRanges.emplace_back(range.value("min", 0.0f), range.value("max", 10.0f));
                    }
                }
                profiles[it.key()] = profile;
            }
        }
        
        ErrorHandler::getInstance().logInfo("Configuration loaded", filename);
        return true;
        
    } catch (const std::exception& e) {
        ERROR_ERROR("Failed to load configuration", e.what(), 
                   "Check file format and permissions. Using default config.", true);
        profiles.clear();
        profiles["default"] = ConfigProfile();
        activeProfileName = "default";
        return false;
    }
}

bool Config::saveToFile(const std::string& filename) {
    try {
        nlohmann::json j;
        j["active_profile"] = activeProfileName;
        j["profiles"] = nlohmann::json::object();
        
        for (const auto& [name, profile] : profiles) {
            nlohmann::json profileJson;
            profileJson["osc_host"] = profile.oscHost;
            profileJson["osc_port"] = profile.oscPort;
            profileJson["audio_device"] = profile.audioDevice;
            profileJson["update_interval_ms"] = profile.updateIntervalMs;
            
            profileJson["cv_ranges"] = nlohmann::json::array();
            for (const auto& range : profile.cvRanges) {
                nlohmann::json rangeJson;
                rangeJson["min"] = range.min;
                rangeJson["max"] = range.max;
                profileJson["cv_ranges"].push_back(rangeJson);
            }
            j["profiles"][name] = profileJson;
        }
        
        std::ofstream file(filename);
        file << j.dump(4);
        
        ErrorHandler::getInstance().logInfo("Configuration saved", filename);
        return true;
        
    } catch (const std::exception& e) {
        ERROR_ERROR("Failed to save configuration", e.what(), 
                   "Check file permissions and disk space.", true);
        return false;
    }
}

bool Config::setActiveProfile(const std::string& name) {
    if (profiles.count(name)) {
        activeProfileName = name;
        return true;
    }
    return false;
}

const ConfigProfile& Config::getActiveProfile() const {
    return profiles.at(activeProfileName);
}

ConfigProfile& Config::getActiveProfile() {
    return profiles.at(activeProfileName);
}

std::vector<std::string> Config::getProfileNames() const {
    std::vector<std::string> names;
    for (const auto& [name, profile] : profiles) {
        names.push_back(name);
    }
    return names;
}

void Config::createProfile(const std::string& name, const ConfigProfile& profile) {
    profiles[name] = profile;
}

void Config::deleteProfile(const std::string& name) {
    if (name != "default") {
        profiles.erase(name);
        if (activeProfileName == name) {
            activeProfileName = "default";
        }
    }
}

CVRange Config::getCVRange(int channel) const {
    const auto& ranges = getActiveProfile().cvRanges;
    if (channel >= 0 && static_cast<size_t>(channel) < ranges.size()) {
        return ranges[channel];
    }
    return CVRange(0.0f, 10.0f); // Default range
}

void Config::setOSCHost(const std::string& host) {
    getActiveProfile().oscHost = host;
}

void Config::setOSCPort(const std::string& port) {
    getActiveProfile().oscPort = port;
}

void Config::setAudioDevice(const std::string& device) {
    getActiveProfile().audioDevice = device;
}

void Config::setUpdateIntervalMs(int interval) {
    getActiveProfile().updateIntervalMs = interval;
}

void Config::setCVRange(int channel, float min, float max) {
    auto& ranges = getActiveProfile().cvRanges;
    if (channel >= 0) {
        if (static_cast<size_t>(channel) >= ranges.size()) {
            ranges.resize(static_cast<size_t>(channel) + 1, CVRange(0.0f, 10.0f));
        }
        ranges[static_cast<size_t>(channel)] = CVRange(min, max);
    }
}

void Config::printConfiguration() const {
    const auto& profile = getActiveProfile();
    std::cout << "\nCurrent Configuration (Profile: " << activeProfileName << "):" << std::endl;
    std::cout << "  OSC Target: " << profile.oscHost << ":" << profile.oscPort << std::endl;
    std::cout << "  Audio Device: " << (profile.audioDevice.empty() ? "default" : profile.audioDevice) << std::endl;
    std::cout << "  Update Rate: " << (1000 / profile.updateIntervalMs) << " Hz" << std::endl;
    std::cout << "  CV Ranges:" << std::endl;
    
    for (size_t i = 0; i < profile.cvRanges.size(); ++i) {
        std::cout << "    Channel " << (i + 1) << ": " 
                  << profile.cvRanges[i].min << "V to " << profile.cvRanges[i].max << "V" << std::endl;
    }
    std::cout << std::endl;
}
