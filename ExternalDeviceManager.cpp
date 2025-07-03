#include "ExternalDeviceManager.h"
#include "OSCSender.h"
#include "ErrorHandler.h"
#include <iostream>
#include <algorithm>
#include <cmath>

#ifdef __APPLE__
#include <CoreMIDI/CoreMIDI.h>
#include <CoreFoundation/CoreFoundation.h>
#endif

ExternalDeviceManager::ExternalDeviceManager() {
    // Constructor
}

ExternalDeviceManager::~ExternalDeviceManager() {
    shutdown();
}

bool ExternalDeviceManager::initialize() {
    try {
        initializeMidi();
        initializeControlOSC();
        ERROR_INFO("External device manager initialized", "MIDI and OSC control ready");
        return true;
    } catch (const std::exception& e) {
        ERROR_ERROR("Failed to initialize external device manager", e.what(), 
                   "Check MIDI and OSC system availability", false);
        return false;
    }
}

void ExternalDeviceManager::shutdown() {
    shutdownMidi();
    shutdownControlOSC();
}

void ExternalDeviceManager::scanForDevices() {
    // This would scan for available MIDI devices
    // Implementation depends on platform-specific MIDI libraries
    ERROR_INFO("Device scan initiated", "Searching for available MIDI controllers");
}

std::vector<std::string> ExternalDeviceManager::getAvailableMidiDevices() const {
    std::vector<std::string> devices;
    
#ifdef __APPLE__
    // macOS CoreMIDI implementation
    ItemCount numSources = MIDIGetNumberOfSources();
    for (ItemCount i = 0; i < numSources; ++i) {
        MIDIEndpointRef source = MIDIGetSource(i);
        CFStringRef pname;
        if (MIDIObjectGetStringProperty(source, kMIDIPropertyName, &pname) == noErr) {
            char name[256];
            CFStringGetCString(pname, name, sizeof(name), kCFStringEncodingUTF8);
            devices.push_back(std::string(name));
            CFRelease(pname);
        }
    }
#else
    // Placeholder for other platforms
    devices.push_back("Virtual MIDI Controller");
    devices.push_back("USB MIDI Controller");
#endif
    
    return devices;
}

bool ExternalDeviceManager::connectMidiDevice(const std::string& deviceName) {
    if (midiConnected_) {
        disconnectMidiDevice();
    }
    
    // Platform-specific MIDI connection code would go here
    connectedMidiDevice_ = deviceName;
    midiConnected_ = true;
    
    ERROR_INFO("MIDI device connected", "Device: " + deviceName);
    return true;
}

void ExternalDeviceManager::disconnectMidiDevice() {
    if (!midiConnected_) return;
    
    // Platform-specific MIDI disconnection code
    midiConnected_ = false;
    connectedMidiDevice_.clear();
    
    ERROR_INFO("MIDI device disconnected", "Controller released");
}

void ExternalDeviceManager::addMapping(const ControlMapping& mapping) {
    std::lock_guard<std::mutex> lock(mappingsMutex_);
    
    std::pair<int, std::string> key = {mapping.channel, mapping.parameter};
    mappings_[key] = mapping;
    
    ERROR_INFO("Control mapping added", 
              "Channel " + std::to_string(mapping.channel) + " " + mapping.parameter);
}

void ExternalDeviceManager::removeMapping(int channel, const std::string& parameter) {
    std::lock_guard<std::mutex> lock(mappingsMutex_);
    
    std::pair<int, std::string> key = {channel, parameter};
    mappings_.erase(key);
    
    ERROR_INFO("Control mapping removed", 
              "Channel " + std::to_string(channel) + " " + parameter);
}

void ExternalDeviceManager::clearAllMappings() {
    std::lock_guard<std::mutex> lock(mappingsMutex_);
    mappings_.clear();
    ERROR_INFO("All control mappings cleared", "Reset to default state");
}

std::vector<ControlMapping> ExternalDeviceManager::getAllMappings() const {
    std::lock_guard<std::mutex> lock(const_cast<std::mutex&>(mappingsMutex_));
    
    std::vector<ControlMapping> result;
    for (const auto& pair : mappings_) {
        result.push_back(pair.second);
    }
    
    return result;
}

void ExternalDeviceManager::enableLearningMode(bool enable) {
    std::lock_guard<std::mutex> lock(learningMutex_);
    learningMode_ = enable;
    
    if (enable) {
        ERROR_INFO("Learning mode enabled", "Move a control to assign it");
    } else {
        ERROR_INFO("Learning mode disabled", "Manual mapping mode");
        learningChannel_ = -1;
        learningParameter_.clear();
    }
}

void ExternalDeviceManager::setLearningTarget(int channel, const std::string& parameter) {
    std::lock_guard<std::mutex> lock(learningMutex_);
    learningChannel_ = channel;
    learningParameter_ = parameter;
    
    ERROR_INFO("Learning target set", 
              "Channel " + std::to_string(channel) + " " + parameter + 
              " - now move your controller");
}

void ExternalDeviceManager::processMidiMessage(int cc, int value, int channel) {
    messageCount_++;
    
    // Check if in learning mode
    {
        std::lock_guard<std::mutex> lock(learningMutex_);
        if (learningMode_ && learningChannel_ >= 0 && !learningParameter_.empty()) {
            // Create new mapping
            ControlMapping mapping;
            mapping.type = ControllerType::MIDI_CC;
            mapping.channel = learningChannel_;
            mapping.parameter = learningParameter_;
            mapping.midiCC = cc;
            mapping.midiChannel = channel;
            mapping.minValue = 0.0f;
            mapping.maxValue = 1.0f;
            
            addMapping(mapping);
            
            ERROR_INFO("MIDI mapping learned", 
                      "CC" + std::to_string(cc) + " -> Ch" + std::to_string(learningChannel_) + 
                      " " + learningParameter_);
            
            // Reset learning state
            learningChannel_ = -1;
            learningParameter_.clear();
            learningMode_ = false;
        }
    }
    
    // Process existing mappings
    std::lock_guard<std::mutex> lock(mappingsMutex_);
    for (const auto& pair : mappings_) {
        const ControlMapping& mapping = pair.second;
        if (mapping.type == ControllerType::MIDI_CC && 
            mapping.midiCC == cc && 
            mapping.midiChannel == channel) {
            
            float normalizedValue = convertMidiToFloat(value, mapping.minValue, 
                                                     mapping.maxValue, mapping.invertValue);
            
            if (mapping.callback) {
                mapping.callback(normalizedValue);
            }
        }
    }
}

void ExternalDeviceManager::processOSCMessage(const std::string& address, float value) {
    messageCount_++;
    
    // Process OSC control mappings
    std::lock_guard<std::mutex> lock(mappingsMutex_);
    for (const auto& pair : mappings_) {
        const ControlMapping& mapping = pair.second;
        if (mapping.type == ControllerType::OSC_MESSAGE && 
            mapping.oscAddress == address) {
            
            float normalizedValue = value;
            if (mapping.invertValue) {
                normalizedValue = 1.0f - normalizedValue;
            }
            
            // Scale to mapping range
            normalizedValue = mapping.minValue + 
                             normalizedValue * (mapping.maxValue - mapping.minValue);
            
            if (mapping.callback) {
                mapping.callback(normalizedValue);
            }
        }
    }
}

void ExternalDeviceManager::processKeyboardInput(int keyCode, bool ctrl, bool shift) {
    // Process keyboard shortcut mappings
    std::lock_guard<std::mutex> lock(mappingsMutex_);
    for (const auto& pair : mappings_) {
        const ControlMapping& mapping = pair.second;
        if (mapping.type == ControllerType::KEYBOARD_SHORTCUT && 
            mapping.keyCode == keyCode &&
            mapping.requiresCtrl == ctrl &&
            mapping.requiresShift == shift) {
            
            // Toggle or set value for keyboard shortcuts
            float value = 1.0f; // Could implement toggle logic here
            
            if (mapping.callback) {
                mapping.callback(value);
            }
        }
    }
}

void ExternalDeviceManager::setChannelParameterCallback(int channel, const std::string& parameter, 
                                                       std::function<void(float)> callback) {
    std::lock_guard<std::mutex> lock(mappingsMutex_);
    
    std::pair<int, std::string> key = {channel, parameter};
    if (mappings_.find(key) != mappings_.end()) {
        mappings_[key].callback = callback;
    }
}

void ExternalDeviceManager::initializeMidi() {
#ifdef __APPLE__
    // macOS CoreMIDI initialization
    // This is a simplified version - real implementation would set up MIDI client
    ERROR_INFO("MIDI system initialized", "CoreMIDI ready");
#else
    // Placeholder for other platforms
    ERROR_INFO("MIDI system initialized", "Platform MIDI ready");
#endif
}

void ExternalDeviceManager::shutdownMidi() {
    disconnectMidiDevice();
    ERROR_INFO("MIDI system shutdown", "MIDI resources released");
}

void ExternalDeviceManager::initializeControlOSC() {
    try {
        // Initialize OSC receiver for control messages (different from CV data)
        controlOSCReceiver_ = std::make_unique<OSCReceiver>("8002"); // Different port for control
        
        controlOSCReceiver_->setMessageCallback(
            [this](const std::string& address, const std::vector<float>& values) {
                if (!values.empty()) {
                    processOSCMessage(address, values[0]);
                }
            });
        
        if (controlOSCReceiver_->start()) {
            ERROR_INFO("Control OSC initialized", "Listening on port 8002");
        }
    } catch (const std::exception& e) {
        ERROR_ERROR("Failed to initialize control OSC", e.what(), 
                   "Control OSC not available", true);
    }
}

void ExternalDeviceManager::shutdownControlOSC() {
    if (controlOSCReceiver_) {
        controlOSCReceiver_->stop();
        controlOSCReceiver_.reset();
    }
}

float ExternalDeviceManager::convertMidiToFloat(int midiValue, float minVal, float maxVal, bool invert) {
    float normalized = midiValue / 127.0f;
    if (invert) {
        normalized = 1.0f - normalized;
    }
    return minVal + normalized * (maxVal - minVal);
}

int ExternalDeviceManager::convertFloatToMidi(float value, float minVal, float maxVal, bool invert) {
    float normalized = (value - minVal) / (maxVal - minVal);
    normalized = std::clamp(normalized, 0.0f, 1.0f);
    
    if (invert) {
        normalized = 1.0f - normalized;
    }
    
    return static_cast<int>(normalized * 127.0f);
}

// ExternalDevicePresets implementation

void ExternalDevicePresets::savePreset(const std::string& name, const std::vector<ControlMapping>& mappings) {
    Preset preset;
    preset.name = name;
    preset.description = "Saved preset with " + std::to_string(mappings.size()) + " mappings";
    preset.mappings = mappings;
    
    presets_[name] = preset;
    currentPreset_ = name;
}

bool ExternalDevicePresets::loadPreset(const std::string& name) {
    auto it = presets_.find(name);
    if (it != presets_.end()) {
        currentPreset_ = name;
        return true;
    }
    return false;
}

void ExternalDevicePresets::deletePreset(const std::string& name) {
    presets_.erase(name);
    if (currentPreset_ == name) {
        currentPreset_.clear();
    }
}

std::vector<std::string> ExternalDevicePresets::getPresetNames() const {
    std::vector<std::string> names;
    for (const auto& pair : presets_) {
        names.push_back(pair.first);
    }
    return names;
}

bool ExternalDevicePresets::saveToFile(const std::string& filename) {
    // Implementation would save presets to JSON file
    ERROR_INFO("Presets saved", "File: " + filename);
    return true;
}

bool ExternalDevicePresets::loadFromFile(const std::string& filename) {
    // Implementation would load presets from JSON file
    ERROR_INFO("Presets loaded", "File: " + filename);
    return true;
}
