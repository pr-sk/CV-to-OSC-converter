#include <iostream>
#include <memory>
#include <thread>
#include <chrono>
#include "OSCMixerEngine.h"
#include "AudioDeviceManager.h"

int main() {
    std::cout << "=== Audio Input Test for CV to OSC Converter ===" << std::endl;
    
    // Create mixer engine with 1 channel
    auto mixerEngine = std::make_shared<OSCMixerEngine>(1);
    
    if (!mixerEngine->initialize()) {
        std::cerr << "Failed to initialize mixer engine!" << std::endl;
        return 1;
    }
    
    std::cout << "Mixer engine initialized successfully" << std::endl;
    
    // Get available input devices
    auto inputDevices = mixerEngine->getAvailableInputDevices();
    std::cout << "\nAvailable input devices:" << std::endl;
    for (const auto& device : inputDevices) {
        std::cout << "  - " << device.deviceName << " (ID: " << device.deviceId << ")" << std::endl;
    }
    
    if (inputDevices.empty()) {
        std::cerr << "No input devices found!" << std::endl;
        return 1;
    }
    
    // Add first available input device to channel 0
    std::cout << "\nAdding input device to channel 0: " << inputDevices[0].deviceName << std::endl;
    if (!mixerEngine->addInputDevice(0, inputDevices[0])) {
        std::cerr << "Failed to add input device!" << std::endl;
        return 1;
    }
    
    // Create a simple OSC output device
    OSCDeviceConfig oscOutput;
    oscOutput.deviceId = "test_osc_output";
    oscOutput.deviceName = "Test OSC Output";
    oscOutput.deviceType = OSCDeviceType::OSC_OUTPUT;
    oscOutput.networkAddress = "127.0.0.1";
    oscOutput.port = 9000;
    oscOutput.oscAddress = "/test/channel/1";
    oscOutput.enabled = true;
    
    std::cout << "Adding OSC output device to channel 0" << std::endl;
    if (!mixerEngine->addOutputDevice(0, oscOutput)) {
        std::cerr << "Failed to add output device!" << std::endl;
        return 1;
    }
    
    // Start channel 0
    std::cout << "\nStarting channel 0..." << std::endl;
    if (!mixerEngine->startChannel(0)) {
        std::cerr << "Failed to start channel!" << std::endl;
        return 1;
    }
    
    std::cout << "\n✅ Audio input test is running!" << std::endl;
    std::cout << "Make some noise into your microphone..." << std::endl;
    std::cout << "OSC messages will be sent to 127.0.0.1:9000" << std::endl;
    std::cout << "\nPress Enter to stop..." << std::endl;
    
    // Run for a while
    std::cin.get();
    
    // Stop and cleanup
    std::cout << "\nStopping channel..." << std::endl;
    mixerEngine->stopChannel(0);
    mixerEngine->shutdown();
    
    std::cout << "Test completed!" << std::endl;
    return 0;
}
