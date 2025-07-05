#pragma once

#include <string>
#include <vector>
#include <portaudio.h>
#include <memory>
#include <mutex>
#include <atomic>
#include <functional>
#include "../core/SignalTypes.h"

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
    
    // Signal type management
    void setGlobalSignalType(SignalType type);
    SignalType getGlobalSignalType() const;
    void forceChannelSignalType(int channel, SignalType type);
    SignalType getChannelSignalType(int channel) const;
    std::string signalTypeToString(SignalType type) const;
    std::string outputModeToString(OutputMode mode) const;
    
    // Auto-detection methods
    void enableAutoDetection(bool enable) { autoDetectionEnabled_ = enable; }
    bool isAutoDetectionEnabled() const { return autoDetectionEnabled_; }
    SignalAnalysis getChannelAnalysis(int channel) const;
    void printSignalAnalysis() const;
    
    // Output mode management
    void setOutputMode(int channel, OutputMode mode);
    OutputMode getOutputMode(int channel) const;
    void setGlobalOutputMode(OutputMode mode);
    OutputMode getGlobalOutputMode() const { return globalOutputMode_; }
    
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
    
    // Signal type management
    SignalType globalSignalType_;
    std::vector<SignalType> channelSignalTypes_;
    bool autoDetectionEnabled_;
    std::vector<SignalAnalysis> channelAnalysis_;
    
    // Output mode management
    OutputMode globalOutputMode_;
    std::vector<OutputMode> channelOutputModes_;
    
    // PortAudio output
    PaStream* outputStream_;
    std::vector<float> outputBuffer_;
    std::mutex outputMutex_;
    
    // Constants for analysis
    static constexpr float CV_STABILITY_THRESHOLD = 0.01f;
    static constexpr float AUDIO_AC_THRESHOLD = 0.1f;
    
    // Internal methods
    bool initializeAudioOutput();
    void cleanupAudioOutput();
    float voltageToSample(float voltage) const;
    
    // Device type detection methods
    bool isDeviceCV(const std::string& deviceName) const;
    bool isDeviceAudio(const std::string& deviceName) const;
    
    // Signal processing methods based on output mode
    float processSignalForOutput(float voltage, OutputMode mode) const;
    float generateDCOutput(float voltage) const;
    float generatePWMOutput(float voltage) const;
    float generateAudioOutput(float voltage) const;
};
