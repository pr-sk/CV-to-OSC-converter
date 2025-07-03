#include <iostream>
#include <string>
#include <locale>

int main() {
    // Set UTF-8 locale
    std::setlocale(LC_ALL, "en_US.UTF-8");
    
    std::cout << "UTF-8 Display Test" << std::endl;
    std::cout << "==================" << std::endl;
    
    // Test Russian device names
    std::string devices[] = {
        "Микрофон MacBook Pro",
        "Микрофон (prubtsov)",
        "Динамики MacBook Pro",
        "Наушники",
        "Устройство по умолчанию"
    };
    
    std::cout << "\nTest Russian device names:" << std::endl;
    for (const auto& device : devices) {
        std::cout << "Device: " << device << std::endl;
        
        // Check byte representation
        std::cout << "  Bytes: ";
        for (unsigned char c : device) {
            if (c >= 32 && c <= 126) {
                std::cout << c;
            } else {
                std::cout << "\\x" << std::hex << (int)c << std::dec;
            }
        }
        std::cout << std::endl;
        
        // Check length
        std::cout << "  Length: " << device.length() << " bytes" << std::endl;
        std::cout << "  Valid UTF-8: " << (device.length() > 0 ? "Yes" : "No") << std::endl;
        std::cout << std::endl;
    }
    
    // Test other languages
    std::cout << "Multi-language test:" << std::endl;
    std::cout << "English: Audio Device" << std::endl;
    std::cout << "Russian: Аудио устройство" << std::endl;
    std::cout << "Japanese: オーディオデバイス" << std::endl;
    std::cout << "Chinese: 音频设备" << std::endl;
    std::cout << "German: Audio-Gerät" << std::endl;
    std::cout << "French: Périphérique audio" << std::endl;
    std::cout << "Italian: Dispositivo audio" << std::endl;
    
    return 0;
}
