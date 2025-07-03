#include <iostream>
#include <vector>
#include <thread>
#include <chrono>
#include <atomic>
#include <memory>
#include "CVReader.h"
#include "OSCSender.h"
#include "Config.h"
#include "CommandLineInterface.h"
#include "AudioDeviceManager.h"
#include "ErrorHandler.h"
#include "PerformanceMonitor.h"
#include "ConfigWatcher.h"
#include "MacOSPermissions.h"
#include "Version.h"

class CVToOSCConverter {
private:
    std::unique_ptr<CVReader> cvReader;
    std::unique_ptr<OSCSender> oscSender;
    std::unique_ptr<PerformanceMonitor> performanceMonitor;
    std::unique_ptr<ConfigWatcher> configWatcher;
    std::atomic<bool> running;
    Config config;
    std::string configFile;
    std::vector<float> cvBuffer;  // Reusable buffer
    std::vector<float> normalizedBuffer;  // Normalized values buffer
    std::vector<std::string> oscAddresses;  // Pre-computed OSC addresses

public:
    CVToOSCConverter(const std::string& configFile = "config.json", const CLIOptions* cliOptions = nullptr) 
        : running(false), configFile(configFile) {
        // Load configuration
        config.loadFromFile(configFile);
        
        // Initialize performance monitor
        performanceMonitor = std::make_unique<PerformanceMonitor>();
        performanceMonitor->setConfig(MonitorConfigFactory::createHighPerformanceConfig());

        // Initialize config watcher
        configWatcher = std::make_unique<ConfigWatcher>(configFile);
        configWatcher->start([this](const Config& newConfig) {
            onConfigChanged(newConfig);
        });

        // Configure error handling based on CLI options
        if (cliOptions) {
            ErrorHandler& errorHandler = ErrorHandler::getInstance();
            errorHandler.setLogLevel(ErrorHandler::stringToSeverity(cliOptions->logLevel));
            errorHandler.setConsoleOutput(!cliOptions->quiet);
            
            // Apply CLI overrides
            if (!cliOptions->oscHost.empty()) {
                config.setOSCHost(cliOptions->oscHost);
            }
            if (!cliOptions->oscPort.empty()) {
                config.setOSCPort(cliOptions->oscPort);
            }
            if (!cliOptions->audioDevice.empty()) {
                config.setAudioDevice(cliOptions->audioDevice);
            }
            if (cliOptions->updateInterval > 0) {
                config.setUpdateIntervalMs(cliOptions->updateInterval);
            }
        }
        
        // Initialize CV reader and OSC sender with error handling
        try {
            cvReader = std::make_unique<CVReader>(config.getAudioDevice());
            oscSender = std::make_unique<OSCSender>(config.getOSCHost(), config.getOSCPort());
        } catch (const std::exception& e) {
            ERROR_CRITICAL("Failed to initialize audio/network components", e.what(), 
                          "Check audio devices and network connectivity");
            throw;
        }
        
        // Pre-allocate buffers and addresses for better performance
        int channelCount = cvReader->getChannelCount();
        cvBuffer.reserve(channelCount);
        normalizedBuffer.reserve(channelCount);
        oscAddresses.reserve(channelCount);
        
        // Pre-compute OSC addresses to avoid string concatenation in hot loop
        for (int i = 0; i < channelCount; ++i) {
            oscAddresses.push_back("/cv/channel/" + std::to_string(i + 1));
        }
        
        if (!cliOptions || !cliOptions->quiet) {
            std::cout << "Initialized with " << channelCount << " channels" << std::endl;
            config.printConfiguration();
        }
    }

    void start() {
        running = true;
        
        // Start performance monitoring
        performanceMonitor->start();
        
        ErrorHandler::getInstance().logInfo("Starting CV to OSC converter", 
                                           "OSC target: " + config.getOSCHost() + ":" + config.getOSCPort());
        std::cout << "Starting CV to OSC converter..." << std::endl;
        std::cout << "OSC target: " << config.getOSCHost() << ":" << config.getOSCPort() << std::endl;
        
        // Add error callback for monitoring
        ErrorHandler::getInstance().addErrorCallback([this](const ErrorInfo& error) {
            if (error.severity >= ErrorSeverity::ERROR_LEVEL) {
                // Handle critical errors that might require stopping the converter
                if (error.category == ErrorCategory::AUDIO && !error.recoverable) {
                    running = false;
                }
            }
        });
        
        // Main conversion loop with optimized performance
        auto lastUpdateTime = std::chrono::steady_clock::now();
        const auto updateInterval = std::chrono::milliseconds(config.getUpdateIntervalMs());
        
        // Performance monitoring variables
        auto lastPerformanceCheck = std::chrono::steady_clock::now();
        int cycleCount = 0;
        
        while (running) {
            try {
                performanceMonitor->recordCycleStart();
                
                // Read CV values from audio interface (zero-copy version)
                cvReader->readChannels(cvBuffer);
                
                // Normalize all values first
                normalizedBuffer.resize(cvBuffer.size());
                for (size_t i = 0; i < cvBuffer.size(); ++i) {
                    normalizedBuffer[i] = normalizeCV(cvBuffer[i], static_cast<int>(i));
                }
                
                // Send all OSC messages in a batch for better performance
                {
                    auto networkStart = std::chrono::steady_clock::now();
                    if (!oscSender->sendFloatBatch(oscAddresses, normalizedBuffer)) {
                        performanceMonitor->recordOSCMessageFailed();
                        PERFORMANCE_WARNING("OSC transmission failed", 
                                           "Some CV data may be lost", 
                                           "Check network connectivity");
                    } else {
                        performanceMonitor->recordOSCMessageSent();
                    }
                    auto networkEnd = std::chrono::steady_clock::now();
                    auto networkLatency = std::chrono::duration_cast<std::chrono::nanoseconds>(networkEnd - networkStart);
                    performanceMonitor->recordNetworkLatency(networkLatency);
                }
                
                // Performance monitoring
                cycleCount++;
                auto now = std::chrono::steady_clock::now();
                auto performanceElapsed = now - lastPerformanceCheck;
                
                if (performanceElapsed > std::chrono::seconds(10)) {
                    double actualRate = cycleCount / std::chrono::duration<double>(performanceElapsed).count();
                    double expectedRate = 1000.0 / config.getUpdateIntervalMs();
                    
                    if (actualRate < expectedRate * 0.9) { // More than 10% slower than expected
                        std::string details = "Actual: " + std::to_string(actualRate) + " Hz, Expected: " + 
                                            std::to_string(expectedRate) + " Hz";
                        PERFORMANCE_WARNING("Performance degradation detected", details, 
                                           "Consider reducing update rate or checking system load");
                    }
                    
                    lastPerformanceCheck = now;
                    cycleCount = 0;
                }
                
                // Precise timing control
                auto elapsed = now - lastUpdateTime;
                if (elapsed < updateInterval) {
                    std::this_thread::sleep_for(updateInterval - elapsed);
                }
                lastUpdateTime = std::chrono::steady_clock::now();
                performanceMonitor->recordCycleEnd();
                
            } catch (const std::exception& e) {
                ERROR_ERROR("Exception in main conversion loop", e.what(), 
                           "Attempting to continue", true);
                std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Brief pause before retrying
            }
        }
    }

    void stop() {
        running = false;
        ErrorHandler::getInstance().logInfo("Stopping CV to OSC converter", "Graceful shutdown requested");
        ErrorHandler::getInstance().removeAllCallbacks();
        std::cout << "Stopping CV to OSC converter..." << std::endl;
    }
    
    Config& getConfig() { return config; }
    const Config& getConfig() const { return config; }

private:
    void onConfigChanged(const Config& newConfig) {
        std::cout << "Configuration changed - hot reloading..." << std::endl;
        
        // Update internal config
        config = newConfig;
        
        // Update OSC sender target if changed
        if (oscSender) {
            oscSender->setTarget(config.getOSCHost(), config.getOSCPort());
        }
        
        // Apply new CV ranges (already handled by config)
        std::cout << "Configuration reloaded successfully" << std::endl;
        
        // Log the change
        ErrorHandler::getInstance().logInfo("Configuration hot-reloaded", 
                                          "New OSC target: " + config.getOSCHost() + ":" + config.getOSCPort());
    }
    
    float normalizeCV(float cvValue, int channel) {
        // Convert CV range to normalized 0-1 range
        // Assuming CV input range is configurable per channel
        CVRange range = config.getCVRange(channel);
        
        if (range.min >= range.max) {
            return 0.0f;
        }
        
        // Clamp to range
        cvValue = std::max(range.min, std::min(range.max, cvValue));
        
        // Normalize to 0-1
        return (cvValue - range.min) / (range.max - range.min);
    }
};

int main(int argc, char* argv[]) {
    CommandLineInterface cli(argc, argv);
    
    if (!cli.parseArguments()) {
        return 1;
    }
    
    const CLIOptions& options = cli.getOptions();
    
    // Handle special options first
    if (options.help) {
        cli.printHelp();
        return 0;
    }
    
    if (options.version) {
        cli.printVersion();
        return 0;
    }
    
    // Check for permission options
    if (options.checkPermissions) {
#ifdef __APPLE__
        std::cout << MacOSPermissions::generatePermissionReport() << std::endl;
#else
        std::cout << "🔐 Permission Status Report\n";
        std::cout << "============================\n";
        std::cout << "Platform: Non-Apple system\n";
        std::cout << "Permissions: Not applicable on this platform\n";
        std::cout << "All Required Permissions: ✅ Granted\n";
#endif
        return 0;
    }
    
    if (options.requestPermissions) {
        std::cout << "🔐 Requesting all required permissions..." << std::endl;
#ifdef __APPLE__
        MacOSPermissions::requestAllRequiredPermissions([](bool granted) {
            if (granted) {
                std::cout << "✅ All permissions granted! You can now run the application normally." << std::endl;
            } else {
                std::cout << "❌ Some permissions were denied. The application may not function properly." << std::endl;
                std::cout << "Please enable the required permissions in System Preferences." << std::endl;
            }
        });
        
        // Wait a bit for the async callback to complete
        std::this_thread::sleep_for(std::chrono::seconds(3));
#else
        std::cout << "✅ Permissions are not required on this platform." << std::endl;
#endif
        return 0;
    }
    
    if (options.listDevices) {
        AudioDeviceManager deviceManager;
        if (!deviceManager.initialize()) {
            std::cerr << "Failed to initialize audio device manager!" << std::endl;
            return 1;
        }
        
        if (options.verbose) {
            deviceManager.runDetailedDiagnostics();
        } else {
            // Simple listing for non-verbose mode
            auto inputDevices = deviceManager.getInputDevices();
            std::cout << "Available Input Devices:" << std::endl;
            for (const auto& device : inputDevices) {
                std::cout << "  [" << device.index << "] " << device.name;
                if (device.isDefaultInput) {
                    std::cout << " (default)";
                }
                std::cout << " - " << device.maxInputChannels << " channels";
                if (!device.isCurrentlyAvailable) {
                    std::cout << " [UNAVAILABLE]";
                }
                std::cout << std::endl;
            }
            
            // If no devices are available, show helpful message
            bool hasAvailable = false;
            for (const auto& device : inputDevices) {
                if (device.isCurrentlyAvailable) {
                    hasAvailable = true;
                    break;
                }
            }
            
            if (!hasAvailable && !inputDevices.empty()) {
                std::cout << "\n⚠️  All devices are UNAVAILABLE. Run with --verbose for detailed diagnostics." << std::endl;
                std::cout << "🔧 Quick fix: ./cv_to_osc_converter --request-permissions" << std::endl;
            }
        }
        
        deviceManager.cleanup();
        return 0;
    }
    
    // Interactive mode
    if (options.interactive) {
        if (!cli.runInteractiveMode()) {
            return 0; // User chose to exit
        }
    }
    
    // Print header unless in quiet mode
    if (!options.quiet) {
        std::cout << Version::getAppTitle() << std::endl;
        std::cout << std::string(Version::getAppTitle().length(), '=') << std::endl;
        if (Version::isDevelopment()) {
            std::cout << "⚠️  Development Build" << std::endl;
        }
    }

    try {
        CVToOSCConverter converter(options.configFile, &options);
        
        // Profile management
        if (!options.quiet) {
            std::cout << "Available Profiles:" << std::endl;
            for (const auto& profileName : converter.getConfig().getProfileNames()) {
                std::cout << "  " << profileName << (profileName == converter.getConfig().getActiveProfileName() ? " (active)" : "") << std::endl;
            }

            std::string newProfile;
            std::cout << "Enter profile to activate or press Enter to continue: ";
            std::getline(std::cin, newProfile);
            if (!newProfile.empty()) {
                if (!converter.getConfig().setActiveProfile(newProfile)) {
                    std::cout << "Profile not found. Continuing with current profile." << std::endl;
                } else {
                    std::cout << "Profile switched to " << converter.getConfig().getActiveProfileName() << std::endl;
                    converter.getConfig().saveToFile("config.json");
                }
            }
        }

        if (options.daemon) {
            // Daemon mode - run without user interaction
            if (!options.quiet) {
                std::cout << "Running in daemon mode. Send SIGTERM to stop." << std::endl;
            }
            converter.start();
        } else {
            // Interactive mode - start conversion in a separate thread
            std::thread converterThread([&converter]() {
                converter.start();
            });

            // Wait for user input to stop
            if (!options.quiet) {
                std::cout << "Press Enter to stop..." << std::endl;
            }
            std::cin.get();

            converter.stop();
            converterThread.join();
        }

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    if (!options.quiet) {
        std::cout << "Converter stopped." << std::endl;
    }
    return 0;
}
