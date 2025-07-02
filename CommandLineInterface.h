#pragma once

#include <string>
#include <vector>
#include <map>
#include <iostream>
#include <sstream>

struct CLIOptions {
    std::string configFile = "config.json";
    std::string logLevel = "info";
    bool interactive = false;
    bool listDevices = false;
    bool daemon = false;
    bool help = false;
    bool version = false;
    std::string oscHost = "";
    std::string oscPort = "";
    std::string audioDevice = "";
    int updateInterval = -1;
    bool verbose = false;
    bool quiet = false;
};

class CommandLineInterface {
private:
    CLIOptions options;
    std::vector<std::string> args;
    
public:
    CommandLineInterface(int argc, char* argv[]);
    
    bool parseArguments();
    void printHelp() const;
    void printVersion() const;
    const CLIOptions& getOptions() const { return options; }
    
    // Interactive mode functions
    bool runInteractiveMode();
    void showMainMenu();
    void showConfigurationMenu();
    void showDeviceSelectionMenu();
    void showMonitoringMenu();
    
private:
    void addArgument(const std::string& arg) { args.push_back(arg); }
    void printUsage() const;
    
    // Interactive helpers
    std::string getUserInput(const std::string& prompt, const std::string& defaultValue = "");
    bool getUserConfirmation(const std::string& prompt);
    void clearScreen();
    void pauseForUser();
};

// ANSI Color codes for better terminal output
namespace Colors {
    const std::string RESET = "\033[0m";
    const std::string RED = "\033[31m";
    const std::string GREEN = "\033[32m";
    const std::string YELLOW = "\033[33m";
    const std::string BLUE = "\033[34m";
    const std::string MAGENTA = "\033[35m";
    const std::string CYAN = "\033[36m";
    const std::string WHITE = "\033[37m";
    const std::string BOLD = "\033[1m";
}
