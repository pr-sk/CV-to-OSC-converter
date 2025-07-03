#pragma once

#include <string>
#include <vector>

/**
 * @brief CV Writer class for outputting CV signals from OSC data
 * 
 * This class handles conversion from OSC messages to CV output signals.
 * It's designed to work alongside CVReader for bidirectional CV/OSC conversion.
 */
class CVWriter {
public:
    CVWriter();
    explicit CVWriter(const std::string& deviceName);
    ~CVWriter();
    
    // Initialization
    bool initialize(const std::string& deviceName = "");
    void shutdown();
    bool isInitialized() const { return initialized_; }
    
    // Device management
    std::string getCurrentDeviceName() const { return deviceName_; }
    int getChannelCount() const { return channelCount_; }
    double getSampleRate() const { return sampleRate_; }
    
    // CV output
    bool writeChannel(int channelId, float voltage);
    bool writeChannels(const std::vector<float>& voltages);
    
    // Configuration
    void setVoltageRange(float minVoltage, float maxVoltage);
    void setChannelCount(int channelCount);
    
    // Error handling
    std::string getLastError() const { return lastError_; }
    
private:
    bool initialized_;
    std::string deviceName_;
    int channelCount_;
    double sampleRate_;
    float minVoltage_;
    float maxVoltage_;
    std::string lastError_;
    
    // Internal methods
    bool initializeAudioOutput();
    void cleanupAudioOutput();
    float voltageToSample(float voltage);
};
