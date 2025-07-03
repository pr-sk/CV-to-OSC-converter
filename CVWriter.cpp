#include "CVWriter.h"
#include <iostream>
#include <algorithm>

CVWriter::CVWriter() 
    : initialized_(false), deviceName_(""), channelCount_(8), 
      sampleRate_(44100.0), minVoltage_(-10.0f), maxVoltage_(10.0f) {
}

CVWriter::CVWriter(const std::string& deviceName)
    : initialized_(false), deviceName_(deviceName), channelCount_(8),
      sampleRate_(44100.0), minVoltage_(-10.0f), maxVoltage_(10.0f) {
    initialize(deviceName);
}

CVWriter::~CVWriter() {
    shutdown();
}

bool CVWriter::initialize(const std::string& deviceName) {
    if (initialized_) {
        shutdown();
    }
    
    if (!deviceName.empty()) {
        deviceName_ = deviceName;
    }
    
    if (deviceName_.empty()) {
        deviceName_ = "Default Output Device";
    }
    
    // For now, this is a stub implementation
    // In a real implementation, this would initialize audio output
    initialized_ = initializeAudioOutput();
    
    if (initialized_) {
        std::cout << "CVWriter initialized for device: " << deviceName_ << std::endl;
    } else {
        lastError_ = "Failed to initialize audio output device";
        std::cerr << lastError_ << std::endl;
    }
    
    return initialized_;
}

void CVWriter::shutdown() {
    if (initialized_) {
        cleanupAudioOutput();
        initialized_ = false;
        std::cout << "CVWriter shutdown" << std::endl;
    }
}

bool CVWriter::writeChannel(int channelId, float voltage) {
    if (!initialized_) {
        lastError_ = "CVWriter not initialized";
        return false;
    }
    
    if (channelId < 0 || channelId >= channelCount_) {
        lastError_ = "Invalid channel ID: " + std::to_string(channelId);
        return false;
    }
    
    // Clamp voltage to range
    float clampedVoltage = std::clamp(voltage, minVoltage_, maxVoltage_);
    
    // Convert voltage to audio sample
    float sample = voltageToSample(clampedVoltage);
    
    // In a real implementation, this would write to the audio output buffer
    // For now, we just log it in debug mode
    #ifdef DEBUG
    std::cout << "CVWriter: Channel " << channelId << " = " << clampedVoltage 
              << "V (sample: " << sample << ")" << std::endl;
    #else
    // Suppress unused variable warning in release builds
    (void)sample;
    #endif
    
    return true;
}

bool CVWriter::writeChannels(const std::vector<float>& voltages) {
    if (!initialized_) {
        lastError_ = "CVWriter not initialized";
        return false;
    }
    
    bool allSuccess = true;
    for (size_t i = 0; i < voltages.size() && i < static_cast<size_t>(channelCount_); ++i) {
        if (!writeChannel(static_cast<int>(i), voltages[i])) {
            allSuccess = false;
        }
    }
    
    return allSuccess;
}

void CVWriter::setVoltageRange(float minVoltage, float maxVoltage) {
    if (minVoltage < maxVoltage) {
        minVoltage_ = minVoltage;
        maxVoltage_ = maxVoltage;
    } else {
        lastError_ = "Invalid voltage range: min must be less than max";
    }
}

void CVWriter::setChannelCount(int channelCount) {
    if (channelCount > 0 && channelCount <= 32) {
        channelCount_ = channelCount;
    } else {
        lastError_ = "Invalid channel count: must be between 1 and 32";
    }
}

bool CVWriter::initializeAudioOutput() {
    // Stub implementation - in a real system this would:
    // 1. Initialize audio output device
    // 2. Set up output streams
    // 3. Configure sample rate and channel count
    // 4. Start audio output thread
    
    // For now, just return true to indicate success
    return true;
}

void CVWriter::cleanupAudioOutput() {
    // Stub implementation - in a real system this would:
    // 1. Stop audio output thread
    // 2. Clean up output streams
    // 3. Release audio device
}

float CVWriter::voltageToSample(float voltage) {
    // Convert voltage (-10V to +10V) to audio sample (-1.0 to +1.0)
    float normalized = (voltage - minVoltage_) / (maxVoltage_ - minVoltage_);
    return (normalized * 2.0f) - 1.0f; // Convert 0-1 to -1 to +1
}
