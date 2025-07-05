#include "OSCMixerEngine.h"
#include "Config.h"
#include <iostream>
#include <fstream>
#include <chrono>
#include <algorithm>
#include <cmath>
#include <thread>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

OSCMixerEngine::OSCMixerEngine() 
    : engineRunning_(false)
    , learningMode_(false)
    , learningChannelId_(-1)
    , messagesThisSecond_(0) {
    // Initialize with default 8 channels
    mixerState_.channels.clear();
    mixerState_.channels.reserve(8);
    for (int i = 0; i < 8; ++i) {
        auto channel = std::make_unique<MixerChannel>(i);
        channel->channelId = i;
        channel->channelName = "Channel " + std::to_string(i + 1);
        channel->state = ChannelState::STOPPED;
        mixerState_.channels.push_back(std::move(channel));
    }
    std::cout << "Initialized " << mixerState_.channels.size() << " channels in constructor" << std::endl;
}

OSCMixerEngine::OSCMixerEngine(int numChannels) 
    : engineRunning_(false)
    , learningMode_(false)
    , learningChannelId_(-1)
    , messagesThisSecond_(0) {
    // Initialize with specified number of channels
    int channels = std::max(1, std::min(numChannels, 32)); // Limit to 1-32 channels
    mixerState_.channels.clear();
    mixerState_.channels.reserve(channels);
    for (int i = 0; i < channels; ++i) {
        auto channel = std::make_unique<MixerChannel>(i);
        channel->channelId = i;
        channel->channelName = "Channel " + std::to_string(i + 1);
        channel->state = ChannelState::STOPPED;
        mixerState_.channels.push_back(std::move(channel));
    }
    std::cout << "Initialized " << mixerState_.channels.size() << " channels in parameterized constructor" << std::endl;
}

OSCMixerEngine::~OSCMixerEngine() {
    shutdown();
}

bool OSCMixerEngine::initialize() {
    // Initialize AudioDeviceIntegration if not already done
    if (!audioDeviceIntegration_) {
        audioDeviceIntegration_ = std::make_shared<AudioDeviceIntegration>();
        
        // Create and initialize AudioDeviceManager for integration
        auto audioDeviceManager = std::make_shared<AudioDeviceManager>();
        if (audioDeviceManager->initialize()) {
            audioDeviceIntegration_->initialize(audioDeviceManager.get());
            std::cout << "✅ AudioDeviceIntegration initialized successfully" << std::endl;
        } else {
            std::cerr << "⚠️ Failed to initialize AudioDeviceManager, continuing without real audio" << std::endl;
        }
    }
    
    try {
        std::cout << "Initializing OSC Mixer Engine..." << std::endl;
        
        // Reset mixer state (can't use assignment due to atomic members)
        // mixerState_ = MasterMixerState(); // This line causes error due to atomic members
        // Instead, reset individual components as needed
        mixerState_.masterLevel = 1.0f;
        mixerState_.masterMute = false;
        mixerState_.totalMessagesPerSecond = 0;
        mixerState_.totalActiveConnections = 0;
        mixerState_.totalErrors = 0;
        mixerState_.scanningDevices = false;
        mixerState_.availableDevices.clear();
        
        // Clear device collections
        oscSenders_.clear();
        oscReceivers_.clear();
        deviceStatuses_.clear();
        
        // Initialize performance tracking
        lastStatsUpdate_ = std::chrono::steady_clock::now();
        
        // Start engine thread
        engineRunning_ = true;
        engineThread_ = std::thread(&OSCMixerEngine::engineLoop, this);
        
        std::cout << "OSC Mixer Engine initialized successfully with " 
                  << mixerState_.channels.size() << " channels" << std::endl;
        
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "Failed to initialize OSC Mixer Engine: " << e.what() << std::endl;
        return false;
    }
}

void OSCMixerEngine::shutdown() {
    if (audioDeviceIntegration_) {
        audioDeviceIntegration_->shutdown();
    }
    if (engineRunning_) {
        std::cout << "Shutting down OSC Mixer Engine..." << std::endl;
        
        // Stop all channels first
        for (int i = 0; i < 8; ++i) {
            stopChannel(i);
        }
        
        // Stop discovery thread
        stopDeviceDiscovery();
        
        // Stop engine thread
        engineRunning_ = false;
        stateCondition_.notify_all();
        messageCondition_.notify_all();
        
        if (engineThread_.joinable()) {
            engineThread_.join();
        }
        
        if (discoveryThread_.joinable()) {
            discoveryThread_.join();
        }
        
        // Clean up all devices
        {
            std::lock_guard<std::mutex> lock(deviceMutex_);
            oscSenders_.clear();
            oscReceivers_.clear();
            deviceStatuses_.clear();
        }
        
        std::cout << "OSC Mixer Engine shutdown complete" << std::endl;
    }
}


// Channel Control Methods
bool OSCMixerEngine::startChannel(int channelId) {
    if (!isChannelIdValid(channelId)) {
        std::cerr << "Invalid channel ID: " << channelId << std::endl;
        return false;
    }
    
    auto* channel = mixerState_.getChannel(channelId);
    if (!channel) {
        std::cerr << "Channel not found: " << channelId << std::endl;
        return false;
    }
    
    try {
        std::lock_guard<std::mutex> lock(stateMutex_);
        
        // Check if we have both audio input and output devices - if so, create duplex stream
        std::string audioInputDeviceId;
        std::string audioOutputDeviceId;
        bool hasAudioInput = false;
        bool hasAudioOutput = false;
        
        // First, check what audio devices we have
        for (const auto& deviceConfig : channel->inputDevices) {
            if (deviceConfig.enabled && 
                (deviceConfig.deviceId.find("real_audio_input_") == 0 || 
                 deviceConfig.deviceId.find("audio_input_") == 0)) {
                audioInputDeviceId = deviceConfig.deviceId;
                hasAudioInput = true;
                break;
            }
        }
        
        for (const auto& deviceConfig : channel->outputDevices) {
            if (deviceConfig.enabled && 
                (deviceConfig.deviceId.find("real_audio_output_") == 0 || 
                 deviceConfig.deviceId.find("audio_output_") == 0)) {
                audioOutputDeviceId = deviceConfig.deviceId;
                hasAudioOutput = true;
                break;
            }
        }
        
        // If we have both audio input and output, create duplex stream for direct routing
        if (hasAudioInput && hasAudioOutput && audioDeviceIntegration_) {
            std::cout << "🎉 Creating duplex audio routing for channel " << (channelId + 1) << std::endl;
            if (audioDeviceIntegration_->createAudioRouting(audioInputDeviceId, audioOutputDeviceId)) {
                std::cout << "✅ Duplex audio routing created successfully" << std::endl;
                // Skip individual stream creation since duplex handles both
            } else {
                std::cerr << "⚠️ Failed to create duplex routing, falling back to separate streams" << std::endl;
                hasAudioInput = false; // Force fallback
                hasAudioOutput = false;
            }
        }
        
        // Initialize input devices (only if not using duplex)
        if (!hasAudioInput || !hasAudioOutput) {
            for (const auto& deviceConfig : channel->inputDevices) {
                if (deviceConfig.enabled) {
                    // For audio devices, create audio stream instead of OSC receiver
                    if ((deviceConfig.deviceId.find("real_audio_input_") == 0 || 
                         deviceConfig.deviceId.find("audio_input_") == 0) && audioDeviceIntegration_) {
                        // Extract device index from ID
                        std::string indexStr = deviceConfig.deviceId.substr(deviceConfig.deviceId.find_last_of('_') + 1);
                        try {
                            int deviceIndex = std::stoi(indexStr);
                            if (!audioDeviceIntegration_->createAudioInputStream(deviceConfig.deviceId, deviceIndex)) {
                                std::cerr << "Failed to create audio stream for device: " 
                                          << deviceConfig.deviceId << std::endl;
                                continue;
                            }
                        } catch (const std::exception& e) {
                            std::cerr << "Error parsing device index: " << e.what() << std::endl;
                            continue;
                        }
                    } else {
                        // For network devices, create OSC receiver
                        if (!createOSCReceiver(deviceConfig)) {
                            std::cerr << "Failed to create OSC receiver for device: " 
                                      << deviceConfig.deviceId << std::endl;
                            continue;
                        }
                    }
                }
            }
            
            // Initialize output devices (only if not using duplex)
            for (const auto& deviceConfig : channel->outputDevices) {
                if (deviceConfig.enabled) {
                    // For audio devices, create audio stream instead of OSC sender
                    if ((deviceConfig.deviceId.find("real_audio_output_") == 0 || 
                         deviceConfig.deviceId.find("audio_output_") == 0) && audioDeviceIntegration_) {
                        // Extract device index from ID or use the audioDeviceIndex field
                        int deviceIndex = deviceConfig.audioDeviceIndex;
                        if (deviceIndex < 0) {
                            // Try to extract from deviceId as fallback
                            std::string indexStr = deviceConfig.deviceId.substr(deviceConfig.deviceId.find_last_of('_') + 1);
                            try {
                                deviceIndex = std::stoi(indexStr);
                            } catch (const std::exception& e) {
                                std::cerr << "Error parsing device index: " << e.what() << std::endl;
                                continue;
                            }
                        }
                        
                        if (deviceIndex >= 0) {
                            if (!audioDeviceIntegration_->createAudioOutputStream(deviceConfig.deviceId, deviceIndex)) {
                                std::cerr << "Failed to create audio output stream for device: " 
                                          << deviceConfig.deviceId << std::endl;
                                continue;
                            }
                        }
                    } else {
                        // For network devices, create OSC sender
                        if (!createOSCSender(deviceConfig)) {
                            std::cerr << "Failed to create OSC sender for device: " 
                                      << deviceConfig.deviceId << std::endl;
                            continue;
                        }
                    }
                }
            }
        }
        
        channel->state = ChannelState::RUNNING;
        // Don't reset meters to avoid signal jumps - let them naturally decay
        // channel->inputMeter.reset();
        // channel->outputMeter.reset();
        
        // Update solo/mix logic
        updateSoloMixLogic();
        
        std::cout << "Channel " << (channelId + 1) << " started successfully" << std::endl;
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "Error starting channel " << channelId << ": " << e.what() << std::endl;
        channel->state = ChannelState::ERROR;
        return false;
    }
}

bool OSCMixerEngine::stopChannel(int channelId) {
    if (!isChannelIdValid(channelId)) {
        return false;
    }
    
    auto* channel = mixerState_.getChannel(channelId);
    if (!channel) {
        return false;
    }
    
    try {
        std::lock_guard<std::mutex> lock(stateMutex_);
        
        // Clean up devices for this channel
        for (const auto& deviceConfig : channel->inputDevices) {
            cleanupDevice(deviceConfig.deviceId);
        }
        
        for (const auto& deviceConfig : channel->outputDevices) {
            cleanupDevice(deviceConfig.deviceId);
        }
        
        channel->state = ChannelState::STOPPED;
        channel->inputMeter.reset();
        channel->outputMeter.reset();
        
        // Update solo/mix logic
        updateSoloMixLogic();
        
        std::cout << "Channel " << (channelId + 1) << " stopped" << std::endl;
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "Error stopping channel " << channelId << ": " << e.what() << std::endl;
        return false;
    }
}

bool OSCMixerEngine::setChannelLevel(int channelId, float levelVolts) {
    if (!isChannelIdValid(channelId)) {
        return false;
    }
    
    auto* channel = mixerState_.getChannel(channelId);
    if (!channel) {
        return false;
    }
    
    // Clamp level to channel range
    levelVolts = std::clamp(levelVolts, channel->minRange, channel->maxRange);
    
    std::lock_guard<std::mutex> lock(stateMutex_);
    channel->levelVolts = levelVolts;
    
    // Note: Fader level is stored but does not affect output signal
    // Output level is determined automatically by input signal strength
    
    return true;
}

bool OSCMixerEngine::setChannelMode(int channelId, ChannelMode mode) {
    if (!isChannelIdValid(channelId)) {
        return false;
    }
    
    auto* channel = mixerState_.getChannel(channelId);
    if (!channel) {
        return false;
    }
    
    std::lock_guard<std::mutex> lock(stateMutex_);
    channel->mode = mode;
    
    // Update solo/mix logic for all channels
    updateSoloMixLogic();
    
    return true;
}

bool OSCMixerEngine::setChannelRange(int channelId, float minRange, float maxRange) {
    if (!isChannelIdValid(channelId)) {
        return false;
    }
    
    if (minRange >= maxRange) {
        std::cerr << "Invalid range: min (" << minRange << ") must be less than max (" << maxRange << ")" << std::endl;
        return false;
    }
    
    auto* channel = mixerState_.getChannel(channelId);
    if (!channel) {
        return false;
    }
    
    std::lock_guard<std::mutex> lock(stateMutex_);
    channel->minRange = minRange;
    channel->maxRange = maxRange;
    
    // Clamp current level to new range
    channel->levelVolts = std::clamp(channel->levelVolts, minRange, maxRange);
    
    return true;
}

// Device Management Methods
void OSCMixerEngine::setAudioDeviceIntegration(std::shared_ptr<AudioDeviceIntegration> integration) {
    audioDeviceIntegration_ = integration;
}

std::vector<OSCDeviceConfig> OSCMixerEngine::getAvailableInputDevices() const {
    return audioDeviceIntegration_ ? audioDeviceIntegration_->getAvailableInputDevices() : std::vector<OSCDeviceConfig>{};
}

std::vector<OSCDeviceConfig> OSCMixerEngine::getAvailableOutputDevices() const {
    return audioDeviceIntegration_ ? audioDeviceIntegration_->getAvailableOutputDevices() : std::vector<OSCDeviceConfig>{};
}

bool OSCMixerEngine::addInputDevice(int channelId, const OSCDeviceConfig& device) {
    if (!isChannelIdValid(channelId)) {
        return false;
    }
    
    if (!validateDeviceConfig(device)) {
        std::cerr << "Invalid device configuration for device: " << device.deviceId << std::endl;
        return false;
    }
    
    auto* channel = mixerState_.getChannel(channelId);
    if (!channel) {
        return false;
    }
    
    // Use unique_lock to allow manual unlock
    std::unique_lock<std::mutex> lock(stateMutex_);
    
    // Check if device already exists
    auto it = std::find_if(channel->inputDevices.begin(), channel->inputDevices.end(),
        [&device](const OSCDeviceConfig& existing) {
            return existing.deviceId == device.deviceId;
        });
    
    if (it != channel->inputDevices.end()) {
        std::cerr << "Input device already exists: " << device.deviceId << std::endl;
        return false;
    }
    
    // Add device
    if (!channel->addInputDevice(device)) {
        std::cerr << "Channel " << channelId << " input device limit reached (8 max)" << std::endl;
        return false;
    }
    
    // Initialize device status
    {
        std::lock_guard<std::mutex> deviceLock(deviceMutex_);
        deviceStatuses_[device.deviceId] = DeviceStatus{
            .deviceId = device.deviceId,
            .status = DeviceConnectionStatus::DISCONNECTED,
            .lastActivity = std::chrono::steady_clock::now()
        };
    }
    
    // If channel is running, create the receiver immediately
    if (channel->state == ChannelState::RUNNING && device.enabled) {
        // For audio devices, we need to create an audio stream
        if ((device.deviceId.find("real_audio_input_") == 0 || 
             device.deviceId.find("audio_input_") == 0) && audioDeviceIntegration_) {
            // Extract device index from ID
            std::string indexStr = device.deviceId.substr(device.deviceId.find_last_of('_') + 1);
            try {
                int deviceIndex = std::stoi(indexStr);
                // Create audio stream using public method
                if (audioDeviceIntegration_->createAudioInputStream(device.deviceId, deviceIndex)) {
                    // Update device status to connected
                    auto& status = deviceStatuses_[device.deviceId];
                    status.status = DeviceConnectionStatus::CONNECTED;
                    status.lastActivity = std::chrono::steady_clock::now();
                } else {
                    std::cerr << "❌ Failed to create audio stream for " << device.deviceId << std::endl;
                }
            } catch (const std::exception& e) {
                std::cerr << "Error parsing device index: " << e.what() << std::endl;
            }
        } else {
            // For network devices, create OSC receiver
            createOSCReceiver(device);
        }
    }
    
    std::cout << "Added input device '" << device.deviceName 
              << "' to channel " << (channelId + 1) << std::endl;
    
    // Check if we need to start the channel (check inside the lock)
    bool needToStartChannel = (channel->state != ChannelState::RUNNING);
    
    // Release the lock before calling startChannel to avoid deadlock
    lock.unlock();
    
    // Auto-start channel if it's not running (outside the lock)
    if (needToStartChannel) {
        std::cout << "🚀 Auto-starting channel " << (channelId + 1) << " after adding input device" << std::endl;
        startChannel(channelId);
    }
    
    return true;
}

bool OSCMixerEngine::addOutputDevice(int channelId, const OSCDeviceConfig& device) {
    if (!isChannelIdValid(channelId)) {
        return false;
    }
    
    if (!validateDeviceConfig(device)) {
        std::cerr << "Invalid device configuration for device: " << device.deviceId << std::endl;
        return false;
    }
    
    auto* channel = mixerState_.getChannel(channelId);
    if (!channel) {
        return false;
    }
    
    std::lock_guard<std::mutex> lock(stateMutex_);
    
    // Check if device already exists
    auto it = std::find_if(channel->outputDevices.begin(), channel->outputDevices.end(),
        [&device](const OSCDeviceConfig& existing) {
            return existing.deviceId == device.deviceId;
        });
    
    if (it != channel->outputDevices.end()) {
        std::cerr << "Output device already exists: " << device.deviceId << std::endl;
        return false;
    }
    
    // Add device
    if (!channel->addOutputDevice(device)) {
        std::cerr << "Channel " << channelId << " output device limit reached (8 max)" << std::endl;
        return false;
    }
    
    // Initialize device status
    {
        std::lock_guard<std::mutex> deviceLock(deviceMutex_);
        deviceStatuses_[device.deviceId] = DeviceStatus{
            .deviceId = device.deviceId,
            .status = DeviceConnectionStatus::DISCONNECTED,
            .lastActivity = std::chrono::steady_clock::now()
        };
    }
    
    // If channel is running, create the sender immediately
    if (channel->state == ChannelState::RUNNING && device.enabled) {
        // For audio devices, we need to create an audio stream
        if ((device.deviceId.find("real_audio_output_") == 0 || 
             device.deviceId.find("audio_output_") == 0) && audioDeviceIntegration_) {
            // Extract device index from ID or use the audioDeviceIndex field
            int deviceIndex = device.audioDeviceIndex;
            if (deviceIndex < 0) {
                // Try to extract from deviceId as fallback
                std::string indexStr = device.deviceId.substr(device.deviceId.find_last_of('_') + 1);
                try {
                    deviceIndex = std::stoi(indexStr);
                } catch (const std::exception& e) {
                    std::cerr << "Error parsing device index from ID: " << e.what() << std::endl;
                }
            }
            
            if (deviceIndex >= 0) {
                // Create audio stream using public method
                if (audioDeviceIntegration_->createAudioOutputStream(device.deviceId, deviceIndex)) {
                    // Update device status to connected
                    auto& status = deviceStatuses_[device.deviceId];
                    status.status = DeviceConnectionStatus::CONNECTED;
                    status.lastActivity = std::chrono::steady_clock::now();
                    std::cout << "✅ Created audio output stream for " << device.deviceName << std::endl;
                } else {
                    std::cerr << "❌ Failed to create audio output stream for " << device.deviceId << std::endl;
                }
            }
        } else {
            // For network devices, create OSC sender
            createOSCSender(device);
        }
    }
    
    std::cout << "Added output device '" << device.deviceName 
              << "' to channel " << (channelId + 1) << std::endl;
    
    return true;
}

bool OSCMixerEngine::removeInputDevice(int channelId, const std::string& deviceId) {
    if (!isChannelIdValid(channelId)) {
        return false;
    }
    
    auto* channel = mixerState_.getChannel(channelId);
    if (!channel) {
        return false;
    }
    
    std::lock_guard<std::mutex> lock(stateMutex_);
    
    // Clean up the device first
    cleanupDevice(deviceId);
    
    // Remove from channel
    channel->removeInputDevice(deviceId);
    
    // Remove device status
    {
        std::lock_guard<std::mutex> deviceLock(deviceMutex_);
        deviceStatuses_.erase(deviceId);
    }
    
    std::cout << "Removed input device '" << deviceId 
              << "' from channel " << (channelId + 1) << std::endl;
    
    // Auto-stop channel if no input devices left
    if (channel->inputDevices.empty() && channel->state == ChannelState::RUNNING) {
        std::cout << "🛑 Auto-stopping channel " << (channelId + 1) << " - no input devices" << std::endl;
        stopChannel(channelId);
    }
    
    return true;
}

bool OSCMixerEngine::removeOutputDevice(int channelId, const std::string& deviceId) {
    if (!isChannelIdValid(channelId)) {
        return false;
    }
    
    auto* channel = mixerState_.getChannel(channelId);
    if (!channel) {
        return false;
    }
    
    std::lock_guard<std::mutex> lock(stateMutex_);
    
    // Clean up the device first
    cleanupDevice(deviceId);
    
    // Remove from channel
    channel->removeOutputDevice(deviceId);
    
    // Remove device status
    {
        std::lock_guard<std::mutex> deviceLock(deviceMutex_);
        deviceStatuses_.erase(deviceId);
    }
    
    std::cout << "Removed output device '" << deviceId 
              << "' from channel " << (channelId + 1) << std::endl;
    
    return true;
}

bool OSCMixerEngine::updateDeviceConfig(const std::string& deviceId, const OSCDeviceConfig& newConfig) {
    if (!validateDeviceConfig(newConfig)) {
        return false;
    }
    
    // Use unique_lock to allow manual unlock before cleanupDevice
    std::unique_lock<std::mutex> lock(stateMutex_);
    
    // Find and update the device in all channels
    bool found = false;
    bool needsCleanup = false;
    bool isInput = false;
    ChannelState channelState = ChannelState::STOPPED;
    int channelIndex = -1;
    
    for (int i = 0; i < mixerState_.channels.size(); ++i) {
        auto& channel = mixerState_.channels[i];
        channelState = channel->state;
        
        // Check input devices
        for (auto& device : channel->inputDevices) {
            if (device.deviceId == deviceId) {
                needsCleanup = true;
                isInput = true;
                channelIndex = i;
                
                // Update configuration
                device = newConfig;
                device.deviceId = deviceId; // Preserve original ID
                
                found = true;
                break;
            }
        }
        
        if (found) break;
        
        // Check output devices
        for (auto& device : channel->outputDevices) {
            if (device.deviceId == deviceId) {
                needsCleanup = true;
                isInput = false;
                channelIndex = i;
                
                // Update configuration
                device = newConfig;
                device.deviceId = deviceId; // Preserve original ID
                
                found = true;
                break;
            }
        }
        
        if (found) break;
    }
    
    // Unlock before cleanup to avoid deadlock
    lock.unlock();
    
    // Clean up old configuration if needed
    if (needsCleanup) {
        // Small delay to ensure any ongoing operations complete
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        
        cleanupDevice(deviceId);
        
        // Another small delay to ensure old connections are fully closed
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        
        // Recreate device with new configuration if channel is running
        if (channelState == ChannelState::RUNNING && newConfig.enabled) {
            // Create new device connection
            if (isInput) {
                createOSCReceiver(newConfig);
            } else {
                createOSCSender(newConfig);
            }
        }
    }
    
    if (found) {
        std::cout << "Updated configuration for device: " << deviceId << std::endl;
    }
    
    return found;
}

// Device Discovery Methods
void OSCMixerEngine::startDeviceDiscovery() {
    if (mixerState_.scanningDevices) {
        return;
    }
    
    mixerState_.scanningDevices = true;
    discoveryThread_ = std::thread(&OSCMixerEngine::discoveryLoop, this);
    
    std::cout << "Started device discovery" << std::endl;
}

void OSCMixerEngine::stopDeviceDiscovery() {
    if (!mixerState_.scanningDevices) {
        return;
    }
    
    mixerState_.scanningDevices = false;
    
    if (discoveryThread_.joinable()) {
        discoveryThread_.join();
    }
    
    std::cout << "Stopped device discovery" << std::endl;
}

std::vector<std::string> OSCMixerEngine::getAvailableDevices() const {
    std::vector<std::string> audioDevices;
    if (audioDeviceIntegration_) {
        auto inputDevices = audioDeviceIntegration_->getAvailableInputDevices();
        auto outputDevices = audioDeviceIntegration_->getAvailableOutputDevices();
        for (const auto& device : inputDevices) {
            audioDevices.push_back(device.deviceName);
        }
        for (const auto& device : outputDevices) {
            audioDevices.push_back(device.deviceName);
        }
    }
    std::lock_guard<std::mutex> lock(const_cast<std::mutex&>(stateMutex_));
    return mixerState_.availableDevices;
}

// Connection Management Methods
bool OSCMixerEngine::connectDevice(const std::string& deviceId) {
    std::lock_guard<std::mutex> lock(deviceMutex_);
    
    auto it = deviceStatuses_.find(deviceId);
    if (it == deviceStatuses_.end()) {
        return false;
    }
    
    it->second.status = DeviceConnectionStatus::CONNECTING;
    it->second.lastActivity = std::chrono::steady_clock::now();
    
    // TODO: Implement actual connection logic
    
    return true;
}

bool OSCMixerEngine::disconnectDevice(const std::string& deviceId) {
    cleanupDevice(deviceId);
    
    std::lock_guard<std::mutex> lock(deviceMutex_);
    auto it = deviceStatuses_.find(deviceId);
    if (it != deviceStatuses_.end()) {
        it->second.status = DeviceConnectionStatus::DISCONNECTED;
        it->second.lastActivity = std::chrono::steady_clock::now();
    }
    
    return true;
}

DeviceStatus OSCMixerEngine::getDeviceStatus(const std::string& deviceId) const {
    std::lock_guard<std::mutex> lock(const_cast<std::mutex&>(deviceMutex_));
    
    auto it = deviceStatuses_.find(deviceId);
    if (it != deviceStatuses_.end()) {
        return it->second;
    }
    
    return DeviceStatus{.deviceId = deviceId, .status = DeviceConnectionStatus::DISCONNECTED};
}

std::vector<DeviceStatus> OSCMixerEngine::getAllDeviceStatuses() const {
    std::lock_guard<std::mutex> lock(const_cast<std::mutex&>(deviceMutex_));
    
    std::vector<DeviceStatus> statuses;
    statuses.reserve(deviceStatuses_.size());
    
    for (const auto& [deviceId, status] : deviceStatuses_) {
        statuses.push_back(status);
    }
    
    return statuses;
}

// Message Processing Methods
void OSCMixerEngine::sendOSCMessage(int channelId, const std::string& deviceId, float value) {
    // Find the output device to get the correct OSC address
    std::string oscAddress = "/channel/" + std::to_string(channelId + 1) + "/out";
    
    auto* channel = mixerState_.getChannel(channelId);
    if (channel) {
        for (const auto& outputDevice : channel->outputDevices) {
            if (outputDevice.deviceId == deviceId) {
                oscAddress = outputDevice.oscAddress;
                break;
            }
        }
    }
    
    OSCMessage message;
    message.address = oscAddress;
    message.floatValues = {value};
    message.type = OSCMessageType::FLOAT;
    message.sourceChannelId = channelId;
    message.deviceId = deviceId;
    message.timestamp = std::chrono::steady_clock::now();
    
    sendOSCMessage(channelId, deviceId, message);
}

void OSCMixerEngine::sendOSCMessage(int channelId, const std::string& deviceId, const OSCMessage& message) {
    std::lock_guard<std::mutex> lock(messageMutex_);
    messageQueue_.push(message);
    messageCondition_.notify_one();
    
    // Update statistics
    messagesThisSecond_++;
}

// Learning Mode Methods
void OSCMixerEngine::enableLearningMode(bool enabled) {
    std::lock_guard<std::mutex> lock(learningMutex_);
    learningMode_ = enabled;
    
    if (!enabled) {
        learningChannelId_ = -1;
        learningParameter_.clear();
    }
    
    std::cout << "Learning mode " << (enabled ? "enabled" : "disabled") << std::endl;
}


void OSCMixerEngine::setLearningTarget(int channelId, const std::string& parameter) {
    if (!isChannelIdValid(channelId)) {
        return;
    }
    
    std::lock_guard<std::mutex> lock(learningMutex_);
    learningChannelId_ = channelId;
    learningParameter_ = parameter;
    
    std::cout << "Learning target set: Channel " << (channelId + 1) 
              << ", Parameter: " << parameter << std::endl;
}

// Statistics Methods
void OSCMixerEngine::resetStatistics() {
    std::lock_guard<std::mutex> lock(stateMutex_);
    
    mixerState_.totalMessagesPerSecond = 0;
    mixerState_.totalErrors = 0;
    
    for (auto& channel : mixerState_.channels) {
        channel->messagesReceived = 0;
        channel->messagesSent = 0;
        channel->errors = 0;
    }
    
    std::lock_guard<std::mutex> deviceLock(deviceMutex_);
    for (auto& [deviceId, status] : deviceStatuses_) {
        status.messageCount = 0;
        status.latencyMs = 0.0f;
    }
    
    std::cout << "Statistics reset" << std::endl;
}

int OSCMixerEngine::getTotalMessagesPerSecond() const {
    return mixerState_.totalMessagesPerSecond;
}

int OSCMixerEngine::getTotalActiveConnections() const {
    return mixerState_.totalActiveConnections;
}

int OSCMixerEngine::getTotalErrors() const {
    return mixerState_.totalErrors;
}

// Configuration Methods
bool OSCMixerEngine::loadConfiguration(const std::string& filePath) {
    try {
        std::ifstream file(filePath);
        if (!file.is_open()) {
            std::cerr << "Cannot open configuration file: " << filePath << std::endl;
            return false;
        }
        
        json config;
        file >> config;
        
        // Stop all channels before loading new configuration
        for (int i = 0; i < 8; ++i) {
            stopChannel(i);
        }
        
        std::lock_guard<std::mutex> lock(stateMutex_);
        
        // Load mixer settings
        if (config.contains("mixer")) {
            auto mixer = config["mixer"];
            if (mixer.contains("masterLevel")) {
                mixerState_.masterLevel = mixer["masterLevel"];
            }
            if (mixer.contains("masterMute")) {
                mixerState_.masterMute = mixer["masterMute"];
            }
        }
        
        // Load channel configurations
        if (config.contains("channels")) {
            auto channels = config["channels"];
            for (size_t i = 0; i < channels.size() && i < mixerState_.channels.size(); ++i) {
                auto channelConfig = channels[i];
                auto* channel = mixerState_.getChannel(static_cast<int>(i));
                
                if (channel && channelConfig.contains("name")) {
                    channel->channelName = channelConfig["name"];
                }
                if (channel && channelConfig.contains("levelVolts")) {
                    channel->levelVolts = channelConfig["levelVolts"];
                }
                if (channel && channelConfig.contains("minRange")) {
                    channel->minRange = channelConfig["minRange"];
                }
                if (channel && channelConfig.contains("maxRange")) {
                    channel->maxRange = channelConfig["maxRange"];
                }
                if (channel && channelConfig.contains("color")) {
                    auto color = channelConfig["color"];
                    if (color.size() >= 3) {
                        channel->channelColor[0] = color[0];
                        channel->channelColor[1] = color[1];
                        channel->channelColor[2] = color[2];
                    }
                }
                
                // Load input devices
                if (channel && channelConfig.contains("inputDevices")) {
                    channel->inputDevices.clear();
                    for (const auto& deviceJson : channelConfig["inputDevices"]) {
                        auto device = deserializeDeviceConfig(deviceJson);
                        channel->addInputDevice(device);
                    }
                }
                
                // Load output devices
                if (channel && channelConfig.contains("outputDevices")) {
                    channel->outputDevices.clear();
                    for (const auto& deviceJson : channelConfig["outputDevices"]) {
                        auto device = deserializeDeviceConfig(deviceJson);
                        channel->addOutputDevice(device);
                    }
                }
            }
        }
        
        std::cout << "Configuration loaded from: " << filePath << std::endl;
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "Error loading configuration: " << e.what() << std::endl;
        return false;
    }
}

bool OSCMixerEngine::saveConfiguration(const std::string& filePath) {
    try {
        json config;
        
        std::lock_guard<std::mutex> lock(stateMutex_);
        
        // Save mixer settings
        config["version"] = "2.0.0";
        config["mixer"]["name"] = "Professional OSC Mixer";
        config["mixer"]["channels"] = 8;
        config["mixer"]["masterLevel"] = mixerState_.masterLevel;
        config["mixer"]["masterMute"] = mixerState_.masterMute;
        
        // Save channel configurations
        config["channels"] = json::array();
        for (const auto& channel : mixerState_.channels) {
            json channelConfig;
            channelConfig["id"] = channel->channelId;
            channelConfig["name"] = channel->channelName;
            channelConfig["levelVolts"] = channel->levelVolts;
            channelConfig["minRange"] = channel->minRange;
            channelConfig["maxRange"] = channel->maxRange;
            channelConfig["color"] = {channel->channelColor[0], channel->channelColor[1], channel->channelColor[2]};
            
            // Save input devices
            channelConfig["inputDevices"] = json::array();
            for (const auto& device : channel->inputDevices) {
                channelConfig["inputDevices"].push_back(serializeDeviceConfig(device));
            }
            
            // Save output devices
            channelConfig["outputDevices"] = json::array();
            for (const auto& device : channel->outputDevices) {
                channelConfig["outputDevices"].push_back(serializeDeviceConfig(device));
            }
            
            config["channels"].push_back(channelConfig);
        }
        
        // Write to file
        std::ofstream file(filePath);
        if (!file.is_open()) {
            std::cerr << "Cannot create configuration file: " << filePath << std::endl;
            return false;
        }
        
        file << config.dump(2);
        
        std::cout << "Configuration saved to: " << filePath << std::endl;
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "Error saving configuration: " << e.what() << std::endl;
        return false;
    }
}

// Private Methods Implementation

void OSCMixerEngine::engineLoop() {
    std::cout << "OSC Mixer Engine loop started" << std::endl;
    
    int loopCount = 0;
    while (engineRunning_) {
        try {
            loopCount++;
            
            // Debug log every 100 iterations
            if (loopCount % 100 == 0) {
                std::cout << "[OSCMixerEngine::engineLoop] Iteration #" << loopCount << std::endl;
                
                // Log channel states
                for (size_t i = 0; i < mixerState_.channels.size(); ++i) {
                    auto* channel = mixerState_.channels[i].get();
                    if (channel && channel->state == ChannelState::RUNNING) {
                        std::cout << "  Channel " << i << ": RUNNING, level=" 
                                  << channel->levelVolts << "V" << std::endl;
                    }
                }
            }
            
            // Process message queue
            processMessageQueue();
            
            // Update device statuses
            updateDeviceStatuses();
            
            // Update performance stats
            updatePerformanceStats();
            
            // Check for solo/mute changes
            updateSoloMixLogic();
            
            // Sleep briefly to avoid consuming too much CPU
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            
        } catch (const std::exception& e) {
            std::cerr << "Error in engine loop: " << e.what() << std::endl;
        }
    }
    
    std::cout << "OSC Mixer Engine loop stopped" << std::endl;
}

void OSCMixerEngine::discoveryLoop() {
    std::cout << "Device discovery loop started" << std::endl;
    
    while (mixerState_.scanningDevices) {
        try {
            // Simulate device discovery
            std::vector<std::string> discoveredDevices;
            
            // Add some example devices
            discoveredDevices.push_back("TouchDesigner@127.0.0.1:9000");
            discoveredDevices.push_back("Ableton Live@127.0.0.1:9001");
            discoveredDevices.push_back("Max/MSP@127.0.0.1:9002");
            discoveredDevices.push_back("Modular Synth@192.168.1.100:8000");
            
            // Update available devices list
            {
                std::lock_guard<std::mutex> lock(stateMutex_);
                mixerState_.availableDevices = discoveredDevices;
            }
            
            std::cout << "Discovered " << discoveredDevices.size() << " devices" << std::endl;
            
            // Wait before next scan
            std::this_thread::sleep_for(std::chrono::seconds(5));
            
        } catch (const std::exception& e) {
            std::cerr << "Error in discovery loop: " << e.what() << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }
    
    std::cout << "Device discovery loop stopped" << std::endl;
}

void OSCMixerEngine::processMessageQueue() {
    std::unique_lock<std::mutex> lock(messageMutex_);
    
    while (!messageQueue_.empty()) {
        OSCMessage message = messageQueue_.front();
        messageQueue_.pop();
        
        // Unlock while processing to avoid blocking
        lock.unlock();
        
        try {
            // Route the message based on type
            if (message.sourceChannelId >= 0) {
                routeOutputMessage(message);
            } else {
                routeInputMessage(message);
            }
        } catch (const std::exception& e) {
            std::cerr << "Error processing OSC message: " << e.what() << std::endl;
            handleDeviceError(message.deviceId, e.what());
        }
        
        lock.lock();
    }
}

void OSCMixerEngine::updateDeviceStatuses() {
    std::lock_guard<std::mutex> lock(deviceMutex_);
    
    auto now = std::chrono::steady_clock::now();
    int activeConnections = 0;
    
    for (auto& [deviceId, status] : deviceStatuses_) {
        // Check for timeouts
        auto timeSinceActivity = std::chrono::duration_cast<std::chrono::milliseconds>(
            now - status.lastActivity);
        
        if (timeSinceActivity.count() > 30000) { // 30 seconds timeout
            if (status.status == DeviceConnectionStatus::CONNECTED) {
                status.status = DeviceConnectionStatus::TIMEOUT;
                status.lastError = "Connection timeout";
            }
        }
        
        // Count active connections
        if (status.status == DeviceConnectionStatus::CONNECTED) {
            activeConnections++;
        }
    }
    
    mixerState_.totalActiveConnections = activeConnections;
}

void OSCMixerEngine::updatePerformanceStats() {
    auto now = std::chrono::steady_clock::now();
    auto timeSinceUpdate = std::chrono::duration_cast<std::chrono::seconds>(
        now - lastStatsUpdate_);
    
    if (timeSinceUpdate.count() >= 1) {
        // Update messages per second
        mixerState_.totalMessagesPerSecond = messagesThisSecond_.exchange(0);
        lastStatsUpdate_ = now;
        
        // Update channel statistics with continuous monitoring
        for (auto& channel : mixerState_.channels) {
            if (channel->state == ChannelState::RUNNING) {
                auto now = std::chrono::steady_clock::now();
                
                // Check for real OSC activity
                auto timeSinceInput = std::chrono::duration_cast<std::chrono::milliseconds>(
                    now - channel->inputMeter.lastUpdate);
                auto timeSinceOutput = std::chrono::duration_cast<std::chrono::milliseconds>(
                    now - channel->outputMeter.lastUpdate);
                
                // Process audio from connected input devices (real audio hardware)
                bool hasActiveInput = false;
                float inputSignal = 0.0f;
                
                // Get audio input from connected devices
                if (!channel->inputDevices.empty()) {
                    for (const auto& inputDevice : channel->inputDevices) {
                        if (inputDevice.enabled) {
// --- REAL AUDIO INPUT ---
hasActiveInput = true;
/* Patch: начинаем чтение истинного входа
   Подразумевается, что AudioDeviceIntegration реализована и может возвращать sample для данного inputDevice.deviceId */
if (audioDeviceIntegration_) {
    float realInput = audioDeviceIntegration_->getInputSample(inputDevice.deviceId);
    inputSignal = realInput;
} else {
    inputSignal = 0.0f; // fallback
}
break; // Use first enabled device
                        }
                    }
                }
                
                // Process OSC input messages
                if (timeSinceInput.count() <= 100) {
                    // We have recent OSC input, use it
                    hasActiveInput = true;
                    // inputSignal from OSC (current level from meter)
                    inputSignal = channel->inputMeter.getCurrentLevel();
                }
                
                // Process any active input (audio or OSC)
                if (hasActiveInput) {
                    // Pass signal at full volume - NO processing applied
                    // Signal goes from input to output at 100% strength
                    float processedSignal = inputSignal;
                    // REMOVED ALL PROCESSING:
                    // - No gain: processedSignal *= channel->controls.gainKnob;
                    // - No offset: processedSignal += channel->controls.offsetKnob;
                    // - No filter: processedSignal *= channel->controls.filterKnob;
                    // - No mix: processedSignal *= channel->controls.mixKnob;
                    // - No fader: processedSignal *= channel->getNormalizedLevel();
                    
                    // NO MUTE/SOLO/MASTER CONTROLS - signal passes at 100%
                    // REMOVED:
                    // - No mute: if (channel->isMuted()) processedSignal *= 0.1f;
                    // - No master level: processedSignal *= mixerState_.masterLevel;
                    // - No master mute: if (mixerState_.masterMute) processedSignal *= 0.1f;
                    // 
                    // processedSignal = inputSignal (100% passthrough)
                    
                    // Update meters with processed signals
                    channel->inputMeter.addSample(inputSignal);
                    channel->outputMeter.addSample(processedSignal);
                    
                    // ВАЖНО: Обновляем levelVolts для отображения в GUI
                    channel->levelVolts = processedSignal;
                    
                    // ДЕТАЛЬНАЯ ДИАГНОСТИКА: Логируем каждое обновление
                    if (processedSignal > 0.001) {
                        printf("[GUI UPDATE] Channel %d: Setting levelVolts=%.6fV (inputSignal=%.6fV)\n", 
                               channel->channelId, processedSignal, inputSignal);
                        
                        // Проверяем, действительно ли значение записалось
                        double verifyValue = channel->levelVolts;
                        printf("[GUI VERIFY] Channel %d: Read back levelVolts=%.6fV\n", 
                               channel->channelId, verifyValue);
                    }
                    
                    // Send to output devices if configured
                    if (!channel->outputDevices.empty()) {
                        for (const auto& outputDevice : channel->outputDevices) {
                            if (outputDevice.enabled) {
                                // Send to real audio output or OSC output
                                if (audioDeviceIntegration_ && 
                                    (outputDevice.deviceId.find("real_audio_output_") == 0 || 
                                     outputDevice.deviceId.find("audio_output_") == 0)) {
                                    audioDeviceIntegration_->sendOutputSample(outputDevice.deviceId, processedSignal);
                                } else {
                                    // Send OSC message
                                    sendOSCMessage(channel->channelId, outputDevice.deviceId, processedSignal);
                                }
                            }
                        }
                    }
                    
                    // Log signal processing periodically
                    static auto lastDebugLog = std::chrono::steady_clock::now();
                    auto currentTime = std::chrono::steady_clock::now();
                    auto timeSinceDebug = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - lastDebugLog);
                    if (timeSinceDebug.count() > 3000) { // Every 3 seconds
                        printf("🚀 Channel %d: Input %.2fV → Output %.2fV (100%% PASSTHROUGH - NO PROCESSING, devices=%zu)\n",
                               channel->channelId + 1, inputSignal, processedSignal, 
                               channel->inputDevices.size());
                        lastDebugLog = currentTime;
                    }
                } else {
                    // Decay peak levels gradually if no recent activity
                    if (timeSinceInput.count() > 100) {
                        channel->inputMeter.peakLevel *= 0.98f; // Slower decay for real signals
                    }
                    if (timeSinceOutput.count() > 100) {
                        channel->outputMeter.peakLevel *= 0.98f; // Slower decay for real signals
                    }
                }
            }
        }
    }
}

bool OSCMixerEngine::createOSCSender(const OSCDeviceConfig& config) {
    try {
        std::lock_guard<std::mutex> lock(deviceMutex_);
        
        // Create new OSC sender
        auto sender = std::make_unique<OSCSender>(config.networkAddress, std::to_string(config.port));
        
        oscSenders_[config.deviceId] = std::move(sender);
        
        // Update device status
        auto& status = deviceStatuses_[config.deviceId];
        status.status = DeviceConnectionStatus::CONNECTED;
        status.lastActivity = std::chrono::steady_clock::now();
        
        std::cout << "Created OSC sender for device: " << config.deviceId 
                  << " (" << config.networkAddress << ":" << config.port << ")" << std::endl;
        
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "Error creating OSC sender: " << e.what() << std::endl;
        handleDeviceError(config.deviceId, e.what());
        return false;
    }
}

bool OSCMixerEngine::createOSCReceiver(const OSCDeviceConfig& config) {
    try {
        std::lock_guard<std::mutex> lock(deviceMutex_);
        
        // Debug: Print the localPort value
        std::cout << "DEBUG: createOSCReceiver for device " << config.deviceId 
                  << " with localPort = " << config.localPort << std::endl;
        
        // Create new OSC receiver
        auto receiver = std::make_unique<OSCReceiver>(std::to_string(config.localPort));
        
        // Set up message callback
        receiver->setMessageCallback([this, config](const std::string& address, const std::vector<float>& values) {
            if (!values.empty()) {
                // Create OSC message
                OSCMessage message;
                message.address = address;
                message.floatValues = values;
                message.type = OSCMessageType::FLOAT;
                message.deviceId = config.deviceId;
                message.timestamp = std::chrono::steady_clock::now();
                
                // Add to message queue
                {
                    std::lock_guard<std::mutex> msgLock(messageMutex_);
                    messageQueue_.push(message);
                    messageCondition_.notify_one();
                }
                
                // Update device status
                {
                    std::lock_guard<std::mutex> devLock(deviceMutex_);
                    auto& status = deviceStatuses_[config.deviceId];
                    status.messageCount++;
                    status.lastActivity = std::chrono::steady_clock::now();
                }
            }
        });
        
        if (!receiver->start()) {
            std::cerr << "Failed to start OSC receiver for " << config.deviceId << std::endl;
            return false;
        }
        
        oscReceivers_[config.deviceId] = std::move(receiver);
        
        // Update device status
        auto& status = deviceStatuses_[config.deviceId];
        status.status = DeviceConnectionStatus::CONNECTED;
        status.lastActivity = std::chrono::steady_clock::now();
        
        std::cout << "Created OSC receiver for device: " << config.deviceId 
                  << " (port " << config.localPort << ")" << std::endl;
        
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "Error creating OSC receiver: " << e.what() << std::endl;
        handleDeviceError(config.deviceId, e.what());
        return false;
    }
}

void OSCMixerEngine::cleanupDevice(const std::string& deviceId) {
    std::lock_guard<std::mutex> lock(deviceMutex_);
    
    // Remove OSC sender
    auto senderIt = oscSenders_.find(deviceId);
    if (senderIt != oscSenders_.end()) {
        // Ensure sender is properly shut down before removal
        // This prevents hanging connections
        senderIt->second.reset();
        oscSenders_.erase(senderIt);
        std::cout << "Cleaned up OSC sender for device: " << deviceId << std::endl;
    }
    
    // Remove OSC receiver
    auto receiverIt = oscReceivers_.find(deviceId);
    if (receiverIt != oscReceivers_.end()) {
        // Properly stop receiver before removal to avoid hang
        if (receiverIt->second) {
            receiverIt->second->stop();
        }
        receiverIt->second.reset();
        oscReceivers_.erase(receiverIt);
        std::cout << "Cleaned up OSC receiver for device: " << deviceId << std::endl;
    }
    
    // For audio devices, remove audio stream
    if ((deviceId.find("audio_") == 0 || deviceId.find("real_audio_") == 0) && audioDeviceIntegration_) {
        audioDeviceIntegration_->removeAudioStream(deviceId);
        std::cout << "Cleaned up audio stream for device: " << deviceId << std::endl;
    }
}

void OSCMixerEngine::routeInputMessage(const OSCMessage& message) {
    int targetChannelId = -1;
    std::string address = message.address;
    
    // Parse OSC address to determine channel
    if (address.find("/channel/") != std::string::npos ||
        address.find("/ch/") != std::string::npos ||
        address.find("/cv/") != std::string::npos) {
        
        size_t lastSlash = address.find_last_of('/');
        if (lastSlash != std::string::npos && lastSlash + 1 < address.length()) {
            try {
                int channelNum = std::stoi(address.substr(lastSlash + 1));
                if (channelNum >= 1 && channelNum <= 8) {
                    targetChannelId = channelNum - 1; // Convert to 0-based
                }
            } catch (const std::exception&) {
                // Invalid channel number
            }
        }
    }
    
    if (targetChannelId >= 0 && !message.floatValues.empty()) {
        auto* channel = mixerState_.getChannel(targetChannelId);
        if (channel && channel->state == ChannelState::RUNNING) {
            float receivedValue = message.floatValues[0];
            
            // Update input meter with raw received value
            channel->inputMeter.addSample(receivedValue);
            channel->messagesReceived++;
            
            // 100% PASSTHROUGH - NO PROCESSING APPLIED
            // Signal passes from input to output unchanged
            float processedSignal = receivedValue;
            // REMOVED ALL PROCESSING:
            // - No gain: processedSignal *= channel->controls.gainKnob;
            // - No offset: processedSignal += channel->controls.offsetKnob;
            // - No filter: processedSignal *= channel->controls.filterKnob;
            // - No mix: processedSignal *= channel->controls.mixKnob;
            // - No fader: processedSignal *= channel->getNormalizedLevel();
            // - No mute: if (channel->isMuted()) processedSignal = 0.0f;
            // - No master: processedSignal *= mixerState_.masterLevel;
            // 
            // processedSignal = receivedValue (100% passthrough)
            
            // Update output meter and send to output devices
            if (!channel->outputDevices.empty()) {
                channel->outputMeter.addSample(processedSignal);
                
                // Send processed signal to all output devices
                for (const auto& outputDevice : channel->outputDevices) {
                    if (outputDevice.enabled) {
                        sendOSCMessage(targetChannelId, outputDevice.deviceId, processedSignal);
                    }
                }
            }
            
            // Learning mode
            if (learningMode_ && learningChannelId_ == targetChannelId) {
                std::cout << "Learning: Channel " << (targetChannelId + 1) 
                          << " mapped to OSC address: " << address << std::endl;
            }
        }
    }
}

void OSCMixerEngine::routeOutputMessage(const OSCMessage& message) {
    auto* channel = mixerState_.getChannel(message.sourceChannelId);
    if (!channel || channel->state != ChannelState::RUNNING) {
        return;
    }
    
    if (!shouldChannelBeAudible(message.sourceChannelId)) {
        return;
    }
    
    std::lock_guard<std::mutex> lock(deviceMutex_);
    
    // Check if this is a real audio output device
    if ((message.deviceId.find("real_audio_output_") == 0 || 
         message.deviceId.find("audio_output_") == 0) && audioDeviceIntegration_) {
        
        try {
            // 100% PASSTHROUGH - NO PROCESSING
            // Signal value passes unchanged to audio output
            float processedValue = message.floatValues.empty() ? 0.0f : message.floatValues[0];
            // REMOVED ALL PROCESSING:
            // - No fader: processedValue *= channel->getNormalizedLevel();
            // - No master level: processedValue *= mixerState_.masterLevel;
            // - No master mute: if (mixerState_.masterMute) processedValue = 0.0f;
            // 
            // processedValue = original signal (100% passthrough)
            
            // Send to audio output device
            bool success = audioDeviceIntegration_->sendOutputSample(message.deviceId, processedValue);
            
            if (success) {
                channel->messagesSent++;
                channel->outputMeter.addSample(processedValue);
                
                auto& status = deviceStatuses_[message.deviceId];
                status.messageCount++;
                status.lastActivity = std::chrono::steady_clock::now();
            } else {
                handleDeviceError(message.deviceId, "Failed to send audio output");
            }
            
        } catch (const std::exception& e) {
            handleDeviceError(message.deviceId, e.what());
        }
        
    } else {
        // Handle OSC output devices
        auto senderIt = oscSenders_.find(message.deviceId);
        if (senderIt == oscSenders_.end()) {
            return;
        }
        
        try {
            auto* sender = senderIt->second.get();
            
            // 100% PASSTHROUGH - NO PROCESSING
            // Signal value passes unchanged to OSC output
            float processedValue = message.floatValues.empty() ? 0.0f : message.floatValues[0];
            // REMOVED ALL PROCESSING:
            // - No fader: processedValue *= channel->getNormalizedLevel();
            // - No master level: processedValue *= mixerState_.masterLevel;
            // - No master mute: if (mixerState_.masterMute) processedValue = 0.0f;
            // 
            // processedValue = original signal (100% passthrough)
            
            // Send the message
            bool success = sender->sendFloat(message.address, processedValue);
            
            if (success) {
                channel->messagesSent++;
                channel->outputMeter.addSample(processedValue);
                
                auto& status = deviceStatuses_[message.deviceId];
                status.messageCount++;
                status.lastActivity = std::chrono::steady_clock::now();
            } else {
                handleDeviceError(message.deviceId, "Failed to send OSC message");
            }
            
        } catch (const std::exception& e) {
            handleDeviceError(message.deviceId, e.what());
        }
    }
}

void OSCMixerEngine::updateSoloMixLogic() {
    auto soloChannels = mixerState_.getSoloChannels();
    bool hasSolo = !soloChannels.empty();
    
    static bool wasInSolo = false;
    if (hasSolo != wasInSolo) {
        if (hasSolo) {
            std::cout << "Solo mode activated - " << soloChannels.size() << " channel(s) soloed" << std::endl;
        } else {
            std::cout << "Solo mode deactivated - all channels in mix mode" << std::endl;
        }
        wasInSolo = hasSolo;
    }
}

bool OSCMixerEngine::shouldChannelBeAudible(int channelId) const {
    auto* channel = mixerState_.getChannel(channelId);
    if (!channel || channel->state != ChannelState::RUNNING) {
        return false;
    }
    
    if (!mixerState_.hasSoloChannels()) {
        return true;
    }
    
    return channel->mode == ChannelMode::SOLO;
}

void OSCMixerEngine::handleDeviceError(const std::string& deviceId, const std::string& error) {
    std::lock_guard<std::mutex> lock(deviceMutex_);
    
    auto it = deviceStatuses_.find(deviceId);
    if (it != deviceStatuses_.end()) {
        it->second.status = DeviceConnectionStatus::ERROR;
        it->second.lastError = error;
        it->second.lastActivity = std::chrono::steady_clock::now();
    }
    
    mixerState_.totalErrors++;
    std::cerr << "Device error [" << deviceId << "]: " << error << std::endl;
}

void OSCMixerEngine::logError(const std::string& error) {
    std::cerr << "OSC Mixer Engine Error: " << error << std::endl;
    mixerState_.totalErrors++;
}

nlohmann::json OSCMixerEngine::serializeDeviceConfig(const OSCDeviceConfig& config) const {
    nlohmann::json deviceJson;
    
    deviceJson["deviceId"] = config.deviceId;
    deviceJson["deviceName"] = config.deviceName;
    deviceJson["protocolType"] = static_cast<int>(config.protocolType);
    deviceJson["networkAddress"] = config.networkAddress;
    deviceJson["port"] = config.port;
    deviceJson["localAddress"] = config.localAddress;
    deviceJson["localPort"] = config.localPort;
    deviceJson["oscAddress"] = config.oscAddress;
    deviceJson["oscMessage"] = config.oscMessage;
    deviceJson["signalLevel"] = config.signalLevel;
    deviceJson["enabled"] = config.enabled;
    
    deviceJson["supportedTypes"] = nlohmann::json::array();
    for (auto type : config.supportedTypes) {
        deviceJson["supportedTypes"].push_back(static_cast<int>(type));
    }
    
    return deviceJson;
}

OSCDeviceConfig OSCMixerEngine::deserializeDeviceConfig(const nlohmann::json& deviceJson) const {
    OSCDeviceConfig config;
    
    if (deviceJson.contains("deviceId")) config.deviceId = deviceJson["deviceId"];
    if (deviceJson.contains("deviceName")) config.deviceName = deviceJson["deviceName"];
    if (deviceJson.contains("protocolType")) config.protocolType = static_cast<OSCProtocolType>(deviceJson["protocolType"]);
    if (deviceJson.contains("networkAddress")) config.networkAddress = deviceJson["networkAddress"];
    if (deviceJson.contains("port")) config.port = deviceJson["port"];
    if (deviceJson.contains("localAddress")) config.localAddress = deviceJson["localAddress"];
    if (deviceJson.contains("localPort")) config.localPort = deviceJson["localPort"];
    if (deviceJson.contains("oscAddress")) config.oscAddress = deviceJson["oscAddress"];
    if (deviceJson.contains("enabled")) config.enabled = deviceJson["enabled"];
    
    if (deviceJson.contains("supportedTypes")) {
        config.supportedTypes.clear();
        for (auto typeInt : deviceJson["supportedTypes"]) {
            config.supportedTypes.push_back(static_cast<OSCMessageType>(typeInt));
        }
    }
    
    return config;
}

bool OSCMixerEngine::validateDeviceConfig(const OSCDeviceConfig& config) const {
    if (config.deviceId.empty()) {
        std::cerr << "Device ID cannot be empty" << std::endl;
        return false;
    }
    
    if (config.networkAddress.empty()) {
        std::cerr << "Network address cannot be empty" << std::endl;
        return false;
    }
    
    if (config.port <= 0 || config.port > 65535) {
        std::cerr << "Invalid port number: " << config.port << std::endl;
        return false;
    }
    
    if (config.oscAddress.empty() || config.oscAddress[0] != '/') {
        std::cerr << "OSC address must start with '/'" << std::endl;
        return false;
    }
    
    if (config.supportedTypes.empty()) {
        std::cerr << "Device must support at least one message type" << std::endl;
        return false;
    }
    
    return true;
}

bool OSCMixerEngine::isChannelIdValid(int channelId) const {
    return channelId >= 0 && channelId < 8;
}

bool OSCMixerEngine::isDeviceIdValid(const std::string& deviceId) const {
    return !deviceId.empty() && deviceId.length() <= 256;
}

// Additional mixer control implementations
bool OSCMixerEngine::start() {
    if (engineRunning_) {
        std::cout << "OSC Mixer Engine already running" << std::endl;
        return true;
    }
    
    try {
        std::cout << "Starting OSC Mixer Engine..." << std::endl;
        
        // Don't reset mixer state - preserve channel configurations
        // Just start the engine thread
        engineRunning_ = true;
        engineThread_ = std::thread(&OSCMixerEngine::engineLoop, this);
        
        // Start any channels that were previously active
        for (auto& channel : mixerState_.channels) {
            if (channel->state == ChannelState::RUNNING) {
                // Re-initialize devices for active channels
                for (const auto& deviceConfig : channel->inputDevices) {
                    if (deviceConfig.enabled) {
                        createOSCReceiver(deviceConfig);
                    }
                }
                for (const auto& deviceConfig : channel->outputDevices) {
                    if (deviceConfig.enabled) {
                        createOSCSender(deviceConfig);
                    }
                }
            }
        }
        
        std::cout << "OSC Mixer Engine started successfully" << std::endl;
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "Failed to start OSC Mixer Engine: " << e.what() << std::endl;
        engineRunning_ = false;
        return false;
    }
}

void OSCMixerEngine::stop() {
    if (!engineRunning_) {
        std::cout << "OSC Mixer Engine already stopped" << std::endl;
        return;
    }
    
    std::cout << "Stopping OSC Mixer Engine..." << std::endl;
    
    // Stop engine thread but don't reset channel states
    engineRunning_ = false;
    stateCondition_.notify_all();
    messageCondition_.notify_all();
    
    if (engineThread_.joinable()) {
        engineThread_.join();
    }
    
    // Clean up network connections but preserve channel configurations
    {
        std::lock_guard<std::mutex> lock(deviceMutex_);
        oscSenders_.clear();
        oscReceivers_.clear();
    }
    
    std::cout << "OSC Mixer Engine stopped" << std::endl;
}

bool OSCMixerEngine::isSoloMode() const {
    std::lock_guard<std::mutex> lock(const_cast<std::mutex&>(stateMutex_));
    return mixerState_.hasSoloChannels();
}

void OSCMixerEngine::setSoloMode(bool solo) {
    std::lock_guard<std::mutex> lock(stateMutex_);
    // If disabling solo mode, clear all solo channels
    if (!solo) {
        for (auto& channel : mixerState_.channels) {
            if (channel->mode == ChannelMode::SOLO) {
                channel->mode = ChannelMode::MIX;
            }
        }
    }
    updateSoloMixLogic();
}

void OSCMixerEngine::setMasterVolume(float volume) {
    std::lock_guard<std::mutex> lock(stateMutex_);
    mixerState_.masterLevel = std::clamp(volume, 0.0f, 1.0f);
}

void OSCMixerEngine::setMasterMute(bool mute) {
    std::lock_guard<std::mutex> lock(stateMutex_);
    mixerState_.masterMute = mute;
}

void OSCMixerEngine::setChannelSolo(int channelId, bool solo) {
    if (!isChannelIdValid(channelId)) return;
    
    auto* channel = mixerState_.getChannel(channelId);
    if (channel) {
        std::lock_guard<std::mutex> lock(stateMutex_);
        channel->mode = solo ? ChannelMode::SOLO : ChannelMode::MIX;
        updateSoloMixLogic();
    }
}

void OSCMixerEngine::setChannelMute(int channelId, bool mute) {
    if (!isChannelIdValid(channelId)) return;
    
    auto* channel = mixerState_.getChannel(channelId);
    if (channel) {
        std::lock_guard<std::mutex> lock(stateMutex_);
        channel->mode = mute ? ChannelMode::MUTE : ChannelMode::MIX;
        updateSoloMixLogic();
    }
}

float OSCMixerEngine::getChannelLevel(int channelId) const {
    if (!isChannelIdValid(channelId)) return 0.0f;
    
    auto* channel = mixerState_.getChannel(channelId);
    return channel ? channel->levelVolts : 0.0f;
}

bool OSCMixerEngine::isChannelMuted(int channelId) const {
    if (!isChannelIdValid(channelId)) return false;
    
    auto* channel = mixerState_.getChannel(channelId);
    return channel ? (channel->mode == ChannelMode::MUTE) : false;
}

bool OSCMixerEngine::isChannelSolo(int channelId) const {
    if (!isChannelIdValid(channelId)) return false;
    
    auto* channel = mixerState_.getChannel(channelId);
    return channel ? (channel->mode == ChannelMode::SOLO) : false;
}
