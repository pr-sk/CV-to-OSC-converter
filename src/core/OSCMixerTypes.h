#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <atomic>
#include <deque>
#include <chrono>

// OSC Protocol Types
enum class OSCProtocolType {
    UDP_UNICAST,
    UDP_MULTICAST,
    TCP
};

enum class OSCMessageType {
    FLOAT,
    INT,
    STRING,
    BLOB,
    BUNDLE
};

enum class OSCDeviceType {
    AUDIO_INPUT,
    AUDIO_OUTPUT,
    MIDI_INPUT,
    MIDI_OUTPUT,
    CV_INPUT,
    CV_OUTPUT,
    OSC_INPUT,
    OSC_OUTPUT,
    VIRTUAL
};

// OSC Device Configuration
struct OSCDeviceConfig {
    std::string deviceId;
    std::string deviceName;
    std::string description = "";
    OSCDeviceType deviceType = OSCDeviceType::OSC_INPUT;
    OSCProtocolType protocolType = OSCProtocolType::UDP_UNICAST;
    std::string networkAddress = "127.0.0.1";
    int port = 9000;
    std::string localAddress = "0.0.0.0";
    int localPort = 0; // 0 = auto assign
    std::string oscAddress = "/channel/1";
    std::string oscMessage = "";
    float signalLevel = 1.0f;
    float signalOffset = 0.0f;
    bool invertSignal = false;
    bool enabled = true;
    bool connected = false;
    bool autoReconnect = true;
    
    // Advanced OSC settings
    std::string namespace_ = "";
    std::string pattern = "";
    std::vector<OSCMessageType> supportedTypes;
    int maxBundleSize = 1024;
    int timeout = 5000; // ms
    int timeoutMs = 5000; // ms (alternative naming)
    int maxRetries = 3;
    int bufferSize = 8192;
    bool useTimestamps = false;
    bool useTimeTag = false; // alternative naming
    bool useBundles = false;
    
    // Audio device integration
    int audioDeviceIndex = -1; // PortAudio device index for real audio devices
    
    OSCDeviceConfig() {
        supportedTypes = {OSCMessageType::FLOAT, OSCMessageType::INT};
    }
    
    // Get connection string for display
    std::string getConnectionString() const {
        return networkAddress + ":" + std::to_string(port);
    }
};

// Signal Level Meter
struct SignalMeter {
    std::deque<float> levelHistory;
    float currentLevel = 0.0f;
    float peakLevel = 0.0f;
    float rmsLevel = 0.0f;
    std::chrono::steady_clock::time_point lastUpdate;
    static const size_t HISTORY_SIZE = 100;
    
    void addSample(float level) {
        currentLevel = level;
        levelHistory.push_back(level);
        if (levelHistory.size() > HISTORY_SIZE) {
            levelHistory.pop_front();
        }
        
        // Calculate peak and RMS
        peakLevel = std::max(peakLevel, std::abs(level));
        
        float sum = 0.0f;
        for (float val : levelHistory) {
            sum += val * val;
        }
        rmsLevel = std::sqrt(sum / levelHistory.size());
        
        lastUpdate = std::chrono::steady_clock::now();
    }
    
    void reset() {
        levelHistory.clear();
        currentLevel = 0.0f;
        peakLevel = 0.0f;
        rmsLevel = 0.0f;
        lastUpdate = std::chrono::steady_clock::now();
    }
    
    // Get peak-program-meter level for display
    float getPPMLevel() const {
        return peakLevel;
    }
    
    // Get current level for digital display
    float getCurrentLevel() const {
        return currentLevel;
    }
    
    // Get RMS level for averaging
    float getRMSLevel() const {
        return rmsLevel;
    }
};

// Channel States
enum class ChannelState {
    STOPPED,
    RUNNING,
    ERROR
};

enum class ChannelMode {
    MIX,
    SOLO,
    MUTE
};

// Channel Controls Structure
struct ChannelControls {
    float gainKnob = 1.0f;
    float offsetKnob = 0.0f;
    float filterKnob = 1.0f;
    float mixKnob = 1.0f;
    bool muteButton = false;
    bool soloButton = false;
    bool linkButton = false;
};

// Mixer Channel
struct MixerChannel {
    int channelId;
    std::string channelName;
    
    // Input Devices (OSC Receivers IN)
    std::vector<OSCDeviceConfig> inputDevices; // Up to 8 devices
    
    // Output Devices (OSC Senders OUT)
    std::vector<OSCDeviceConfig> outputDevices; // Up to 8 devices
    
    // Channel Controls
    float levelVolts = 0.0f;        // Level slider in Volts
    float minRange = -10.0f;        // Min voltage range
    float maxRange = 10.0f;         // Max voltage range
    ChannelControls controls;       // All channel control knobs and buttons
    
    // Channel State
    ChannelState state = ChannelState::STOPPED;
    ChannelMode mode = ChannelMode::MIX;
    
    // Signal Monitoring
    SignalMeter inputMeter;
    SignalMeter outputMeter;
    
    // Visual Settings
    float channelColor[3] = {0.2f, 0.8f, 0.2f}; // Green default
    bool showInMaster = true;
    
    // Statistics
    std::atomic<int> messagesReceived{0};
    std::atomic<int> messagesSent{0};
    std::atomic<int> errors{0};
    
    MixerChannel(int id) : channelId(id) {
        channelName = "Channel " + std::to_string(id + 1);
        
        // Reserve space for up to 8 devices each
        inputDevices.reserve(8);
        outputDevices.reserve(8);
    }
    
    // Add input device
    bool addInputDevice(const OSCDeviceConfig& device) {
        if (inputDevices.size() < 8) {
            inputDevices.push_back(device);
            return true;
        }
        return false;
    }
    
    // Add output device
    bool addOutputDevice(const OSCDeviceConfig& device) {
        if (outputDevices.size() < 8) {
            outputDevices.push_back(device);
            return true;
        }
        return false;
    }
    
    // Remove devices
    void removeInputDevice(const std::string& deviceId) {
        inputDevices.erase(
            std::remove_if(inputDevices.begin(), inputDevices.end(),
                [&deviceId](const OSCDeviceConfig& dev) { return dev.deviceId == deviceId; }),
            inputDevices.end());
    }
    
    void removeOutputDevice(const std::string& deviceId) {
        outputDevices.erase(
            std::remove_if(outputDevices.begin(), outputDevices.end(),
                [&deviceId](const OSCDeviceConfig& dev) { return dev.deviceId == deviceId; }),
            outputDevices.end());
    }
    
    // Get normalized level (0.0 - 1.0)
    float getNormalizedLevel() const {
        return (levelVolts - minRange) / (maxRange - minRange);
    }
    
    // Set level from normalized value
    void setNormalizedLevel(float normalized) {
        levelVolts = minRange + normalized * (maxRange - minRange);
    }
    
    // State query methods
    bool isRunning() const {
        return state == ChannelState::RUNNING;
    }
    
    bool isMuted() const {
        return mode == ChannelMode::MUTE || controls.muteButton;
    }
    
    bool isSolo() const {
        return mode == ChannelMode::SOLO || controls.soloButton;
    }
};

// Master Mixer State
struct MasterMixerState {
    std::vector<std::unique_ptr<MixerChannel>> channels;
    
    // Global settings
    float masterLevel = 1.0f;
    bool masterMute = false;
    
    // Performance monitoring
    std::atomic<int> totalMessagesPerSecond{0};
    std::atomic<int> totalActiveConnections{0};
    std::atomic<int> totalErrors{0};
    
    // Device discovery
    std::vector<std::string> availableDevices;
    bool scanningDevices = false;
    
    MasterMixerState() {
        // Initialize 8 channels
        channels.reserve(8);
        for (int i = 0; i < 8; ++i) {
            channels.push_back(std::make_unique<MixerChannel>(i));
        }
    }
    
    MixerChannel* getChannel(int channelId) {
        if (channelId >= 0 && channelId < static_cast<int>(channels.size())) {
            return channels[channelId].get();
        }
        return nullptr;
    }
    
    const MixerChannel* getChannel(int channelId) const {
        if (channelId >= 0 && channelId < static_cast<int>(channels.size())) {
            return channels[channelId].get();
        }
        return nullptr;
    }
    
    // Get all channels in solo mode
    std::vector<int> getSoloChannels() const {
        std::vector<int> soloChannels;
        for (const auto& channel : channels) {
            if (channel->mode == ChannelMode::SOLO && channel->state == ChannelState::RUNNING) {
                soloChannels.push_back(channel->channelId);
            }
        }
        return soloChannels;
    }
    
    // Check if any channel is in solo mode
    bool hasSoloChannels() const {
        return !getSoloChannels().empty();
    }
    
    // Get running channels
    std::vector<int> getRunningChannels() const {
        std::vector<int> runningChannels;
        for (const auto& channel : channels) {
            if (channel->isRunning()) {
                runningChannels.push_back(channel->channelId);
            }
        }
        return runningChannels;
    }
    
    // Get total device count
    int getTotalDeviceCount() const {
        int count = 0;
        for (const auto& channel : channels) {
            count += static_cast<int>(channel->inputDevices.size() + channel->outputDevices.size());
        }
        return count;
    }
    
    // Set total channels (dummy method for compatibility)
    void setTotalChannels(int numChannels) {
        // This method is provided for compatibility but
        // actual channel count is managed by channels vector
        (void)numChannels; // Suppress unused parameter warning
    }
};

// OSC Message Queue for thread-safe communication
struct OSCMessage {
    std::string address;
    std::vector<float> floatValues;
    std::vector<int> intValues;
    std::vector<std::string> stringValues;
    OSCMessageType type;
    std::chrono::steady_clock::time_point timestamp;
    int sourceChannelId;
    int targetChannelId;
    std::string deviceId;
};

// Device Connection Status
enum class DeviceConnectionStatus {
    DISCONNECTED,
    CONNECTING,
    CONNECTED,
    ERROR,
    TIMEOUT
};

struct DeviceStatus {
    std::string deviceId;
    DeviceConnectionStatus status = DeviceConnectionStatus::DISCONNECTED;
    std::string lastError;
    std::chrono::steady_clock::time_point lastActivity;
    int messageCount = 0;
    float latencyMs = 0.0f;
    
    // Status query methods
    bool isConnected() const {
        return status == DeviceConnectionStatus::CONNECTED;
    }
    
    bool hasError() const {
        return status == DeviceConnectionStatus::ERROR || status == DeviceConnectionStatus::TIMEOUT;
    }
};
