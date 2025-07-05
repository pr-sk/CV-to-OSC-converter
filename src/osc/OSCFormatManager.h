#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <functional>
#include <chrono>
#include <nlohmann/json.hpp>

// Forward declarations
class OSCSender;

enum class OSCDataType {
    FLOAT = 0,
    INT = 1,
    STRING = 2,
    BLOB = 3,
    DOUBLE = 4,
    BOOLEAN = 5,
    ARRAY = 6
};

enum class OSCConditionType {
    ALWAYS = 0,
    GREATER_THAN = 1,
    LESS_THAN = 2,
    EQUAL = 3,
    RANGE = 4,
    CHANGED = 5,
    THRESHOLD = 6
};

struct OSCCondition {
    OSCConditionType type = OSCConditionType::ALWAYS;
    float value1 = 0.0f;
    float value2 = 0.0f;
    float hysteresis = 0.0f;
    mutable bool lastState = false;
    mutable float lastValue = 0.0f;
    
    bool evaluate(float currentValue) const;
    
    nlohmann::json toJson() const;
    void fromJson(const nlohmann::json& j);
};

struct OSCMessageTemplate {
    std::string name;
    std::string addressPattern;  // e.g., "/synth/{channel}/freq"
    std::vector<OSCDataType> argumentTypes;
    std::vector<std::string> argumentSources; // "cv", "constant", "calculated"
    std::vector<float> constantValues;
    std::vector<std::string> calculationFormulas; // e.g., "cv * 440 + 220"
    OSCCondition condition;
    float scaleFactor = 1.0f;
    float offset = 0.0f;
    bool enabled = true;
    std::string description;
    
    // Advanced features
    std::chrono::milliseconds sendInterval{10}; // Throttling
    std::chrono::steady_clock::time_point lastSent{};
    bool bundleOptimization = true;
    int priority = 0; // For bundle ordering
    
    nlohmann::json toJson() const;
    void fromJson(const nlohmann::json& j);
    
    std::string generateAddress(int channel) const;
    std::vector<float> generateArguments(const std::vector<float>& cvValues, int channel) const;
    bool shouldSend() const;
};

struct OSCTarget {
    std::string name;
    std::string host;
    std::string port;
    bool enabled = true;
    std::vector<std::string> enabledTemplates; // Template names to use for this target
    
    // Security features
    bool requiresAuth = false;
    std::string authToken;
    bool useEncryption = false;
    std::string encryptionKey;
    
    nlohmann::json toJson() const;
    void fromJson(const nlohmann::json& j);
};

struct OSCPreset {
    std::string name;
    std::string description;
    std::vector<OSCMessageTemplate> templates;
    std::vector<OSCTarget> targets;
    std::map<int, std::pair<float, float>> cvRanges; // Channel -> {min, max}
    std::chrono::system_clock::time_point created;
    std::chrono::system_clock::time_point lastUsed;
    
    nlohmann::json toJson() const;
    void fromJson(const nlohmann::json& j);
};

// OSC Learning system for incoming messages
struct OSCLearnedPattern {
    std::string address;
    std::vector<OSCDataType> argumentTypes;
    std::vector<float> lastValues;
    std::chrono::system_clock::time_point lastReceived;
    int receiveCount = 0;
    bool isActive = true;
    
    nlohmann::json toJson() const;
    void fromJson(const nlohmann::json& j);
};

class OSCFormatManager {
private:
    std::vector<OSCMessageTemplate> messageTemplates;
    std::vector<OSCTarget> targets;
    std::vector<OSCPreset> presets;
    std::string activePresetName;
    
    // Learning mode
    bool learningMode = false;
    std::vector<OSCLearnedPattern> learnedPatterns;
    std::function<void(const OSCLearnedPattern&)> learningCallback;
    
    // Recording/Playback
    bool recordingMode = false;
    std::vector<std::pair<std::chrono::steady_clock::time_point, nlohmann::json>> recordedData;
    std::chrono::steady_clock::time_point recordingStart;
    
    // Statistics
    std::map<std::string, size_t> messageSentCount;
    std::map<std::string, size_t> messageReceivedCount;
    std::chrono::steady_clock::time_point statsStartTime;
    
    // Expression evaluator for calculations
    float evaluateExpression(const std::string& expression, float cv, int channel) const;
    
public:
    OSCFormatManager();
    ~OSCFormatManager() = default;
    
    // Template management
    void addMessageTemplate(const OSCMessageTemplate& tmpl);
    void removeMessageTemplate(const std::string& name);
    void updateMessageTemplate(const std::string& name, const OSCMessageTemplate& tmpl);
    OSCMessageTemplate* getMessageTemplate(const std::string& name);
    std::vector<OSCMessageTemplate>& getMessageTemplates() { return messageTemplates; }
    const std::vector<OSCMessageTemplate>& getMessageTemplates() const { return messageTemplates; }
    
    // Target management
    void addTarget(const OSCTarget& target);
    void removeTarget(const std::string& name);
    void updateTarget(const std::string& name, const OSCTarget& target);
    OSCTarget* getTarget(const std::string& name);
    std::vector<OSCTarget>& getTargets() { return targets; }
    const std::vector<OSCTarget>& getTargets() const { return targets; }
    
    // Preset management
    void savePreset(const std::string& name, const std::string& description = "");
    bool loadPreset(const std::string& name);
    void deletePreset(const std::string& name);
    std::vector<std::string> getPresetNames() const;
    const OSCPreset* getPreset(const std::string& name) const;
    std::string getActivePresetName() const { return activePresetName; }
    
    // Message generation
    struct GeneratedMessage {
        std::string address;
        std::vector<float> arguments;
        OSCDataType primaryType;
        std::string targetName;
        int priority;
    };
    
    std::vector<GeneratedMessage> generateMessages(const std::vector<float>& cvValues);
    
    // Learning mode
    void setLearningMode(bool enabled);
    bool isLearningMode() const { return learningMode; }
    void setLearningCallback(std::function<void(const OSCLearnedPattern&)> callback);
    void learnOSCMessage(const std::string& address, const std::vector<float>& args);
    const std::vector<OSCLearnedPattern>& getLearnedPatterns() const { return learnedPatterns; }
    void clearLearnedPatterns();
    OSCMessageTemplate createTemplateFromPattern(const OSCLearnedPattern& pattern);
    
    // Recording/Playback
    void startRecording();
    void stopRecording();
    bool isRecording() const { return recordingMode; }
    void saveRecording(const std::string& filename);
    bool loadRecording(const std::string& filename);
    void playbackRecording(std::function<void(const std::vector<float>&)> callback);
    void recordCVData(const std::vector<float>& cvValues);
    
    // Statistics and analytics
    void recordMessageSent(const std::string& address);
    void recordMessageReceived(const std::string& address);
    std::map<std::string, size_t> getMessageSentStats() const { return messageSentCount; }
    std::map<std::string, size_t> getMessageReceivedStats() const { return messageReceivedCount; }
    void resetStatistics();
    std::string generateStatsReport() const;
    
    // Configuration persistence
    void saveConfiguration(const std::string& filename);
    bool loadConfiguration(const std::string& filename);
    nlohmann::json exportConfiguration() const;
    void importConfiguration(const nlohmann::json& config);
    
    // Built-in template library
    void loadBuiltinTemplates();
    void addBuiltinTemplate(const std::string& category, const OSCMessageTemplate& tmpl);
    std::map<std::string, std::vector<OSCMessageTemplate>> getBuiltinTemplates() const;
    
    // Validation and testing
    bool validateTemplate(const OSCMessageTemplate& tmpl) const;
    std::vector<std::string> testTemplate(const OSCMessageTemplate& tmpl, const std::vector<float>& testValues) const;
    
private:
    std::map<std::string, std::vector<OSCMessageTemplate>> builtinTemplates;
    void initializeBuiltinTemplates();
};

// Built-in template categories
namespace OSCTemplateLibrary {
    // Synthesizer control templates
    std::vector<OSCMessageTemplate> getSynthTemplates();
    
    // DAW control templates  
    std::vector<OSCMessageTemplate> getDAWTemplates();
    
    // Effects control templates
    std::vector<OSCMessageTemplate> getEffectsTemplates();
    
    // Lighting control templates
    std::vector<OSCMessageTemplate> getLightingTemplates();
    
    // VJ/Video control templates
    std::vector<OSCMessageTemplate> getVideoTemplates();
    
    // Game engine templates
    std::vector<OSCMessageTemplate> getGameEngineTemplates();
}
