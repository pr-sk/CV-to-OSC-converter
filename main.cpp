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

class CVToOSCConverter {
private:
    std::unique_ptr<CVReader> cvReader;
    std::unique_ptr<OSCSender> oscSender;
    std::atomic<bool> running;
    Config config;
    std::vector<float> cvBuffer;  // Reusable buffer
    std::vector<float> normalizedBuffer;  // Normalized values buffer
    std::vector<std::string> oscAddresses;  // Pre-computed OSC addresses

public:
    CVToOSCConverter(const std::string& configFile = "config.json", const CLIOptions* cliOptions = nullptr) : running(false) {
        // Load configuration
        config.loadFromFile(configFile);
        
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
        ErrorHandler::getInstance().logInfo("Starting CV to OSC converter", 
                                           "OSC target: " + config.getOSCHost() + ":" + config.getOSCPort());
        std::cout << "Starting CV to OSC converter..." << std::endl;
        std::cout << "OSC target: " << config.getOSCHost() << ":" << config.getOSCPort() << std::endl;
        
        // Add error callback for monitoring
        ErrorHandler::getInstance().addErrorCallback([this](const ErrorInfo& error) {
            if (error.severity >= ErrorSeverity::ERROR) {
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
                // Read CV values from audio interface (zero-copy version)
                cvReader->readChannels(cvBuffer);
                
                // Normalize all values first
                normalizedBuffer.resize(cvBuffer.size());
                for (size_t i = 0; i < cvBuffer.size(); ++i) {
                    normalizedBuffer[i] = normalizeCV(cvBuffer[i], static_cast<int>(i));
                }
                
                // Send all OSC messages in a batch for better performance
                if (!oscSender->sendFloatBatch(oscAddresses, normalizedBuffer)) {
                    // Network error occurred, but continue trying
                    PERFORMANCE_WARNING("OSC transmission failed", 
                                       "Some CV data may be lost", 
                                       "Check network connectivity");
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

private:
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
    
    if (options.listDevices) {
        AudioDeviceManager deviceManager;
        if (!deviceManager.initialize()) {
            std::cerr << "Failed to initialize audio device manager!" << std::endl;
            return 1;
        }
        
        if (options.verbose) {
            deviceManager.printDeviceList();
            std::cout << std::endl;
            std::cout << deviceManager.getDeviceStatusReport() << std::endl;
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
        std::cout << "CV to OSC Converter v1.0" << std::endl;
        std::cout << "=========================" << std::endl;
    }

    try {
        CVToOSCConverter converter(options.configFile, &options);
        
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
