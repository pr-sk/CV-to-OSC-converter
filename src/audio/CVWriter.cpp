#include "CVWriter.h"
#include "../core/SignalTypes.h"
#include <iostream>
#include <algorithm>
#include <portaudio.h>
#include <cmath>
#include <iomanip>
#include <string>

CVWriter::CVWriter() 
    : initialized_(false), deviceName_(""), channelCount_(8), 
      sampleRate_(44100.0), minVoltage_(-10.0f), maxVoltage_(10.0f),
      globalSignalType_(SignalType::AUTO_DETECT), 
      autoDetectionEnabled_(true), globalOutputMode_(OutputMode::AUTO_DETECT), 
      outputStream_(nullptr) {
    channelSignalTypes_.resize(channelCount_, SignalType::AUTO_DETECT);
    channelOutputModes_.resize(channelCount_, OutputMode::AUTO_DETECT);
    channelAnalysis_.resize(channelCount_);
    outputBuffer_.resize(channelCount_, 0.0f);
    
    // Initialize analysis structures
    for (int i = 0; i < channelCount_; ++i) {
        channelAnalysis_[i] = {}; // Zero-initialize
        channelAnalysis_[i].detectedType = SignalType::UNKNOWN;
    }
}

CVWriter::CVWriter(const std::string& deviceName)
    : initialized_(false), deviceName_(deviceName), channelCount_(8),
      sampleRate_(44100.0), minVoltage_(-10.0f), maxVoltage_(10.0f),
      globalSignalType_(SignalType::AUTO_DETECT), 
      autoDetectionEnabled_(true), globalOutputMode_(OutputMode::AUTO_DETECT),
      outputStream_(nullptr) {
    channelSignalTypes_.resize(channelCount_, SignalType::AUTO_DETECT);
    channelOutputModes_.resize(channelCount_, OutputMode::AUTO_DETECT);
    channelAnalysis_.resize(channelCount_);
    outputBuffer_.resize(channelCount_, 0.0f);
    
    // Initialize analysis structures
    for (int i = 0; i < channelCount_; ++i) {
        channelAnalysis_[i] = {}; // Zero-initialize
        channelAnalysis_[i].detectedType = SignalType::UNKNOWN;
    }
    
    // Try to detect device type based on name
    if (isDeviceCV(deviceName)) {
        globalSignalType_ = SignalType::CV_SIGNAL;
        globalOutputMode_ = OutputMode::DC_OUTPUT;
        std::cout << "Output device detected as CV interface: " << deviceName << std::endl;
    } else if (isDeviceAudio(deviceName)) {
        globalSignalType_ = SignalType::AUDIO_SIGNAL;
        globalOutputMode_ = OutputMode::AUDIO_OUTPUT;
        std::cout << "Output device detected as audio interface: " << deviceName << std::endl;
    } else {
        globalSignalType_ = SignalType::AUTO_DETECT;
        globalOutputMode_ = OutputMode::AUTO_DETECT;
        std::cout << "Output device type unknown, will auto-detect: " << deviceName << std::endl;
    }
    
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
    
    // Determine output mode for this channel
    OutputMode channelMode = channelOutputModes_[channelId];
    if (channelMode == OutputMode::AUTO_DETECT) {
        // Use global mode if channel is auto-detect
        channelMode = globalOutputMode_;
        
        // If global is also auto-detect, choose based on signal type
        if (channelMode == OutputMode::AUTO_DETECT) {
            SignalType channelType = channelSignalTypes_[channelId];
            if (channelType == SignalType::AUTO_DETECT) {
                channelType = globalSignalType_;
            }
            
            // Choose appropriate output mode based on signal type
            switch (channelType) {
                case SignalType::CV_SIGNAL:
                    channelMode = OutputMode::DC_OUTPUT;
                    break;
                case SignalType::AUDIO_SIGNAL:
                    channelMode = OutputMode::AUDIO_OUTPUT;
                    break;
                default:
                    channelMode = OutputMode::DC_OUTPUT; // Default to DC
                    break;
            }
        }
    }
    
    // Process signal based on determined output mode
    float sample = processSignalForOutput(clampedVoltage, channelMode);
    
    // Store in output buffer (thread-safe)
    {
        std::lock_guard<std::mutex> lock(outputMutex_);
        if (channelId < static_cast<int>(outputBuffer_.size())) {
            outputBuffer_[channelId] = sample;
        }
    }
    
    // In a real implementation, this would write to the audio output buffer
    // For now, we just log it in debug mode
    #ifdef DEBUG
    std::cout << "CVWriter: Channel " << channelId << " = " << clampedVoltage 
              << "V (sample: " << sample << ", mode: " << outputModeToString(channelMode) 
              << ")" << std::endl;
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

float CVWriter::voltageToSample(float voltage) const {
    // Convert voltage (-10V to +10V) to audio sample (-1.0 to +1.0)
    float normalized = (voltage - minVoltage_) / (maxVoltage_ - minVoltage_);
    return (normalized * 2.0f) - 1.0f; // Convert 0-1 to -1 to +1
}

// Signal type management methods
void CVWriter::setGlobalSignalType(SignalType type) {
    globalSignalType_ = type;
    std::cout << "CVWriter global signal type set to: " << signalTypeToString(type) << std::endl;
}

SignalType CVWriter::getGlobalSignalType() const {
    return globalSignalType_;
}

void CVWriter::forceChannelSignalType(int channel, SignalType type) {
    if (channel >= 0 && channel < static_cast<int>(channelSignalTypes_.size())) {
        channelSignalTypes_[channel] = type;
        std::cout << "CVWriter channel " << channel << " signal type forced to: " 
                  << signalTypeToString(type) << std::endl;
    }
}

SignalType CVWriter::getChannelSignalType(int channel) const {
    if (channel >= 0 && channel < static_cast<int>(channelSignalTypes_.size())) {
        return channelSignalTypes_[channel];
    }
    return SignalType::UNKNOWN;
}

std::string CVWriter::signalTypeToString(SignalType type) const {
    switch (type) {
        case SignalType::CV_SIGNAL: return "CV Signal";
        case SignalType::AUDIO_SIGNAL: return "Audio Signal";
        case SignalType::AUTO_DETECT: return "Auto-Detect";
        case SignalType::UNKNOWN: return "Unknown";
        default: return "Invalid";
    }
}

// Device type detection methods
bool CVWriter::isDeviceCV(const std::string& deviceName) const {
    // Check for common CV interface names
    std::string lowerName = deviceName;
    std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);
    
    return lowerName.find("cv") != std::string::npos ||
           lowerName.find("control voltage") != std::string::npos ||
           lowerName.find("eurorack") != std::string::npos ||
           lowerName.find("modular") != std::string::npos ||
           lowerName.find("voltage") != std::string::npos ||
           lowerName.find("gate") != std::string::npos ||
           lowerName.find("trigger") != std::string::npos;
}

bool CVWriter::isDeviceAudio(const std::string& deviceName) const {
    // Check for common audio interface names
    std::string lowerName = deviceName;
    std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);
    
    return lowerName.find("speaker") != std::string::npos ||
           lowerName.find("headphone") != std::string::npos ||
           lowerName.find("audio") != std::string::npos ||
           lowerName.find("звук") != std::string::npos ||
           lowerName.find("динамик") != std::string::npos ||
           lowerName.find("наушники") != std::string::npos ||
           lowerName.find("output") != std::string::npos;
}

// Output mode management methods
void CVWriter::setOutputMode(int channel, OutputMode mode) {
    if (channel >= 0 && channel < static_cast<int>(channelOutputModes_.size())) {
        channelOutputModes_[channel] = mode;
        std::cout << "CVWriter channel " << channel << " output mode set to: " 
                  << outputModeToString(mode) << std::endl;
    }
}

OutputMode CVWriter::getOutputMode(int channel) const {
    if (channel >= 0 && channel < static_cast<int>(channelOutputModes_.size())) {
        return channelOutputModes_[channel];
    }
    return OutputMode::AUTO_DETECT;
}

void CVWriter::setGlobalOutputMode(OutputMode mode) {
    globalOutputMode_ = mode;
    std::cout << "CVWriter global output mode set to: " << outputModeToString(mode) << std::endl;
}

// Signal processing methods
float CVWriter::processSignalForOutput(float voltage, OutputMode mode) const {
    switch (mode) {
        case OutputMode::DC_OUTPUT:
            return generateDCOutput(voltage);
        case OutputMode::PWM_OUTPUT:
            return generatePWMOutput(voltage);
        case OutputMode::AUDIO_OUTPUT:
            return generateAudioOutput(voltage);
        case OutputMode::AUTO_DETECT:
        default:
            // Default to DC output for unknown modes
            return generateDCOutput(voltage);
    }
}

float CVWriter::generateDCOutput(float voltage) const {
    // Direct DC voltage conversion for CV outputs
    return voltageToSample(voltage);
}

float CVWriter::generatePWMOutput(float voltage) const {
    // PWM modulation for devices that don't support true DC
    // This would generate a PWM signal with duty cycle proportional to voltage
    float normalized = (voltage - minVoltage_) / (maxVoltage_ - minVoltage_);
    normalized = std::clamp(normalized, 0.0f, 1.0f);
    
    // Simple PWM: return +1 for duty cycle portion, -1 for rest
    // In real implementation, this would be time-based
    static float phase = 0.0f;
    phase += 0.01f; // Increment phase
    if (phase > 1.0f) phase -= 1.0f;
    
    return (phase < normalized) ? 1.0f : -1.0f;
}

float CVWriter::generateAudioOutput(float voltage) const {
    // Standard audio signal generation
    // This could generate tones, noise, or other audio representations
    float normalized = (voltage - minVoltage_) / (maxVoltage_ - minVoltage_);
    normalized = std::clamp(normalized, 0.0f, 1.0f);
    
    // Simple approach: generate sine wave with frequency proportional to voltage
    static float phase = 0.0f;
    float frequency = 200.0f + (normalized * 2000.0f); // 200Hz to 2200Hz
    phase += (2.0f * M_PI * frequency) / sampleRate_;
    if (phase > 2.0f * M_PI) phase -= 2.0f * M_PI;
    
    return std::sin(phase) * 0.5f; // 50% amplitude
}

// Analysis methods
SignalAnalysis CVWriter::getChannelAnalysis(int channel) const {
    if (channel >= 0 && channel < static_cast<int>(channelAnalysis_.size())) {
        return channelAnalysis_[channel];
    }
    return SignalAnalysis{};
}

void CVWriter::printSignalAnalysis() const {
    std::cout << "\n=== CVWriter Signal Analysis Report ===" << std::endl;
    std::cout << "Global Signal Type: " << signalTypeToString(globalSignalType_) << std::endl;
    std::cout << "Global Output Mode: " << outputModeToString(globalOutputMode_) << std::endl;
    std::cout << "Auto-Detection: " << (autoDetectionEnabled_ ? "Enabled" : "Disabled") << std::endl;
    
    for (int i = 0; i < channelCount_; ++i) {
        const auto& analysis = channelAnalysis_[i];
        std::cout << "\nChannel " << i << ":" << std::endl;
        std::cout << "  Signal Type: " << signalTypeToString(getChannelSignalType(i)) << std::endl;
        std::cout << "  Output Mode: " << outputModeToString(getOutputMode(i)) << std::endl;
        std::cout << "  Confidence: " << std::fixed << std::setprecision(2) << analysis.confidence << std::endl;
    }
    std::cout << "======================================\n" << std::endl;
}

// Helper method for output mode string conversion
std::string CVWriter::outputModeToString(OutputMode mode) const {
    switch (mode) {
        case OutputMode::DC_OUTPUT: return "DC Output";
        case OutputMode::PWM_OUTPUT: return "PWM Output";
        case OutputMode::AUDIO_OUTPUT: return "Audio Output";
        case OutputMode::AUTO_DETECT: return "Auto-Detect";
        default: return "Unknown";
    }
}
