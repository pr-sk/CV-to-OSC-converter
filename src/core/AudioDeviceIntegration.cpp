#include "AudioDeviceIntegration.h"
#include "AudioDeviceManager.h"
#include <iostream>
#include <sstream>
#include <string>
#include <cmath>
#include <cstdlib>
#include <map>
#include <vector>
#include <algorithm>
#include <chrono>

AudioDeviceIntegration::AudioDeviceIntegration() 
    : audioDeviceManager_(nullptr), initialized_(false)
    , streamManager_(std::make_unique<RealAudioStreamManager>()) {
}

AudioDeviceIntegration::~AudioDeviceIntegration() {
    shutdown();
}

float AudioDeviceIntegration::getInputSample(const std::string& deviceId) const {
    if (!initialized_ || !audioDeviceManager_ || !streamManager_) {
        return 0.0f;
    }
    
    // Check if we have a real audio stream for this device
    if (streamManager_->isStreamRunning(deviceId)) {
        // Get real audio input level from the stream
        float cvLevel = streamManager_->getInputLevel(deviceId);
        
        // ДЕТАЛЬНАЯ ДИАГНОСТИКА: Логируем каждое значение > 0
        if (cvLevel > 0.001f) {
            printf("[AUDIO INPUT] Device %s: Raw level=%.6fV\n", deviceId.c_str(), cvLevel);
        }
        
        // Log occasionally for debugging
        static auto lastLog = std::chrono::steady_clock::now();
        auto now = std::chrono::steady_clock::now();
        if (std::chrono::duration_cast<std::chrono::seconds>(now - lastLog).count() > 5) {
            if (cvLevel > 0.1f) {
                std::cout << "🎤 Real audio input from " << deviceId << ": " << cvLevel << "V" << std::endl;
            }
            lastLog = now;
        }
        
        return cvLevel;
    }
    
    // If no real stream, try to create one
    OSCDeviceConfig config;
    config.deviceId = deviceId;
    AudioDeviceInfo deviceInfo = getAudioDeviceInfo(config);
    
    if (!deviceInfo.isCurrentlyAvailable || deviceInfo.index < 0) {
        return 0.0f;
    }
    
    // Create audio stream for this device
    if (!streamManager_->hasStream(deviceId)) {
        std::cout << "🎤 Creating real audio stream for device: " << deviceId 
                  << " (index: " << deviceInfo.index << ")" << std::endl;
        
        // Cast away const to create stream (this is a design limitation)
        AudioDeviceIntegration* mutableThis = const_cast<AudioDeviceIntegration*>(this);
        if (mutableThis->streamManager_->createInputStream(deviceId, deviceInfo.index)) {
            std::cout << "✅ Successfully created audio stream for " << deviceId << std::endl;
        } else {
            std::cerr << "❌ Failed to create audio stream for " << deviceId << std::endl;
        }
    }
    
    // Try to get level again after stream creation
    if (streamManager_->isStreamRunning(deviceId)) {
        return streamManager_->getInputLevel(deviceId);
    }
    
    // Fallback to 0 if stream creation failed
    return 0.0f;
}

bool AudioDeviceIntegration::sendOutputSample(const std::string& deviceId, float sample) {
    if (!initialized_ || !audioDeviceManager_ || !streamManager_) {
        return false;
    }
    
    // Check if we have a real audio stream for this device
    if (streamManager_->isStreamRunning(deviceId)) {
        // Send the sample to the audio output stream
        streamManager_->sendOutputData(deviceId, sample);
        
        // Log occasionally for debugging
        static auto lastLog = std::chrono::steady_clock::now();
        auto now = std::chrono::steady_clock::now();
        if (std::chrono::duration_cast<std::chrono::seconds>(now - lastLog).count() > 5) {
            if (std::abs(sample) > 0.1f) {
                std::cout << "🔊 Real audio output to " << deviceId << ": " << sample << "V" << std::endl;
            }
            lastLog = now;
        }
        
        return true;
    }
    
    // If no real stream exists, output stream should have been created when device was added
    std::cerr << "❌ No audio output stream running for device: " << deviceId << std::endl;
    return false;
}

bool AudioDeviceIntegration::initialize(AudioDeviceManager* deviceManager) {
    if (!deviceManager) {
        std::cerr << "AudioDeviceIntegration: Invalid device manager" << std::endl;
        return false;
    }
    
    audioDeviceManager_ = deviceManager;
    
    // Initialize stream manager
    if (streamManager_) {
        streamManager_->initialize(deviceManager);
    }
    
    // Set up device change callback
    audioDeviceManager_->addDeviceChangeCallback(
        [this](const std::vector<AudioDeviceInfo>& devices) {
            onAudioDeviceChange(devices);
        });
    
    initialized_ = true;
    std::cout << "AudioDeviceIntegration: Initialized successfully" << std::endl;
    return true;
}

void AudioDeviceIntegration::shutdown() {
    if (streamManager_) {
        streamManager_->shutdown();
    }
    if (initialized_ && audioDeviceManager_) {
        audioDeviceManager_->removeAllCallbacks();
        audioDeviceManager_ = nullptr;
    }
    initialized_ = false;
}

std::vector<OSCDeviceConfig> AudioDeviceIntegration::getAvailableInputDevices() const {
    std::vector<OSCDeviceConfig> oscDevices;
    
    if (!initialized_ || !audioDeviceManager_) {
        return oscDevices;
    }
    
    auto inputDevices = audioDeviceManager_->getInputDevices();
    for (const auto& device : inputDevices) {
        if (device.isCurrentlyAvailable && device.maxInputChannels > 0) {
            oscDevices.push_back(createInputDeviceConfig(device));
        }
    }
    
    return oscDevices;
}

std::vector<OSCDeviceConfig> AudioDeviceIntegration::getAvailableOutputDevices() const {
    std::vector<OSCDeviceConfig> oscDevices;
    
    if (!initialized_ || !audioDeviceManager_) {
        return oscDevices;
    }
    
    auto outputDevices = audioDeviceManager_->getOutputDevices();
    for (const auto& device : outputDevices) {
        if (device.isCurrentlyAvailable && device.maxOutputChannels > 0) {
            oscDevices.push_back(createOutputDeviceConfig(device));
        }
    }
    
    return oscDevices;
}

float AudioDeviceIntegration::processAndConvertAudioSignal(float rawSample, const std::string& deviceId) const {
    // Track signal characteristics for auto-detection
    static std::map<std::string, std::vector<float>> signalHistory;
    static std::map<std::string, std::string> detectedTypes;
    
    auto& history = signalHistory[deviceId];
    
    // Calculate absolute value for analysis
    float absValue = std::abs(rawSample);
    
    // Update signal history (keep last 100 samples)
    history.push_back(absValue);
    if (history.size() > 100) {
        history.erase(history.begin());
    }
    
    // Analyze signal characteristics if we have enough samples
    if (history.size() >= 10) {
        float rms = 0.0f;
        float peak = 0.0f;
        float variance = 0.0f;
        float mean = 0.0f;
        
        // Calculate statistics
        for (float val : history) {
            mean += val;
            peak = std::max(peak, val);
        }
        mean /= history.size();
        
        for (float val : history) {
            float diff = val - mean;
            rms += diff * diff;
            variance += diff * diff;
        }
        rms = std::sqrt(rms / history.size());
        variance /= history.size();
        
        std::string& detectedType = detectedTypes[deviceId];
        std::string previousType = detectedType;
        
        // Auto-detect signal type based on characteristics
        if (peak > 0.1f && rms > 0.02f && variance > 0.001f) {
            // Likely audio signal - dynamic, varying amplitude
            detectedType = "AUDIO";
            // Convert audio (-1 to 1) to CV (0 to 10V)
            float cvSignal = (rawSample + 1.0f) * 5.0f;
            
            // Log type change
            if (previousType != detectedType) {
                std::cout << "🎵 AudioDeviceIntegration: " << deviceId 
                          << " detected as AUDIO signal, converting to CV range (0-10V)" << std::endl;
            }
            
            return std::max(0.0f, std::min(10.0f, cvSignal));
            
        } else if (absValue > 0.001f && variance < 0.01f) {
            // Likely CV signal - more stable, less varying
            detectedType = "CV";
            // Assume input is already in reasonable CV range, scale if needed
            float cvSignal = rawSample;
            
            // If signal seems to be in -1 to 1 range, scale to 0-10V
            if (peak <= 1.0f) {
                cvSignal = (rawSample + 1.0f) * 5.0f;
            } else if (peak <= 5.0f) {
                // Might be 0-5V CV, scale to 0-10V
                cvSignal = rawSample * 2.0f;
            }
            // Otherwise assume already in 0-10V range
            
            // Log type change
            if (previousType != detectedType) {
                std::cout << "🔌 AudioDeviceIntegration: " << deviceId 
                          << " detected as CV signal, passing through" << std::endl;
            }
            
            return std::max(0.0f, std::min(10.0f, cvSignal));
            
        } else {
            // No significant signal or noise
            detectedType = "NONE";
            return 0.0f;
        }
    }
    
    // Default: treat as audio and convert
    return std::max(0.0f, std::min(10.0f, (rawSample + 1.0f) * 5.0f));
}

OSCDeviceConfig AudioDeviceIntegration::createInputDeviceConfig(const AudioDeviceInfo& audioDevice) const {
    return createDeviceConfig(audioDevice, true);
}

OSCDeviceConfig AudioDeviceIntegration::createOutputDeviceConfig(const AudioDeviceInfo& audioDevice) const {
    return createDeviceConfig(audioDevice, false);
}

void AudioDeviceIntegration::setDeviceChangeCallback(std::function<void(const std::vector<OSCDeviceConfig>&)> callback) {
    deviceChangeCallback_ = callback;
}

bool AudioDeviceIntegration::validateAudioDevice(const OSCDeviceConfig& device) const {
    if (!initialized_ || !audioDeviceManager_) {
        return false;
    }
    
    // Check if the device ID corresponds to a real audio device
    // Format: "audio_input_<index>" or "audio_output_<index>"
    if (device.deviceId.find("audio_") != 0) {
        return false;
    }
    
    try {
        // Extract device index from ID
        std::string prefix = device.deviceId.substr(0, device.deviceId.find_last_of('_'));
        std::string indexStr = device.deviceId.substr(device.deviceId.find_last_of('_') + 1);
        int deviceIndex = std::stoi(indexStr);
        
        // Validate device exists and is available
        AudioDeviceInfo audioInfo = audioDeviceManager_->findDeviceByIndex(deviceIndex);
        return audioInfo.index == deviceIndex && audioInfo.isCurrentlyAvailable;
        
    } catch (const std::exception&) {
        return false;
    }
}

AudioDeviceInfo AudioDeviceIntegration::getAudioDeviceInfo(const OSCDeviceConfig& device) const {
    AudioDeviceInfo invalidDevice;
    
    if (!validateAudioDevice(device)) {
        return invalidDevice;
    }
    
    try {
        // Extract device index from ID
        std::string indexStr = device.deviceId.substr(device.deviceId.find_last_of('_') + 1);
        int deviceIndex = std::stoi(indexStr);
        
        return audioDeviceManager_->findDeviceByIndex(deviceIndex);
        
    } catch (const std::exception&) {
        return invalidDevice;
    }
}

std::string AudioDeviceIntegration::generateDeviceId(const AudioDeviceInfo& device, bool isInput) const {
    std::ostringstream oss;
    oss << "audio_" << (isInput ? "input" : "output") << "_" << device.index;
    return oss.str();
}

OSCDeviceConfig AudioDeviceIntegration::createDeviceConfig(const AudioDeviceInfo& device, bool isInput) const {
    OSCDeviceConfig config;
    
    config.deviceId = generateDeviceId(device, isInput);
    config.deviceName = device.name + (isInput ? " (Input)" : " (Output)");
    config.protocolType = OSCProtocolType::UDP_UNICAST;
    config.networkAddress = "127.0.0.1";
    config.port = isInput ? 9000 : 9001; // Default ports
    config.localAddress = "0.0.0.0";
    config.localPort = isInput ? (9000 + device.index) : (9100 + device.index); // Assign unique ports
    config.oscAddress = isInput ? "/cv/input" : "/cv/output";
    config.enabled = true;
    config.connected = device.isCurrentlyAvailable;
    
    // Set supported types based on device capabilities
    config.supportedTypes = {OSCMessageType::FLOAT};
    if (isInput && device.maxInputChannels > 0) {
        config.supportedTypes.push_back(OSCMessageType::INT);
    }
    if (!isInput && device.maxOutputChannels > 0) {
        config.supportedTypes.push_back(OSCMessageType::INT);
    }
    
    return config;
}

void AudioDeviceIntegration::onAudioDeviceChange(const std::vector<AudioDeviceInfo>& devices) {
    if (!deviceChangeCallback_) {
        return;
    }
    
    // Convert all available devices to OSC device configs
    std::vector<OSCDeviceConfig> allOscDevices;
    
    // Add input devices
    for (const auto& device : devices) {
        if (device.maxInputChannels > 0 && device.isCurrentlyAvailable) {
            allOscDevices.push_back(createInputDeviceConfig(device));
        }
        if (device.maxOutputChannels > 0 && device.isCurrentlyAvailable) {
            allOscDevices.push_back(createOutputDeviceConfig(device));
        }
    }
    
    std::cout << "AudioDeviceIntegration: Device change detected, " 
              << allOscDevices.size() << " OSC devices available" << std::endl;
    
    deviceChangeCallback_(allOscDevices);
}

bool AudioDeviceIntegration::createAudioInputStream(const std::string& deviceId, int deviceIndex) {
    if (!initialized_ || !streamManager_) {
        return false;
    }
    
    // Check microphone permission first on macOS
#ifdef __APPLE__
    if (!CHECK_MICROPHONE_PERMISSION()) {
        std::cerr << "❌ Microphone permission not granted. Cannot open audio device." << std::endl;
        std::cerr << "Please grant microphone access in System Preferences > Security & Privacy > Privacy > Microphone" << std::endl;
        
        // Try to request permission (this might show a dialog)
        MacOSPermissions::requestMicrophonePermission([deviceId, deviceIndex, this](bool granted) {
            if (granted) {
                std::cout << "Microphone permission granted. Please try selecting the device again." << std::endl;
            }
        });
        
        return false;
    }
#endif
    
    std::cout << "🎤 AudioDeviceIntegration: Creating audio input stream for device: " << deviceId 
              << " (index: " << deviceIndex << ")" << std::endl;
    
    bool result = streamManager_->createInputStream(deviceId, deviceIndex);
    
    if (result) {
        std::cout << "✅ AudioDeviceIntegration: Successfully created audio input stream for " << deviceId << std::endl;
    } else {
        std::cerr << "❌ AudioDeviceIntegration: Failed to create audio input stream for " << deviceId << std::endl;
    }
    
    return result;
}

bool AudioDeviceIntegration::createAudioOutputStream(const std::string& deviceId, int deviceIndex) {
    if (!initialized_ || !streamManager_) {
        return false;
    }
    
    std::cout << "🔊 AudioDeviceIntegration: Creating audio output stream for device: " << deviceId 
              << " (index: " << deviceIndex << ")" << std::endl;
    
    bool result = streamManager_->createOutputStream(deviceId, deviceIndex);
    
    if (result) {
        std::cout << "✅ AudioDeviceIntegration: Successfully created audio output stream for " << deviceId << std::endl;
    } else {
        std::cerr << "❌ AudioDeviceIntegration: Failed to create audio output stream for " << deviceId << std::endl;
    }
    
    return result;
}

bool AudioDeviceIntegration::createAudioRouting(const std::string& inputDeviceId, const std::string& outputDeviceId) {
    if (!initialized_ || !streamManager_ || !audioDeviceManager_) {
        return false;
    }
    
    std::cout << "🔄 AudioDeviceIntegration: Creating audio routing from " << inputDeviceId 
              << " to " << outputDeviceId << std::endl;
    
    // Get device indices from IDs
    int inputIndex = -1;
    int outputIndex = -1;
    
    try {
        // Extract input device index
        if (inputDeviceId.find("audio_input_") != std::string::npos || 
            inputDeviceId.find("real_audio_input_") != std::string::npos) {
            std::string indexStr = inputDeviceId.substr(inputDeviceId.find_last_of('_') + 1);
            inputIndex = std::stoi(indexStr);
        }
        
        // Extract output device index
        if (outputDeviceId.find("audio_output_") != std::string::npos || 
            outputDeviceId.find("real_audio_output_") != std::string::npos) {
            std::string indexStr = outputDeviceId.substr(outputDeviceId.find_last_of('_') + 1);
            outputIndex = std::stoi(indexStr);
        }
        
        if (inputIndex < 0 || outputIndex < 0) {
            std::cerr << "❌ Invalid device indices: input=" << inputIndex 
                      << ", output=" << outputIndex << std::endl;
            return false;
        }
        
        // Create a unique ID for the duplex stream
        std::string duplexId = "duplex_" + inputDeviceId + "_to_" + outputDeviceId;
        
        // Remove existing streams if any
        streamManager_->removeStream(inputDeviceId);
        streamManager_->removeStream(outputDeviceId);
        streamManager_->removeStream(duplexId);
        
        // Create duplex stream using the manager
        if (streamManager_->createDuplexStream(duplexId, inputIndex, outputIndex)) {
            std::cout << "✅ Successfully created duplex audio stream" << std::endl;
            return true;
        }
        
    } catch (const std::exception& e) {
        std::cerr << "❌ Error creating audio routing: " << e.what() << std::endl;
        return false;
    }
    
    return false;
}

void AudioDeviceIntegration::removeAudioStream(const std::string& deviceId) {
    if (!initialized_ || !streamManager_) {
        return;
    }
    
    std::cout << "🗑️ AudioDeviceIntegration: Removing audio stream for device: " << deviceId << std::endl;
    streamManager_->removeStream(deviceId);
}
