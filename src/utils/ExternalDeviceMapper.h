#pragma once

#include <string>
#include <vector>
#include <map>
#include <functional>
#include <memory>
#include <mutex>
#include <atomic>
#include <thread>
#include <chrono>
#include "../core/OSCMixerTypes.h"

// External Device Mapping Types
enum class ExternalDeviceType {
    MIDI_CONTROLLER,
    OSC_CONTROLLER,
    KEYBOARD_SHORTCUT,
    TOUCH_OSC,
    LEMUR,
    CUSTOM_PROTOCOL
};

enum class MappingParameterType {
    CHANNEL_LEVEL,
    CHANNEL_MUTE,
    CHANNEL_SOLO,
    MASTER_LEVEL,
    MASTER_MUTE,
    DEVICE_SELECT,
    LEARNING_MODE_TOGGLE,
    CUSTOM_PARAMETER
};

// External Device Mapping Configuration
struct ExternalDeviceMapping {
    std::string mappingId;
    std::string deviceId;
    ExternalDeviceType deviceType;
    
    // Input parameters (what triggers the mapping)
    std::string inputAddress;          // OSC address or MIDI CC number
    std::string inputPattern;          // Pattern matching for complex inputs
    float inputMin = 0.0f;            // Input range minimum
    float inputMax = 1.0f;            // Input range maximum
    
    // Output parameters (what gets controlled)
    MappingParameterType parameterType;
    int targetChannelId = -1;         // -1 for global parameters
    std::string targetDeviceId;       // Target device ID if applicable
    float outputMin = 0.0f;           // Output range minimum
    float outputMax = 1.0f;           // Output range maximum
    
    // Mapping behavior
    bool bidirectional = false;       // If true, changes propagate back to device
    bool inverted = false;            // Invert the mapping
    std::string customScript;         // Custom transformation script
    
    // Metadata
    std::string name;
    std::string description;
    bool enabled = true;
    
    ExternalDeviceMapping() {
        mappingId = "mapping_" + std::to_string(arc4random() % 100000);
    }
};

// Learning Mode Configuration
struct LearningModeConfig {
    bool enabled = false;
    MappingParameterType targetParameter;
    int targetChannelId = -1;
    std::string targetDeviceId;
    std::chrono::milliseconds timeout = std::chrono::milliseconds(10000);
    bool autoCommit = true;
    
    // Callback for when learning is complete
    std::function<void(const ExternalDeviceMapping&)> onLearningComplete;
    std::function<void(const std::string&)> onLearningTimeout;
    std::function<void(const std::string&)> onLearningError;
};

/**
 * @brief External Device Mapper manages mappings between external controllers and mixer parameters
 */
class ExternalDeviceMapper {
public:
    ExternalDeviceMapper();
    ~ExternalDeviceMapper();
    
    // Lifecycle
    bool initialize();
    void shutdown();
    
    // Device Registration
    bool registerDevice(const std::string& deviceId, ExternalDeviceType type);
    bool unregisterDevice(const std::string& deviceId);
    std::vector<std::string> getRegisteredDevices() const;
    
    // Mapping Management
    bool addMapping(const ExternalDeviceMapping& mapping);
    bool removeMapping(const std::string& mappingId);
    bool updateMapping(const std::string& mappingId, const ExternalDeviceMapping& mapping);
    ExternalDeviceMapping* getMapping(const std::string& mappingId);
    std::vector<ExternalDeviceMapping> getAllMappings() const;
    std::vector<ExternalDeviceMapping> getMappingsForDevice(const std::string& deviceId) const;
    std::vector<ExternalDeviceMapping> getMappingsForParameter(MappingParameterType parameterType, int channelId = -1) const;
    
    // Learning Mode
    bool startLearning(const LearningModeConfig& config);
    bool stopLearning();
    bool isLearningActive() const { return learningActive_; }
    LearningModeConfig getCurrentLearningConfig() const { return currentLearningConfig_; }
    
    // Input Processing
    void processOSCInput(const std::string& address, const std::vector<float>& values);
    void processMIDIInput(int channel, int cc, int value);
    void processKeyboardInput(const std::string& keyCode, bool pressed);
    void processCustomInput(const std::string& deviceId, const std::string& parameter, float value);
    
    // Output Callbacks
    void setOSCOutputCallback(std::function<void(const std::string&, const std::vector<float>&)> callback);
    void setMIDIOutputCallback(std::function<void(int, int, int)> callback);
    void setParameterChangeCallback(std::function<void(MappingParameterType, int, float)> callback);
    
    // Configuration
    bool loadMappings(const std::string& filePath);
    bool saveMappings(const std::string& filePath);
    void clearAllMappings();
    
    // Statistics
    int getTotalMappings() const;
    int getActiveMappings() const;
    int getProcessedInputsPerSecond() const;
    
    // Validation
    bool validateMapping(const ExternalDeviceMapping& mapping) const;
    std::vector<std::string> getMappingErrors(const ExternalDeviceMapping& mapping) const;
    
private:
    // Core state
    std::map<std::string, ExternalDeviceMapping> mappings_;
    std::map<std::string, ExternalDeviceType> registeredDevices_;
    std::mutex mappingsMutex_;
    
    // Learning mode
    std::atomic<bool> learningActive_{false};
    LearningModeConfig currentLearningConfig_;
    std::chrono::steady_clock::time_point learningStartTime_;
    std::mutex learningMutex_;
    
    // Callbacks
    std::function<void(const std::string&, const std::vector<float>&)> oscOutputCallback_;
    std::function<void(int, int, int)> midiOutputCallback_;
    std::function<void(MappingParameterType, int, float)> parameterChangeCallback_;
    
    // Performance monitoring
    std::atomic<int> inputsProcessedThisSecond_{0};
    std::chrono::steady_clock::time_point lastStatsUpdate_;
    
    // Internal methods
    void processMapping(const ExternalDeviceMapping& mapping, float inputValue);
    float transformValue(float input, float inputMin, float inputMax, float outputMin, float outputMax, bool inverted) const;
    bool matchesPattern(const std::string& input, const std::string& pattern) const;
    void triggerParameterChange(MappingParameterType parameterType, int channelId, const std::string& deviceId, float value);
    void updateLearningMode();
    void completeLearning(const ExternalDeviceMapping& mapping);
    void timeoutLearning();
    
    // Validation helpers
    bool isValidOSCAddress(const std::string& address) const;
    bool isValidMIDICC(const std::string& cc) const;
    bool isValidParameterType(MappingParameterType type) const;
    bool isValidChannelId(int channelId) const;
    
    // Threading
    void startProcessingThread();
    void stopProcessingThread();
    std::atomic<bool> processingThreadRunning_{false};
    std::thread processingThread_;
};

// Utility functions for mapping management
std::string mappingParameterTypeToString(MappingParameterType type);
MappingParameterType stringToMappingParameterType(const std::string& str);
std::string externalDeviceTypeToString(ExternalDeviceType type);
ExternalDeviceType stringToExternalDeviceType(const std::string& str);

// Preset management for common device mappings
class DeviceMappingPresets {
public:
    static std::vector<ExternalDeviceMapping> getTouchOSCMixerPreset();
    static std::vector<ExternalDeviceMapping> getLemurMixerPreset();
    static std::vector<ExternalDeviceMapping> getMIDIControllerPreset();
    static std::vector<ExternalDeviceMapping> getKeyboardShortcutsPreset();
    
    static bool savePreset(const std::string& name, const std::vector<ExternalDeviceMapping>& mappings);
    static std::vector<ExternalDeviceMapping> loadPreset(const std::string& name);
    static std::vector<std::string> getAvailablePresets();
    static bool deletePreset(const std::string& name);
};
