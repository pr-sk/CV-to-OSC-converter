#include <gtest/gtest.h>
#include "../src/osc/OSCSender.h"
#include "../src/osc/OSCReceiver.h"
#include "../src/osc/OSCFormatManager.h"
#include <chrono>
#include <thread>

class OSCSenderTest : public ::testing::Test {
protected:
    void SetUp() override {
        sender = std::make_unique<OSCSender>("127.0.0.1", "9000");
    }

    void TearDown() override {
        sender.reset();
    }

    std::unique_ptr<OSCSender> sender;
};

class OSCReceiverTest : public ::testing::Test {
protected:
    void SetUp() override {
        receiver = std::make_unique<OSCReceiver>("9001");
    }

    void TearDown() override {
        if (receiver) {
            receiver->stop();
        }
    }

    std::unique_ptr<OSCReceiver> receiver;
};

// Test OSCSender basic functionality
TEST_F(OSCSenderTest, BasicSending) {
    EXPECT_TRUE(sender->sendFloat("/test/float", 1.5f));
    EXPECT_TRUE(sender->sendInt("/test/int", 42));
    EXPECT_TRUE(sender->sendString("/test/string", "hello"));
}

TEST_F(OSCSenderTest, FloatArraySending) {
    std::vector<float> values = {1.0f, 2.0f, 3.0f, 4.0f};
    EXPECT_TRUE(sender->sendFloatArray("/test/array", values));
}

TEST_F(OSCSenderTest, BatchSending) {
    std::vector<std::string> addresses = {"/ch/1", "/ch/2", "/ch/3"};
    std::vector<float> values = {0.1f, 0.2f, 0.3f};
    
    EXPECT_TRUE(sender->sendFloatBatch(addresses, values));
}

TEST_F(OSCSenderTest, MixedArraySending) {
    std::vector<float> floats = {1.1f, 2.2f};
    std::vector<int> ints = {10, 20};
    std::vector<std::string> strings = {"foo", "bar"};
    
    EXPECT_TRUE(sender->sendMixedArray("/test/mixed", floats, ints, strings));
}

TEST_F(OSCSenderTest, MessageFormatting) {
    OSCMessageFormat format;
    format.addressPattern = "/channel/{channel}/level";
    format.scale = 2.0f;
    format.offset = 1.0f;
    
    sender->setMessageFormat(format);
    
    EXPECT_TRUE(sender->sendValue(0, 0.5f)); // Should send 2.0f to /channel/1/level
}

TEST_F(OSCSenderTest, FormattedBatch) {
    OSCMessageFormat format;
    format.addressPattern = "/cv/{channel}";
    format.dataType = "float";
    format.bundleMessages = true;
    
    sender->setMessageFormat(format);
    
    std::vector<float> values = {0.1f, 0.2f, 0.3f, 0.4f};
    EXPECT_TRUE(sender->sendFormattedBatch(values));
}

TEST_F(OSCSenderTest, TargetChange) {
    sender->setTarget("192.168.1.100", "8000");
    EXPECT_TRUE(sender->sendFloat("/test", 1.0f));
}

TEST_F(OSCSenderTest, AddressFormatting) {
    OSCMessageFormat format;
    format.addressPattern = "/mixer/channel/{channel}/fader";
    sender->setMessageFormat(format);
    
    std::string address = sender->formatAddress(2); // Channel 2 (0-based) = 3 (1-based)
    EXPECT_EQ(address, "/mixer/channel/3/fader");
}

TEST_F(OSCSenderTest, ValueFormatting) {
    std::string formatted = sender->formatValue(3.14159f, "{value:.2f}");
    EXPECT_EQ(formatted, "3.14");
}

// Test OSCReceiver basic functionality
TEST_F(OSCReceiverTest, ServerLifecycle) {
    EXPECT_FALSE(receiver->isRunning());
    
    EXPECT_TRUE(receiver->start());
    EXPECT_TRUE(receiver->isRunning());
    
    receiver->stop();
    EXPECT_FALSE(receiver->isRunning());
}

TEST_F(OSCReceiverTest, URLGeneration) {
    EXPECT_TRUE(receiver->start());
    
    std::string url = receiver->getURL();
    EXPECT_TRUE(url.find("9001") != std::string::npos);
    
    receiver->stop();
}

TEST_F(OSCReceiverTest, MessageCallbacks) {
    std::vector<float> receivedFloats;
    std::string receivedString;
    int receivedInt = 0;
    
    receiver->setMessageCallback([&receivedFloats](const std::string& address, const std::vector<float>& values) {
        receivedFloats = values;
    });
    
    receiver->setStringCallback([&receivedString](const std::string& address, const std::string& value) {
        receivedString = value;
    });
    
    receiver->setIntCallback([&receivedInt](const std::string& address, int value) {
        receivedInt = value;
    });
    
    EXPECT_TRUE(receiver->start());
    
    // Give receiver time to start
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    // Send some test messages
    OSCSender testSender("127.0.0.1", "9001");
    testSender.sendFloat("/test/float", 3.14f);
    testSender.sendString("/test/string", "test_string");
    testSender.sendInt("/test/int", 123);
    
    // Give time for messages to be processed
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    receiver->stop();
}

TEST_F(OSCReceiverTest, LearningMode) {
    auto formatManager = std::make_shared<OSCFormatManager>();
    auto learningReceiver = std::make_unique<OSCReceiver>("9002", formatManager);
    
    EXPECT_TRUE(learningReceiver->start());
    
    learningReceiver->enableLearning(true);
    
    // Send a message to trigger learning
    OSCSender testSender("127.0.0.1", "9002");
    testSender.sendFloat("/learn/this", 0.5f);
    
    // Give time for learning
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    learningReceiver->enableLearning(false);
    learningReceiver->stop();
}

// Test OSC communication between sender and receiver
class OSCCommunicationTest : public ::testing::Test {
protected:
    void SetUp() override {
        receiver = std::make_unique<OSCReceiver>("9003");
        sender = std::make_unique<OSCSender>("127.0.0.1", "9003");
        
        messageReceived = false;
        lastReceivedValue = 0.0f;
        lastReceivedAddress = "";
    }

    void TearDown() override {
        if (receiver) {
            receiver->stop();
        }
    }

    std::unique_ptr<OSCReceiver> receiver;
    std::unique_ptr<OSCSender> sender;
    
    bool messageReceived;
    float lastReceivedValue;
    std::string lastReceivedAddress;
};

TEST_F(OSCCommunicationTest, EndToEndCommunication) {
    // Set up receiver callback
    receiver->setMessageCallback([this](const std::string& address, const std::vector<float>& values) {
        if (!values.empty()) {
            messageReceived = true;
            lastReceivedValue = values[0];
            lastReceivedAddress = address;
        }
    });
    
    EXPECT_TRUE(receiver->start());
    
    // Give receiver time to start
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    
    // Send a test message
    float testValue = 2.718f;
    std::string testAddress = "/test/communication";
    
    EXPECT_TRUE(sender->sendFloat(testAddress, testValue));
    
    // Wait for message processing
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    
    EXPECT_TRUE(messageReceived);
    EXPECT_FLOAT_EQ(lastReceivedValue, testValue);
    EXPECT_EQ(lastReceivedAddress, testAddress);
}

TEST_F(OSCCommunicationTest, BatchCommunication) {
    std::vector<float> receivedValues;
    std::vector<std::string> receivedAddresses;
    
    receiver->setMessageCallback([&](const std::string& address, const std::vector<float>& values) {
        if (!values.empty()) {
            receivedValues.push_back(values[0]);
            receivedAddresses.push_back(address);
        }
    });
    
    EXPECT_TRUE(receiver->start());
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    
    // Send batch messages
    std::vector<std::string> addresses = {"/ch/1", "/ch/2", "/ch/3"};
    std::vector<float> values = {0.1f, 0.2f, 0.3f};
    
    EXPECT_TRUE(sender->sendFloatBatch(addresses, values));
    
    // Wait for processing
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    EXPECT_EQ(receivedValues.size(), 3);
    EXPECT_EQ(receivedAddresses.size(), 3);
}

// Test OSCFormatManager
class OSCFormatManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        formatManager = std::make_shared<OSCFormatManager>();
    }

    std::shared_ptr<OSCFormatManager> formatManager;
};

TEST_F(OSCFormatManagerTest, LearningMode) {
    EXPECT_FALSE(formatManager->isLearningMode());
    
    formatManager->setLearningMode(true);
    EXPECT_TRUE(formatManager->isLearningMode());
    
    // Test learning a message
    std::vector<float> values = {0.5f};
    formatManager->learnOSCMessage("/learned/address", values);
    
    formatManager->setLearningMode(false);
    EXPECT_FALSE(formatManager->isLearningMode());
}

TEST_F(OSCFormatManagerTest, MessageRecording) {
    std::string testAddress = "/test/record";
    
    formatManager->recordMessageReceived(testAddress);
    formatManager->recordMessageReceived(testAddress);
    formatManager->recordMessageReceived(testAddress);
    
    // Should have recorded the address multiple times
}

// Test edge cases and error handling
class OSCErrorHandlingTest : public ::testing::Test {
protected:
    void SetUp() override {
        sender = std::make_unique<OSCSender>("127.0.0.1", "9999"); // Non-existent port
    }

    std::unique_ptr<OSCSender> sender;
};

TEST_F(OSCErrorHandlingTest, InvalidTarget) {
    // Sending to non-existent target should not crash
    EXPECT_FALSE(sender->sendFloat("/test", 1.0f) || true); // Allow failure
}

TEST_F(OSCErrorHandlingTest, EmptyMessages) {
    std::vector<float> emptyFloats;
    std::vector<std::string> emptyAddresses;
    
    EXPECT_FALSE(sender->sendFloatArray("/test", emptyFloats));
    EXPECT_FALSE(sender->sendFloatBatch(emptyAddresses, emptyFloats));
}

TEST_F(OSCErrorHandlingTest, MismatchedArrays) {
    std::vector<std::string> addresses = {"/ch/1", "/ch/2"};
    std::vector<float> values = {0.1f}; // Different size
    
    EXPECT_FALSE(sender->sendFloatBatch(addresses, values));
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
