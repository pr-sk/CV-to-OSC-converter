#include "OSCReceiver.h"
#include "ErrorHandler.h"
#include <iostream>
#include <string>
#include <chrono>
#include <thread>
#include <signal.h>
#include <iomanip>

static bool running = true;

void signalHandler(int signal) {
    std::cout << "\nReceived signal " << signal << ", shutting down..." << std::endl;
    running = false;
}

int main(int argc, char* argv[]) {
    std::string port = "9000";
    
    // Parse command line arguments
    if (argc > 1) {
        port = argv[1];
    }
    
    std::cout << "CV to OSC Converter - OSC Receiver Test Tool" << std::endl;
    std::cout << "=============================================" << std::endl;
    std::cout << "Listening on port: " << port << std::endl;
    std::cout << "Press Ctrl+C to stop" << std::endl;
    std::cout << "=============================================" << std::endl;
    
    // Setup signal handling
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);
    
    try {
        // Create OSC receiver
        OSCReceiver receiver(port);
        
        // Set up callbacks
        receiver.setMessageCallback([](const std::string& address, const std::vector<float>& values) {
            auto now = std::chrono::system_clock::now();
            auto time_t = std::chrono::system_clock::to_time_t(now);
            auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                now.time_since_epoch()) % 1000;
            
            std::cout << "[" << std::put_time(std::localtime(&time_t), "%H:%M:%S") 
                      << "." << std::setfill('0') << std::setw(3) << ms.count() << "] "
                      << "OSC: " << address << " = [";
            
            for (size_t i = 0; i < values.size(); ++i) {
                std::cout << values[i];
                if (i < values.size() - 1) std::cout << ", ";
            }
            std::cout << "]" << std::endl;
        });
        
        receiver.setStringCallback([](const std::string& address, const std::string& value) {
            auto now = std::chrono::system_clock::now();
            auto time_t = std::chrono::system_clock::to_time_t(now);
            auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                now.time_since_epoch()) % 1000;
            
            std::cout << "[" << std::put_time(std::localtime(&time_t), "%H:%M:%S") 
                      << "." << std::setfill('0') << std::setw(3) << ms.count() << "] "
                      << "OSC: " << address << " = \"" << value << "\"" << std::endl;
        });
        
        receiver.setIntCallback([](const std::string& address, int value) {
            auto now = std::chrono::system_clock::now();
            auto time_t = std::chrono::system_clock::to_time_t(now);
            auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                now.time_since_epoch()) % 1000;
            
            std::cout << "[" << std::put_time(std::localtime(&time_t), "%H:%M:%S") 
                      << "." << std::setfill('0') << std::setw(3) << ms.count() << "] "
                      << "OSC: " << address << " = " << value << std::endl;
        });
        
        // Start the receiver
        if (!receiver.start()) {
            std::cerr << "Failed to start OSC receiver on port " << port << std::endl;
            return 1;
        }
        
        std::cout << "OSC receiver started successfully!" << std::endl;
        std::cout << "Server URL: " << receiver.getURL() << std::endl;
        std::cout << "Waiting for OSC messages..." << std::endl;
        std::cout << std::endl;
        
        // Main loop
        while (running) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        
        std::cout << "\nStopping OSC receiver..." << std::endl;
        receiver.stop();
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    std::cout << "OSC receiver test tool stopped." << std::endl;
    return 0;
}
