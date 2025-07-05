#include "OSCFormatManager.h"
#include <iostream>
#include <sstream>
#include <algorithm>
#include <cstring>

OSCFormatManager::OSCFormatManager() {
    initializeBuiltinTemplates();
    resetStatistics();
}

void OSCFormatManager::addMessageTemplate(const OSCMessageTemplate& tmpl) {
    messageTemplates.push_back(tmpl);
}

void OSCFormatManager::removeMessageTemplate(const std::string& name) {
    messageTemplates.erase(
        std::remove_if(messageTemplates.begin(), messageTemplates.end(),
            [&](const OSCMessageTemplate& tmpl) { return tmpl.name == name; }),
        messageTemplates.end());
}

void OSCFormatManager::updateMessageTemplate(const std::string& name, const OSCMessageTemplate& tmpl) {
    auto it = std::find_if(messageTemplates.begin(), messageTemplates.end(),
        [&](const OSCMessageTemplate& existing) { return existing.name == name; });
    if (it != messageTemplates.end()) {
        *it = tmpl;
    }
}

OSCMessageTemplate* OSCFormatManager::getMessageTemplate(const std::string& name) {
    auto it = std::find_if(messageTemplates.begin(), messageTemplates.end(),
        [&](const OSCMessageTemplate& tmpl) { return tmpl.name == name; });
    return (it != messageTemplates.end()) ? &(*it) : nullptr;
}

void OSCFormatManager::addTarget(const OSCTarget& target) {
    targets.push_back(target);
}

void OSCFormatManager::removeTarget(const std::string& name) {
    targets.erase(
        std::remove_if(targets.begin(), targets.end(),
            [&](const OSCTarget& target) { return target.name == name; }),
        targets.end());
}

OSCTarget* OSCFormatManager::getTarget(const std::string& name) {
    auto it = std::find_if(targets.begin(), targets.end(),
        [&](const OSCTarget& target) { return target.name == name; });
    return (it != targets.end()) ? &(*it) : nullptr;
}

void OSCFormatManager::savePreset(const std::string& name, const std::string& description) {
    OSCPreset preset;
    preset.name = name;
    preset.description = description;
    preset.templates = messageTemplates;
    preset.targets = targets;
    preset.created = std::chrono::system_clock::now();
    presets.push_back(preset);
}

bool OSCFormatManager::loadPreset(const std::string& name) {
    auto it = std::find_if(presets.begin(), presets.end(),
        [&](const OSCPreset& preset) { return preset.name == name; });
    if (it != presets.end()) {
        messageTemplates = it->templates;
        targets = it->targets;
        activePresetName = it->name;
        return true;
    }
    return false;
}

std::vector<OSCFormatManager::GeneratedMessage> OSCFormatManager::generateMessages(const std::vector<float>& cvValues) {
    std::vector<GeneratedMessage> generatedMessages;
    for (const auto& tmpl : messageTemplates) {
        if (!tmpl.enabled) continue;
        for (size_t channel = 0; channel < cvValues.size(); ++channel) {
            if (!tmpl.condition.evaluate(cvValues[channel])) continue;
            GeneratedMessage msg;
            msg.address = tmpl.generateAddress(channel);
            msg.arguments = tmpl.generateArguments(cvValues, channel);
            msg.primaryType = OSCDataType::FLOAT;
            msg.priority = tmpl.priority;
            generatedMessages.push_back(msg);
        }
    }
    return generatedMessages;
}

void OSCFormatManager::setLearningMode(bool enabled) {
    learningMode = enabled;
    if (enabled) clearLearnedPatterns();
}

void OSCFormatManager::learnOSCMessage(const std::string& address, const std::vector<float>& args) {
    if (!learningMode) return;
    auto it = std::find_if(learnedPatterns.begin(), learnedPatterns.end(),
        [&](const OSCLearnedPattern& pattern) { return pattern.address == address; });
    OSCLearnedPattern* pattern = nullptr;
    if (it != learnedPatterns.end()) {
        it->lastReceived = std::chrono::system_clock::now();
        it->lastValues = args;
        it->receiveCount++;
        pattern = &(*it);
    } else {
        OSCLearnedPattern newPattern;
        newPattern.address = address;
        newPattern.argumentTypes.resize(args.size(), OSCDataType::FLOAT);
        newPattern.lastValues = args;
        newPattern.lastReceived = std::chrono::system_clock::now();
        newPattern.receiveCount = 1;
        learnedPatterns.push_back(newPattern);
        pattern = &learnedPatterns.back();
    }
    if (learningCallback && pattern) {
        learningCallback(*pattern);
    }
}

bool OSCCondition::evaluate(float currentValue) const {
    switch (type) {
    case OSCConditionType::ALWAYS:
        return true;
    case OSCConditionType::GREATER_THAN:
        return currentValue > value1;
    case OSCConditionType::LESS_THAN:
        return currentValue < value1;
    case OSCConditionType::EQUAL:
        return currentValue == value1;
    case OSCConditionType::RANGE:
        return currentValue >= value1 && currentValue <= value2;
    case OSCConditionType::CHANGED:
        if (lastValue != currentValue) {
            const_cast<OSCCondition*>(this)->lastValue = currentValue;
            return true;
        }
        break;
    case OSCConditionType::THRESHOLD:
        if (!lastState && currentValue > value1 + hysteresis) {
            const_cast<OSCCondition*>(this)->lastState = true;
            return true;
        } else if (lastState && currentValue < value1 - hysteresis) {
            const_cast<OSCCondition*>(this)->lastState = false;
            return true;
        }
        break;
    }
    return false;
}

void OSCFormatManager::resetStatistics() {
    messageSentCount.clear();
    messageReceivedCount.clear();
    statsStartTime = std::chrono::steady_clock::now();
}

void OSCFormatManager::initializeBuiltinTemplates() {
    // Basic CV template
    OSCMessageTemplate basic;
    basic.name = "basic_cv";
    basic.description = "Basic CV value transmission";
    basic.addressPattern = "/cv/{channel}";
    basic.enabled = true;
    basic.priority = 1;
    basic.condition.type = OSCConditionType::ALWAYS;
    basic.argumentTypes = {OSCDataType::FLOAT};
    basic.argumentSources = {"cv"};
    addMessageTemplate(basic);
    
    // Gate/trigger template
    OSCMessageTemplate gate;
    gate.name = "gate";
    gate.description = "Gate/trigger detection";
    gate.addressPattern = "/gate/{channel}";
    gate.enabled = true;
    gate.priority = 2;
    gate.condition.type = OSCConditionType::THRESHOLD;
    gate.condition.value1 = 0.5f;
    gate.condition.hysteresis = 0.1f;
    gate.argumentTypes = {OSCDataType::INT};
    gate.argumentSources = {"calculated"};
    gate.calculationFormulas = {"cv > 0.5 ? 1 : 0"};
    addMessageTemplate(gate);
}

void OSCFormatManager::clearLearnedPatterns() {
    learnedPatterns.clear();
}

void OSCFormatManager::setLearningCallback(std::function<void(const OSCLearnedPattern&)> callback) {
    learningCallback = callback;
}

// OSCMessageTemplate method implementations
std::string OSCMessageTemplate::generateAddress(int channel) const {
    std::string result = addressPattern;
    size_t pos = result.find("{channel}");
    if (pos != std::string::npos) {
        result.replace(pos, 9, std::to_string(channel));
    }
    return result;
}

std::vector<float> OSCMessageTemplate::generateArguments(const std::vector<float>& cvValues, int channel) const {
    std::vector<float> args;
    
    // Generate arguments based on argumentTypes and argumentSources
    for (size_t i = 0; i < argumentTypes.size(); ++i) {
        if (i < argumentSources.size()) {
            const std::string& source = argumentSources[i];
            if (source == "cv") {
                // Use CV value for this channel
                float value = (channel < static_cast<int>(cvValues.size())) ? cvValues[channel] : 0.0f;
                args.push_back(value * scaleFactor + offset);
            } else if (source == "constant") {
                // Use constant value
                float value = (i < constantValues.size()) ? constantValues[i] : 0.0f;
                args.push_back(value);
            } else if (source == "calculated") {
                // Use calculation formula (simplified for now)
                float cv = (channel < static_cast<int>(cvValues.size())) ? cvValues[channel] : 0.0f;
                float result = cv; // Default to CV value
                if (i < calculationFormulas.size() && !calculationFormulas[i].empty()) {
                    // Simple expression evaluation - for gate example
                    if (calculationFormulas[i] == "cv > 0.5 ? 1 : 0") {
                        result = (cv > 0.5f) ? 1.0f : 0.0f;
                    }
                }
                args.push_back(result);
            }
        }
    }
    
    return args;
}

bool OSCMessageTemplate::shouldSend() const {
    auto now = std::chrono::steady_clock::now();
    return (now - lastSent) >= sendInterval;
}

void OSCFormatManager::recordMessageReceived(const std::string& address) {
    messageReceivedCount[address]++;
}

void OSCFormatManager::recordMessageSent(const std::string& address) {
    messageSentCount[address]++;
}
