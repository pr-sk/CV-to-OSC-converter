#include "CommandLineInterface.h"
#include "Config.h"
#include "CVReader.h"
#include "AudioDeviceManager.h"
#include "Version.h"
#include <algorithm>
#include <iomanip>
#include <cstdlib>

#ifdef _WIN32
#include <windows.h>
#else
#include <termios.h>
#include <unistd.h>
#endif

CommandLineInterface::CommandLineInterface(int argc, char* argv[]) {
    for (int i = 1; i < argc; ++i) {
        args.push_back(std::string(argv[i]));
    }
}

bool CommandLineInterface::parseArguments() {
    for (size_t i = 0; i < args.size(); ++i) {
        const std::string& arg = args[i];
        
        if (arg == "-h" || arg == "--help") {
            options.help = true;
        }
        else if (arg == "-v" || arg == "--version") {
            options.version = true;
        }
        else if (arg == "-i" || arg == "--interactive") {
            options.interactive = true;
        }
        else if (arg == "-l" || arg == "--list-devices") {
            options.listDevices = true;
        }
        else if (arg == "-d" || arg == "--daemon") {
            options.daemon = true;
        }
        else if (arg == "--verbose") {
            options.verbose = true;
        }
        else if (arg == "-q" || arg == "--quiet") {
            options.quiet = true;
        }
        else if (arg == "-c" || arg == "--config") {
            if (i + 1 < args.size()) {
                options.configFile = args[++i];
            } else {
                std::cerr << "Error: --config requires a filename" << std::endl;
                return false;
            }
        }
        else if (arg == "--osc-host") {
            if (i + 1 < args.size()) {
                options.oscHost = args[++i];
            } else {
                std::cerr << "Error: --osc-host requires a hostname" << std::endl;
                return false;
            }
        }
        else if (arg == "--osc-port") {
            if (i + 1 < args.size()) {
                options.oscPort = args[++i];
            } else {
                std::cerr << "Error: --osc-port requires a port number" << std::endl;
                return false;
            }
        }
        else if (arg == "--audio-device") {
            if (i + 1 < args.size()) {
                options.audioDevice = args[++i];
            } else {
                std::cerr << "Error: --audio-device requires a device name" << std::endl;
                return false;
            }
        }
        else if (arg == "--update-interval") {
            if (i + 1 < args.size()) {
                try {
                    options.updateInterval = std::stoi(args[++i]);
                } catch (const std::exception& e) {
                    std::cerr << "Error: Invalid update interval: " << args[i] << std::endl;
                    return false;
                }
            } else {
                std::cerr << "Error: --update-interval requires a number (ms)" << std::endl;
                return false;
            }
        }
        else if (arg == "--log-level") {
            if (i + 1 < args.size()) {
                options.logLevel = args[++i];
                std::transform(options.logLevel.begin(), options.logLevel.end(), 
                             options.logLevel.begin(), ::tolower);
                if (options.logLevel != "debug" && options.logLevel != "info" && 
                    options.logLevel != "warn" && options.logLevel != "error") {
                    std::cerr << "Error: Invalid log level. Use: debug, info, warn, error" << std::endl;
                    return false;
                }
            } else {
                std::cerr << "Error: --log-level requires a level" << std::endl;
                return false;
            }
        }
        else {
            std::cerr << "Error: Unknown option: " << arg << std::endl;
            return false;
        }
    }
    
    return true;
}

void CommandLineInterface::printHelp() const {
    std::cout << Colors::BOLD << Version::getAppTitle() << Colors::RESET << std::endl;
    std::cout << "Convert Control Voltage signals to Open Sound Control messages" << std::endl;
    std::cout << std::endl;
    
    printUsage();
    
    std::cout << Colors::BOLD << "Options:" << Colors::RESET << std::endl;
    std::cout << "  -h, --help              Show this help message" << std::endl;
    std::cout << "  -v, --version           Show version information" << std::endl;
    std::cout << "  -i, --interactive       Run in interactive mode" << std::endl;
    std::cout << "  -l, --list-devices      List available audio devices" << std::endl;
    std::cout << "  -d, --daemon            Run as daemon (background mode)" << std::endl;
    std::cout << "  -c, --config FILE       Use specific config file (default: config.json)" << std::endl;
    std::cout << "  --verbose               Enable verbose output" << std::endl;
    std::cout << "  -q, --quiet             Suppress non-essential output" << std::endl;
    std::cout << std::endl;
    
    std::cout << Colors::BOLD << "Configuration Overrides:" << Colors::RESET << std::endl;
    std::cout << "  --osc-host HOST         Override OSC target host" << std::endl;
    std::cout << "  --osc-port PORT         Override OSC target port" << std::endl;
    std::cout << "  --audio-device NAME     Override audio device" << std::endl;
    std::cout << "  --update-interval MS    Override update interval (milliseconds)" << std::endl;
    std::cout << "  --log-level LEVEL       Set log level (debug, info, warn, error)" << std::endl;
    std::cout << std::endl;
    
    std::cout << Colors::BOLD << "Examples:" << Colors::RESET << std::endl;
    std::cout << "  ./cv_to_osc_converter                     # Run with default settings" << std::endl;
    std::cout << "  ./cv_to_osc_converter -i                  # Run in interactive mode" << std::endl;
    std::cout << "  ./cv_to_osc_converter -l                  # List audio devices" << std::endl;
    std::cout << "  ./cv_to_osc_converter --osc-host 192.168.1.100 --osc-port 8000" << std::endl;
    std::cout << "  ./cv_to_osc_converter -c my_config.json   # Use custom config file" << std::endl;
    std::cout << "  ./cv_to_osc_converter -d --quiet          # Run as quiet daemon" << std::endl;
}

void CommandLineInterface::printVersion() const {
    std::cout << Colors::BOLD << Version::getAppTitle() << Colors::RESET << std::endl;
    std::cout << std::endl;
    std::cout << "Version: " << Version::getVersionWithGit() << std::endl;
    std::cout << "Build: " << Version::getBuildInfo() << std::endl;
    std::cout << "Platform: ";
#ifdef __APPLE__
    std::cout << "macOS";
#elif __linux__
    std::cout << "Linux";
#elif _WIN32
    std::cout << "Windows";
#else
    std::cout << "Unknown";
#endif
    std::cout << std::endl;
    std::cout << "Compiler: " << __VERSION__ << std::endl;
    
    if (Version::isDevelopment()) {
        std::cout << std::endl;
        std::cout << Colors::YELLOW << "⚠️  Development Build" << Colors::RESET << std::endl;
        std::cout << "This is a development version and may contain bugs." << std::endl;
    }
}

void CommandLineInterface::printUsage() const {
    std::cout << Colors::BOLD << "Usage:" << Colors::RESET << std::endl;
    std::cout << "  cv_to_osc_converter [OPTIONS]" << std::endl;
    std::cout << std::endl;
}

bool CommandLineInterface::runInteractiveMode() {
    clearScreen();
    std::cout << Colors::BOLD << Colors::CYAN 
              << "===================================================" << std::endl
              << "   CV to OSC Converter - Interactive Mode" << std::endl
              << "===================================================" 
              << Colors::RESET << std::endl << std::endl;
    
    while (true) {
        showMainMenu();
        std::string choice = getUserInput("Select option [1-6]", "1");
        
        if (choice == "1") {
            return true; // Start converter
        }
        else if (choice == "2") {
            showConfigurationMenu();
        }
        else if (choice == "3") {
            showDeviceSelectionMenu();
        }
        else if (choice == "4") {
            showMonitoringMenu();
        }
        else if (choice == "5") {
            // Run tests
            std::cout << Colors::YELLOW << "Running automated tests..." << Colors::RESET << std::endl;
            int result = system("./run_tests.sh");
            if (result == 0) {
                std::cout << Colors::GREEN << "All tests passed!" << Colors::RESET << std::endl;
            } else {
                std::cout << Colors::RED << "Some tests failed!" << Colors::RESET << std::endl;
            }
            pauseForUser();
        }
        else if (choice == "6" || choice == "q" || choice == "quit") {
            std::cout << Colors::GREEN << "Goodbye!" << Colors::RESET << std::endl;
            return false; // Exit
        }
        else {
            std::cout << Colors::RED << "Invalid option. Please try again." << Colors::RESET << std::endl;
            pauseForUser();
        }
    }
}

void CommandLineInterface::showMainMenu() {
    clearScreen();
    std::cout << Colors::BOLD << Colors::BLUE << "Main Menu" << Colors::RESET << std::endl;
    std::cout << std::string(40, '-') << std::endl;
    std::cout << "1. " << Colors::GREEN << "Start CV to OSC Converter" << Colors::RESET << std::endl;
    std::cout << "2. " << Colors::YELLOW << "Configuration Settings" << Colors::RESET << std::endl;
    std::cout << "3. " << Colors::CYAN << "Audio Device Selection" << Colors::RESET << std::endl;
    std::cout << "4. " << Colors::MAGENTA << "Monitoring & Diagnostics" << Colors::RESET << std::endl;
    std::cout << "5. " << Colors::WHITE << "Run Tests" << Colors::RESET << std::endl;
    std::cout << "6. " << Colors::RED << "Exit" << Colors::RESET << std::endl;
    std::cout << std::endl;
}

void CommandLineInterface::showConfigurationMenu() {
    clearScreen();
    std::cout << Colors::BOLD << Colors::YELLOW << "Configuration Settings" << Colors::RESET << std::endl;
    std::cout << std::string(40, '-') << std::endl;
    
    // Load current config
    Config config;
    config.loadFromFile(options.configFile);
    
    std::cout << "Current Configuration:" << std::endl;
    std::cout << "  OSC Host: " << Colors::CYAN << config.getOSCHost() << Colors::RESET << std::endl;
    std::cout << "  OSC Port: " << Colors::CYAN << config.getOSCPort() << Colors::RESET << std::endl;
    std::cout << "  Audio Device: " << Colors::CYAN << (config.getAudioDevice().empty() ? "default" : config.getAudioDevice()) << Colors::RESET << std::endl;
    std::cout << "  Update Rate: " << Colors::CYAN << (1000 / config.getUpdateIntervalMs()) << " Hz" << Colors::RESET << std::endl;
    std::cout << std::endl;
    
    std::cout << "1. Change OSC Host" << std::endl;
    std::cout << "2. Change OSC Port" << std::endl;
    std::cout << "3. Change Audio Device" << std::endl;
    std::cout << "4. Change Update Rate" << std::endl;
    std::cout << "5. Edit CV Ranges" << std::endl;
    std::cout << "6. Save Configuration" << std::endl;
    std::cout << "7. Back to Main Menu" << std::endl;
    std::cout << std::endl;
    
    std::string choice = getUserInput("Select option [1-7]", "7");
    
    if (choice == "1") {
        std::string newHost = getUserInput("Enter OSC Host", config.getOSCHost());
        config.setOSCHost(newHost);
        std::cout << Colors::GREEN << "OSC Host updated to: " << newHost << Colors::RESET << std::endl;
    }
    else if (choice == "2") {
        std::string newPort = getUserInput("Enter OSC Port", config.getOSCPort());
        config.setOSCPort(newPort);
        std::cout << Colors::GREEN << "OSC Port updated to: " << newPort << Colors::RESET << std::endl;
    }
    else if (choice == "3") {
        std::string newDevice = getUserInput("Enter Audio Device (empty for default)", config.getAudioDevice());
        config.setAudioDevice(newDevice);
        std::cout << Colors::GREEN << "Audio Device updated to: " << (newDevice.empty() ? "default" : newDevice) << Colors::RESET << std::endl;
    }
    else if (choice == "4") {
        std::string rateStr = getUserInput("Enter Update Rate (Hz)", std::to_string(1000 / config.getUpdateIntervalMs()));
        try {
            int rate = std::stoi(rateStr);
            if (rate > 0 && rate <= 1000) {
                config.setUpdateIntervalMs(1000 / rate);
                std::cout << Colors::GREEN << "Update Rate updated to: " << rate << " Hz" << Colors::RESET << std::endl;
            } else {
                std::cout << Colors::RED << "Invalid rate. Must be between 1-1000 Hz" << Colors::RESET << std::endl;
            }
        } catch (...) {
            std::cout << Colors::RED << "Invalid number format" << Colors::RESET << std::endl;
        }
    }
    else if (choice == "6") {
        if (config.saveToFile(options.configFile)) {
            std::cout << Colors::GREEN << "Configuration saved successfully!" << Colors::RESET << std::endl;
        } else {
            std::cout << Colors::RED << "Failed to save configuration!" << Colors::RESET << std::endl;
        }
    }
    
    if (choice != "7") {
        pauseForUser();
        showConfigurationMenu(); // Recursively show menu again
    }
}

void CommandLineInterface::showDeviceSelectionMenu() {
    clearScreen();
    std::cout << Colors::BOLD << Colors::CYAN << "Audio Device Selection" << Colors::RESET << std::endl;
    std::cout << std::string(40, '-') << std::endl;
    
    AudioDeviceManager deviceManager;
    if (!deviceManager.initialize()) {
        std::cout << Colors::RED << "Failed to initialize audio device manager!" << Colors::RESET << std::endl;
        pauseForUser();
        return;
    }
    
    while (true) {
        clearScreen();
        std::cout << Colors::BOLD << Colors::CYAN << "Audio Device Selection" << Colors::RESET << std::endl;
        std::cout << std::string(40, '-') << std::endl;
        
        std::cout << "1. List All Audio Devices" << std::endl;
        std::cout << "2. List Input Devices Only" << std::endl;
        std::cout << "3. Show Device Details" << std::endl;
        std::cout << "4. Test Device" << std::endl;
        std::cout << "5. Search Devices" << std::endl;
        std::cout << "6. Refresh Device List" << std::endl;
        std::cout << "7. Device Status Report" << std::endl;
        std::cout << "8. Back to Main Menu" << std::endl;
        std::cout << std::endl;
        
        std::string choice = getUserInput("Select option [1-8]", "8");
        
        if (choice == "1") {
            deviceManager.printDeviceList();
            pauseForUser();
        }
        else if (choice == "2") {
            auto inputDevices = deviceManager.getInputDevices();
            std::cout << Colors::YELLOW << "\nInput Devices (" << inputDevices.size() << " found):" << Colors::RESET << std::endl;
            std::cout << std::string(80, '-') << std::endl;
            
            for (const auto& device : inputDevices) {
                std::cout << "[" << device.index << "] " << device.name 
                          << " (" << device.maxInputChannels << " channels, " 
                          << device.hostApi << ")";
                if (device.isDefaultInput) {
                    std::cout << Colors::GREEN << " [DEFAULT]" << Colors::RESET;
                }
                if (!device.isCurrentlyAvailable) {
                    std::cout << Colors::RED << " [UNAVAILABLE]" << Colors::RESET;
                }
                std::cout << std::endl;
            }
            pauseForUser();
        }
        else if (choice == "3") {
            std::string indexStr = getUserInput("Enter device index", "0");
            try {
                int index = std::stoi(indexStr);
                deviceManager.printDeviceDetails(index);
            } catch (...) {
                std::cout << Colors::RED << "Invalid device index!" << Colors::RESET << std::endl;
            }
            pauseForUser();
        }
        else if (choice == "4") {
            std::string indexStr = getUserInput("Enter device index to test", "0");
            std::string channelsStr = getUserInput("Enter number of channels", "2");
            try {
                int index = std::stoi(indexStr);
                int channels = std::stoi(channelsStr);
                
                std::cout << "Testing device " << index << " with " << channels << " channels..." << std::endl;
                
                if (deviceManager.testDevice(index, channels)) {
                    std::cout << Colors::GREEN << "✓ Device test PASSED" << Colors::RESET << std::endl;
                } else {
                    std::cout << Colors::RED << "✗ Device test FAILED" << Colors::RESET << std::endl;
                }
                
                bool formatSupported = deviceManager.canDeviceHandleFormat(index, channels, 44100.0);
                std::cout << "Format support (" << channels << " ch, 44.1kHz): " 
                          << (formatSupported ? Colors::GREEN + "SUPPORTED" : Colors::RED + "NOT SUPPORTED")
                          << Colors::RESET << std::endl;
                          
            } catch (...) {
                std::cout << Colors::RED << "Invalid input!" << Colors::RESET << std::endl;
            }
            pauseForUser();
        }
        else if (choice == "5") {
            std::string searchTerm = getUserInput("Enter search term", "");
            if (!searchTerm.empty()) {
                auto matchingDevices = deviceManager.findDevicesContaining(searchTerm);
                std::cout << Colors::YELLOW << "\nDevices matching '" << searchTerm << "' (" 
                          << matchingDevices.size() << " found):" << Colors::RESET << std::endl;
                std::cout << std::string(60, '-') << std::endl;
                
                for (const auto& device : matchingDevices) {
                    std::cout << "[" << device.index << "] " << device.name;
                    if (device.maxInputChannels > 0) {
                        std::cout << " (" << device.maxInputChannels << " in)";
                    }
                    if (device.isDefaultInput) {
                        std::cout << Colors::GREEN << " [DEFAULT INPUT]" << Colors::RESET;
                    }
                    std::cout << std::endl;
                }
            }
            pauseForUser();
        }
        else if (choice == "6") {
            std::cout << "Refreshing device list..." << std::endl;
            deviceManager.refreshDeviceList();
            bool hasChanges = deviceManager.detectDeviceChanges();
            std::cout << Colors::GREEN << "Device list refreshed!" << Colors::RESET;
            if (hasChanges) {
                std::cout << Colors::YELLOW << " (Changes detected)" << Colors::RESET;
            }
            std::cout << std::endl;
            pauseForUser();
        }
        else if (choice == "7") {
            std::cout << Colors::YELLOW << deviceManager.getDeviceStatusReport() << Colors::RESET << std::endl;
            pauseForUser();
        }
        else if (choice == "8") {
            break;
        }
        else {
            std::cout << Colors::RED << "Invalid option. Please try again." << Colors::RESET << std::endl;
            pauseForUser();
        }
    }
    
    deviceManager.cleanup();
}

void CommandLineInterface::showMonitoringMenu() {
    clearScreen();
    std::cout << Colors::BOLD << Colors::MAGENTA << "Monitoring & Diagnostics" << Colors::RESET << std::endl;
    std::cout << std::string(40, '-') << std::endl;
    
    std::cout << "1. View Current CV Values (Live Monitor)" << std::endl;
    std::cout << "2. Test OSC Connection" << std::endl;
    std::cout << "3. Audio Device Status" << std::endl;
    std::cout << "4. Performance Metrics" << std::endl;
    std::cout << "5. Back to Main Menu" << std::endl;
    std::cout << std::endl;
    
    std::string choice = getUserInput("Select option [1-5]", "5");
    
    if (choice != "5") {
        std::cout << Colors::YELLOW << "Monitoring features will be enhanced in upcoming updates." << Colors::RESET << std::endl;
        pauseForUser();
        showMonitoringMenu();
    }
}

std::string CommandLineInterface::getUserInput(const std::string& prompt, const std::string& defaultValue) {
    std::cout << Colors::BOLD << prompt;
    if (!defaultValue.empty()) {
        std::cout << " [" << Colors::CYAN << defaultValue << Colors::RESET << Colors::BOLD << "]";
    }
    std::cout << ": " << Colors::RESET;
    
    std::string input;
    std::getline(std::cin, input);
    
    if (input.empty() && !defaultValue.empty()) {
        return defaultValue;
    }
    
    return input;
}

bool CommandLineInterface::getUserConfirmation(const std::string& prompt) {
    std::string input = getUserInput(prompt + " (y/n)", "n");
    std::transform(input.begin(), input.end(), input.begin(), ::tolower);
    return input == "y" || input == "yes";
}

void CommandLineInterface::clearScreen() {
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
}

void CommandLineInterface::pauseForUser() {
    std::cout << std::endl << Colors::BOLD << "Press Enter to continue..." << Colors::RESET;
    std::cin.get();
}
