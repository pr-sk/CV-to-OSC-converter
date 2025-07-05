#include "Localization.h"
#include <iostream>

int main() {
    auto& loc = Localization::getInstance();
    loc.initialize();
    
    std::cout << "Testing Localization System" << std::endl;
    std::cout << "=============================" << std::endl;
    
    auto languages = loc.getAvailableLanguages();
    
    for (const auto& [lang, nativeName] : languages) {
        std::cout << "\n--- Testing " << nativeName << " ---" << std::endl;
        loc.setLanguage(lang);
        
        std::cout << "Window main: " << loc.getText("window.main") << std::endl;
        std::cout << "Menu file: " << loc.getText("menu.file") << std::endl;
        std::cout << "Menu settings: " << loc.getText("menu.settings") << std::endl;
        std::cout << "Menu language: " << loc.getText("menu.language") << std::endl;
        std::cout << "Button start: " << loc.getText("button.start") << std::endl;
        std::cout << "Button stop: " << loc.getText("button.stop") << std::endl;
        std::cout << "Audio device: " << loc.getText("audio.current_device") << std::endl;
        std::cout << "OSC connected: " << loc.getText("osc.connected") << std::endl;
    }
    
    std::cout << "\n✅ Localization test completed!" << std::endl;
    return 0;
}
