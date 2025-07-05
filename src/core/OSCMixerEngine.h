#pragma once

#include "OSCMixerTypes.h"
#include "OSCSender.h"
#include "OSCReceiver.h"
#include "AudioDeviceIntegration.h"
#include <thread>
#include <mutex>
#include <queue>
#include <condition_variable>
#include <unordered_map>

class OSCMixerEngine {
public:
    OSCMixerEngine();
    explicit OSCMixerEngine(int numChannels);
    ~OSCMixerEngine();
    
    // Lifecycle
    bool initialize();
    void shutdown();
    bool isRunning() const { return engineRunning_; }
    
    // Mixer State
    MasterMixerState* getMixerState() { return &mixerState_; }
    const MasterMixerState* getMixerState() const { return &mixerState_; }
    
    // Channel Control
    bool startChannel(int channelId);
    bool stopChannel(int channelId);
    bool setChannelLevel(int channelId, float levelVolts);
    bool setChannelMode(int channelId, ChannelMode mode);
    bool setChannelRange(int channelId, float minRange, float maxRange);
    
    // Device Management
    bool addInputDevice(int channelId, const OSCDeviceConfig& device);
    bool addOutputDevice(int channelId, const OSCDeviceConfig& device);
    bool removeInputDevice(int channelId, const std::string& deviceId);
    bool removeOutputDevice(int channelId, const std::string& deviceId);
    bool updateDeviceConfig(const std::string& deviceId, const OSCDeviceConfig& newConfig);
    
    // Device Discovery
    void startDeviceDiscovery();
    void stopDeviceDiscovery();
    std::vector<std::string> getAvailableDevices() const;
    
    // Audio Device Integration
    void setAudioDeviceIntegration(std::shared_ptr<AudioDeviceIntegration> integration);
    std::vector<OSCDeviceConfig> getAvailableInputDevices() const;
    std::vector<OSCDeviceConfig> getAvailableOutputDevices() const;
    
    // Connection Management
    bool connectDevice(const std::string& deviceId);
    bool disconnectDevice(const std::string& deviceId);
    DeviceStatus getDeviceStatus(const std::string& deviceId) const;
    std::vector<DeviceStatus> getAllDeviceStatuses() const;
    
    // Message Processing
    void sendOSCMessage(int channelId, const std::string& deviceId, float value);
    void sendOSCMessage(int channelId, const std::string& deviceId, const OSCMessage& message);
    
    // Learning Mode for MIDI/OSC mapping
    void enableLearningMode(bool enabled);
    bool isLearningModeEnabled() const { return learningMode_; }
    void setLearningTarget(int channelId, const std::string& parameter);
    
    // Statistics
    void resetStatistics();
    int getTotalMessagesPerSecond() const;
    int getTotalActiveConnections() const;
    int getTotalErrors() const;
    
    // Configuration
    bool loadConfiguration(const std::string& filePath);
    bool saveConfiguration(const std::string& filePath);
    
    // Additional mixer controls
    bool start();
    void stop();
    bool isSoloMode() const;
    void setSoloMode(bool solo);
    void setMasterVolume(float volume);
    void setMasterMute(bool mute);
    void setChannelSolo(int channelId, bool solo);
    void setChannelMute(int channelId, bool mute);
    float getChannelLevel(int channelId) const;
    bool isChannelMuted(int channelId) const;
    bool isChannelSolo(int channelId) const;
    
private:
    // Core state
    MasterMixerState mixerState_;
    std::atomic<bool> engineRunning_{false};
    
    // Threading
    std::thread engineThread_;
    std::thread discoveryThread_;
    std::mutex stateMutex_;
    std::condition_variable stateCondition_;
    
    // OSC Communication
    std::unordered_map<std::string, std::unique_ptr<OSCSender>> oscSenders_;
    std::unordered_map<std::string, std::unique_ptr<OSCReceiver>> oscReceivers_;
    
    // Message Queue
    std::queue<OSCMessage> messageQueue_;
    std::mutex messageMutex_;
    std::condition_variable messageCondition_;
    
    // Device Status Tracking
    std::unordered_map<std::string, DeviceStatus> deviceStatuses_;
    std::mutex deviceMutex_;
    
    // Audio Device Integration
    std::shared_ptr<AudioDeviceIntegration> audioDeviceIntegration_;
    
    // Learning Mode
    std::atomic<bool> learningMode_{false};
    int learningChannelId_ = -1;
    std::string learningParameter_;
    std::mutex learningMutex_;
    
    // Performance Monitoring
    std::chrono::steady_clock::time_point lastStatsUpdate_;
    std::atomic<int> messagesThisSecond_{0};
    
    // Core Engine Methods
    void engineLoop();
    void discoveryLoop();
    void processMessageQueue();
    void updateDeviceStatuses();
    void updatePerformanceStats();
    
    // Device Management Internal
    bool createOSCSender(const OSCDeviceConfig& config);
    bool createOSCReceiver(const OSCDeviceConfig& config);
    void cleanupDevice(const std::string& deviceId);
    
    // Message Routing
    void routeInputMessage(const OSCMessage& message);
    void routeOutputMessage(const OSCMessage& message);
    
    // Solo/Mix Logic
    void updateSoloMixLogic();
    bool shouldChannelBeAudible(int channelId) const;
    
    // Error Handling
    void handleDeviceError(const std::string& deviceId, const std::string& error);
    void logError(const std::string& error);
    
    // Configuration Helpers
    nlohmann::json serializeDeviceConfig(const OSCDeviceConfig& config) const;
    OSCDeviceConfig deserializeDeviceConfig(const nlohmann::json& json) const;
    
    // Validation
    bool validateDeviceConfig(const OSCDeviceConfig& config) const;
    bool isChannelIdValid(int channelId) const;
    bool isDeviceIdValid(const std::string& deviceId) const;
};
