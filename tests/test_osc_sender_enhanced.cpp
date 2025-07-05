#include <gtest/gtest.h>
#include "../src/osc/OSCSenderEnhanced.h"
#include "../src/osc/OSCReceiver.h"
#include <thread>
#include <chrono>
#include <atomic>

class OSCSenderEnhancedTest : public ::testing::Test {
protected:
    void SetUp() override {
        sender = std::make_unique<OSCSenderEnhanced>();
        receiver = std::make_unique<OSCReceiver>();
        
        messagesReceived = 0;
        lastFloatReceived = 0.0f;
        lastIntReceived = 0;
        lastStringReceived.clear();
        lastFloatArrayReceived.clear();
    }
    
    void TearDown() override {
        if (receiver) {
            receiver->stop();
        }
    }
    
    // Helper to setup receiver
    void setupReceiver(const std::string& port, OSCReceiver::Protocol protocol = OSCReceiver::Protocol::UDP) {
        receiver->setFloatHandler([this](const std::string& address, float value) {
            messagesReceived++;
            lastFloatReceived = value;
            receivedAddress = address;
        });
        
        receiver->setIntHandler([this](const std::string& address, int value) {
            messagesReceived++;
            lastIntReceived = value;
            receivedAddress = address;
        });
        
        receiver->setStringHandler([this](const std::string& address, const std::string& value) {
            messagesReceived++;
            lastStringReceived = value;
            receivedAddress = address;
        });
        
        receiver->setFloatArrayHandler([this](const std::string& address, const std::vector<float>& values) {
            messagesReceived++;
            lastFloatArrayReceived = values;
            receivedAddress = address;
        });
        
        receiver->start(port, protocol);
        std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Allow server to start
    }
    
    std::unique_ptr<OSCSenderEnhanced> sender;
    std::unique_ptr<OSCReceiver> receiver;
    
    std::atomic<int> messagesReceived{0};
    float lastFloatReceived;
    int lastIntReceived;
    std::string lastStringReceived;
    std::vector<float> lastFloatArrayReceived;
    std::string receivedAddress;
};

// Basic connection tests
TEST_F(OSCSenderEnhancedTest, ConnectDisconnectUDP) {
    EXPECT_FALSE(sender->isConnected());
    
    EXPECT_TRUE(sender->connect("localhost", "9001", OSCTransport::Protocol::UDP));
    EXPECT_TRUE(sender->isConnected());
    EXPECT_EQ(sender->getProtocol(), OSCTransport::Protocol::UDP);
    EXPECT_EQ(sender->getProtocolName(), "UDP");
    
    EXPECT_TRUE(sender->disconnect());
    EXPECT_FALSE(sender->isConnected());
}

TEST_F(OSCSenderEnhancedTest, ConnectDisconnectTCP) {
    EXPECT_FALSE(sender->isConnected());
    
    // Setup TCP receiver first
    setupReceiver("9002", OSCReceiver::Protocol::TCP);
    
    EXPECT_TRUE(sender->connect("localhost", "9002", OSCTransport::Protocol::TCP));
    EXPECT_TRUE(sender->isConnected());
    EXPECT_EQ(sender->getProtocol(), OSCTransport::Protocol::TCP);
    EXPECT_EQ(sender->getProtocolName(), "TCP");
    
    EXPECT_TRUE(sender->disconnect());
    EXPECT_FALSE(sender->isConnected());
}

// Protocol switching test
TEST_F(OSCSenderEnhancedTest, ProtocolSwitching) {
    // Start with UDP
    EXPECT_TRUE(sender->connect("localhost", "9003", OSCTransport::Protocol::UDP));
    EXPECT_EQ(sender->getProtocol(), OSCTransport::Protocol::UDP);
    
    // Switch to TCP (should reconnect)
    sender->setProtocol(OSCTransport::Protocol::TCP);
    EXPECT_EQ(sender->getProtocol(), OSCTransport::Protocol::TCP);
    
    // Switch back to UDP
    sender->setProtocol(OSCTransport::Protocol::UDP);
    EXPECT_EQ(sender->getProtocol(), OSCTransport::Protocol::UDP);
}

// Message sending tests - UDP
TEST_F(OSCSenderEnhancedTest, SendFloatUDP) {
    setupReceiver("9010");
    
    ASSERT_TRUE(sender->connect("localhost", "9010", OSCTransport::Protocol::UDP));
    
    EXPECT_TRUE(sender->sendFloat("/test/float", 3.14f));
    
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    EXPECT_EQ(messagesReceived, 1);
    EXPECT_FLOAT_EQ(lastFloatReceived, 3.14f);
    EXPECT_EQ(receivedAddress, "/test/float");
}

TEST_F(OSCSenderEnhancedTest, SendIntUDP) {
    setupReceiver("9011");
    
    ASSERT_TRUE(sender->connect("localhost", "9011", OSCTransport::Protocol::UDP));
    
    EXPECT_TRUE(sender->sendInt("/test/int", 42));
    
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    EXPECT_EQ(messagesReceived, 1);
    EXPECT_EQ(lastIntReceived, 42);
    EXPECT_EQ(receivedAddress, "/test/int");
}

TEST_F(OSCSenderEnhancedTest, SendStringUDP) {
    setupReceiver("9012");
    
    ASSERT_TRUE(sender->connect("localhost", "9012", OSCTransport::Protocol::UDP));
    
    EXPECT_TRUE(sender->sendString("/test/string", "Hello OSC"));
    
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    EXPECT_EQ(messagesReceived, 1);
    EXPECT_EQ(lastStringReceived, "Hello OSC");
    EXPECT_EQ(receivedAddress, "/test/string");
}

TEST_F(OSCSenderEnhancedTest, SendFloatArrayUDP) {
    setupReceiver("9013");
    
    ASSERT_TRUE(sender->connect("localhost", "9013", OSCTransport::Protocol::UDP));
    
    std::vector<float> testArray = {1.0f, 2.0f, 3.0f, 4.0f, 5.0f};
    EXPECT_TRUE(sender->sendFloatArray("/test/array", testArray));
    
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    EXPECT_EQ(messagesReceived, 1);
    EXPECT_EQ(lastFloatArrayReceived.size(), 5);
    for (size_t i = 0; i < testArray.size(); ++i) {
        EXPECT_FLOAT_EQ(lastFloatArrayReceived[i], testArray[i]);
    }
    EXPECT_EQ(receivedAddress, "/test/array");
}

// Message sending tests - TCP
TEST_F(OSCSenderEnhancedTest, SendFloatTCP) {
    setupReceiver("9020", OSCReceiver::Protocol::TCP);
    
    ASSERT_TRUE(sender->connect("localhost", "9020", OSCTransport::Protocol::TCP));
    std::this_thread::sleep_for(std::chrono::milliseconds(100)); // TCP connection time
    
    EXPECT_TRUE(sender->sendFloat("/test/float", 2.71f));
    
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    EXPECT_EQ(messagesReceived, 1);
    EXPECT_FLOAT_EQ(lastFloatReceived, 2.71f);
    EXPECT_EQ(receivedAddress, "/test/float");
}

// Batch sending test
TEST_F(OSCSenderEnhancedTest, SendFloatBatch) {
    setupReceiver("9030");
    
    ASSERT_TRUE(sender->connect("localhost", "9030", OSCTransport::Protocol::UDP));
    
    std::vector<std::string> addresses = {
        "/channel/1", "/channel/2", "/channel/3", "/channel/4"
    };
    std::vector<float> values = {0.1f, 0.2f, 0.3f, 0.4f};
    
    EXPECT_TRUE(sender->sendFloatBatch(addresses, values));
    
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    
    // Should receive all messages in the bundle
    EXPECT_GE(messagesReceived, 4);
}

// Error handling tests
TEST_F(OSCSenderEnhancedTest, SendWithoutConnection) {
    EXPECT_FALSE(sender->sendFloat("/test", 1.0f));
    EXPECT_FALSE(sender->sendInt("/test", 1));
    EXPECT_FALSE(sender->sendString("/test", "test"));
}

TEST_F(OSCSenderEnhancedTest, BatchSendMismatchedSizes) {
    ASSERT_TRUE(sender->connect("localhost", "9040", OSCTransport::Protocol::UDP));
    
    std::vector<std::string> addresses = {"/a", "/b", "/c"};
    std::vector<float> values = {1.0f, 2.0f}; // Mismatched size
    
    EXPECT_FALSE(sender->sendFloatBatch(addresses, values));
}

// Error callback test
TEST_F(OSCSenderEnhancedTest, ErrorCallback) {
    std::string lastError;
    sender->setErrorCallback([&lastError](const std::string& error) {
        lastError = error;
    });
    
    // Try to send without connection
    sender->sendFloat("/test", 1.0f);
    
    EXPECT_FALSE(lastError.empty());
    EXPECT_NE(lastError.find("Not connected"), std::string::npos);
}

// Statistics test
TEST_F(OSCSenderEnhancedTest, Statistics) {
    setupReceiver("9050");
    
    sender->resetStatistics();
    auto stats = sender->getStatistics();
    EXPECT_EQ(stats.messagesSent, 0);
    EXPECT_EQ(stats.errors, 0);
    
    ASSERT_TRUE(sender->connect("localhost", "9050", OSCTransport::Protocol::UDP));
    
    // Send some messages
    sender->sendFloat("/test", 1.0f);
    sender->sendInt("/test", 2);
    sender->sendString("/test", "three");
    
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    stats = sender->getStatistics();
    EXPECT_EQ(stats.messagesSent, 3);
    EXPECT_EQ(stats.errors, 0);
    EXPECT_GT(stats.bytesSent, 0);
    
    // Force an error
    sender->disconnect();
    sender->sendFloat("/test", 4.0f);
    
    stats = sender->getStatistics();
    EXPECT_EQ(stats.messagesSent, 3);
    EXPECT_EQ(stats.errors, 1);
}

// TCP-specific options test
TEST_F(OSCSenderEnhancedTest, TCPOptions) {
    setupReceiver("9060", OSCReceiver::Protocol::TCP);
    
    ASSERT_TRUE(sender->connect("localhost", "9060", OSCTransport::Protocol::TCP));
    
    // These should not crash even though they only apply to TCP
    sender->setAutoReconnect(true);
    sender->setReconnectDelay(5);
    sender->setConnectionTimeout(10);
    
    // Should still be able to send
    EXPECT_TRUE(sender->sendFloat("/test", 1.0f));
}

// Performance test
TEST_F(OSCSenderEnhancedTest, PerformanceTest) {
    setupReceiver("9070");
    
    ASSERT_TRUE(sender->connect("localhost", "9070", OSCTransport::Protocol::UDP));
    
    auto start = std::chrono::high_resolution_clock::now();
    
    const int numMessages = 1000;
    for (int i = 0; i < numMessages; ++i) {
        sender->sendFloat("/perf/test", static_cast<float>(i));
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    // Should be reasonably fast (less than 1 second for 1000 messages)
    EXPECT_LT(duration.count(), 1000);
    
    auto stats = sender->getStatistics();
    EXPECT_EQ(stats.messagesSent, numMessages);
}

// Multiple protocol test
TEST_F(OSCSenderEnhancedTest, MultipleProtocolsSequential) {
    // Test UDP first
    setupReceiver("9080");
    
    ASSERT_TRUE(sender->connect("localhost", "9080", OSCTransport::Protocol::UDP));
    EXPECT_TRUE(sender->sendFloat("/udp/test", 1.0f));
    
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    EXPECT_EQ(messagesReceived, 1);
    
    sender->disconnect();
    receiver->stop();
    
    // Reset and test TCP
    messagesReceived = 0;
    setupReceiver("9081", OSCReceiver::Protocol::TCP);
    
    ASSERT_TRUE(sender->connect("localhost", "9081", OSCTransport::Protocol::TCP));
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    EXPECT_TRUE(sender->sendFloat("/tcp/test", 2.0f));
    
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    EXPECT_EQ(messagesReceived, 1);
}
