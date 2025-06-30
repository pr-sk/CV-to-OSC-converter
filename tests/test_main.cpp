#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <fstream>
#include <filesystem>
#include <thread>
#include <chrono>

// Include our headers
#include "../Config.h"
#include "../OSCSender.h"
#include "../CVReader.h"

class CVToOSCTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create test directory
        testDir = "test_output";
        std::filesystem::create_directories(testDir);
        
        // Create test config file path
        testConfigPath = testDir + "/test_config.json";
    }
    
    void TearDown() override {
        // Clean up test files
        std::filesystem::remove_all(testDir);
    }
    
    std::string testDir;
    std::string testConfigPath;
};

// Test Config class functionality
TEST_F(CVToOSCTest, ConfigDefaultValues) {
    Config config;
    
    EXPECT_EQ(config.getOSCHost(), "127.0.0.1");
    EXPECT_EQ(config.getOSCPort(), "9000");
    EXPECT_EQ(config.getAudioDevice(), "");
    EXPECT_EQ(config.getUpdateIntervalMs(), 10);
    
    // Test default CV range
    CVRange range = config.getCVRange(0);
    EXPECT_FLOAT_EQ(range.min, 0.0f);
    EXPECT_FLOAT_EQ(range.max, 10.0f);
}

TEST_F(CVToOSCTest, ConfigSaveAndLoad) {
    Config config;
    
    // Modify some values
    config.setOSCHost("192.168.1.100");
    config.setOSCPort("8000");
    config.setUpdateIntervalMs(20);
    config.setCVRange(0, -5.0f, 5.0f);
    
    // Save to file
    ASSERT_TRUE(config.saveToFile(testConfigPath));
    
    // Create new config and load
    Config loadedConfig;
    ASSERT_TRUE(loadedConfig.loadFromFile(testConfigPath));
    
    // Verify values
    EXPECT_EQ(loadedConfig.getOSCHost(), "192.168.1.100");
    EXPECT_EQ(loadedConfig.getOSCPort(), "8000");
    EXPECT_EQ(loadedConfig.getUpdateIntervalMs(), 20);
    
    CVRange range = loadedConfig.getCVRange(0);
    EXPECT_FLOAT_EQ(range.min, -5.0f);
    EXPECT_FLOAT_EQ(range.max, 5.0f);
}

TEST_F(CVToOSCTest, ConfigInvalidFile) {
    Config config;
    
    // Try to load non-existent file
    EXPECT_TRUE(config.loadFromFile("non_existent_file.json"));
    
    // Should still have default values
    EXPECT_EQ(config.getOSCHost(), "127.0.0.1");
    EXPECT_EQ(config.getOSCPort(), "9000");
}

TEST_F(CVToOSCTest, ConfigCVRangeEdgeCases) {
    Config config;
    
    // Test negative channel index
    config.setCVRange(-1, 1.0f, 2.0f);
    CVRange range = config.getCVRange(-1);
    EXPECT_FLOAT_EQ(range.min, 0.0f);  // Should return default
    EXPECT_FLOAT_EQ(range.max, 10.0f);
    
    // Test large channel index
    config.setCVRange(100, 3.0f, 7.0f);
    range = config.getCVRange(100);
    EXPECT_FLOAT_EQ(range.min, 3.0f);
    EXPECT_FLOAT_EQ(range.max, 7.0f);
}

// Test OSCSender class (mock tests since we don't want to send actual OSC)
class MockOSCSender : public OSCSender {
public:
    MockOSCSender() : OSCSender("127.0.0.1", "9000") {}
    
    // Override methods for testing
    bool testSendFloat(const std::string& address, float value) {
        lastAddress = address;
        lastValue = value;
        return true;
    }
    
    bool testSendFloatBatch(const std::vector<std::string>& addresses, 
                           const std::vector<float>& values) {
        if (addresses.size() != values.size()) return false;
        batchAddresses = addresses;
        batchValues = values;
        return true;
    }
    
    std::string lastAddress;
    float lastValue = 0.0f;
    std::vector<std::string> batchAddresses;
    std::vector<float> batchValues;
};

TEST_F(CVToOSCTest, OSCSenderBasicFunctionality) {
    // Note: This test doesn't actually send OSC messages to avoid network dependencies
    // In a real scenario, you might want to use a mock OSC receiver
    
    MockOSCSender sender;
    
    // Test single message
    EXPECT_TRUE(sender.testSendFloat("/cv/channel/1", 0.5f));
    EXPECT_EQ(sender.lastAddress, "/cv/channel/1");
    EXPECT_FLOAT_EQ(sender.lastValue, 0.5f);
    
    // Test batch messages
    std::vector<std::string> addresses = {"/cv/channel/1", "/cv/channel/2"};
    std::vector<float> values = {0.3f, 0.7f};
    
    EXPECT_TRUE(sender.testSendFloatBatch(addresses, values));
    EXPECT_EQ(sender.batchAddresses.size(), 2);
    EXPECT_EQ(sender.batchValues.size(), 2);
    EXPECT_FLOAT_EQ(sender.batchValues[0], 0.3f);
    EXPECT_FLOAT_EQ(sender.batchValues[1], 0.7f);
}

TEST_F(CVToOSCTest, OSCSenderInvalidInput) {
    MockOSCSender sender;
    
    // Test mismatched batch sizes
    std::vector<std::string> addresses = {"/cv/channel/1", "/cv/channel/2"};
    std::vector<float> values = {0.3f};  // Size mismatch
    
    EXPECT_FALSE(sender.testSendFloatBatch(addresses, values));
}

// Test CV value normalization logic
TEST_F(CVToOSCTest, CVNormalization) {
    Config config;
    
    // Test standard 0-10V range
    config.setCVRange(0, 0.0f, 10.0f);
    CVRange range = config.getCVRange(0);
    
    // Test normalization function logic
    auto normalize = [&range](float cvValue) -> float {
        if (range.min >= range.max) return 0.0f;
        
        // Clamp to range
        cvValue = std::max(range.min, std::min(range.max, cvValue));
        
        // Normalize to 0-1
        return (cvValue - range.min) / (range.max - range.min);
    };
    
    EXPECT_FLOAT_EQ(normalize(0.0f), 0.0f);    // Min value
    EXPECT_FLOAT_EQ(normalize(5.0f), 0.5f);    // Mid value
    EXPECT_FLOAT_EQ(normalize(10.0f), 1.0f);   // Max value
    EXPECT_FLOAT_EQ(normalize(-1.0f), 0.0f);   // Below range (clamped)
    EXPECT_FLOAT_EQ(normalize(11.0f), 1.0f);   // Above range (clamped)
    
    // Test bipolar range
    config.setCVRange(1, -5.0f, 5.0f);
    range = config.getCVRange(1);
    
    EXPECT_FLOAT_EQ(normalize(-5.0f), 0.0f);   // Min value
    EXPECT_FLOAT_EQ(normalize(0.0f), 0.5f);    // Center value
    EXPECT_FLOAT_EQ(normalize(5.0f), 1.0f);    // Max value
}

// Integration test for JSON parsing
TEST_F(CVToOSCTest, JSONConfigIntegration) {
    // Create a test JSON file with various configurations
    std::string jsonContent = R"({
        "osc_host": "192.168.1.50",
        "osc_port": "7000",
        "audio_device": "Test Device",
        "update_interval_ms": 50,
        "cv_ranges": [
            {
                "min": -10.0,
                "max": 10.0
            },
            {
                "min": 0.0,
                "max": 5.0
            }
        ]
    })";
    
    // Write to test file
    std::ofstream file(testConfigPath);
    file << jsonContent;
    file.close();
    
    // Load and verify
    Config config;
    ASSERT_TRUE(config.loadFromFile(testConfigPath));
    
    EXPECT_EQ(config.getOSCHost(), "192.168.1.50");
    EXPECT_EQ(config.getOSCPort(), "7000");
    EXPECT_EQ(config.getAudioDevice(), "Test Device");
    EXPECT_EQ(config.getUpdateIntervalMs(), 50);
    
    CVRange range0 = config.getCVRange(0);
    EXPECT_FLOAT_EQ(range0.min, -10.0f);
    EXPECT_FLOAT_EQ(range0.max, 10.0f);
    
    CVRange range1 = config.getCVRange(1);
    EXPECT_FLOAT_EQ(range1.min, 0.0f);
    EXPECT_FLOAT_EQ(range1.max, 5.0f);
}

// Performance test for configuration operations
TEST_F(CVToOSCTest, ConfigPerformance) {
    Config config;
    
    auto start = std::chrono::high_resolution_clock::now();
    
    // Perform many operations
    for (int i = 0; i < 1000; ++i) {
        config.setCVRange(i % 8, -5.0f, 5.0f);
        CVRange range = config.getCVRange(i % 8);
        (void)range; // Suppress unused variable warning
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    // Should complete in reasonable time (less than 10ms)
    EXPECT_LT(duration.count(), 10000);
}

// Test error handling for malformed JSON
TEST_F(CVToOSCTest, MalformedJSONHandling) {
    // Create malformed JSON
    std::string malformedJson = R"({
        "osc_host": "192.168.1.50",
        "osc_port": 7000,  // Missing quotes - this is invalid JSON
        "cv_ranges": [
            {
                "min": not_a_number,
                "max": 10.0
            }
        ]
    })";
    
    std::ofstream file(testConfigPath);
    file << malformedJson;
    file.close();
    
    Config config;
    // Should handle gracefully and return false
    EXPECT_FALSE(config.loadFromFile(testConfigPath));
    
    // Should still have default values
    EXPECT_EQ(config.getOSCHost(), "127.0.0.1");
    EXPECT_EQ(config.getOSCPort(), "9000");
}

// Test thread safety (basic test)
TEST_F(CVToOSCTest, ConfigThreadSafety) {
    Config config;
    std::atomic<bool> testPassed{true};
    
    // Create multiple threads accessing config
    std::vector<std::thread> threads;
    
    for (int i = 0; i < 4; ++i) {
        threads.emplace_back([&config, &testPassed, i]() {
            try {
                for (int j = 0; j < 100; ++j) {
                    config.setCVRange(i, static_cast<float>(j), static_cast<float>(j + 10));
                    CVRange range = config.getCVRange(i);
                    if (range.min < 0 || range.max < range.min) {
                        testPassed = false;
                        break;
                    }
                }
            } catch (...) {
                testPassed = false;
            }
        });
    }
    
    // Wait for all threads
    for (auto& thread : threads) {
        thread.join();
    }
    
    EXPECT_TRUE(testPassed.load());
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
