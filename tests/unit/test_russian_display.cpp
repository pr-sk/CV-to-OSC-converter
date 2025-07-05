#include <iostream>
#include <string>
#include <vector>
#include <locale>

int main() {
    // Set locale for proper UTF-8 handling
    std::setlocale(LC_ALL, "en_US.UTF-8");
    
    std::cout << "Testing Russian text display:" << std::endl;
    std::cout << "==============================" << std::endl;
    
    // Test strings with Russian characters (same as in device names)
    std::vector<std::string> testStrings = {
        "Микрофон MacBook Pro",
        "Микрофон (prubtsov)", 
        "NDI Audio",
        "VB-Cable",
        "Динамики MacBook Pro"
    };
    
    for (const auto& str : testStrings) {
        std::cout << "Device: " << str << std::endl;
        std::cout << "Length: " << str.length() << " bytes" << std::endl;
        
        // Check each byte
        std::cout << "Bytes: ";
        for (unsigned char c : str) {
            std::cout << std::hex << (int)c << " ";
        }
        std::cout << std::dec << std::endl;
        std::cout << "---" << std::endl;
    }
    
    // Test C-style strings (as used by PortAudio)
    const char* deviceName = "Микрофон MacBook Pro";
    std::string converted = deviceName ? deviceName : "Unknown Device";
    
    std::cout << "\nC-style conversion test:" << std::endl;
    std::cout << "Original: " << deviceName << std::endl;
    std::cout << "Converted: " << converted << std::endl;
    std::cout << "Match: " << (converted == testStrings[0] ? "YES" : "NO") << std::endl;
    
    return 0;
}
