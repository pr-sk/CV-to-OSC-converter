#include <iostream>
#include <exception>
#include "GuiApplication.h"
#include "Version.h"

int main(int argc, char* argv[]) {
    try {
        // Print application info
        std::cout << Version::getAppTitle() << " - GUI Version" << std::endl;
        std::cout << "Version: " << Version::getVersionWithGit() << std::endl;
        std::cout << std::string(50, '=') << std::endl;
        
        // Create and initialize GUI application
        GuiApplication app;
        
        if (!app.initialize()) {
            std::cerr << "Failed to initialize GUI application" << std::endl;
            return -1;
        }
        
        std::cout << "Starting GUI..." << std::endl;
        
        // Run the application
        app.run();
        
        // Cleanup
        app.shutdown();
        
        std::cout << "Application terminated successfully" << std::endl;
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return -1;
    } catch (...) {
        std::cerr << "Unknown fatal error occurred" << std::endl;
        return -1;
    }
}
