#include "ExternalDeviceMapper.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <regex>
#include <chrono>
#include <iostream>

using json = nlohmann::json;

ExternalDeviceMapper::ExternalDeviceMapper() {
    
}

ExternalDeviceMapper::~ExternalDeviceMapper() {
    shutdown();
}

bool ExternalDeviceMapper::initialize() {
    startProcessingThread();
    return true;
}

void ExternalDeviceMapper::shutdown() {
    stopProcessingThread();
    clearAllMappings();
}

// Device Registration
bool ExternalDeviceMapper::registerDevice(const std::string& deviceId, ExternalDeviceType type) {
    std::lock_guard<std::mutex> lock(mappingsMutex_);
    registeredDevices_[deviceId] = type;
    
    std::cout << "Registered device: " << deviceId << " type: " << static_cast<int>(type) << std::endl;
    return true;
}

bool ExternalDeviceMapper::unregisterDevice(const std::string& deviceId) {
    std::lock_guard<std::mutex> lock(mappingsMutex_);
    
    // Remove all mappings for this device
    auto it = mappings_.begin();
    while (it != mappings_.end()) {
        if (it->second.deviceId == deviceId) {
            it = mappings_.erase(it);
        } else {
            ++it;
        }
    }
    
    registeredDevices_.erase(deviceId);
    std::cout << "Unregistered device: " << deviceId << std::endl;
    return true;
}

std::vector<std::string> ExternalDeviceMapper::getRegisteredDevices() const {
    std::lock_guard<std::mutex> lock(const_cast<std::mutex&>(mappingsMutex_));
    std::vector<std::string> devices;
    for (const auto& pair : registeredDevices_) {
        devices.push_back(pair.first);
    }
    return devices;
}

// Mapping Management
bool ExternalDeviceMapper::addMapping(const ExternalDeviceMapping& mapping) {
    if (!validateMapping(mapping)) {
        return false;
    }
    
    std::lock_guard<std::mutex> lock(mappingsMutex_);
    mappings_[mapping.mappingId] = mapping;
    
    std::cout << "Added mapping: " << mapping.name << " (" << mapping.mappingId << ")" << std::endl;
    return true;
}

bool ExternalDeviceMapper::removeMapping(const std::string& mappingId) {
    std::lock_guard<std::mutex> lock(mappingsMutex_);
    auto it = mappings_.find(mappingId);
    if (it != mappings_.end()) {
        std::cout << "Removed mapping: " << it->second.name << std::endl;
        mappings_.erase(it);
        return true;
    }
    return false;
}

bool ExternalDeviceMapper::updateMapping(const std::string& mappingId, const ExternalDeviceMapping& mapping) {
    if (!validateMapping(mapping)) {
        return false;
    }
    
    std::lock_guard<std::mutex> lock(mappingsMutex_);
    auto it = mappings_.find(mappingId);
    if (it != mappings_.end()) {
        it->second = mapping;
        std::cout << "Updated mapping: " << mapping.name << std::endl;
        return true;
    }
    return false;
}

ExternalDeviceMapping* ExternalDeviceMapper::getMapping(const std::string& mappingId) {
    std::lock_guard<std::mutex> lock(mappingsMutex_);
    auto it = mappings_.find(mappingId);
    return (it != mappings_.end()) ? &it->second : nullptr;
}

std::vector<ExternalDeviceMapping> ExternalDeviceMapper::getAllMappings() const {
    std::lock_guard<std::mutex> lock(const_cast<std::mutex&>(mappingsMutex_));
    std::vector<ExternalDeviceMapping> result;
    for (const auto& pair : mappings_) {
        result.push_back(pair.second);
    }
    return result;
}

std::vector<ExternalDeviceMapping> ExternalDeviceMapper::getMappingsForDevice(const std::string& deviceId) const {
    std::lock_guard<std::mutex> lock(const_cast<std::mutex&>(mappingsMutex_));
    std::vector<ExternalDeviceMapping> result;
    for (const auto& pair : mappings_) {
        if (pair.second.deviceId == deviceId) {
            result.push_back(pair.second);
        }
    }
    return result;
}

std::vector<ExternalDeviceMapping> ExternalDeviceMapper::getMappingsForParameter(MappingParameterType parameterType, int channelId) const {
    std::lock_guard<std::mutex> lock(const_cast<std::mutex&>(mappingsMutex_));
    std::vector<ExternalDeviceMapping> result;
    for (const auto& pair : mappings_) {
        if (pair.second.parameterType == parameterType && 
            (channelId == -1 || pair.second.targetChannelId == channelId)) {
            result.push_back(pair.second);
        }
    }
    return result;
}

// Learning Mode
bool ExternalDeviceMapper::startLearning(const LearningModeConfig& config) {
    std::lock_guard<std::mutex> lock(learningMutex_);
    
    if (learningActive_) {
        return false; // Already learning
    }
    
    currentLearningConfig_ = config;
    learningStartTime_ = std::chrono::steady_clock::now();
    learningActive_ = true;
    
    std::cout << "Learning mode started for parameter: " << static_cast<int>(config.targetParameter) << std::endl;
    return true;
}

bool ExternalDeviceMapper::stopLearning() {
    std::lock_guard<std::mutex> lock(learningMutex_);
    
    if (!learningActive_) {
        return false;
    }
    
    learningActive_ = false;
    std::cout << "Learning mode stopped" << std::endl;
    return true;
}

// Input Processing
void ExternalDeviceMapper::processOSCInput(const std::string& address, const std::vector<float>& values) {
    inputsProcessedThisSecond_++;
    
    // Handle learning mode
    if (learningActive_) {
        std::lock_guard<std::mutex> lock(learningMutex_);
        
        // Create a new mapping from learned input
        ExternalDeviceMapping mapping;
        mapping.deviceType = ExternalDeviceType::OSC_CONTROLLER;
        mapping.inputAddress = address;
        mapping.parameterType = currentLearningConfig_.targetParameter;
        mapping.targetChannelId = currentLearningConfig_.targetChannelId;
        mapping.targetDeviceId = currentLearningConfig_.targetDeviceId;
        mapping.name = "Learned OSC Mapping: " + address;
        mapping.enabled = true;
        
        if (!values.empty()) {
            // Determine range from current input
            float value = values[0];
            mapping.inputMin = 0.0f;
            mapping.inputMax = 1.0f; // Assume normalized OSC values
            mapping.outputMin = 0.0f;
            mapping.outputMax = 1.0f;
        }
        
        completeLearning(mapping);
        return;
    }
    
    // Process existing mappings
    std::lock_guard<std::mutex> lock(mappingsMutex_);
    for (const auto& pair : mappings_) {
        const ExternalDeviceMapping& mapping = pair.second;
        
        if (!mapping.enabled || mapping.deviceType != ExternalDeviceType::OSC_CONTROLLER) {
            continue;
        }
        
        if (matchesPattern(address, mapping.inputAddress) && !values.empty()) {
            processMapping(mapping, values[0]);
        }
    }
}

void ExternalDeviceMapper::processMIDIInput(int channel, int cc, int value) {
    inputsProcessedThisSecond_++;
    
    // Handle learning mode
    if (learningActive_) {
        std::lock_guard<std::mutex> lock(learningMutex_);
        
        ExternalDeviceMapping mapping;
        mapping.deviceType = ExternalDeviceType::MIDI_CONTROLLER;
        mapping.inputAddress = "cc" + std::to_string(cc);
        mapping.parameterType = currentLearningConfig_.targetParameter;
        mapping.targetChannelId = currentLearningConfig_.targetChannelId;
        mapping.targetDeviceId = currentLearningConfig_.targetDeviceId;
        mapping.name = "Learned MIDI Mapping: CC" + std::to_string(cc);
        mapping.inputMin = 0.0f;
        mapping.inputMax = 127.0f;
        mapping.outputMin = 0.0f;
        mapping.outputMax = 1.0f;
        mapping.enabled = true;
        
        completeLearning(mapping);
        return;
    }
    
    // Process existing mappings
    std::lock_guard<std::mutex> lock(mappingsMutex_);
    for (const auto& pair : mappings_) {
        const ExternalDeviceMapping& mapping = pair.second;
        
        if (!mapping.enabled || mapping.deviceType != ExternalDeviceType::MIDI_CONTROLLER) {
            continue;
        }
        
        std::string ccAddress = "cc" + std::to_string(cc);
        if (mapping.inputAddress == ccAddress) {
            float normalizedValue = static_cast<float>(value) / 127.0f;
            processMapping(mapping, normalizedValue);
        }
    }
}

void ExternalDeviceMapper::processKeyboardInput(const std::string& keyCode, bool pressed) {
    if (!pressed) return; // Only process key press, not release
    
    inputsProcessedThisSecond_++;
    
    // Handle learning mode
    if (learningActive_) {
        std::lock_guard<std::mutex> lock(learningMutex_);
        
        ExternalDeviceMapping mapping;
        mapping.deviceType = ExternalDeviceType::KEYBOARD_SHORTCUT;
        mapping.inputAddress = keyCode;
        mapping.parameterType = currentLearningConfig_.targetParameter;
        mapping.targetChannelId = currentLearningConfig_.targetChannelId;
        mapping.targetDeviceId = currentLearningConfig_.targetDeviceId;
        mapping.name = "Learned Keyboard Mapping: " + keyCode;
        mapping.inputMin = 0.0f;
        mapping.inputMax = 1.0f;
        mapping.outputMin = 0.0f;
        mapping.outputMax = 1.0f;
        mapping.enabled = true;
        
        completeLearning(mapping);
        return;
    }
    
    // Process existing mappings
    std::lock_guard<std::mutex> lock(mappingsMutex_);
    for (const auto& pair : mappings_) {
        const ExternalDeviceMapping& mapping = pair.second;
        
        if (!mapping.enabled || mapping.deviceType != ExternalDeviceType::KEYBOARD_SHORTCUT) {
            continue;
        }
        
        if (mapping.inputAddress == keyCode) {
            processMapping(mapping, 1.0f); // Toggle value
        }
    }
}

void ExternalDeviceMapper::processCustomInput(const std::string& deviceId, const std::string& parameter, float value) {
    inputsProcessedThisSecond_++;
    
    // Process custom input mappings
    std::lock_guard<std::mutex> lock(mappingsMutex_);
    for (const auto& pair : mappings_) {
        const ExternalDeviceMapping& mapping = pair.second;
        
        if (!mapping.enabled || mapping.deviceId != deviceId) {
            continue;
        }
        
        if (mapping.inputAddress == parameter) {
            processMapping(mapping, value);
        }
    }
}

// Output Callbacks
void ExternalDeviceMapper::setOSCOutputCallback(std::function<void(const std::string&, const std::vector<float>&)> callback) {
    oscOutputCallback_ = callback;
}

void ExternalDeviceMapper::setMIDIOutputCallback(std::function<void(int, int, int)> callback) {
    midiOutputCallback_ = callback;
}

void ExternalDeviceMapper::setParameterChangeCallback(std::function<void(MappingParameterType, int, float)> callback) {
    parameterChangeCallback_ = callback;
}

// Configuration
bool ExternalDeviceMapper::loadMappings(const std::string& filePath) {
    try {
        std::ifstream file(filePath);
        if (!file.is_open()) {
            return false;
        }
        
        json j;
        file >> j;
        
        std::lock_guard<std::mutex> lock(mappingsMutex_);
        mappings_.clear();
        
        for (const auto& mappingJson : j["mappings"]) {
            ExternalDeviceMapping mapping;
            mapping.mappingId = mappingJson["mappingId"];
            mapping.deviceId = mappingJson["deviceId"];
            mapping.deviceType = static_cast<ExternalDeviceType>(mappingJson["deviceType"]);
            mapping.inputAddress = mappingJson["inputAddress"];
            mapping.inputMin = mappingJson["inputMin"];
            mapping.inputMax = mappingJson["inputMax"];
            mapping.parameterType = static_cast<MappingParameterType>(mappingJson["parameterType"]);
            mapping.targetChannelId = mappingJson["targetChannelId"];
            mapping.outputMin = mappingJson["outputMin"];
            mapping.outputMax = mappingJson["outputMax"];
            mapping.bidirectional = mappingJson["bidirectional"];
            mapping.inverted = mappingJson["inverted"];
            mapping.name = mappingJson["name"];
            mapping.description = mappingJson["description"];
            mapping.enabled = mappingJson["enabled"];
            
            mappings_[mapping.mappingId] = mapping;
        }
        
        std::cout << "Loaded " << mappings_.size() << " mappings from " << filePath << std::endl;
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error loading mappings: " << e.what() << std::endl;
        return false;
    }
}

bool ExternalDeviceMapper::saveMappings(const std::string& filePath) {
    try {
        json j;
        j["mappings"] = json::array();
        
        std::lock_guard<std::mutex> lock(mappingsMutex_);
        for (const auto& pair : mappings_) {
            const ExternalDeviceMapping& mapping = pair.second;
            
            json mappingJson;
            mappingJson["mappingId"] = mapping.mappingId;
            mappingJson["deviceId"] = mapping.deviceId;
            mappingJson["deviceType"] = static_cast<int>(mapping.deviceType);
            mappingJson["inputAddress"] = mapping.inputAddress;
            mappingJson["inputMin"] = mapping.inputMin;
            mappingJson["inputMax"] = mapping.inputMax;
            mappingJson["parameterType"] = static_cast<int>(mapping.parameterType);
            mappingJson["targetChannelId"] = mapping.targetChannelId;
            mappingJson["outputMin"] = mapping.outputMin;
            mappingJson["outputMax"] = mapping.outputMax;
            mappingJson["bidirectional"] = mapping.bidirectional;
            mappingJson["inverted"] = mapping.inverted;
            mappingJson["name"] = mapping.name;
            mappingJson["description"] = mapping.description;
            mappingJson["enabled"] = mapping.enabled;
            
            j["mappings"].push_back(mappingJson);
        }
        
        std::ofstream file(filePath);
        file << j.dump(4);
        
        std::cout << "Saved " << mappings_.size() << " mappings to " << filePath << std::endl;
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error saving mappings: " << e.what() << std::endl;
        return false;
    }
}

void ExternalDeviceMapper::clearAllMappings() {
    std::lock_guard<std::mutex> lock(mappingsMutex_);
    mappings_.clear();
    std::cout << "Cleared all mappings" << std::endl;
}

// Statistics
int ExternalDeviceMapper::getTotalMappings() const {
    std::lock_guard<std::mutex> lock(const_cast<std::mutex&>(mappingsMutex_));
    return static_cast<int>(mappings_.size());
}

int ExternalDeviceMapper::getActiveMappings() const {
    std::lock_guard<std::mutex> lock(const_cast<std::mutex&>(mappingsMutex_));
    int count = 0;
    for (const auto& pair : mappings_) {
        if (pair.second.enabled) {
            count++;
        }
    }
    return count;
}

int ExternalDeviceMapper::getProcessedInputsPerSecond() const {
    return inputsProcessedThisSecond_.load();
}

// Validation
bool ExternalDeviceMapper::validateMapping(const ExternalDeviceMapping& mapping) const {
    if (mapping.mappingId.empty() || mapping.deviceId.empty()) {
        return false;
    }
    
    if (mapping.inputAddress.empty()) {
        return false;
    }
    
    if (!isValidParameterType(mapping.parameterType)) {
        return false;
    }
    
    if (mapping.targetChannelId < -1 || mapping.targetChannelId >= 8) {
        return false;
    }
    
    return true;
}

std::vector<std::string> ExternalDeviceMapper::getMappingErrors(const ExternalDeviceMapping& mapping) const {
    std::vector<std::string> errors;
    
    if (mapping.mappingId.empty()) {
        errors.push_back("Mapping ID cannot be empty");
    }
    
    if (mapping.deviceId.empty()) {
        errors.push_back("Device ID cannot be empty");
    }
    
    if (mapping.inputAddress.empty()) {
        errors.push_back("Input address cannot be empty");
    }
    
    if (!isValidParameterType(mapping.parameterType)) {
        errors.push_back("Invalid parameter type");
    }
    
    if (mapping.targetChannelId < -1 || mapping.targetChannelId >= 8) {
        errors.push_back("Target channel ID must be between -1 and 7");
    }
    
    return errors;
}

// Internal methods
void ExternalDeviceMapper::processMapping(const ExternalDeviceMapping& mapping, float inputValue) {
    // Transform input value to output range
    float outputValue = transformValue(inputValue, mapping.inputMin, mapping.inputMax, 
                                     mapping.outputMin, mapping.outputMax, mapping.inverted);
    
    // Trigger parameter change
    triggerParameterChange(mapping.parameterType, mapping.targetChannelId, 
                          mapping.targetDeviceId, outputValue);
    
    // Handle bidirectional mappings
    if (mapping.bidirectional && oscOutputCallback_) {
        std::vector<float> values = {outputValue};
        oscOutputCallback_(mapping.inputAddress, values);
    }
}

float ExternalDeviceMapper::transformValue(float input, float inputMin, float inputMax, 
                                         float outputMin, float outputMax, bool inverted) const {
    // Normalize input to 0-1 range
    float normalized = (input - inputMin) / (inputMax - inputMin);
    
    // Clamp to valid range
    normalized = std::max(0.0f, std::min(1.0f, normalized));
    
    // Invert if needed
    if (inverted) {
        normalized = 1.0f - normalized;
    }
    
    // Scale to output range
    return outputMin + normalized * (outputMax - outputMin);
}

bool ExternalDeviceMapper::matchesPattern(const std::string& input, const std::string& pattern) const {
    // Simple pattern matching - can be enhanced with regex
    if (pattern.find('*') != std::string::npos) {
        // Wildcard matching
        std::string regexPattern = pattern;
        std::replace(regexPattern.begin(), regexPattern.end(), '*', '.');
        regexPattern = "^" + regexPattern + "$";
        
        try {
            std::regex regex(regexPattern);
            return std::regex_match(input, regex);
        } catch (const std::exception&) {
            return false;
        }
    } else {
        // Exact match
        return input == pattern;
    }
}

void ExternalDeviceMapper::triggerParameterChange(MappingParameterType parameterType, int channelId, 
                                                 const std::string& deviceId, float value) {
    if (parameterChangeCallback_) {
        parameterChangeCallback_(parameterType, channelId, value);
    }
    
    std::cout << "Parameter change: type=" << static_cast<int>(parameterType) 
              << " channel=" << channelId << " value=" << value << std::endl;
}

void ExternalDeviceMapper::updateLearningMode() {
    if (!learningActive_) return;
    
    std::lock_guard<std::mutex> lock(learningMutex_);
    
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - learningStartTime_);
    
    if (elapsed >= currentLearningConfig_.timeout) {
        timeoutLearning();
    }
}

void ExternalDeviceMapper::completeLearning(const ExternalDeviceMapping& mapping) {
    if (currentLearningConfig_.autoCommit) {
        addMapping(mapping);
    }
    
    if (currentLearningConfig_.onLearningComplete) {
        currentLearningConfig_.onLearningComplete(mapping);
    }
    
    learningActive_ = false;
    std::cout << "Learning completed for mapping: " << mapping.name << std::endl;
}

void ExternalDeviceMapper::timeoutLearning() {
    if (currentLearningConfig_.onLearningTimeout) {
        currentLearningConfig_.onLearningTimeout("Learning mode timed out");
    }
    
    learningActive_ = false;
    std::cout << "Learning mode timed out" << std::endl;
}

// Validation helpers
bool ExternalDeviceMapper::isValidOSCAddress(const std::string& address) const {
    return !address.empty() && address[0] == '/';
}

bool ExternalDeviceMapper::isValidMIDICC(const std::string& cc) const {
    if (cc.substr(0, 2) != "cc") return false;
    try {
        int ccNum = std::stoi(cc.substr(2));
        return ccNum >= 0 && ccNum <= 127;
    } catch (const std::exception&) {
        return false;
    }
}

bool ExternalDeviceMapper::isValidParameterType(MappingParameterType type) const {
    return type >= MappingParameterType::CHANNEL_LEVEL && type <= MappingParameterType::CUSTOM_PARAMETER;
}

bool ExternalDeviceMapper::isValidChannelId(int channelId) const {
    return channelId >= -1 && channelId < 8;
}

// Threading
void ExternalDeviceMapper::startProcessingThread() {
    if (processingThreadRunning_) return;
    
    processingThreadRunning_ = true;
    processingThread_ = std::thread([this]() {
        while (processingThreadRunning_) {
            updateLearningMode();
            
            // Update statistics
            auto now = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - lastStatsUpdate_);
            if (elapsed.count() >= 1) {
                inputsProcessedThisSecond_ = 0;
                lastStatsUpdate_ = now;
            }
            
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    });
}

void ExternalDeviceMapper::stopProcessingThread() {
    if (!processingThreadRunning_) return;
    
    processingThreadRunning_ = false;
    if (processingThread_.joinable()) {
        processingThread_.join();
    }
}

// Utility functions
std::string mappingParameterTypeToString(MappingParameterType type) {
    switch (type) {
        case MappingParameterType::CHANNEL_LEVEL: return "Channel Level";
        case MappingParameterType::CHANNEL_MUTE: return "Channel Mute";
        case MappingParameterType::CHANNEL_SOLO: return "Channel Solo";
        case MappingParameterType::MASTER_LEVEL: return "Master Level";
        case MappingParameterType::MASTER_MUTE: return "Master Mute";
        case MappingParameterType::DEVICE_SELECT: return "Device Select";
        case MappingParameterType::LEARNING_MODE_TOGGLE: return "Learning Mode Toggle";
        case MappingParameterType::CUSTOM_PARAMETER: return "Custom Parameter";
        default: return "Unknown";
    }
}

MappingParameterType stringToMappingParameterType(const std::string& str) {
    if (str == "Channel Level") return MappingParameterType::CHANNEL_LEVEL;
    if (str == "Channel Mute") return MappingParameterType::CHANNEL_MUTE;
    if (str == "Channel Solo") return MappingParameterType::CHANNEL_SOLO;
    if (str == "Master Level") return MappingParameterType::MASTER_LEVEL;
    if (str == "Master Mute") return MappingParameterType::MASTER_MUTE;
    if (str == "Device Select") return MappingParameterType::DEVICE_SELECT;
    if (str == "Learning Mode Toggle") return MappingParameterType::LEARNING_MODE_TOGGLE;
    return MappingParameterType::CUSTOM_PARAMETER;
}

std::string externalDeviceTypeToString(ExternalDeviceType type) {
    switch (type) {
        case ExternalDeviceType::MIDI_CONTROLLER: return "MIDI Controller";
        case ExternalDeviceType::OSC_CONTROLLER: return "OSC Controller";
        case ExternalDeviceType::KEYBOARD_SHORTCUT: return "Keyboard Shortcut";
        case ExternalDeviceType::TOUCH_OSC: return "TouchOSC";
        case ExternalDeviceType::LEMUR: return "Lemur";
        case ExternalDeviceType::CUSTOM_PROTOCOL: return "Custom Protocol";
        default: return "Unknown";
    }
}

ExternalDeviceType stringToExternalDeviceType(const std::string& str) {
    if (str == "MIDI Controller") return ExternalDeviceType::MIDI_CONTROLLER;
    if (str == "OSC Controller") return ExternalDeviceType::OSC_CONTROLLER;
    if (str == "Keyboard Shortcut") return ExternalDeviceType::KEYBOARD_SHORTCUT;
    if (str == "TouchOSC") return ExternalDeviceType::TOUCH_OSC;
    if (str == "Lemur") return ExternalDeviceType::LEMUR;
    return ExternalDeviceType::CUSTOM_PROTOCOL;
}

// DeviceMappingPresets implementation
std::vector<ExternalDeviceMapping> DeviceMappingPresets::getTouchOSCMixerPreset() {
    std::vector<ExternalDeviceMapping> mappings;
    
    // Create TouchOSC preset mappings for 8-channel mixer
    for (int i = 0; i < 8; i++) {
        // Channel level fader
        ExternalDeviceMapping levelMapping;
        levelMapping.name = "TouchOSC Channel " + std::to_string(i + 1) + " Level";
        levelMapping.deviceType = ExternalDeviceType::TOUCH_OSC;
        levelMapping.inputAddress = "/mixer/fader" + std::to_string(i + 1);
        levelMapping.parameterType = MappingParameterType::CHANNEL_LEVEL;
        levelMapping.targetChannelId = i;
        levelMapping.bidirectional = true;
        mappings.push_back(levelMapping);
        
        // Channel mute button
        ExternalDeviceMapping muteMapping;
        muteMapping.name = "TouchOSC Channel " + std::to_string(i + 1) + " Mute";
        muteMapping.deviceType = ExternalDeviceType::TOUCH_OSC;
        muteMapping.inputAddress = "/mixer/mute" + std::to_string(i + 1);
        muteMapping.parameterType = MappingParameterType::CHANNEL_MUTE;
        muteMapping.targetChannelId = i;
        muteMapping.bidirectional = true;
        mappings.push_back(muteMapping);
        
        // Channel solo button
        ExternalDeviceMapping soloMapping;
        soloMapping.name = "TouchOSC Channel " + std::to_string(i + 1) + " Solo";
        soloMapping.deviceType = ExternalDeviceType::TOUCH_OSC;
        soloMapping.inputAddress = "/mixer/solo" + std::to_string(i + 1);
        soloMapping.parameterType = MappingParameterType::CHANNEL_SOLO;
        soloMapping.targetChannelId = i;
        soloMapping.bidirectional = true;
        mappings.push_back(soloMapping);
    }
    
    // Master level
    ExternalDeviceMapping masterMapping;
    masterMapping.name = "TouchOSC Master Level";
    masterMapping.deviceType = ExternalDeviceType::TOUCH_OSC;
    masterMapping.inputAddress = "/mixer/master";
    masterMapping.parameterType = MappingParameterType::MASTER_LEVEL;
    masterMapping.targetChannelId = -1;
    masterMapping.bidirectional = true;
    mappings.push_back(masterMapping);
    
    return mappings;
}

std::vector<ExternalDeviceMapping> DeviceMappingPresets::getMIDIControllerPreset() {
    std::vector<ExternalDeviceMapping> mappings;
    
    // Create MIDI controller preset for standard 8-fader controller
    for (int i = 0; i < 8; i++) {
        ExternalDeviceMapping mapping;
        mapping.name = "MIDI Channel " + std::to_string(i + 1) + " Level";
        mapping.deviceType = ExternalDeviceType::MIDI_CONTROLLER;
        mapping.inputAddress = "cc" + std::to_string(i + 1); // CC1-CC8
        mapping.parameterType = MappingParameterType::CHANNEL_LEVEL;
        mapping.targetChannelId = i;
        mapping.inputMin = 0.0f;
        mapping.inputMax = 127.0f;
        mappings.push_back(mapping);
    }
    
    return mappings;
}

std::vector<ExternalDeviceMapping> DeviceMappingPresets::getKeyboardShortcutsPreset() {
    std::vector<ExternalDeviceMapping> mappings;
    
    // Create keyboard shortcut preset
    for (int i = 0; i < 8; i++) {
        ExternalDeviceMapping muteMapping;
        muteMapping.name = "Keyboard Mute Channel " + std::to_string(i + 1);
        muteMapping.deviceType = ExternalDeviceType::KEYBOARD_SHORTCUT;
        muteMapping.inputAddress = "m" + std::to_string(i + 1);
        muteMapping.parameterType = MappingParameterType::CHANNEL_MUTE;
        muteMapping.targetChannelId = i;
        mappings.push_back(muteMapping);
        
        ExternalDeviceMapping soloMapping;
        soloMapping.name = "Keyboard Solo Channel " + std::to_string(i + 1);
        soloMapping.deviceType = ExternalDeviceType::KEYBOARD_SHORTCUT;
        soloMapping.inputAddress = "s" + std::to_string(i + 1);
        soloMapping.parameterType = MappingParameterType::CHANNEL_SOLO;
        soloMapping.targetChannelId = i;
        mappings.push_back(soloMapping);
    }
    
    return mappings;
}

std::vector<ExternalDeviceMapping> DeviceMappingPresets::getLemurMixerPreset() {
    // Similar to TouchOSC but with Lemur-specific addresses
    return getTouchOSCMixerPreset(); // Simplified for now
}

bool DeviceMappingPresets::savePreset(const std::string& name, const std::vector<ExternalDeviceMapping>& mappings) {
    // Save preset to file
    try {
        json j;
        j["name"] = name;
        j["mappings"] = json::array();
        
        for (const auto& mapping : mappings) {
            json mappingJson;
            mappingJson["mappingId"] = mapping.mappingId;
            mappingJson["deviceId"] = mapping.deviceId;
            mappingJson["deviceType"] = static_cast<int>(mapping.deviceType);
            mappingJson["inputAddress"] = mapping.inputAddress;
            mappingJson["inputMin"] = mapping.inputMin;
            mappingJson["inputMax"] = mapping.inputMax;
            mappingJson["parameterType"] = static_cast<int>(mapping.parameterType);
            mappingJson["targetChannelId"] = mapping.targetChannelId;
            mappingJson["outputMin"] = mapping.outputMin;
            mappingJson["outputMax"] = mapping.outputMax;
            mappingJson["bidirectional"] = mapping.bidirectional;
            mappingJson["inverted"] = mapping.inverted;
            mappingJson["name"] = mapping.name;
            mappingJson["description"] = mapping.description;
            mappingJson["enabled"] = mapping.enabled;
            
            j["mappings"].push_back(mappingJson);
        }
        
        std::string filename = "preset_" + name + ".json";
        std::ofstream file(filename);
        file << j.dump(4);
        
        return true;
    } catch (const std::exception&) {
        return false;
    }
}

std::vector<ExternalDeviceMapping> DeviceMappingPresets::loadPreset(const std::string& name) {
    std::vector<ExternalDeviceMapping> mappings;
    
    try {
        std::string filename = "preset_" + name + ".json";
        std::ifstream file(filename);
        if (!file.is_open()) {
            return mappings;
        }
        
        json j;
        file >> j;
        
        for (const auto& mappingJson : j["mappings"]) {
            ExternalDeviceMapping mapping;
            mapping.mappingId = mappingJson["mappingId"];
            mapping.deviceId = mappingJson["deviceId"];
            mapping.deviceType = static_cast<ExternalDeviceType>(mappingJson["deviceType"]);
            mapping.inputAddress = mappingJson["inputAddress"];
            mapping.inputMin = mappingJson["inputMin"];
            mapping.inputMax = mappingJson["inputMax"];
            mapping.parameterType = static_cast<MappingParameterType>(mappingJson["parameterType"]);
            mapping.targetChannelId = mappingJson["targetChannelId"];
            mapping.outputMin = mappingJson["outputMin"];
            mapping.outputMax = mappingJson["outputMax"];
            mapping.bidirectional = mappingJson["bidirectional"];
            mapping.inverted = mappingJson["inverted"];
            mapping.name = mappingJson["name"];
            mapping.description = mappingJson["description"];
            mapping.enabled = mappingJson["enabled"];
            
            mappings.push_back(mapping);
        }
    } catch (const std::exception&) {
        // Return empty vector on error
    }
    
    return mappings;
}

std::vector<std::string> DeviceMappingPresets::getAvailablePresets() {
    std::vector<std::string> presets;
    
    // Add built-in presets
    presets.push_back("TouchOSC Mixer");
    presets.push_back("MIDI Controller");
    presets.push_back("Keyboard Shortcuts");
    presets.push_back("Lemur Mixer");
    
    // TODO: Scan for saved preset files
    
    return presets;
}

bool DeviceMappingPresets::deletePreset(const std::string& name) {
    try {
        std::string filename = "preset_" + name + ".json";
        return std::remove(filename.c_str()) == 0;
    } catch (const std::exception&) {
        return false;
    }
}
