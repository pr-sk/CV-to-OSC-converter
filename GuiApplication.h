#pragma once

#include <memory>
#include <vector>
#include <atomic>
#include <thread>
#include <mutex>
#include <deque>
#include <chrono>
#include <map>
#include <functional>
#include "AudioDeviceManager.h"
#include "OSCReceiver.h"

// Forward declarations
class CommandManager;
class HotKeyManager;
class CVReader;
class CVWriter;
class OSCSender;
class Config;
class DragDropManager;
class ExternalDeviceManager;
class ExternalDevicePresets;
struct GLFWwindow;


// Data structures for GUI
struct CVChannelData {
    int channelId;
    std::string name;
    float currentValue;
    float normalizedValue;
    std::deque<float> history;
    bool enabled = true;
    float minRange = -10.0f;
    float maxRange = 10.0f;
    std::string oscAddress;
    
    // Visual settings
    bool showInPlot = true;
    float plotColor[3] = {1.0f, 1.0f, 1.0f};
    
    // Faders and knobs for real-time control
    struct ChannelControls {
        float fader = 0.0f;          // Main fader (0.0 - 1.0)
        float gainKnob = 1.0f;       // Gain control (0.0 - 2.0)
        float offsetKnob = 0.0f;     // Offset control (-1.0 - 1.0)
        float filterKnob = 1.0f;     // Filter cutoff (0.0 - 1.0)
        float mixKnob = 1.0f;        // Mix with input (0.0 - 1.0)
        bool muteButton = false;     // Mute channel
        bool soloButton = false;     // Solo channel
        bool linkButton = false;     // Link to next channel
        
        // External device mappings
        int faderMidiCC = -1;        // MIDI CC for fader (-1 = unmapped)
        int gainMidiCC = -1;         // MIDI CC for gain knob
        int offsetMidiCC = -1;       // MIDI CC for offset knob
        int filterMidiCC = -1;       // MIDI CC for filter knob
        int mixMidiCC = -1;          // MIDI CC for mix knob
        int muteMidiCC = -1;         // MIDI CC for mute button
        int soloMidiCC = -1;         // MIDI CC for solo button
        
        // OSC mappings
        std::string faderOSCAddress = "";
        std::string gainOSCAddress = "";
        std::string offsetOSCAddress = "";
        std::string filterOSCAddress = "";
        std::string mixOSCAddress = "";
        std::string muteOSCAddress = "";
        std::string soloOSCAddress = "";
    } controls;
    
    void addSample(float value) {
        currentValue = value;
        history.push_back(value);
        if (history.size() > 1000) { // Keep last 1000 samples
            history.pop_front();
        }
    }
    
    // Apply channel processing with controls
    float processValue(float inputValue) {
        if (controls.muteButton) return 0.0f;
        
        float processed = inputValue;
        
        // Apply offset
        processed += controls.offsetKnob * (maxRange - minRange) * 0.5f;
        
        // Apply gain
        processed *= controls.gainKnob;
        
        // Apply fader
        processed *= controls.fader;
        
        // Mix with original (if mix < 1.0)
        if (controls.mixKnob < 1.0f) {
            processed = processed * controls.mixKnob + inputValue * (1.0f - controls.mixKnob);
        }
        
        // Clamp to range
        processed = std::clamp(processed, minRange, maxRange);
        
        return processed;
    }
};

struct OSCConnectionInfo {
    std::string host;
    std::string port;
    bool connected = false;
    int messagesSent = 0;
    int messagesPerSecond = 0;
    std::chrono::steady_clock::time_point lastUpdate;
};

struct OSCTestResult {
    bool hasResult = false;
    bool success = false;
    float latencyMs = 0.0f;
    std::string errorMessage;
    std::chrono::steady_clock::time_point timestamp;
};

struct AudioDeviceRefreshResult {
    bool hasResult = false;
    bool success = false;
    int deviceCount = 0;
    std::string errorMessage;
    std::chrono::steady_clock::time_point timestamp;
};


class GuiApplication {
public:
    GuiApplication();
    ~GuiApplication();
    
    // Main application lifecycle
    bool initialize();
    void run();
    void shutdown();
    
    // Configuration
    void loadConfig(const std::string& configFile = "config.json");
    void saveConfig(const std::string& configFile = "config.json");
    
    // CV/OSC control
    void startConversion();
    void stopConversion();
    void startOSCListening(); // New Method
    void stopOSCListening(); // New Method
    bool isRunning() const { return running_; }
    
    // Command system
    void executeCommand(std::unique_ptr<class Command> command);
    void undo();
    void redo();
    bool canUndo() const;
    bool canRedo() const;
    
    // Hot keys
    void setupHotKeys();
    void processHotKeys();
    void showHotKeyEditor();
    
    // OSC testing
    void testOSCConnection();
    
    // Audio device management
    void refreshAudioDevices();
    
    // Performance monitoring
    void resetPerformanceCounters();
    void updatePerformanceMetrics();
    
private:
    // Core components
    std::unique_ptr<CVReader> cvReader_;
    std::unique_ptr<CVWriter> cvWriter_; // For OSC to CV conversion
    std::unique_ptr<OSCSender> oscSender_;
    std::unique_ptr<OSCReceiver> oscReceiver_; // New Member for OSC Receiving
    std::unique_ptr<Config> config_;
    std::unique_ptr<CommandManager> commandManager_;
    std::unique_ptr<HotKeyManager> hotKeyManager_;
    std::unique_ptr<ExternalDeviceManager> externalDeviceManager_;
    std::unique_ptr<ExternalDevicePresets> devicePresets_;
    
    // GUI state
    GLFWwindow* window_;
    bool running_;
    bool showDemo_;
    bool showMetrics_;
    
    // OSC listening state
    std::string oscListenPort_;
    bool oscListening_;
    
    // Data management
    std::vector<CVChannelData> channels_;
    OSCConnectionInfo oscInfo_;
    OSCTestResult oscTestResult_;
    AudioDeviceInfo audioInfo_;
    AudioDeviceRefreshResult audioDeviceRefreshResult_;
    bool audioDeviceRefreshInProgress_ = false;
    std::thread workerThread_;
    std::mutex dataMutex_;
    std::atomic<bool> workerRunning_;
    
    // GUI Windows
    bool showMainWindow_ = true;
    bool showConfigWindow_ = true;
    bool showChannelConfig_ = false;
    bool showOSCConfig_ = true;
    bool showAudioConfig_ = true;
    bool showPerformanceWindow_ = true;
    bool showAboutWindow_ = false;
    bool showThemeEditor_ = false;
    bool showHotKeyEditor_ = false;
    bool showWelcomeDialog_ = true;
    bool firstLaunch_ = true;
    
    // New control windows
    bool showMixerWindow_ = true;
    bool showExternalMappingWindow_ = false;
    
    // Enhanced visualization windows
    struct ChannelVisualizationState {
        bool showIndividualWindows = false;
        std::vector<uint8_t> showChannelWindow; // Using uint8_t instead of bool for ImGui compatibility
        float zoomLevel = 1.0f;
        float timeRange = 10.0f; // seconds
        float voltageMin = -12.0f;
        float voltageMax = 12.0f;
        bool autoScale = true;
    } visualizationState_;
    
    // Drag & Drop state
    struct DragDropState {
        bool dragging = false;
        int sourceChannel = -1;
        int targetChannel = -1;
    } dragDropState_;
    
    // Performance monitoring
    struct PerformanceData {
        float fps = 0.0f;
        float cpuUsage = 0.0f;
        int droppedFrames = 0;
        
        // Memory metrics
        float totalMemoryMB = 0.0f;
        float usedMemoryMB = 0.0f;
        float audioBufferMs = 0.0f;
        
        // Audio metrics
        float audioLatencyMs = 0.0f;
        int bufferUnderruns = 0;
        float actualSampleRate = 0.0f;
        
        // Network metrics
        int oscMessagesPerSec = 0;
        float networkLatencyMs = 0.0f;
        int oscFailedSends = 0;
        
        std::chrono::steady_clock::time_point lastUpdate;
    } performanceData_;
    
    // GUI Methods
    void setupImGuiStyle();
    void setupFonts();
    void renderMainMenuBar();
    void renderMainWindow();
    void renderChannelVisualization();
    void renderChannelConfiguration();
    void renderOSCConfiguration();
    void renderAudioConfiguration();
    void renderPerformanceMonitor();
    void renderAboutWindow();
    
    // Real-time visualization
    void renderRealtimePlot();
    void renderChannelMeters();
    void renderOSCStatus();
    
    // Enhanced visualization methods
    void renderIndividualChannelWindows();
    void renderChannelWindow(int channelIndex);
    void renderVisualizationControls();
    
    // Channel controls rendering
    void renderChannelControls();
    void renderChannelStrip(CVChannelData& channel);
    void renderMixerView();
    void renderExternalMappingWindow();
    
    // Configuration dialogs
    void renderChannelConfigDialog(CVChannelData& channel);
    void handleDragAndDrop();
    
    // Worker thread
    void workerLoop();
    void updateChannelData();
    void updateOSCStatus();
    
    // Utility methods
    void initializeChannels();
    void resetChannelHistory();
    const char* getChannelColorName(int channelId);
    void applyChannelConfiguration();
    void arrangeWindows();
    void showWelcomeDialog();
};
