#pragma once

#include <string>
#include <vector>
#include <memory>
#include "OSCFormatManager.h"
#include "OSCSender.h"

class OSCAdvancedCLI {
private:
    std::shared_ptr<OSCFormatManager> formatManager;
    std::unique_ptr<OSCReceiver> receiver;
    bool interactive;
    
public:
    OSCAdvancedCLI(std::shared_ptr<OSCFormatManager> formatManager);
    ~OSCAdvancedCLI() = default;
    
    // Main command processing
    bool processCommand(const std::string& command, const std::vector<std::string>& args);
    void runInteractiveMode();
    void printHelp() const;
    
    // Template management commands
    void listTemplates() const;
    void createTemplate();
    void editTemplate(const std::string& name);
    void deleteTemplate(const std::string& name);
    void testTemplate(const std::string& name);
    void importBuiltinTemplates();
    
    // Preset management commands
    void listPresets() const;
    void savePreset(const std::string& name);
    void loadPreset(const std::string& name);
    void deletePreset(const std::string& name);
    void exportPreset(const std::string& name, const std::string& filename);
    void importPreset(const std::string& filename);
    
    // Target management commands
    void listTargets() const;
    void addTarget();
    void editTarget(const std::string& name);
    void deleteTarget(const std::string& name);
    void testTarget(const std::string& name);
    
    // Learning mode commands
    void startLearning();
    void stopLearning();
    void showLearnedPatterns() const;
    void convertLearned(const std::string& address);
    void clearLearned();
    
    // Recording/Playback commands
    void startRecording();
    void stopRecording();
    void saveRecording(const std::string& filename);
    void loadRecording(const std::string& filename);
    void playRecording();
    
    // Bidirectional OSC commands
    void startReceiver(const std::string& port);
    void stopReceiver();
    void showReceiverStatus() const;
    void testSend(const std::string& address, const std::string& value);
    
    // Analytics and statistics
    void showStatistics() const;
    void resetStatistics();
    void exportStatistics(const std::string& filename);
    
    // Configuration
    void saveConfiguration(const std::string& filename);
    void loadConfiguration(const std::string& filename);
    void showConfiguration() const;
    
private:
    // Helper methods
    std::string promptString(const std::string& prompt, const std::string& defaultValue = "") const;
    float promptFloat(const std::string& prompt, float defaultValue = 0.0f) const;
    int promptInt(const std::string& prompt, int defaultValue = 0) const;
    bool promptBool(const std::string& prompt, bool defaultValue = false) const;
    
    OSCDataType promptDataType() const;
    OSCConditionType promptConditionType() const;
    
    void printTemplate(const OSCMessageTemplate& tmpl) const;
    void printTarget(const OSCTarget& target) const;
    void printPreset(const OSCPreset& preset) const;
    void printLearnedPattern(const OSCLearnedPattern& pattern) const;
    
    std::vector<std::string> splitString(const std::string& str, char delimiter) const;
    std::string trim(const std::string& str) const;
    
    // Interactive helpers
    OSCMessageTemplate createTemplateInteractive() const;
    OSCTarget createTargetInteractive() const;
    OSCCondition createConditionInteractive() const;
    
    // Command parsing
    struct Command {
        std::string name;
        std::vector<std::string> args;
        std::string description;
    };
    
    std::vector<Command> parseCommands(const std::string& input) const;
    void showCommandHelp(const std::string& category = "") const;
};

// Built-in template wizard
class OSCTemplateWizard {
public:
    static OSCMessageTemplate createSynthTemplate();
    static OSCMessageTemplate createDAWTemplate();
    static OSCMessageTemplate createLightingTemplate();
    static OSCMessageTemplate createCustomTemplate();
    
    static void showTemplateCategories();
    static std::vector<OSCMessageTemplate> getTemplatesForCategory(const std::string& category);
};
