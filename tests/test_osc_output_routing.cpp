#include <iostream>
#include <thread>
#include <chrono>
#include <vector>
#include <memory>
#include <cmath>
#include <atomic>
#include <sstream>
#include <iomanip>
#include <mutex>
#include <condition_variable>
#include <unordered_map>
#include "../src/core/OSCMixerEngine.h"
#include "../src/network/OSCReceiver.h"
#include "../src/audio/AudioProcessor.h"

// Test OSC receiver to capture messages sent by the mixer
class TestOSCReceiver {
private:
    std::unique_ptr<OSCReceiver> receiver;
    std::vector<std::pair<std::string, float>> receivedMessages;
    std::mutex messageMutex;
    std::condition_variable messageCV;
    std::atomic<bool> messageReceived{false};
    std::string deviceName;
    int port;

public:
    TestOSCReceiver(const std::string& name, int port) : deviceName(name), port(port) {
        receiver = std::make_unique<OSCReceiver>(std::to_string(port));
        
        receiver->setMessageCallback([this](const std::string& address, const std::vector<float>& values) {
            if (!values.empty()) {
                std::lock_guard<std::mutex> lock(messageMutex);
                receivedMessages.push_back({address, values[0]});
                messageReceived = true;
                messageCV.notify_all();
                
                std::cout << "🎯 " << deviceName << " received: " << address 
                          << " = " << values[0] << std::endl;
            }
        });
    }
    
    bool start() {
        return receiver->start();
    }
    
    void stop() {
        receiver->stop();
    }
    
    bool waitForMessage(int timeoutMs = 5000) {
        std::unique_lock<std::mutex> lock(messageMutex);
        return messageCV.wait_for(lock, std::chrono::milliseconds(timeoutMs), 
                                  [this] { return messageReceived.load(); });
    }
    
    std::vector<std::pair<std::string, float>> getMessages() {
        std::lock_guard<std::mutex> lock(messageMutex);
        return receivedMessages;
    }
    
    void clearMessages() {
        std::lock_guard<std::mutex> lock(messageMutex);
        receivedMessages.clear();
        messageReceived = false;
    }
    
    size_t getMessageCount() {
        std::lock_guard<std::mutex> lock(messageMutex);
        return receivedMessages.size();
    }
};

// Test configuration for different OSC devices
struct TestDevice {
    std::string name;
    std::string address;
    int port;
    std::string oscPath;
};

void testOSCOutputRouting() {
    std::cout << "\n=== Testing OSC Output Routing ===\n" << std::endl;
    
    // Define test devices matching the GUI options
    std::vector<TestDevice> testDevices = {
        {"Ableton Live", "127.0.0.1", 9001, "/channel/1"},
        {"TouchOSC", "127.0.0.1", 8000, "/channel/1"},
        {"Max/MSP", "127.0.0.1", 7000, "/channel/1"},
        {"TouchDesigner", "127.0.0.1", 9000, "/channel/1"},
        {"VCV Rack", "127.0.0.1", 8001, "/channel/1"}
    };
    
    // Create test receivers for each device
    std::unordered_map<std::string, std::unique_ptr<TestOSCReceiver>> receivers;
    
    for (const auto& device : testDevices) {
        auto receiver = std::make_unique<TestOSCReceiver>(device.name, device.port);
        if (!receiver->start()) {
            std::cerr << "❌ Failed to start receiver for " << device.name 
                      << " on port " << device.port << std::endl;
            continue;
        }
        receivers[device.name] = std::move(receiver);
        std::cout << "✅ Started test receiver for " << device.name 
                  << " on port " << device.port << std::endl;
    }
    
    // Create mixer engine
    auto mixerEngine = std::make_shared<OSCMixerEngine>();
    mixerEngine->initialize();
    
    // Test each device
    for (const auto& device : testDevices) {
        std::cout << "\n📡 Testing output to " << device.name << "..." << std::endl;
        
        // Configure output device
        OSCDeviceConfig outputConfig;
        outputConfig.deviceId = "test_" + device.name;
        outputConfig.deviceName = device.name;
        outputConfig.deviceType = OSCDeviceType::OSC_OUTPUT;
        outputConfig.networkAddress = device.address;
        outputConfig.port = device.port;
        outputConfig.oscAddress = device.oscPath;
        outputConfig.enabled = true;
        outputConfig.protocolType = OSCProtocolType::UDP_UNICAST;
        
        // Add to channel 0
        if (!mixerEngine->addOutputDevice(0, outputConfig)) {
            std::cerr << "❌ Failed to add output device: " << device.name << std::endl;
            continue;
        }
        
        // Configure input device (simulated audio input)
        OSCDeviceConfig inputConfig;
        inputConfig.deviceId = "test_input_" + device.name;
        inputConfig.deviceName = "Test Input";
        inputConfig.deviceType = OSCDeviceType::OSC_INPUT;
        inputConfig.networkAddress = "127.0.0.1";
        inputConfig.port = 9100; // Different port for input
        inputConfig.localPort = 9100;
        inputConfig.oscAddress = "/channel/1";
        inputConfig.enabled = true;
        
        mixerEngine->addInputDevice(0, inputConfig);
        
        // Start the channel
        mixerEngine->startChannel(0);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        // Clear any previous messages
        if (receivers.find(device.name) != receivers.end()) {
            receivers[device.name]->clearMessages();
        }
        
        // Send test signal
        OSCMessage testMessage;
        testMessage.address = "/channel/1";
        testMessage.floatValues = {0.75f}; // Test CV value
        testMessage.type = OSCMessageType::FLOAT;
        testMessage.deviceId = inputConfig.deviceId;
        testMessage.timestamp = std::chrono::steady_clock::now();
        
        // Inject the message directly
        mixerEngine->sendOSCMessage(0, inputConfig.deviceId, testMessage);
        
        // Wait for the message to be routed
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        
        // Check if the correct device received the message
        bool messageReceived = false;
        if (receivers.find(device.name) != receivers.end()) {
            auto& receiver = receivers[device.name];
            if (receiver->waitForMessage(2000)) {
                auto messages = receiver->getMessages();
                if (!messages.empty()) {
                    messageReceived = true;
                    std::cout << "✅ " << device.name << " received " 
                              << messages.size() << " message(s)" << std::endl;
                    for (const auto& [address, value] : messages) {
                        std::cout << "   📨 " << address << " = " << value << std::endl;
                    }
                }
            }
        }
        
        if (!messageReceived) {
            std::cerr << "❌ " << device.name << " did not receive any messages!" << std::endl;
        }
        
        // Clean up
        mixerEngine->stopChannel(0);
        mixerEngine->removeOutputDevice(0, outputConfig.deviceId);
        mixerEngine->removeInputDevice(0, inputConfig.deviceId);
        
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    // Stop all receivers
    for (auto& [name, receiver] : receivers) {
        receiver->stop();
    }
    
    mixerEngine->stop();
}

void testAudioToOSCRouting() {
    std::cout << "\n=== Testing Audio Input to OSC Output Routing ===\n" << std::endl;
    
    // Create test receiver for Ableton
    auto abletonReceiver = std::make_unique<TestOSCReceiver>("Ableton Live", 9001);
    if (!abletonReceiver->start()) {
        std::cerr << "❌ Failed to start Ableton receiver" << std::endl;
        return;
    }
    
    // Create mixer engine
    auto mixerEngine = std::make_shared<OSCMixerEngine>();
    mixerEngine->initialize();
    mixerEngine->start();
    
    // Add Ableton as output device
    OSCDeviceConfig abletonConfig;
    abletonConfig.deviceId = "ableton_osc_out";
    abletonConfig.deviceName = "Ableton Live";
    abletonConfig.deviceType = OSCDeviceType::OSC_OUTPUT;
    abletonConfig.networkAddress = "127.0.0.1";
    abletonConfig.port = 9001;
    abletonConfig.oscAddress = "/live/track/1/volume";
    abletonConfig.enabled = true;
    
    if (!mixerEngine->addOutputDevice(0, abletonConfig)) {
        std::cerr << "❌ Failed to add Ableton output device" << std::endl;
        return;
    }
    
    // Simulate audio input by sending CV values
    std::cout << "📊 Simulating audio input..." << std::endl;
    
    for (int i = 0; i < 10; i++) {
        // Simulate varying audio levels
        float audioLevel = 0.5f + 0.4f * sin(i * 0.628f); // Sine wave 0.1 to 0.9
        
        // This would normally come from real audio input
        // For testing, we'll inject it directly
        OSCMessage audioMessage;
        audioMessage.address = "/channel/1";
        audioMessage.floatValues = {audioLevel};
        audioMessage.type = OSCMessageType::FLOAT;
        audioMessage.sourceChannelId = 0;
        audioMessage.timestamp = std::chrono::steady_clock::now();
        
        // Process through mixer (simulating audio input processing)
        mixerEngine->sendOSCMessage(0, abletonConfig.deviceId, audioLevel);
        
        std::cout << "📤 Sent CV value: " << audioLevel << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    // Check messages received by Ableton
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    auto messages = abletonReceiver->getMessages();
    std::cout << "\n📊 Ableton received " << messages.size() << " messages:" << std::endl;
    
    if (messages.empty()) {
        std::cerr << "❌ No messages received by Ableton!" << std::endl;
    } else {
        for (size_t i = 0; i < std::min(messages.size(), size_t(5)); i++) {
            std::cout << "   " << messages[i].first << " = " 
                      << messages[i].second << std::endl;
        }
        if (messages.size() > 5) {
            std::cout << "   ... and " << (messages.size() - 5) << " more" << std::endl;
        }
    }
    
    abletonReceiver->stop();
    mixerEngine->stop();
}

void testMultipleOutputDevices() {
    std::cout << "\n=== Testing Multiple Output Devices ===\n" << std::endl;
    
    // Create receivers
    auto abletonReceiver = std::make_unique<TestOSCReceiver>("Ableton", 9001);
    auto touchOSCReceiver = std::make_unique<TestOSCReceiver>("TouchOSC", 8000);
    
    if (!abletonReceiver->start() || !touchOSCReceiver->start()) {
        std::cerr << "❌ Failed to start receivers" << std::endl;
        return;
    }
    
    // Create mixer
    auto mixerEngine = std::make_shared<OSCMixerEngine>();
    mixerEngine->initialize();
    mixerEngine->start();
    
    // Add both as output devices to same channel
    OSCDeviceConfig abletonConfig;
    abletonConfig.deviceId = "ableton_out";
    abletonConfig.deviceName = "Ableton Live";
    abletonConfig.deviceType = OSCDeviceType::OSC_OUTPUT;
    abletonConfig.networkAddress = "127.0.0.1";
    abletonConfig.port = 9001;
    abletonConfig.oscAddress = "/live/track/1/volume";
    abletonConfig.enabled = true;
    
    OSCDeviceConfig touchConfig;
    touchConfig.deviceId = "touch_out";
    touchConfig.deviceName = "TouchOSC";
    touchConfig.deviceType = OSCDeviceType::OSC_OUTPUT;
    touchConfig.networkAddress = "127.0.0.1";
    touchConfig.port = 8000;
    touchConfig.oscAddress = "/1/fader1";
    touchConfig.enabled = true;
    
    mixerEngine->addOutputDevice(0, abletonConfig);
    mixerEngine->addOutputDevice(0, touchConfig);
    
    // Send test signal
    std::cout << "📤 Sending to multiple outputs..." << std::endl;
    
    for (int i = 0; i < 5; i++) {
        float value = 0.2f * i;
        mixerEngine->sendOSCMessage(0, abletonConfig.deviceId, value);
        mixerEngine->sendOSCMessage(0, touchConfig.deviceId, value);
        std::cout << "   Sent: " << value << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }
    
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    // Check results
    auto abletonMessages = abletonReceiver->getMessages();
    auto touchMessages = touchOSCReceiver->getMessages();
    
    std::cout << "\n📊 Results:" << std::endl;
    std::cout << "   Ableton received: " << abletonMessages.size() << " messages" << std::endl;
    std::cout << "   TouchOSC received: " << touchMessages.size() << " messages" << std::endl;
    
    if (abletonMessages.empty() || touchMessages.empty()) {
        std::cerr << "❌ Not all devices received messages!" << std::endl;
    } else {
        std::cout << "✅ All devices received messages successfully" << std::endl;
    }
    
    abletonReceiver->stop();
    touchOSCReceiver->stop();
    mixerEngine->stop();
}

int main() {
    std::cout << "🚀 OSC Output Routing Test Suite" << std::endl;
    std::cout << "================================\n" << std::endl;
    
    try {
        // Test 1: Basic routing to different devices
        testOSCOutputRouting();
        
        // Test 2: Audio input to OSC output
        testAudioToOSCRouting();
        
        // Test 3: Multiple output devices
        testMultipleOutputDevices();
        
        std::cout << "\n✅ All tests completed!" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "\n❌ Test failed with exception: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
