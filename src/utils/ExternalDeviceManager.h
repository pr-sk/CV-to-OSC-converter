#pragma once

#include <map>
#include <vector>
#include <string>
#include <functional>
#include <memory>
#include <mutex>
#include "OSCReceiver.h"

// Forward declarations

// External device controller types
enum class ControllerType {
    MIDI_CC,
    OSC_MESSAGE,
    KEYBOARD_SHORTCUT
};

// Control mapping structure
struct ControlMapping {
    ControllerType type;
    int channel;              // Channel index (0-7)
    std::string parameter;    // Parameter name (fader, gainKnob, etc.)
    
    // MIDI specific
    int midiCC = -1;
    int midiChannel = 0;
    
    // OSC specific  
    std::string oscAddress;
    
    // Keyboard specific
    int keyCode = -1;
    bool requiresCtrl = false;
    bool requiresShift = false;
    
    // Value mapping
    float minValue = 0.0f;
    float maxValue = 1.0f;
    bool invertValue = false;
    
    // Callback for value changes
    std::function<void(float)> callback;
};

// External device manager
class ExternalDeviceManager {
public:
    ExternalDeviceManager();
    ~ExternalDeviceManager();
    
    // Initialization
    bool initialize();
    void shutdown();
    
    // Device discovery and connection
    void scanForDevices();
    std::vector<std::string> getAvailableMidiDevices() const;
    bool connectMidiDevice(const std::string& deviceName);
    void disconnectMidiDevice();
    
    // Mapping management
    void addMapping(const ControlMapping& mapping);
    void removeMapping(int channel, const std::string& parameter);
    void clearAllMappings();
    std::vector<ControlMapping> getAllMappings() const;
    
    // Learning mode for automatic mapping
    void enableLearningMode(bool enable);
    bool isLearningMode() const { return learningMode_; }
    void setLearningTarget(int channel, const std::string& parameter);
    
    // Value processing and callbacks
    void processMidiMessage(int cc, int value, int channel = 0);
    void processOSCMessage(const std::string& address, float value);
    void processKeyboardInput(int keyCode, bool ctrl, bool shift);
    
    // Channel parameter access
    void setChannelParameterCallback(int channel, const std::string& parameter, 
                                   std::function<void(float)> callback);
    
    // Status and diagnostics
    bool isMidiConnected() const { return midiConnected_; }
    std::string getConnectedMidiDevice() const { return connectedMidiDevice_; }
    int getMessageCount() const { return messageCount_; }
    void resetMessageCount() { messageCount_ = 0; }
    
private:
    // Device state
    bool midiConnected_ = false;
    std::string connectedMidiDevice_;
    std::unique_ptr<OSCReceiver> controlOSCReceiver_;
    
    // Mapping storage
    std::map<std::pair<int, std::string>, ControlMapping> mappings_;
    std::mutex mappingsMutex_;
    
    // Learning mode
    bool learningMode_ = false;
    int learningChannel_ = -1;
    std::string learningParameter_;
    std::mutex learningMutex_;
    
    // Statistics
    int messageCount_ = 0;
    
    // MIDI handling (platform specific)
    void initializeMidi();
    void shutdownMidi();
    void handleMidiInput(int cc, int value, int channel);
    
    // OSC handling for control
    void initializeControlOSC();
    void shutdownControlOSC();
    
    // Value conversion utilities
    float convertMidiToFloat(int midiValue, float minVal, float maxVal, bool invert = false);
    int convertFloatToMidi(float value, float minVal, float maxVal, bool invert = false);
    
    // Mapping utilities
    std::string generateMappingKey(int channel, const std::string& parameter);
    void executeMapping(const ControlMapping& mapping, float value);
};

// Preset management for external device configurations
class ExternalDevicePresets {
public:
    struct Preset {
        std::string name;
        std::string description;
        std::vector<ControlMapping> mappings;
    };
    
    // Preset management
    void savePreset(const std::string& name, const std::vector<ControlMapping>& mappings);
    bool loadPreset(const std::string& name);
    void deletePreset(const std::string& name);
    std::vector<std::string> getPresetNames() const;
    
    // File I/O
    bool saveToFile(const std::string& filename);
    bool loadFromFile(const std::string& filename);
    
private:
    std::map<std::string, Preset> presets_;
    std::string currentPreset_;
};
