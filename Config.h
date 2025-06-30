#pragma once

#include <string>
#include <vector>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>

struct CVRange {
    float min;
    float max;
    
    CVRange(float min = 0.0f, float max = 5.0f) : min(min), max(max) {}
};

class Config {
private:
    std::string oscHost;
    std::string oscPort;
    std::string audioDevice;
    int updateIntervalMs;
    std::vector<CVRange> cvRanges;
    
public:
    Config() {
        // Default configuration
        oscHost = "127.0.0.1";
        oscPort = "9000";
        audioDevice = "";  // Use default device
        updateIntervalMs = 10;  // 100 Hz update rate
        
        // Default CV ranges for 8 channels (typical eurorack: 0-10V)
        cvRanges.resize(8, CVRange(0.0f, 10.0f));
    }
    
    bool loadFromFile(const std::string& filename) {
        try {
            std::ifstream file(filename);
            if (!file.is_open()) {
                std::cout << "Config file '" << filename << "' not found, using defaults" << std::endl;
                saveToFile(filename);  // Create default config file
                return true;
            }
            
            nlohmann::json j;
            file >> j;
            
            if (j.contains("osc_host")) {
                oscHost = j["osc_host"];
            }
            
            if (j.contains("osc_port")) {
                oscPort = j["osc_port"];
            }
            
            if (j.contains("audio_device")) {
                audioDevice = j["audio_device"];
            }
            
            if (j.contains("update_interval_ms")) {
                updateIntervalMs = j["update_interval_ms"];
            }
            
            if (j.contains("cv_ranges") && j["cv_ranges"].is_array()) {
                cvRanges.clear();
                for (const auto& range : j["cv_ranges"]) {
                    float min = range.contains("min") ? range["min"].get<float>() : 0.0f;
                    float max = range.contains("max") ? range["max"].get<float>() : 10.0f;
                    cvRanges.emplace_back(min, max);
                }
            }
            
            std::cout << "Configuration loaded from " << filename << std::endl;
            return true;
            
        } catch (const std::exception& e) {
            std::cerr << "Error loading config: " << e.what() << std::endl;
            std::cout << "Using default configuration" << std::endl;
            return false;
        }
    }
    
    bool saveToFile(const std::string& filename) {
        try {
            nlohmann::json j;
            
            j["osc_host"] = oscHost;
            j["osc_port"] = oscPort;
            j["audio_device"] = audioDevice;
            j["update_interval_ms"] = updateIntervalMs;
            
            nlohmann::json rangesJson = nlohmann::json::array();
            for (const auto& range : cvRanges) {
                nlohmann::json rangeJson;
                rangeJson["min"] = range.min;
                rangeJson["max"] = range.max;
                rangesJson.push_back(rangeJson);
            }
            j["cv_ranges"] = rangesJson;
            
            std::ofstream file(filename);
            file << j.dump(4);  // Pretty print with 4 spaces
            
            std::cout << "Configuration saved to " << filename << std::endl;
            return true;
            
        } catch (const std::exception& e) {
            std::cerr << "Error saving config: " << e.what() << std::endl;
            return false;
        }
    }
    
    // Getters
    const std::string& getOSCHost() const { return oscHost; }
    const std::string& getOSCPort() const { return oscPort; }
    const std::string& getAudioDevice() const { return audioDevice; }
    int getUpdateIntervalMs() const { return updateIntervalMs; }
    
    CVRange getCVRange(int channel) const {
        if (channel >= 0 && static_cast<size_t>(channel) < cvRanges.size()) {
            return cvRanges[channel];
        }
        return CVRange(0.0f, 10.0f);  // Default range
    }
    
    // Setters
    void setOSCHost(const std::string& host) { oscHost = host; }
    void setOSCPort(const std::string& port) { oscPort = port; }
    void setAudioDevice(const std::string& device) { audioDevice = device; }
    void setUpdateIntervalMs(int interval) { updateIntervalMs = interval; }
    
    void setCVRange(int channel, float min, float max) {
        if (channel >= 0) {
            if (static_cast<size_t>(channel) >= cvRanges.size()) {
                cvRanges.resize(static_cast<size_t>(channel) + 1, CVRange(0.0f, 10.0f));
            }
            cvRanges[static_cast<size_t>(channel)] = CVRange(min, max);
        }
    }
    
    void printConfiguration() const {
        std::cout << "\nCurrent Configuration:" << std::endl;
        std::cout << "  OSC Target: " << oscHost << ":" << oscPort << std::endl;
        std::cout << "  Audio Device: " << (audioDevice.empty() ? "default" : audioDevice) << std::endl;
        std::cout << "  Update Rate: " << (1000 / updateIntervalMs) << " Hz" << std::endl;
        std::cout << "  CV Ranges:" << std::endl;
        
        for (size_t i = 0; i < cvRanges.size(); ++i) {
            std::cout << "    Channel " << (i + 1) << ": " 
                      << cvRanges[i].min << "V to " << cvRanges[i].max << "V" << std::endl;
        }
        std::cout << std::endl;
    }
};
