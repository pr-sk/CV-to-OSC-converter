#include <gtest/gtest.h>
#include "../src/core/OSCMixerEngine.h"
#include "../src/core/OSCMixerTypes.h"
#include <chrono>
#include <thread>

class OSCMixerEngineTest : public ::testing::Test {
protected:
    void SetUp() override {
        engine = std::make_unique<OSCMixerEngine>();
    }

    void TearDown() override {
        if (engine) {
            engine->shutdown();
        }
    }

    std::unique_ptr<OSCMixerEngine> engine;
};

// Test basic engine lifecycle
TEST_F(OSCMixerEngineTest, EngineLifecycle) {
    EXPECT_FALSE(engine->isRunning());
    
    EXPECT_TRUE(engine->initialize());
    EXPECT_TRUE(engine->isRunning());
    
    engine->shutdown();
    EXPECT_FALSE(engine->isRunning());
}

// Test constructor with custom channel count
TEST_F(OSCMixerEngineTest, CustomChannelCount) {
    auto customEngine = std::make_unique<OSCMixerEngine>(16);
    EXPECT_TRUE(customEngine->initialize());
    
    auto* mixerState = customEngine->getMixerState();
    EXPECT_EQ(mixerState->channels.size(), 16);
    
    customEngine->shutdown();
}

// Test channel operations
TEST_F(OSCMixerEngineTest, ChannelOperations) {
    EXPECT_TRUE(engine->initialize());
    
    // Test starting a channel
    EXPECT_TRUE(engine->startChannel(0));
    
    // Test setting channel level
    EXPECT_TRUE(engine->setChannelLevel(0, 5.0f));
    EXPECT_FLOAT_EQ(engine->getChannelLevel(0), 5.0f);
    
    // Test setting channel mode
    EXPECT_TRUE(engine->setChannelMode(0, ChannelMode::SOLO));
    EXPECT_TRUE(engine->isChannelSolo(0));
    EXPECT_FALSE(engine->isChannelMuted(0));
    
    EXPECT_TRUE(engine->setChannelMode(0, ChannelMode::MUTE));
    EXPECT_FALSE(engine->isChannelSolo(0));
    EXPECT_TRUE(engine->isChannelMuted(0));
    
    // Test setting channel range
    EXPECT_TRUE(engine->setChannelRange(0, -5.0f, 5.0f));
    
    // Test stopping a channel
    EXPECT_TRUE(engine->stopChannel(0));
}

// Test device management
TEST_F(OSCMixerEngineTest, DeviceManagement) {
    EXPECT_TRUE(engine->initialize());
    
    OSCDeviceConfig inputDevice;
    inputDevice.deviceId = "test_input";
    inputDevice.deviceName = "Test Input Device";
    inputDevice.networkAddress = "127.0.0.1";
    inputDevice.port = 9000;
    inputDevice.localPort = 9001;
    inputDevice.oscAddress = "/test/input";
    inputDevice.enabled = true;
    
    OSCDeviceConfig outputDevice;
    outputDevice.deviceId = "test_output";
    outputDevice.deviceName = "Test Output Device";
    outputDevice.networkAddress = "127.0.0.1";
    outputDevice.port = 9002;
    outputDevice.localPort = 9003;
    outputDevice.oscAddress = "/test/output";
    outputDevice.enabled = true;
    
    // Test adding devices
    EXPECT_TRUE(engine->addInputDevice(0, inputDevice));
    EXPECT_TRUE(engine->addOutputDevice(0, outputDevice));
    
    // Test device status
    DeviceStatus status = engine->getDeviceStatus("test_input");
    EXPECT_EQ(status.deviceId, "test_input");
    
    // Test updating device config
    outputDevice.port = 9004;
    EXPECT_TRUE(engine->updateDeviceConfig("test_output", outputDevice));
    
    // Test removing devices
    EXPECT_TRUE(engine->removeInputDevice(0, "test_input"));
    EXPECT_TRUE(engine->removeOutputDevice(0, "test_output"));
}

// Test device discovery
TEST_F(OSCMixerEngineTest, DeviceDiscovery) {
    EXPECT_TRUE(engine->initialize());
    
    engine->startDeviceDiscovery();
    
    // Wait a bit for discovery to find devices
    std::this_thread::sleep_for(std::chrono::seconds(1));
    
    auto devices = engine->getAvailableDevices();
    EXPECT_GT(devices.size(), 0);
    
    engine->stopDeviceDiscovery();
}

// Test learning mode
TEST_F(OSCMixerEngineTest, LearningMode) {
    EXPECT_TRUE(engine->initialize());
    
    EXPECT_FALSE(engine->isLearningModeEnabled());
    
    engine->enableLearningMode(true);
    EXPECT_TRUE(engine->isLearningModeEnabled());
    
    engine->setLearningTarget(0, "level");
    
    engine->enableLearningMode(false);
    EXPECT_FALSE(engine->isLearningModeEnabled());
}

// Test master controls
TEST_F(OSCMixerEngineTest, MasterControls) {
    EXPECT_TRUE(engine->initialize());
    
    // Test master volume
    engine->setMasterVolume(0.8f);
    auto* mixerState = engine->getMixerState();
    EXPECT_FLOAT_EQ(mixerState->masterLevel, 0.8f);
    
    // Test solo mode
    EXPECT_FALSE(engine->isSoloMode());
    engine->setChannelSolo(0, true);
    EXPECT_TRUE(engine->isSoloMode());
    
    engine->setSoloMode(false);
    EXPECT_FALSE(engine->isSoloMode());
    EXPECT_FALSE(engine->isChannelSolo(0));
}

// Test statistics
TEST_F(OSCMixerEngineTest, Statistics) {
    EXPECT_TRUE(engine->initialize());
    
    int initialMessages = engine->getTotalMessagesPerSecond();
    int initialConnections = engine->getTotalActiveConnections();
    int initialErrors = engine->getTotalErrors();
    
    engine->resetStatistics();
    
    EXPECT_EQ(engine->getTotalMessagesPerSecond(), 0);
    EXPECT_EQ(engine->getTotalErrors(), 0);
}

// Test configuration save/load
TEST_F(OSCMixerEngineTest, ConfigurationSaveLoad) {
    EXPECT_TRUE(engine->initialize());
    
    std::string configPath = "/tmp/test_mixer_config.json";
    
    // Configure some settings
    engine->setMasterVolume(0.7f);
    engine->setChannelLevel(0, 3.0f);
    
    // Save configuration
    EXPECT_TRUE(engine->saveConfiguration(configPath));
    
    // Reset settings
    engine->setMasterVolume(1.0f);
    engine->setChannelLevel(0, 0.0f);
    
    // Load configuration
    EXPECT_TRUE(engine->loadConfiguration(configPath));
    
    // Verify settings were restored
    auto* mixerState = engine->getMixerState();
    EXPECT_FLOAT_EQ(mixerState->masterLevel, 0.7f);
    EXPECT_FLOAT_EQ(engine->getChannelLevel(0), 3.0f);
    
    // Clean up
    std::remove(configPath.c_str());
}

// Test OSC message sending
TEST_F(OSCMixerEngineTest, OSCMessageSending) {
    EXPECT_TRUE(engine->initialize());
    
    // Test sending simple float message
    engine->sendOSCMessage(0, "test_device", 0.5f);
    
    // Test sending complex OSC message
    OSCMessage message;
    message.address = "/test/address";
    message.floatValues = {0.1f, 0.2f, 0.3f};
    message.type = OSCMessageType::FLOAT;
    message.sourceChannelId = 0;
    message.deviceId = "test_device";
    message.timestamp = std::chrono::steady_clock::now();
    
    engine->sendOSCMessage(0, "test_device", message);
}

// Test invalid inputs
TEST_F(OSCMixerEngineTest, InvalidInputHandling) {
    EXPECT_TRUE(engine->initialize());
    
    // Test invalid channel IDs
    EXPECT_FALSE(engine->startChannel(-1));
    EXPECT_FALSE(engine->startChannel(100));
    EXPECT_FALSE(engine->setChannelLevel(-1, 1.0f));
    EXPECT_FALSE(engine->setChannelLevel(100, 1.0f));
    
    // Test invalid device config
    OSCDeviceConfig invalidDevice;
    invalidDevice.deviceId = ""; // Empty ID is invalid
    EXPECT_FALSE(engine->addInputDevice(0, invalidDevice));
    
    // Test invalid range
    EXPECT_FALSE(engine->setChannelRange(0, 10.0f, 5.0f)); // min > max
}

// Test concurrent operations
TEST_F(OSCMixerEngineTest, ConcurrentOperations) {
    EXPECT_TRUE(engine->initialize());
    
    // Start multiple channels concurrently
    std::vector<std::thread> threads;
    for (int i = 0; i < 4; ++i) {
        threads.emplace_back([this, i]() {
            engine->startChannel(i);
            engine->setChannelLevel(i, static_cast<float>(i));
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            engine->stopChannel(i);
        });
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
}

// Test performance monitoring
TEST_F(OSCMixerEngineTest, PerformanceMonitoring) {
    EXPECT_TRUE(engine->initialize());
    
    // Let the engine run for a short time to collect performance data
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    // Performance metrics should be available
    int messagesPerSecond = engine->getTotalMessagesPerSecond();
    int activeConnections = engine->getTotalActiveConnections();
    int totalErrors = engine->getTotalErrors();
    
    EXPECT_GE(messagesPerSecond, 0);
    EXPECT_GE(activeConnections, 0);
    EXPECT_GE(totalErrors, 0);
}

// Test multiple engine instances
TEST_F(OSCMixerEngineTest, MultipleInstances) {
    auto engine1 = std::make_unique<OSCMixerEngine>(4);
    auto engine2 = std::make_unique<OSCMixerEngine>(8);
    
    EXPECT_TRUE(engine1->initialize());
    EXPECT_TRUE(engine2->initialize());
    
    EXPECT_TRUE(engine1->isRunning());
    EXPECT_TRUE(engine2->isRunning());
    
    engine1->shutdown();
    engine2->shutdown();
    
    EXPECT_FALSE(engine1->isRunning());
    EXPECT_FALSE(engine2->isRunning());
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
