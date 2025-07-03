// OpenGL/GLEW includes (must be first!)
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "GuiApplication.h"
#include "CVReader.h"
#include "CVWriter.h"
#include "OSCSender.h"
#include "Config.h"
#include "AudioDeviceManager.h"
#include "Version.h"
#include "DragDropManager.h"
#include "GuiThemes.h"
#include "ThemeEditor.h"
#include "CommandSystem.h"
#include "HotKeyManager.h"
#include "HotKeyEditor.h"
#include "Localization.h"
#include "FileDialog.h"
#include "ExternalDeviceManager.h"

// ImGui includes
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "implot.h"

#include <iostream>
#include <algorithm>
#include <sstream>
#include <iomanip>

#ifdef __APPLE__
#include <mach/mach.h>
#include <mach/task.h>
#endif

GuiApplication::GuiApplication() 
    : window_(nullptr), running_(false), showDemo_(false), showMetrics_(false),
      workerRunning_(false) {
    // Initialize OSC listening port from config or default
    oscListenPort_ = "8001"; // Default OSC listening port
    oscListening_ = false;
    
    // Initialize visualization state
    visualizationState_.showChannelWindow.resize(8, false); // Default 8 channels
}

GuiApplication::~GuiApplication() {
    shutdown();
}

bool GuiApplication::initialize() {
    // Initialize GLFW with macOS specific hints
    glfwSetErrorCallback([](int error, const char* description) {
        std::cerr << "GLFW Error " << error << ": " << description << std::endl;
    });
    
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return false;
    }

    // Setup OpenGL context
#ifdef __APPLE__
    // macOS: Use OpenGL 3.2 Core Profile for better compatibility
    const char* glsl_version = "#version 150";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_COCOA_RETINA_FRAMEBUFFER, GLFW_TRUE);
#else
    // Other platforms: Use OpenGL 3.0
    const char* glsl_version = "#version 130";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
#endif

    // Create window
    window_ = glfwCreateWindow(1280, 720, "CV to OSC Converter - GUI", NULL, NULL);
    if (window_ == NULL) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return false;
    }
    
    glfwMakeContextCurrent(window_);
    glfwSwapInterval(1); // Enable vsync

    // Initialize OpenGL loader
    if (glewInit() != GLEW_OK) {
        std::cerr << "Failed to initialize OpenGL loader" << std::endl;
        return false;
    }

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImPlot::CreateContext();
    
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    // Note: Docking and Viewports not available in this ImGui version

    // Setup Dear ImGui style
    setupImGuiStyle();

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(window_, true);
    ImGui_ImplOpenGL3_Init(glsl_version);
    
    // Setup fonts after backend initialization
    // setupFonts(); // Temporarily disabled due to ImGui backend issue

    // Initialize application components
    config_ = std::make_unique<Config>();
    commandManager_ = std::make_unique<CommandManager>();
    hotKeyManager_ = std::make_unique<HotKeyManager>();
    externalDeviceManager_ = std::make_unique<ExternalDeviceManager>();
    devicePresets_ = std::make_unique<ExternalDevicePresets>();
    
    // Initialize localization
    Localization::getInstance().initialize();
    
    loadConfig();
    
    // Set language from config
    Localization::getInstance().setLanguage(config_->getLanguage());
    
    // Initialize channels based on configuration
    initializeChannels();
    
    // Setup hot keys
    setupHotKeys();
    
    // Arrange windows for first launch
    if (firstLaunch_) {
        arrangeWindows();
    }
    
    return true;
}

void GuiApplication::run() {
    running_ = true;
    
    while (!glfwWindowShouldClose(window_) && running_) {
        glfwPollEvents();

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // Note: Docking disabled - not available in this ImGui version

        // Render GUI components
        renderMainMenuBar();
        
        if (showMainWindow_) renderMainWindow();
        if (showConfigWindow_) renderChannelConfiguration();
        if (showOSCConfig_) renderOSCConfiguration();
        if (showAudioConfig_) renderAudioConfiguration();
        if (showPerformanceWindow_) renderPerformanceMonitor();
        if (showAboutWindow_) renderAboutWindow();
        if (showThemeEditor_) ThemeEditor::ShowThemeEditor(&showThemeEditor_);
        if (showHotKeyEditor_) HotKeyEditor::ShowHotKeyEditor(hotKeyManager_.get(), &showHotKeyEditor_);
        
        // Render individual channel windows
        renderIndividualChannelWindows();
        
        // Render new control windows
        if (showMixerWindow_) renderMixerView();
        if (showExternalMappingWindow_) renderExternalMappingWindow();
        
        // Show welcome dialog
        showWelcomeDialog();
        
        // Demo windows for development
        if (showDemo_) ImGui::ShowDemoWindow(&showDemo_);
        if (showMetrics_) ImGui::ShowMetricsWindow(&showMetrics_);

        // Handle drag and drop
        handleDragAndDrop();
        
        // Process hot keys
        if (hotKeyManager_) {
            hotKeyManager_->processHotKeys();
        }
        
        // Update performance metrics
        updatePerformanceMetrics();

        // Rendering
        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window_, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        // Note: Multi-viewport support disabled - not available in this ImGui version

        glfwSwapBuffers(window_);
    }
    
    running_ = false;
}

void GuiApplication::shutdown() {
    if (workerRunning_) {
        stopConversion();
    }
    
    if (window_) {
        // Cleanup
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImPlot::DestroyContext();
        ImGui::DestroyContext();

        glfwDestroyWindow(window_);
        glfwTerminate();
        window_ = nullptr;
    }
}

void GuiApplication::setupFonts() {
    // Font setup disabled due to ImGui backend issues
    // Using default fonts only
}

void GuiApplication::setupImGuiStyle() {
    ImGuiStyle& style = ImGui::GetStyle();
    
    // Dark theme with custom colors
    ImGui::StyleColorsDark();
    
    // Custom color scheme for audio application
    ImVec4* colors = style.Colors;
    colors[ImGuiCol_WindowBg] = ImVec4(0.08f, 0.08f, 0.10f, 1.00f);
    colors[ImGuiCol_ChildBg] = ImVec4(0.12f, 0.12f, 0.14f, 1.00f);
    colors[ImGuiCol_PopupBg] = ImVec4(0.12f, 0.12f, 0.14f, 1.00f);
    colors[ImGuiCol_Border] = ImVec4(0.25f, 0.25f, 0.27f, 1.00f);
    colors[ImGuiCol_FrameBg] = ImVec4(0.16f, 0.16f, 0.18f, 1.00f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.22f, 0.22f, 0.24f, 1.00f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.28f, 0.28f, 0.30f, 1.00f);
    colors[ImGuiCol_TitleBg] = ImVec4(0.06f, 0.06f, 0.08f, 1.00f);
    colors[ImGuiCol_TitleBgActive] = ImVec4(0.10f, 0.10f, 0.12f, 1.00f);
    colors[ImGuiCol_MenuBarBg] = ImVec4(0.10f, 0.10f, 0.12f, 1.00f);
    colors[ImGuiCol_Button] = ImVec4(0.20f, 0.20f, 0.22f, 1.00f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.28f, 0.28f, 0.30f, 1.00f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.35f, 0.35f, 0.37f, 1.00f);
    colors[ImGuiCol_Header] = ImVec4(0.20f, 0.20f, 0.22f, 1.00f);
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.25f, 0.25f, 0.27f, 1.00f);
    colors[ImGuiCol_HeaderActive] = ImVec4(0.30f, 0.30f, 0.32f, 1.00f);
    
    // Audio-specific accent colors
    colors[ImGuiCol_SliderGrab] = ImVec4(0.00f, 0.65f, 0.35f, 1.00f);
    colors[ImGuiCol_SliderGrabActive] = ImVec4(0.00f, 0.75f, 0.45f, 1.00f);
    colors[ImGuiCol_CheckMark] = ImVec4(0.00f, 0.75f, 0.45f, 1.00f);
    
    // Rounded corners
    style.WindowRounding = 4.0f;
    style.ChildRounding = 4.0f;
    style.FrameRounding = 3.0f;
    style.PopupRounding = 3.0f;
    style.ScrollbarRounding = 3.0f;
    style.GrabRounding = 3.0f;
    style.TabRounding = 3.0f;
}

void GuiApplication::renderMainMenuBar() {
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu(_("menu.file"))) {
            if (ImGui::MenuItem("Load Config", "Ctrl+O")) {
                std::string configPath = FileDialog::openFile(
                    "Load Configuration",
                    {{"Configuration Files", "*.json"}, {"All Files", "*.*"}},
                    FileDialog::getConfigPath()
                );
                if (!configPath.empty()) {
                    loadConfig(configPath);
                }
            }
            if (ImGui::MenuItem("Save Config", "Ctrl+S")) {
                saveConfig();
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Exit", "Alt+F4")) {
                running_ = false;
            }
            ImGui::EndMenu();
        }
        
        if (ImGui::BeginMenu(_("menu.view"))) {
            if (ImGui::MenuItem("Show All Windows", "Ctrl+Shift+A")) {
                showMainWindow_ = true;
                showConfigWindow_ = true;
                showOSCConfig_ = true;
                showAudioConfig_ = true;
                showPerformanceWindow_ = true;
                arrangeWindows();
            }
            if (ImGui::MenuItem("Show Welcome Dialog")) {
                showWelcomeDialog_ = true;
            }
            ImGui::Separator();
            ImGui::MenuItem(_("window.main"), "Ctrl+1", &showMainWindow_);
            ImGui::MenuItem(_("window.channels"), "Ctrl+2", &showConfigWindow_);
            ImGui::MenuItem(_("window.osc"), "Ctrl+3", &showOSCConfig_);
            ImGui::MenuItem(_("window.audio"), "Ctrl+4", &showAudioConfig_);
            ImGui::MenuItem(_("window.performance"), "Ctrl+5", &showPerformanceWindow_);
            ImGui::Separator();
            ImGui::MenuItem("CV Mixer", "Ctrl+6", &showMixerWindow_);
            ImGui::MenuItem("External Device Mapping", "Ctrl+7", &showExternalMappingWindow_);
            ImGui::Separator();
            if (ImGui::BeginMenu("Individual Channel Windows")) {
                ImGui::MenuItem("Show All Individual Windows", nullptr, &visualizationState_.showIndividualWindows);
                ImGui::Separator();
                for (int i = 0; i < static_cast<int>(visualizationState_.showChannelWindow.size()); ++i) {
                    std::string label = "Channel " + std::to_string(i + 1) + " Window";
                    bool isChecked = static_cast<bool>(visualizationState_.showChannelWindow[i]);
                    if (ImGui::MenuItem(label.c_str(), nullptr, isChecked)) {
                        visualizationState_.showChannelWindow[i] = isChecked ? 0 : 1;
                    }
                }
                ImGui::EndMenu();
            }
            ImGui::Separator();
            ImGui::MenuItem("ImGui Demo", nullptr, &showDemo_);
            ImGui::MenuItem("ImGui Metrics", nullptr, &showMetrics_);
            ImGui::EndMenu();
        }
        
        if (ImGui::BeginMenu("Control")) {
            if (ImGui::MenuItem("Start Conversion", nullptr, false, !workerRunning_)) {
                startConversion();
            }
            if (ImGui::MenuItem("Stop Conversion", nullptr, false, workerRunning_)) {
                stopConversion();
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Reset Channel History")) {
                resetChannelHistory();
            }
            ImGui::EndMenu();
        }
        
        if (ImGui::BeginMenu(_("menu.settings"))) {
            if (ImGui::BeginMenu(_("menu.language"))) {
                auto& localization = Localization::getInstance();
                auto currentLang = localization.getCurrentLanguage();
                auto availableLanguages = localization.getAvailableLanguages();
                
                for (const auto& [lang, nativeName] : availableLanguages) {
                    bool isSelected = (lang == currentLang);
                    if (ImGui::MenuItem(nativeName.c_str(), nullptr, isSelected)) {
                        if (lang != currentLang) {
                            localization.setLanguage(lang);
                            config_->setLanguage(lang);
                            saveConfig(); // Save language preference
                        }
                    }
                }
                ImGui::EndMenu();
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Theme Editor", "Ctrl+Shift+T", showThemeEditor_)) {
                showThemeEditor_ = !showThemeEditor_;
            }
            if (ImGui::MenuItem("Hot Key Editor", "Ctrl+Shift+H", showHotKeyEditor_)) {
                showHotKeyEditor_ = !showHotKeyEditor_;
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Dark Professional")) {
                GuiThemes::ApplyDarkProfessional();
            }
            if (ImGui::MenuItem("Neon Synthwave")) {
                GuiThemes::ApplyNeonSynthwave();
            }
            if (ImGui::MenuItem("Light Professional")) {
                GuiThemes::ApplyLightProfessional();
            }
            if (ImGui::MenuItem("Retro Terminal")) {
                GuiThemes::ApplyRetroTerminal();
            }
            if (ImGui::MenuItem("macOS Modern")) {
                GuiThemes::ApplyMacOSModern();
            }
            ImGui::EndMenu();
        }
        
        if (ImGui::BeginMenu(_("menu.help"))) {
            if (ImGui::MenuItem("About")) {
                showAboutWindow_ = true;
            }
            ImGui::EndMenu();
        }
        
        // Status indicators on the right side
        float menuBarHeight = ImGui::GetFrameHeight();
        ImGui::SameLine(ImGui::GetWindowWidth() - 200);
        
        // Connection status
        if (workerRunning_) {
            ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "RUNNING");
        } else {
            ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "STOPPED");
        }
        
        ImGui::SameLine();
        if (oscInfo_.connected) {
            ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "OSC OK");
        } else {
            ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "OSC ERR");
        }
        
        ImGui::EndMainMenuBar();
    }
}

void GuiApplication::renderMainWindow() {
    ImGui::Begin(_("window.main"), &showMainWindow_);
    
    // Control panel at the top
    ImGui::Text("Control Panel");
    ImGui::Separator();
    
    if (ImGui::Button(workerRunning_ ? _("button.stop") : _("button.start"))) {
        if (workerRunning_) {
            stopConversion();
        } else {
            startConversion();
        }
    }
    
    ImGui::SameLine();
    if (ImGui::Button("Reset History")) {
        resetChannelHistory();
    }
    
    // Status information
    ImGui::Spacing();
    ImGui::Text("Status Information");
    ImGui::Separator();
    
    ImGui::Text("%s: %s", _("audio.current_device"), audioInfo_.name.c_str());
    ImGui::Text("Channels: %d", audioInfo_.maxInputChannels);
    ImGui::Text("%s: %.0f Hz", _("audio.sample_rate"), audioInfo_.defaultSampleRate);
    ImGui::Text("OSC Target: %s:%s", oscInfo_.host.c_str(), oscInfo_.port.c_str());
    ImGui::Text("Messages Sent: %d (%d/sec)", oscInfo_.messagesSent, oscInfo_.messagesPerSecond);
    
    ImGui::Spacing();
    
    // Channel visualization
    renderChannelVisualization();
    
    ImGui::End();
}

void GuiApplication::renderChannelVisualization() {
    ImGui::Text("Real-time CV Visualization");
    ImGui::Separator();
    
    // Channel meters
    renderChannelMeters();
    
    ImGui::Spacing();
    
    // Real-time plot
    renderRealtimePlot();
}

void GuiApplication::renderChannelMeters() {
    ImGui::Text("Channel Meters");
    
    // Create a table for better layout
    if (ImGui::BeginTable("ChannelMeters", 5, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
        ImGui::TableSetupColumn("Ch", ImGuiTableColumnFlags_WidthFixed, 30.0f);
        ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthFixed, 80.0f);
        ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthFixed, 60.0f);
        ImGui::TableSetupColumn("Meter", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("OSC", ImGuiTableColumnFlags_WidthFixed, 30.0f);
        ImGui::TableHeadersRow();
        
        std::lock_guard<std::mutex> lock(dataMutex_);
        
        for (auto& channel : channels_) {
            ImGui::TableNextRow();
            
            // Channel number
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("%d", channel.channelId + 1);
            
            // Channel name (editable)
            ImGui::TableSetColumnIndex(1);
            char nameBuffer[64];
            strncpy(nameBuffer, channel.name.c_str(), sizeof(nameBuffer));
            nameBuffer[sizeof(nameBuffer) - 1] = '\0';
            ImGui::PushID(channel.channelId);
            if (ImGui::InputText("##name", nameBuffer, sizeof(nameBuffer))) {
                channel.name = nameBuffer;
            }
            ImGui::PopID();
            
            // Current value
            ImGui::TableSetColumnIndex(2);
            ImGui::Text("%.3f", channel.currentValue);
            
            // Meter visualization
            ImGui::TableSetColumnIndex(3);
            float normalizedValue = (channel.currentValue - channel.minRange) / (channel.maxRange - channel.minRange);
            normalizedValue = std::clamp(normalizedValue, 0.0f, 1.0f);
            
            ImGui::PushID(channel.channelId + 1000);
            ImGui::ProgressBar(normalizedValue, ImVec2(-1, 0), "");
            ImGui::PopID();
            
            // Enable/disable OSC
            ImGui::TableSetColumnIndex(4);
            ImGui::PushID(channel.channelId + 2000);
            ImGui::Checkbox("##osc", &channel.enabled);
            ImGui::PopID();
        }
        
        ImGui::EndTable();
    }
}

void GuiApplication::renderRealtimePlot() {
    ImGui::Text("Real-time Plot");
    
    if (ImPlot::BeginPlot("CV Signals", ImVec2(-1, 300))) {
        ImPlot::SetupAxes("Time", "Voltage");
        ImPlot::SetupAxisLimits(ImAxis_X1, -10, 0, ImGuiCond_Always);
        ImPlot::SetupAxisLimits(ImAxis_Y1, -12, 12, ImGuiCond_FirstUseEver);
        
        std::lock_guard<std::mutex> lock(dataMutex_);
        
        for (const auto& channel : channels_) {
            if (!channel.showInPlot || channel.history.empty()) continue;
            
            // Prepare data for plotting
            std::vector<float> timeData, valueData;
            timeData.reserve(channel.history.size());
            valueData.reserve(channel.history.size());
            
            float timeStep = 10.0f / channel.history.size(); // Last 10 seconds
            for (size_t i = 0; i < channel.history.size(); ++i) {
                timeData.push_back(-10.0f + i * timeStep);
                valueData.push_back(channel.history[i]);
            }
            
            ImPlot::SetNextLineStyle(ImVec4(channel.plotColor[0], channel.plotColor[1], channel.plotColor[2], 1.0f));
            ImPlot::PlotLine(channel.name.c_str(), timeData.data(), valueData.data(), (int)timeData.size());
        }
        
        ImPlot::EndPlot();
    }
}

void GuiApplication::startConversion() {
    if (workerRunning_) return;
    
    try {
        // Initialize CV reader and OSC sender
        cvReader_ = std::make_unique<CVReader>(audioInfo_.name);
        oscSender_ = std::make_unique<OSCSender>(oscInfo_.host, oscInfo_.port);
        
        // Start worker thread
        workerRunning_ = true;
        workerThread_ = std::thread(&GuiApplication::workerLoop, this);
        
        std::cout << "Conversion started" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Failed to start conversion: " << e.what() << std::endl;
        workerRunning_ = false;
    }
}

void GuiApplication::stopConversion() {
    if (!workerRunning_) return;
    
    workerRunning_ = false;
    if (workerThread_.joinable()) {
        workerThread_.join();
    }
    
    cvReader_.reset();
    oscSender_.reset();
    
    std::cout << "Conversion stopped" << std::endl;
}

void GuiApplication::startOSCListening() {
    if (oscListening_) return;
    
    try {
        // Initialize OSC receiver
        oscReceiver_ = std::make_unique<OSCReceiver>(oscListenPort_);
        
        // Set up callback for OSC to CV conversion
        oscReceiver_->setMessageCallback([this](const std::string& address, const std::vector<float>& values) {
            // Parse OSC address to determine CV channel
            // Expected format: /cv/channel/N where N is channel number
            size_t lastSlash = address.find_last_of('/');
            if (lastSlash != std::string::npos) {
                try {
                    int channelId = std::stoi(address.substr(lastSlash + 1)) - 1; // Convert to 0-based
                    if (channelId >= 0 && channelId < static_cast<int>(channels_.size()) && !values.empty()) {
                        float oscValue = values[0];
                        
                        // Convert normalized OSC value (0.0-1.0) to CV range
                        std::lock_guard<std::mutex> lock(dataMutex_);
                        float cvValue = channels_[channelId].minRange + 
                                       oscValue * (channels_[channelId].maxRange - channels_[channelId].minRange);
                        
                        // Update channel data
                        channels_[channelId].addSample(cvValue);
                        channels_[channelId].normalizedValue = oscValue;
                        
        // Send CV value to audio output if available
        if (cvWriter_) {
            try {
                cvWriter_->writeChannel(channelId, cvValue);
            } catch (const std::exception& e) {
                std::cerr << "Failed to write CV value: " << e.what() << std::endl;
            }
        }
        
        // Log the conversion (only in debug mode to avoid spam)
        #ifdef DEBUG
        std::cout << "OSC->CV: Channel " << (channelId + 1) 
                 << " = " << cvValue << "V (from OSC " << oscValue << ")" << std::endl;
        #endif
                    }
                } catch (const std::exception& e) {
                    std::cerr << "Error parsing OSC channel: " << e.what() << std::endl;
                }
            }
        });
        
        // Start OSC server
        if (oscReceiver_->start()) {
            oscListening_ = true;
            std::cout << "OSC listening started on port " << oscListenPort_ << std::endl;
        } else {
            oscReceiver_.reset();
            std::cerr << "Failed to start OSC receiver" << std::endl;
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Failed to start OSC listening: " << e.what() << std::endl;
        oscListening_ = false;
    }
}

void GuiApplication::stopOSCListening() {
    if (!oscListening_) return;
    
    if (oscReceiver_) {
        oscReceiver_->stop();
        oscReceiver_.reset();
    }
    
    oscListening_ = false;
    std::cout << "OSC listening stopped" << std::endl;
}

void GuiApplication::workerLoop() {
    std::vector<float> cvBuffer;
    auto lastUpdate = std::chrono::steady_clock::now();
    int messageCount = 0;
    
    while (workerRunning_) {
        try {
            auto now = std::chrono::steady_clock::now();
            
            // Read CV values
            if (cvReader_) {
                cvReader_->readChannels(cvBuffer);
                
                // Update audio device info from CVReader (one-time update)
                static bool audioInfoUpdated = false;
                if (!audioInfoUpdated && cvReader_->isInitialized()) {
                    audioInfo_.name = cvReader_->getCurrentDeviceName();
                    audioInfo_.maxInputChannels = cvReader_->getChannelCount();
                    audioInfo_.defaultSampleRate = cvReader_->getSampleRate();
                    audioInfoUpdated = true;
                }
                
                // Update channel data
                {
                    std::lock_guard<std::mutex> lock(dataMutex_);
                    for (size_t i = 0; i < cvBuffer.size() && i < channels_.size(); ++i) {
                        channels_[i].addSample(cvBuffer[i]);
                        channels_[i].normalizedValue = (cvBuffer[i] - channels_[i].minRange) / 
                                                      (channels_[i].maxRange - channels_[i].minRange);
                    }
                }
                
                // Send OSC messages
                if (oscSender_) {
                    std::vector<std::string> addresses;
                    std::vector<float> values;
                    
                    for (size_t i = 0; i < cvBuffer.size() && i < channels_.size(); ++i) {
                        if (channels_[i].enabled) {
                            addresses.push_back(channels_[i].oscAddress);
                            values.push_back(channels_[i].normalizedValue);
                        }
                    }
                    
                    if (!addresses.empty()) {
                        if (oscSender_->sendFloatBatch(addresses, values)) {
                            messageCount += addresses.size();
                            oscInfo_.connected = true;
                        } else {
                            oscInfo_.connected = false;
                        }
                    }
                }
            }
            
            // Update statistics every second
            auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - lastUpdate);
            if (elapsed.count() >= 1) {
                oscInfo_.messagesPerSecond = messageCount;
                oscInfo_.messagesSent += messageCount;
                messageCount = 0;
                lastUpdate = now;
            }
            
            // Sleep for update interval
            std::this_thread::sleep_for(std::chrono::milliseconds(10)); // 100 Hz update rate
            
        } catch (const std::exception& e) {
            std::cerr << "Error in worker loop: " << e.what() << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }
}

void GuiApplication::loadConfig(const std::string& configFile) {
    if (config_) {
        config_->loadFromFile(configFile);
        
        // Update GUI state from config
        oscInfo_.host = config_->getOSCHost();
        oscInfo_.port = config_->getOSCPort();
        
        // Set device name from config, but this will be updated with actual name when CVReader starts
        audioInfo_.name = config_->getAudioDevice();
        if (audioInfo_.name.empty()) {
            audioInfo_.name = "Default Device";
        }
        
        // Update channels configuration
        initializeChannels();
    }
}

void GuiApplication::saveConfig(const std::string& configFile) {
    if (config_) {
        // Update config from GUI state
        config_->setOSCHost(oscInfo_.host);
        config_->setOSCPort(oscInfo_.port);
        config_->setAudioDevice(audioInfo_.name);
        
        config_->saveToFile(configFile);
    }
}

void GuiApplication::initializeChannels() {
    // Initialize channels based on audio device capabilities
    int channelCount = 8; // Default, should be queried from audio device
    
    channels_.clear();
    channels_.reserve(channelCount);
    
    for (int i = 0; i < channelCount; ++i) {
        CVChannelData channel;
        channel.channelId = i;
        channel.name = "CV " + std::to_string(i + 1);
        channel.oscAddress = "/cv/channel/" + std::to_string(i + 1);
        channel.plotColor[0] = (i % 3 == 0) ? 1.0f : 0.3f;
        channel.plotColor[1] = (i % 3 == 1) ? 1.0f : 0.3f;
        channel.plotColor[2] = (i % 3 == 2) ? 1.0f : 0.3f;
        channels_.push_back(channel);
    }
}

void GuiApplication::resetChannelHistory() {
    std::lock_guard<std::mutex> lock(dataMutex_);
    for (auto& channel : channels_) {
        channel.history.clear();
    }
}

void GuiApplication::renderChannelConfiguration() {
    ImGui::Begin(_("window.channels"), &showConfigWindow_);
    
    ImGui::Text("Configure CV Channels");
    ImGui::Separator();
    
    std::lock_guard<std::mutex> lock(dataMutex_);
    
    for (auto& channel : channels_) {
        ImGui::PushID(channel.channelId);
        
        if (ImGui::CollapsingHeader((channel.name + " (Ch " + std::to_string(channel.channelId + 1) + ")").c_str())) {
            renderChannelConfigDialog(channel);
        }
        
        ImGui::PopID();
    }
    
    ImGui::End();
}

void GuiApplication::renderChannelConfigDialog(CVChannelData& channel) {
    // Channel name
    char nameBuffer[64];
    strncpy(nameBuffer, channel.name.c_str(), sizeof(nameBuffer));
    nameBuffer[sizeof(nameBuffer) - 1] = '\0';
    if (ImGui::InputText("Name", nameBuffer, sizeof(nameBuffer))) {
        channel.name = nameBuffer;
    }
    
    // OSC address
    char oscBuffer[128];
    strncpy(oscBuffer, channel.oscAddress.c_str(), sizeof(oscBuffer));
    oscBuffer[sizeof(oscBuffer) - 1] = '\0';
    if (ImGui::InputText("OSC Address", oscBuffer, sizeof(oscBuffer))) {
        channel.oscAddress = oscBuffer;
    }
    
    // Range settings
    ImGui::SliderFloat("Min Range", &channel.minRange, -20.0f, 0.0f);
    ImGui::SliderFloat("Max Range", &channel.maxRange, 0.0f, 20.0f);
    
    // Visual settings
    ImGui::Checkbox("Enable OSC", &channel.enabled);
    ImGui::SameLine();
    ImGui::Checkbox("Show in Plot", &channel.showInPlot);
    
    ImGui::ColorEdit3("Plot Color", channel.plotColor);
    
    ImGui::Spacing();
}

void GuiApplication::renderOSCConfiguration() {
    ImGui::Begin(_("window.osc"), &showOSCConfig_);
    
    // CV to OSC Section
    ImGui::Text("CV to OSC - Outgoing");
    ImGui::Separator();
    
    // Host and port configuration
    char hostBuffer[256];
    strncpy(hostBuffer, oscInfo_.host.c_str(), sizeof(hostBuffer));
    hostBuffer[sizeof(hostBuffer) - 1] = '\0';
    if (ImGui::InputText(_("osc.host"), hostBuffer, sizeof(hostBuffer))) {
        oscInfo_.host = hostBuffer;
    }
    
    char portBuffer[16];
    strncpy(portBuffer, oscInfo_.port.c_str(), sizeof(portBuffer));
    portBuffer[sizeof(portBuffer) - 1] = '\0';
    if (ImGui::InputText(_("osc.port"), portBuffer, sizeof(portBuffer))) {
        oscInfo_.port = portBuffer;
    }
    
    // Connection status
    ImGui::Text("Status: %s", oscInfo_.connected ? _("osc.connected") : _("osc.disconnected"));
    ImGui::Text("Messages Sent: %d", oscInfo_.messagesSent);
    ImGui::Text("Messages/sec: %d", oscInfo_.messagesPerSecond);
    
    // Test connection button
    if (ImGui::Button("Test Connection")) {
        testOSCConnection();
    }
    
    ImGui::SameLine();
    
    // Show test result
    if (oscTestResult_.hasResult) {
        if (oscTestResult_.success) {
            ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "✓ Test OK (%.1fms)", oscTestResult_.latencyMs);
        } else {
            ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "✗ Test Failed");
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("%s", oscTestResult_.errorMessage.c_str());
            }
        }
    }
    
    ImGui::Spacing();
    ImGui::Spacing();
    
    // OSC to CV Section
    ImGui::Text("OSC to CV - Incoming");
    ImGui::Separator();
    
    // OSC listening port configuration
    char listenPortBuffer[16];
    strncpy(listenPortBuffer, oscListenPort_.c_str(), sizeof(listenPortBuffer));
    listenPortBuffer[sizeof(listenPortBuffer) - 1] = '\0';
    if (ImGui::InputText("Listen Port", listenPortBuffer, sizeof(listenPortBuffer))) {
        oscListenPort_ = listenPortBuffer;
    }
    
    // OSC listening control
    if (ImGui::Button(oscListening_ ? "Stop OSC Listening" : "Start OSC Listening")) {
        if (oscListening_) {
            stopOSCListening();
        } else {
            startOSCListening();
        }
    }
    
    // OSC listening status
    ImGui::SameLine();
    if (oscListening_) {
        ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "LISTENING");
    } else {
        ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "STOPPED");
    }
    
    // Information about OSC to CV format
    ImGui::Spacing();
    ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.8f, 1.0f), "Expected OSC format:");
    ImGui::BulletText("/cv/channel/1 <float_value>");
    ImGui::BulletText("/cv/channel/2 <float_value>");
    ImGui::BulletText("... etc");
    ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.8f, 1.0f), "Value range: 0.0 to 1.0 (normalized)");
    
    ImGui::End();
}

void GuiApplication::renderAudioConfiguration() {
    ImGui::Begin(_("window.audio"), &showAudioConfig_);
    
    ImGui::Text("Audio Device Settings");
    ImGui::Separator();
    
    // Audio device selection
    ImGui::Text("%s: %s", _("audio.current_device"), audioInfo_.name.c_str());
    ImGui::Text("Channels: %d", audioInfo_.maxInputChannels);
    ImGui::Text("%s: %.0f Hz", _("audio.sample_rate"), audioInfo_.defaultSampleRate);
    
    if (ImGui::Button("Refresh Devices")) {
        refreshAudioDevices();
    }
    
    ImGui::SameLine();
    if (audioDeviceRefreshInProgress_) {
        ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Refreshing...");
    } else if (audioDeviceRefreshResult_.hasResult) {
        if (audioDeviceRefreshResult_.success) {
            ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "✓ Found %d devices", audioDeviceRefreshResult_.deviceCount);
        } else {
            ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "✗ Refresh failed");
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("%s", audioDeviceRefreshResult_.errorMessage.c_str());
            }
        }
    }
    
    ImGui::End();
}

void GuiApplication::renderPerformanceMonitor() {
    ImGui::Begin(_("window.performance"), &showPerformanceWindow_);
    
    ImGui::Text("Performance Metrics");
    ImGui::Separator();
    
    ImGui::Text("%s: %.1f", _("performance.fps"), performanceData_.fps);
    ImGui::Text("%s: %.1f%%", _("performance.cpu"), performanceData_.cpuUsage);
    ImGui::Text("Dropped Frames: %d", performanceData_.droppedFrames);
    
    ImGui::Spacing();
    ImGui::Text("Memory Usage");
    ImGui::Separator();
    ImGui::Text("Total Memory: %.1f MB", performanceData_.totalMemoryMB);
    ImGui::Text("Used Memory: %.1f MB", performanceData_.usedMemoryMB);
    ImGui::Text("Audio Buffer: %.1f ms", performanceData_.audioBufferMs);
    
    ImGui::Spacing();
    ImGui::Text("Audio Performance");
    ImGui::Separator();
    ImGui::Text("Audio Latency: %.1f ms", performanceData_.audioLatencyMs);
    ImGui::Text("Buffer Underruns: %d", performanceData_.bufferUnderruns);
    ImGui::Text("Sample Rate: %.0f Hz", performanceData_.actualSampleRate);
    
    ImGui::Spacing();
    ImGui::Text("Network Performance");
    ImGui::Separator();
    ImGui::Text("OSC Messages/sec: %d", performanceData_.oscMessagesPerSec);
    ImGui::Text("Network Latency: %.1f ms", performanceData_.networkLatencyMs);
    ImGui::Text("Failed Sends: %d", performanceData_.oscFailedSends);
    
    ImGui::Spacing();
    if (ImGui::Button("Reset Counters")) {
        resetPerformanceCounters();
    }
    
    ImGui::End();
}

void GuiApplication::renderAboutWindow() {
    ImGui::Begin("About", &showAboutWindow_);
    
    ImGui::Text("%s", Version::getAppTitle().c_str());
    ImGui::Text("Version: %s", Version::getVersionWithGit().c_str());
    ImGui::Text("Build: %s", Version::getBuildInfo().c_str());
    
    ImGui::Separator();
    ImGui::Text("CV to OSC Converter with GUI");
    ImGui::Text("Real-time CV signal visualization and OSC transmission");
    
    ImGui::Spacing();
    if (ImGui::Button("Close")) {
        showAboutWindow_ = false;
    }
    
    ImGui::End();
}

void GuiApplication::testOSCConnection() {
    if (!oscSender_) {
        oscTestResult_.hasResult = true;
        oscTestResult_.success = false;
        oscTestResult_.errorMessage = "OSC sender not initialized";
        oscTestResult_.timestamp = std::chrono::steady_clock::now();
        return;
    }
    
    auto start = std::chrono::high_resolution_clock::now();
    
    // Send a test message
    bool success = oscSender_->sendFloat("/test/ping", 1.0f);
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    oscTestResult_.hasResult = true;
    oscTestResult_.success = success;
    oscTestResult_.latencyMs = duration.count() / 1000.0f;
    oscTestResult_.timestamp = std::chrono::steady_clock::now();
    
    if (!success) {
        oscTestResult_.errorMessage = "Failed to send OSC message to " + oscInfo_.host + ":" + oscInfo_.port;
    }
}

void GuiApplication::refreshAudioDevices() {
    audioDeviceRefreshInProgress_ = true;
    audioDeviceRefreshResult_.hasResult = false;
    
    try {
        // Use AudioDeviceManager to enumerate devices
        AudioDeviceManager deviceManager;
        auto devices = deviceManager.getInputDevices();
        
        // Update audio info with current device count
        audioDeviceRefreshResult_.hasResult = true;
        audioDeviceRefreshResult_.success = true;
        audioDeviceRefreshResult_.deviceCount = static_cast<int>(devices.size());
        audioDeviceRefreshResult_.timestamp = std::chrono::steady_clock::now();
        
        // Update current device info if CVReader is initialized
        if (cvReader_ && cvReader_->isInitialized()) {
            audioInfo_.name = cvReader_->getCurrentDeviceName();
            audioInfo_.maxInputChannels = cvReader_->getChannelCount();
            audioInfo_.defaultSampleRate = cvReader_->getSampleRate();
        }
        
    } catch (const std::exception& e) {
        audioDeviceRefreshResult_.hasResult = true;
        audioDeviceRefreshResult_.success = false;
        audioDeviceRefreshResult_.deviceCount = 0;
        audioDeviceRefreshResult_.errorMessage = std::string("Failed to refresh audio devices: ") + e.what();
        audioDeviceRefreshResult_.timestamp = std::chrono::steady_clock::now();
    }
    
    audioDeviceRefreshInProgress_ = false;
}

void GuiApplication::resetPerformanceCounters() {
    performanceData_.droppedFrames = 0;
    performanceData_.bufferUnderruns = 0;
    performanceData_.oscFailedSends = 0;
    oscInfo_.messagesSent = 0;
    performanceData_.lastUpdate = std::chrono::steady_clock::now();
}

void GuiApplication::updatePerformanceMetrics() {
    auto now = std::chrono::steady_clock::now();
    
    // Update FPS
    static auto lastFrameTime = now;
    auto frameDuration = std::chrono::duration_cast<std::chrono::microseconds>(now - lastFrameTime);
    if (frameDuration.count() > 0) {
        performanceData_.fps = 1000000.0f / frameDuration.count();
    }
    lastFrameTime = now;
    
    // Update memory usage (platform-specific implementation would go here)
    #ifdef __APPLE__
    // macOS memory usage
    task_basic_info_64_data_t info;
    mach_msg_type_number_t size = sizeof(info);
    kern_return_t kr = task_info(mach_task_self(), TASK_BASIC_INFO_64, (task_info_t)&info, &size);
    if (kr == KERN_SUCCESS) {
        performanceData_.usedMemoryMB = info.resident_size / (1024.0f * 1024.0f);
    }
    #endif
    
    // Update audio metrics if CVReader is available
    if (cvReader_ && cvReader_->isInitialized()) {
        performanceData_.actualSampleRate = cvReader_->getSampleRate();
        // Note: These methods may need to be implemented in CVReader
        performanceData_.audioLatencyMs = 10.0f; // Placeholder
        performanceData_.audioBufferMs = 5.0f;   // Placeholder
    }
    
    // Update network metrics
    performanceData_.oscMessagesPerSec = oscInfo_.messagesPerSecond;
    
    // Update CPU usage (simplified estimation)
    static auto lastCpuTime = now;
    auto cpuDuration = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastCpuTime);
    if (cpuDuration.count() > 1000) { // Update every second
        // Simple CPU usage estimation based on frame rate
        float targetFps = 60.0f;
        performanceData_.cpuUsage = std::min(100.0f, (targetFps / std::max(1.0f, performanceData_.fps)) * 50.0f);
        lastCpuTime = now;
    }
}

void GuiApplication::handleDragAndDrop() {
    // Drag and drop implementation for channel reordering
    ImGuiIO& io = ImGui::GetIO();
    
    // Check if we're dragging and have valid source
    if (dragDropState_.dragging && dragDropState_.sourceChannel >= 0) {
        ImGui::SetNextWindowPos(io.MousePos, ImGuiCond_Always);
        ImGui::SetNextWindowBgAlpha(0.8f);
        
        if (ImGui::Begin("DragPreview", nullptr, 
                        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | 
                        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_AlwaysAutoResize |
                        ImGuiWindowFlags_NoSavedSettings)) {
            
            if (dragDropState_.sourceChannel < static_cast<int>(channels_.size())) {
                const auto& channel = channels_[dragDropState_.sourceChannel];
                ImGui::Text("Moving: %s", channel.name.c_str());
                ImGui::Text("Value: %.3fV", channel.currentValue);
            }
        }
        ImGui::End();
        
        // Check if mouse is released to complete drop
        if (!io.MouseDown[0]) {
            if (dragDropState_.targetChannel >= 0 && 
                dragDropState_.targetChannel != dragDropState_.sourceChannel &&
                dragDropState_.targetChannel < static_cast<int>(channels_.size())) {
                
                // Execute channel reorder (simplified without command system for now)
                if (dragDropState_.sourceChannel >= 0 && dragDropState_.sourceChannel < static_cast<int>(channels_.size()) &&
                    dragDropState_.targetChannel >= 0 && dragDropState_.targetChannel < static_cast<int>(channels_.size()) &&
                    dragDropState_.sourceChannel != dragDropState_.targetChannel) {
                    
                    // Simple channel swap
                    std::swap(channels_[dragDropState_.sourceChannel], channels_[dragDropState_.targetChannel]);
                    
                    // Update channel IDs
                    channels_[dragDropState_.sourceChannel].channelId = dragDropState_.sourceChannel;
                    channels_[dragDropState_.targetChannel].channelId = dragDropState_.targetChannel;
                }
            }
            
            // Reset drag state
            dragDropState_.dragging = false;
            dragDropState_.sourceChannel = -1;
            dragDropState_.targetChannel = -1;
        }
    }
}

// Command system implementation
void GuiApplication::executeCommand(std::unique_ptr<Command> command) {
    if (commandManager_) {
        commandManager_->executeCommand(std::move(command));
    }
}

void GuiApplication::undo() {
    if (commandManager_) {
        commandManager_->undo();
    }
}

void GuiApplication::redo() {
    if (commandManager_) {
        commandManager_->redo();
    }
}

bool GuiApplication::canUndo() const {
    return commandManager_ && commandManager_->canUndo();
}

bool GuiApplication::canRedo() const {
    return commandManager_ && commandManager_->canRedo();
}

// Hot keys implementation
void GuiApplication::setupHotKeys() {
    if (!hotKeyManager_) return;
    
    // File operations
    HotKey loadConfigKey(ImGuiKey_O, true); // Ctrl+O (Cmd+O on macOS)
    loadConfigKey.action = [this]() { loadConfig(); };
    loadConfigKey.description = "Load Configuration";
    loadConfigKey.category = "File";
    hotKeyManager_->registerHotKey("file.load", loadConfigKey);
    
    HotKey saveConfigKey(ImGuiKey_S, true); // Ctrl+S (Cmd+S on macOS)
    saveConfigKey.action = [this]() { saveConfig(); };
    saveConfigKey.description = "Save Configuration";
    saveConfigKey.category = "File";
    hotKeyManager_->registerHotKey("file.save", saveConfigKey);
    
    // Edit operations
    HotKey undoAction(ImGuiKey_Z, true); // Ctrl+Z (Cmd+Z on macOS)
    undoAction.action = [this]() { undo(); };
    undoAction.description = "Undo";
    undoAction.category = "Edit";
    hotKeyManager_->registerHotKey("edit.undo", undoAction);
    
    HotKey redoAction(ImGuiKey_Y, true); // Ctrl+Y (Cmd+Y on macOS)
    redoAction.action = [this]() { redo(); };
    redoAction.description = "Redo";
    redoAction.category = "Edit";
    hotKeyManager_->registerHotKey("edit.redo", redoAction);
    
    // Alternative Redo (Ctrl+Shift+Z)
    HotKey redoAlt(ImGuiKey_Z, true, true); // Ctrl+Shift+Z
    redoAlt.action = [this]() { redo(); };
    redoAlt.description = "Redo (Alt)";
    redoAlt.category = "Edit";
    hotKeyManager_->registerHotKey("edit.redo_alt", redoAlt);
    
    // Control operations
    HotKey startStop(ImGuiKey_Space); // Space
    startStop.action = [this]() {
        if (workerRunning_) {
            stopConversion();
        } else {
            startConversion();
        }
    };
    startStop.description = "Start/Stop Conversion";
    startStop.category = "Control";
    hotKeyManager_->registerHotKey("control.startstop", startStop);
    
    HotKey resetHistory(ImGuiKey_R, true); // Ctrl+R
    resetHistory.action = [this]() { resetChannelHistory(); };
    resetHistory.description = "Reset Channel History";
    resetHistory.category = "Control";
    hotKeyManager_->registerHotKey("control.reset", resetHistory);
    
    // Window operations
    HotKey toggleMain(ImGuiKey_1, true); // Ctrl+1
    toggleMain.action = [this]() { showMainWindow_ = !showMainWindow_; };
    toggleMain.description = "Toggle Main Window";
    toggleMain.category = "Windows";
    hotKeyManager_->registerHotKey("window.main", toggleMain);
    
    HotKey toggleConfig(ImGuiKey_2, true); // Ctrl+2
    toggleConfig.action = [this]() { showConfigWindow_ = !showConfigWindow_; };
    toggleConfig.description = "Toggle Channel Configuration";
    toggleConfig.category = "Windows";
    hotKeyManager_->registerHotKey("window.config", toggleConfig);
    
    HotKey toggleOSC(ImGuiKey_3, true); // Ctrl+3
    toggleOSC.action = [this]() { showOSCConfig_ = !showOSCConfig_; };
    toggleOSC.description = "Toggle OSC Configuration";
    toggleOSC.category = "Windows";
    hotKeyManager_->registerHotKey("window.osc", toggleOSC);
    
    HotKey toggleAudio(ImGuiKey_4, true); // Ctrl+4
    toggleAudio.action = [this]() { showAudioConfig_ = !showAudioConfig_; };
    toggleAudio.description = "Toggle Audio Configuration";
    toggleAudio.category = "Windows";
    hotKeyManager_->registerHotKey("window.audio", toggleAudio);
    
    HotKey togglePerf(ImGuiKey_5, true); // Ctrl+5
    togglePerf.action = [this]() { showPerformanceWindow_ = !showPerformanceWindow_; };
    togglePerf.description = "Toggle Performance Monitor";
    togglePerf.category = "Windows";
    hotKeyManager_->registerHotKey("window.performance", togglePerf);
    
    // Appearance
    HotKey toggleThemeEditor(ImGuiKey_T, true, true); // Ctrl+Shift+T
    toggleThemeEditor.action = [this]() { showThemeEditor_ = !showThemeEditor_; };
    toggleThemeEditor.description = "Toggle Theme Editor";
    toggleThemeEditor.category = "Appearance";
    hotKeyManager_->registerHotKey("appearance.theme_editor", toggleThemeEditor);
    
    HotKey toggleHotKeyEditor(ImGuiKey_H, true, true); // Ctrl+Shift+H
    toggleHotKeyEditor.action = [this]() { showHotKeyEditor_ = !showHotKeyEditor_; };
    toggleHotKeyEditor.description = "Toggle Hot Key Editor";
    toggleHotKeyEditor.category = "Appearance";
    hotKeyManager_->registerHotKey("appearance.hotkey_editor", toggleHotKeyEditor);
    
    // Quick channel toggles (F1-F8)
    for (int i = 0; i < 8; ++i) {
        HotKey channelToggle(static_cast<ImGuiKey>(ImGuiKey_F1 + i));
        channelToggle.action = [this, i]() {
            if (i < static_cast<int>(channels_.size())) {
                // Toggle channel directly
                channels_[i].enabled = !channels_[i].enabled;
            }
        };
        channelToggle.description = "Toggle Channel " + std::to_string(i + 1);
        channelToggle.category = "Channels";
        hotKeyManager_->registerHotKey("channel.toggle_" + std::to_string(i + 1), channelToggle);
    }
    
    // Application control
    HotKey exitApp(ImGuiKey_Q, true); // Ctrl+Q (Cmd+Q on macOS)
    exitApp.action = [this]() { running_ = false; };
    exitApp.description = "Exit Application";
    exitApp.category = "Application";
    hotKeyManager_->registerHotKey("app.exit", exitApp);
    
    // View operations
    HotKey showAllWindows(ImGuiKey_A, true, true); // Ctrl+Shift+A
    showAllWindows.action = [this]() {
        showMainWindow_ = true;
        showConfigWindow_ = true;
        showOSCConfig_ = true;
        showAudioConfig_ = true;
        showPerformanceWindow_ = true;
        arrangeWindows();
    };
    showAllWindows.description = "Show All Windows";
    showAllWindows.category = "View";
    hotKeyManager_->registerHotKey("view.show_all", showAllWindows);
    
    // Help
    HotKey showAbout(ImGuiKey_F1, false, true); // Shift+F1
    showAbout.action = [this]() { showAboutWindow_ = true; };
    showAbout.description = "Show About";
    showAbout.category = "Help";
    hotKeyManager_->registerHotKey("help.about", showAbout);
}

void GuiApplication::processHotKeys() {
    if (hotKeyManager_) {
        hotKeyManager_->processHotKeys();
    }
}

void GuiApplication::showHotKeyEditor() {
    showHotKeyEditor_ = true;
}

void GuiApplication::arrangeWindows() {
    // Get display size
    int display_w, display_h;
    glfwGetWindowSize(window_, &display_w, &display_h);
    
    // Calculate optimal window positions and sizes
    float windowWidth = display_w * 0.32f;  // About 1/3 of screen width
    float windowHeight = display_h * 0.45f; // About half screen height
    float spacing = 10.0f;
    
    // Main window - top left
    ImGui::SetNextWindowPos(ImVec2(spacing, 30), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(windowWidth * 1.5f, windowHeight * 1.2f), ImGuiCond_FirstUseEver);
    
    // Channel Configuration - top center
    ImGui::SetNextWindowPos(ImVec2(windowWidth * 1.5f + spacing * 2, 30), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(windowWidth, windowHeight * 1.2f), ImGuiCond_FirstUseEver);
    
    // OSC Configuration - top right
    ImGui::SetNextWindowPos(ImVec2(windowWidth * 2.5f + spacing * 3, 30), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(windowWidth * 0.8f, windowHeight * 0.8f), ImGuiCond_FirstUseEver);
    
    // Audio Configuration - bottom left
    ImGui::SetNextWindowPos(ImVec2(spacing, windowHeight * 1.2f + 60), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(windowWidth, windowHeight * 0.6f), ImGuiCond_FirstUseEver);
    
    // Performance Monitor - bottom right
    ImGui::SetNextWindowPos(ImVec2(windowWidth + spacing * 2, windowHeight * 1.2f + 60), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(windowWidth, windowHeight * 0.6f), ImGuiCond_FirstUseEver);
    
    std::cout << "Windows arranged for first launch" << std::endl;
}


void GuiApplication::showWelcomeDialog() {
    if (!showWelcomeDialog_) return;
    
    // Center the welcome dialog
    ImGuiIO& io = ImGui::GetIO();
    ImVec2 center = ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f);
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    
    if (ImGui::BeginPopupModal("Welcome to CV to OSC Converter!", &showWelcomeDialog_, 
                               ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove)) {
        
        ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "🎵 Welcome to CV to OSC Converter GUI!");
        ImGui::Separator();
        
        ImGui::Text("All windows are now open to give you a complete overview.");
        ImGui::Text("You can close any window you don't need right now.");
        
        ImGui::Spacing();
        ImGui::Text("📋 Available Windows:");
        ImGui::BulletText("Main Window - Real-time visualization and control");
        ImGui::BulletText("Channel Configuration - Set up your CV channels");
        ImGui::BulletText("OSC Configuration - Network settings");
        ImGui::BulletText("Audio Configuration - Audio device selection");
        ImGui::BulletText("Performance Monitor - System performance metrics");
        
        ImGui::Spacing();
        ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "💡 Quick Tips:");
        ImGui::BulletText("Press Space to start/stop conversion");
        ImGui::BulletText("Use F1-F8 to quickly toggle channels");
        ImGui::BulletText("Try Ctrl+Z for undo, Ctrl+Y for redo");
        ImGui::BulletText("Press Ctrl+Shift+H to see all keyboard shortcuts");
        
        ImGui::Spacing();
        ImGui::TextColored(ImVec4(0.0f, 0.8f, 1.0f, 1.0f), "🎨 Customization:");
        ImGui::BulletText("Use Appearance menu to change themes");
        ImGui::BulletText("All windows can be moved and resized");
        ImGui::BulletText("Window visibility can be toggled in View menu");
        
        ImGui::Spacing();
        ImGui::Separator();
        
        bool dontShowAgain = !firstLaunch_;
        if (ImGui::Checkbox("Don't show this dialog again", &dontShowAgain)) {
            firstLaunch_ = !dontShowAgain;
        }
        
        ImGui::SameLine();
        if (ImGui::Button("Get Started!", ImVec2(120, 0))) {
            showWelcomeDialog_ = false;
            ImGui::CloseCurrentPopup();
        }
        
        ImGui::EndPopup();
    }
    
    // Show the popup on first frame
    if (showWelcomeDialog_ && firstLaunch_) {
        ImGui::OpenPopup("Welcome to CV to OSC Converter!");
    }
}

void GuiApplication::renderIndividualChannelWindows() {
    // Ensure channel window array matches channel count
    if (visualizationState_.showChannelWindow.size() != channels_.size()) {
        visualizationState_.showChannelWindow.resize(channels_.size(), false);
    }
    
    // Update individual window states based on showIndividualWindows flag
    if (visualizationState_.showIndividualWindows) {
        for (size_t i = 0; i < visualizationState_.showChannelWindow.size(); ++i) {
            visualizationState_.showChannelWindow[i] = true;
        }
        visualizationState_.showIndividualWindows = false; // Reset flag
    }
    
    // Render each individual channel window
    for (int i = 0; i < static_cast<int>(channels_.size()); ++i) {
        if (visualizationState_.showChannelWindow[i]) {
            renderChannelWindow(i);
        }
    }
}

void GuiApplication::renderChannelWindow(int channelIndex) {
    if (channelIndex < 0 || channelIndex >= static_cast<int>(channels_.size())) {
        return;
    }
    
    const auto& channel = channels_[channelIndex];
    std::string windowTitle = "Channel " + std::to_string(channelIndex + 1) + " - " + channel.name;
    
    bool isOpen = static_cast<bool>(visualizationState_.showChannelWindow[channelIndex]);
    ImGui::Begin(windowTitle.c_str(), &isOpen);
    visualizationState_.showChannelWindow[channelIndex] = isOpen ? 1 : 0;
    
    // Channel status and controls
    ImGui::Text("Channel %d: %s", channelIndex + 1, channel.name.c_str());
    ImGui::Separator();
    
    // Current value display
    ImGui::Text("Current Value: %.3f V", channel.currentValue);
    ImGui::Text("Normalized: %.3f", channel.normalizedValue);
    ImGui::Text("Range: %.1f V to %.1f V", channel.minRange, channel.maxRange);
    ImGui::Text("OSC Address: %s", channel.oscAddress.c_str());
    
    // Status indicators
    ImGui::TextColored(channel.enabled ? ImVec4(0.0f, 1.0f, 0.0f, 1.0f) : ImVec4(1.0f, 0.0f, 0.0f, 1.0f), 
                      channel.enabled ? "ENABLED" : "DISABLED");
    
    ImGui::Spacing();
    
    // Visualization controls
    renderVisualizationControls();
    
    ImGui::Spacing();
    
    // Large meter display
    ImGui::Text("Level Meter");
    float normalizedValue = (channel.currentValue - channel.minRange) / (channel.maxRange - channel.minRange);
    normalizedValue = std::clamp(normalizedValue, 0.0f, 1.0f);
    
    ImGui::ProgressBar(normalizedValue, ImVec2(-1, 40), 
                      (std::to_string(channel.currentValue) + " V").c_str());
    
    ImGui::Spacing();
    
    // Individual channel plot with zoom controls
    if (ImPlot::BeginPlot(("##ChannelPlot" + std::to_string(channelIndex)).c_str(), ImVec2(-1, 300))) {
        // Calculate time range based on zoom level
        float displayTimeRange = visualizationState_.timeRange / visualizationState_.zoomLevel;
        
        // Setup axes with custom limits
        ImPlot::SetupAxes("Time (s)", "Voltage (V)");
        ImPlot::SetupAxisLimits(ImAxis_X1, -displayTimeRange, 0, ImGuiCond_Always);
        
        if (visualizationState_.autoScale) {
            // Auto-scale Y axis based on channel range
            ImPlot::SetupAxisLimits(ImAxis_Y1, channel.minRange, channel.maxRange, ImGuiCond_Always);
        } else {
            // Use manual scale
            ImPlot::SetupAxisLimits(ImAxis_Y1, visualizationState_.voltageMin, visualizationState_.voltageMax, ImGuiCond_Always);
        }
        
        // Plot channel data
        if (!channel.history.empty()) {
            std::lock_guard<std::mutex> lock(dataMutex_);
            
            std::vector<float> timeData, valueData;
            timeData.reserve(channel.history.size());
            valueData.reserve(channel.history.size());
            
            float timeStep = displayTimeRange / channel.history.size();
            for (size_t i = 0; i < channel.history.size(); ++i) {
                timeData.push_back(-displayTimeRange + i * timeStep);
                valueData.push_back(channel.history[i]);
            }
            
            ImPlot::SetNextLineStyle(ImVec4(channel.plotColor[0], channel.plotColor[1], channel.plotColor[2], 1.0f), 2.0f);
            ImPlot::PlotLine(channel.name.c_str(), timeData.data(), valueData.data(), static_cast<int>(timeData.size()));
            
            // Add current value marker
            if (!timeData.empty()) {
                float currentTime = timeData.back();
                ImPlot::SetNextMarkerStyle(ImPlotMarker_Circle, 8.0f, ImVec4(1.0f, 0.0f, 0.0f, 1.0f));
                ImPlot::PlotScatter("Current", &currentTime, &channel.currentValue, 1);
            }
        }
        
        ImPlot::EndPlot();
    }
    
    ImGui::End();
}

void GuiApplication::renderVisualizationControls() {
    ImGui::Text("Visualization Controls");
    ImGui::Separator();
    
    // Zoom controls
    ImGui::Text("Zoom Level: %.1fx", visualizationState_.zoomLevel);
    if (ImGui::SliderFloat("##Zoom", &visualizationState_.zoomLevel, 0.1f, 10.0f, "%.1fx")) {
        visualizationState_.zoomLevel = std::clamp(visualizationState_.zoomLevel, 0.1f, 10.0f);
    }
    
    ImGui::SameLine();
    if (ImGui::Button("Reset Zoom")) {
        visualizationState_.zoomLevel = 1.0f;
    }
    
    // Time range controls
    ImGui::Text("Time Range: %.1f seconds", visualizationState_.timeRange);
    if (ImGui::SliderFloat("##TimeRange", &visualizationState_.timeRange, 1.0f, 60.0f, "%.1fs")) {
        visualizationState_.timeRange = std::clamp(visualizationState_.timeRange, 1.0f, 60.0f);
    }
    
    // Auto-scale toggle
    ImGui::Checkbox("Auto Scale Y-Axis", &visualizationState_.autoScale);
    
    // Manual scale controls (only when auto-scale is off)
    if (!visualizationState_.autoScale) {
        ImGui::Text("Y-Axis Range:");
        ImGui::SliderFloat("Min##YMin", &visualizationState_.voltageMin, -20.0f, 0.0f, "%.1f V");
        ImGui::SliderFloat("Max##YMax", &visualizationState_.voltageMax, 0.0f, 20.0f, "%.1f V");
        
        if (visualizationState_.voltageMin >= visualizationState_.voltageMax) {
            visualizationState_.voltageMax = visualizationState_.voltageMin + 1.0f;
        }
    }
    
    ImGui::Spacing();
}

void GuiApplication::renderChannelControls() {
    // This method is not directly called, but used by renderMixerView
}

void GuiApplication::renderChannelStrip(CVChannelData& channel) {
    ImGui::PushID(channel.channelId);
    
    // Channel header
    ImGui::Text("CH %d", channel.channelId + 1);
    ImGui::Separator();
    
    // Channel name (compact)
    char nameBuffer[16];
    strncpy(nameBuffer, channel.name.c_str(), sizeof(nameBuffer));
    nameBuffer[sizeof(nameBuffer) - 1] = '\0';
    if (ImGui::InputText("##name", nameBuffer, sizeof(nameBuffer))) {
        channel.name = nameBuffer;
    }
    
    ImGui::Spacing();
    
    // Filter knob (using slider as fallback)
    ImGui::Text("FILTER");
    if (ImGui::SliderFloat("##filter", &channel.controls.filterKnob, 0.0f, 1.0f, "%.2f")) {
        // Filter knob changed
    }
    
    // Gain knob  
    ImGui::Text("GAIN");
    if (ImGui::SliderFloat("##gain", &channel.controls.gainKnob, 0.0f, 2.0f, "%.2f")) {
        // Gain knob changed
    }
    
    // Offset knob
    ImGui::Text("OFFSET");
    if (ImGui::SliderFloat("##offset", &channel.controls.offsetKnob, -1.0f, 1.0f, "%.2f")) {
        // Offset knob changed
    }
    
    // Mix knob
    ImGui::Text("MIX");
    if (ImGui::SliderFloat("##mix", &channel.controls.mixKnob, 0.0f, 1.0f, "%.2f")) {
        // Mix knob changed
    }
    
    ImGui::Spacing();
    
    // Main fader (vertical)
    ImGui::Text("LEVEL");
    ImGui::VSliderFloat("##fader", ImVec2(30, 200), &channel.controls.fader, 0.0f, 1.0f, "");
    
    // Fader value display
    ImGui::Text("%.2f", channel.controls.fader);
    
    ImGui::Spacing();
    
    // Buttons
    if (ImGui::Button("M", ImVec2(25, 25))) {
        channel.controls.muteButton = !channel.controls.muteButton;
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Mute");
    }
    
    ImGui::SameLine();
    if (ImGui::Button("S", ImVec2(25, 25))) {
        channel.controls.soloButton = !channel.controls.soloButton;
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Solo");
    }
    
    if (ImGui::Button("L", ImVec2(25, 25))) {
        channel.controls.linkButton = !channel.controls.linkButton;
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Link to next channel");
    }
    
    // Button status colors
    if (channel.controls.muteButton) {
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "MUTE");
    }
    if (channel.controls.soloButton) {
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "SOLO");
    }
    
    // Level meter
    ImGui::Spacing();
    float normalizedValue = (channel.currentValue - channel.minRange) / (channel.maxRange - channel.minRange);
    normalizedValue = std::clamp(normalizedValue, 0.0f, 1.0f);
    
    // Vertical level meter
    ImGui::ProgressBar(normalizedValue, ImVec2(60, 20), "");
    ImGui::Text("%.3fV", channel.currentValue);
    
    ImGui::PopID();
}

void GuiApplication::renderMixerView() {
    ImGui::Begin("CV Mixer", &showMixerWindow_);
    
    ImGui::Text("CV Channel Mixer");
    ImGui::Separator();
    
    // Master controls
    if (ImGui::Button("Reset All")) {
        for (auto& channel : channels_) {
            channel.controls.fader = 0.0f;
            channel.controls.gainKnob = 1.0f;
            channel.controls.offsetKnob = 0.0f;
            channel.controls.filterKnob = 1.0f;
            channel.controls.mixKnob = 1.0f;
            channel.controls.muteButton = false;
            channel.controls.soloButton = false;
            channel.controls.linkButton = false;
        }
    }
    
    ImGui::SameLine();
    if (ImGui::Button("All Faders Up")) {
        for (auto& channel : channels_) {
            channel.controls.fader = 1.0f;
        }
    }
    
    ImGui::Spacing();
    ImGui::Separator();
    
    // Channel strips in horizontal layout
    if (ImGui::BeginTable("MixerStrips", static_cast<int>(channels_.size()), 
                         ImGuiTableFlags_Borders | ImGuiTableFlags_Resizable)) {
        
        for (auto& channel : channels_) {
            ImGui::TableSetupColumn(("Ch " + std::to_string(channel.channelId + 1)).c_str(), 
                                   ImGuiTableColumnFlags_WidthFixed, 80.0f);
        }
        
        ImGui::TableNextRow();
        
        for (auto& channel : channels_) {
            ImGui::TableSetColumnIndex(channel.channelId);
            renderChannelStrip(channel);
        }
        
        ImGui::EndTable();
    }
    
    ImGui::End();
}

void GuiApplication::renderExternalMappingWindow() {
    if (!showExternalMappingWindow_) return;
    
    ImGui::Begin("External Device Mapping", &showExternalMappingWindow_);
    
    ImGui::Text("External Controller Mapping");
    ImGui::Separator();
    
    // Initialize external device manager if needed
    if (!externalDeviceManager_) {
        externalDeviceManager_ = std::make_unique<ExternalDeviceManager>();
        if (!externalDeviceManager_->initialize()) {
            ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Failed to initialize external device manager");
            ImGui::End();
            return;
        }
    }
    
    // Device connection status
    if (externalDeviceManager_->isMidiConnected()) {
        ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "MIDI: Connected");
        ImGui::SameLine();
        ImGui::Text("(%s)", externalDeviceManager_->getConnectedMidiDevice().c_str());
    } else {
        ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "MIDI: Disconnected");
    }
    
    // Device selection
    if (ImGui::Button("Scan for MIDI Devices")) {
        externalDeviceManager_->scanForDevices();
    }
    
    auto midiDevices = externalDeviceManager_->getAvailableMidiDevices();
    if (!midiDevices.empty()) {
        ImGui::Text("Available MIDI Devices:");
        for (const auto& device : midiDevices) {
            if (ImGui::Button(device.c_str())) {
                externalDeviceManager_->connectMidiDevice(device);
            }
        }
    }
    
    ImGui::Spacing();
    ImGui::Separator();
    
    // Learning mode
    bool learningMode = externalDeviceManager_->isLearningMode();
    if (ImGui::Checkbox("Learning Mode", &learningMode)) {
        externalDeviceManager_->enableLearningMode(learningMode);
    }
    
    if (learningMode) {
        ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Learning Mode Active - Move a control to assign it");
    }
    
    ImGui::Spacing();
    
    // Mapping table
    ImGui::Text("Control Mappings");
    if (ImGui::BeginTable("Mappings", 5, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
        ImGui::TableSetupColumn("Channel", ImGuiTableColumnFlags_WidthFixed, 60.0f);
        ImGui::TableSetupColumn("Parameter", ImGuiTableColumnFlags_WidthFixed, 80.0f);
        ImGui::TableSetupColumn("Controller", ImGuiTableColumnFlags_WidthFixed, 80.0f);
        ImGui::TableSetupColumn("Learn", ImGuiTableColumnFlags_WidthFixed, 60.0f);
        ImGui::TableSetupColumn("Clear", ImGuiTableColumnFlags_WidthFixed, 60.0f);
        ImGui::TableHeadersRow();
        
        // Show mapping rows for each channel and parameter
        const std::vector<std::string> parameters = {"fader", "gainKnob", "offsetKnob", "filterKnob", "mixKnob"};
        
        for (int ch = 0; ch < static_cast<int>(channels_.size()); ++ch) {
            for (const auto& param : parameters) {
                ImGui::TableNextRow();
                
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("%d", ch + 1);
                
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("%s", param.c_str());
                
                ImGui::TableSetColumnIndex(2);
                // Show current mapping info
                ImGui::Text("CC: -"); // Placeholder - would show actual mapping
                
                ImGui::TableSetColumnIndex(3);
                ImGui::PushID(ch * 100 + static_cast<int>(param.length()));
                if (ImGui::Button("Learn")) {
                    externalDeviceManager_->enableLearningMode(true);
                    externalDeviceManager_->setLearningTarget(ch, param);
                }
                ImGui::PopID();
                
                ImGui::TableSetColumnIndex(4);
                ImGui::PushID(ch * 100 + static_cast<int>(param.length()) + 50);
                if (ImGui::Button("Clear")) {
                    externalDeviceManager_->removeMapping(ch, param);
                }
                ImGui::PopID();
            }
        }
        
        ImGui::EndTable();
    }
    
    ImGui::Spacing();
    
    // Statistics
    ImGui::Text("Messages Received: %d", externalDeviceManager_->getMessageCount());
    ImGui::SameLine();
    if (ImGui::Button("Reset Count")) {
        externalDeviceManager_->resetMessageCount();
    }
    
    ImGui::End();
}
