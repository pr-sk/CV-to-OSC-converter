#pragma once

#include <string>
#include <vector>
#include <fstream>
#include <iostream>
#include <map>
#include <nlohmann/json.hpp>
#include "Localization.h"

struct CVRange {
    float min;
    float max;
    
    CVRange(float min_val = 0.0f, float max_val = 5.0f) : min(min_val), max(max_val) {}
};

struct ConfigProfile {
    std::string oscHost = "127.0.0.1";
    std::string oscPort = "9000";
    std::string audioDevice = "";
    int updateIntervalMs = 10;
    std::vector<CVRange> cvRanges;
    Language language = Language::English;
    
    ConfigProfile() {
        cvRanges.resize(8, CVRange(0.0f, 10.0f)); // Default CV ranges
    }
};

class Config {
private:
    std::map<std::string, ConfigProfile> profiles;
    std::string activeProfileName = "default";

public:
    Config();
    
    bool loadFromFile(const std::string& filename);
    bool saveToFile(const std::string& filename);
    
    // Profile management
    bool setActiveProfile(const std::string& name);
    const std::string& getActiveProfileName() const { return activeProfileName; }
    const ConfigProfile& getActiveProfile() const;
    ConfigProfile& getActiveProfile();
    std::vector<std::string> getProfileNames() const;
    void createProfile(const std::string& name, const ConfigProfile& profile);
    void deleteProfile(const std::string& name);
    
    // Getters for active profile
    const std::string& getOSCHost() const { return getActiveProfile().oscHost; }
    const std::string& getOSCPort() const { return getActiveProfile().oscPort; }
    const std::string& getAudioDevice() const { return getActiveProfile().audioDevice; }
    int getUpdateIntervalMs() const { return getActiveProfile().updateIntervalMs; }
    Language getLanguage() const { return getActiveProfile().language; }
    CVRange getCVRange(int channel) const;
    
    // Setters for active profile
    void setOSCHost(const std::string& host);
    void setOSCPort(const std::string& port);
    void setAudioDevice(const std::string& device);
    void setUpdateIntervalMs(int interval);
    void setLanguage(Language lang);
    void setCVRange(int channel, float min, float max);
    
    void printConfiguration() const;
};
