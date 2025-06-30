#include <iostream>
#include <fstream>
#include <filesystem>
#include <cassert>
#include <chrono>
#include <sstream>

// Include our headers
#include "../Config.h"

// Simple test framework
class SimpleTest {
private:
    static int testCount;
    static int passedCount;
    static int failedCount;
    
public:
    static void assert_equal(const std::string& expected, const std::string& actual, const std::string& testName) {
        testCount++;
        if (expected == actual) {
            std::cout << "✓ " << testName << " PASSED" << std::endl;
            passedCount++;
        } else {
            std::cout << "✗ " << testName << " FAILED" << std::endl;
            std::cout << "  Expected: " << expected << std::endl;
            std::cout << "  Actual: " << actual << std::endl;
            failedCount++;
        }
    }
    
    static void assert_float_equal(float expected, float actual, const std::string& testName, float tolerance = 0.001f) {
        testCount++;
        if (std::abs(expected - actual) < tolerance) {
            std::cout << "✓ " << testName << " PASSED" << std::endl;
            passedCount++;
        } else {
            std::cout << "✗ " << testName << " FAILED" << std::endl;
            std::cout << "  Expected: " << expected << std::endl;
            std::cout << "  Actual: " << actual << std::endl;
            failedCount++;
        }
    }
    
    static void assert_true(bool condition, const std::string& testName) {
        testCount++;
        if (condition) {
            std::cout << "✓ " << testName << " PASSED" << std::endl;
            passedCount++;
        } else {
            std::cout << "✗ " << testName << " FAILED" << std::endl;
            std::cout << "  Expected: true" << std::endl;
            std::cout << "  Actual: false" << std::endl;
            failedCount++;
        }
    }
    
    static void print_summary() {
        std::cout << "\n" << std::string(50, '=') << std::endl;
        std::cout << "TEST SUMMARY" << std::endl;
        std::cout << std::string(50, '=') << std::endl;
        std::cout << "Total tests: " << testCount << std::endl;
        std::cout << "Passed: " << passedCount << std::endl;
        std::cout << "Failed: " << failedCount << std::endl;
        std::cout << "Success rate: " << (testCount > 0 ? (passedCount * 100.0 / testCount) : 0) << "%" << std::endl;
        
        if (failedCount == 0) {
            std::cout << "\n🎉 ALL TESTS PASSED! 🎉" << std::endl;
        } else {
            std::cout << "\n❌ SOME TESTS FAILED" << std::endl;
        }
    }
    
    static int get_exit_code() {
        return failedCount > 0 ? 1 : 0;
    }
};

int SimpleTest::testCount = 0;
int SimpleTest::passedCount = 0;
int SimpleTest::failedCount = 0;

// Test functions
void test_config_defaults() {
    std::cout << "\n--- Testing Config Default Values ---" << std::endl;
    
    Config config;
    
    SimpleTest::assert_equal("127.0.0.1", config.getOSCHost(), "Default OSC Host");
    SimpleTest::assert_equal("9000", config.getOSCPort(), "Default OSC Port");
    SimpleTest::assert_equal("", config.getAudioDevice(), "Default Audio Device");
    SimpleTest::assert_true(config.getUpdateIntervalMs() == 10, "Default Update Interval");
    
    CVRange range = config.getCVRange(0);
    SimpleTest::assert_float_equal(0.0f, range.min, "Default CV Range Min");
    SimpleTest::assert_float_equal(10.0f, range.max, "Default CV Range Max");
}

void test_config_setters() {
    std::cout << "\n--- Testing Config Setters ---" << std::endl;
    
    Config config;
    
    config.setOSCHost("192.168.1.100");
    config.setOSCPort("8000");
    config.setAudioDevice("Test Device");
    config.setUpdateIntervalMs(20);
    config.setCVRange(0, -5.0f, 5.0f);
    
    SimpleTest::assert_equal("192.168.1.100", config.getOSCHost(), "Set OSC Host");
    SimpleTest::assert_equal("8000", config.getOSCPort(), "Set OSC Port");
    SimpleTest::assert_equal("Test Device", config.getAudioDevice(), "Set Audio Device");
    SimpleTest::assert_true(config.getUpdateIntervalMs() == 20, "Set Update Interval");
    
    CVRange range = config.getCVRange(0);
    SimpleTest::assert_float_equal(-5.0f, range.min, "Set CV Range Min");
    SimpleTest::assert_float_equal(5.0f, range.max, "Set CV Range Max");
}

void test_config_file_operations() {
    std::cout << "\n--- Testing Config File Operations ---" << std::endl;
    
    // Setup test directory
    std::string testDir = "test_output";
    std::filesystem::create_directories(testDir);
    std::string testConfigPath = testDir + "/test_config.json";
    
    // Test save and load
    {
        Config config;
        config.setOSCHost("192.168.1.50");
        config.setOSCPort("7000");
        config.setUpdateIntervalMs(30);
        config.setCVRange(0, -10.0f, 10.0f);
        
        bool saved = config.saveToFile(testConfigPath);
        SimpleTest::assert_true(saved, "Config Save");
        
        Config loadedConfig;
        bool loaded = loadedConfig.loadFromFile(testConfigPath);
        SimpleTest::assert_true(loaded, "Config Load");
        
        SimpleTest::assert_equal("192.168.1.50", loadedConfig.getOSCHost(), "Loaded OSC Host");
        SimpleTest::assert_equal("7000", loadedConfig.getOSCPort(), "Loaded OSC Port");
        SimpleTest::assert_true(loadedConfig.getUpdateIntervalMs() == 30, "Loaded Update Interval");
        
        CVRange range = loadedConfig.getCVRange(0);
        SimpleTest::assert_float_equal(-10.0f, range.min, "Loaded CV Range Min");
        SimpleTest::assert_float_equal(10.0f, range.max, "Loaded CV Range Max");
    }
    
    // Test non-existent file
    {
        Config config;
        bool loaded = config.loadFromFile("non_existent_file.json");
        SimpleTest::assert_true(loaded, "Load Non-existent File (should create default)");
        SimpleTest::assert_equal("127.0.0.1", config.getOSCHost(), "Non-existent File Default Host");
    }
    
    // Cleanup
    std::filesystem::remove_all(testDir);
}

void test_cv_range_edge_cases() {
    std::cout << "\n--- Testing CV Range Edge Cases ---" << std::endl;
    
    Config config;
    
    // Test negative channel index
    config.setCVRange(-1, 1.0f, 2.0f);
    CVRange range = config.getCVRange(-1);
    SimpleTest::assert_float_equal(0.0f, range.min, "Negative Channel Index (should return default)");
    SimpleTest::assert_float_equal(10.0f, range.max, "Negative Channel Index Max");
    
    // Test large channel index
    config.setCVRange(100, 3.0f, 7.0f);
    range = config.getCVRange(100);
    SimpleTest::assert_float_equal(3.0f, range.min, "Large Channel Index Min");
    SimpleTest::assert_float_equal(7.0f, range.max, "Large Channel Index Max");
}

void test_cv_normalization() {
    std::cout << "\n--- Testing CV Normalization Logic ---" << std::endl;
    
    // Test normalization function (copied from main app logic)
    auto normalize = [](float cvValue, float min, float max) -> float {
        if (min >= max) return 0.0f;
        
        // Clamp to range
        cvValue = std::max(min, std::min(max, cvValue));
        
        // Normalize to 0-1
        return (cvValue - min) / (max - min);
    };
    
    // Test standard 0-10V range
    SimpleTest::assert_float_equal(0.0f, normalize(0.0f, 0.0f, 10.0f), "Normalize Min Value");
    SimpleTest::assert_float_equal(0.5f, normalize(5.0f, 0.0f, 10.0f), "Normalize Mid Value");
    SimpleTest::assert_float_equal(1.0f, normalize(10.0f, 0.0f, 10.0f), "Normalize Max Value");
    SimpleTest::assert_float_equal(0.0f, normalize(-1.0f, 0.0f, 10.0f), "Normalize Below Range (clamped)");
    SimpleTest::assert_float_equal(1.0f, normalize(11.0f, 0.0f, 10.0f), "Normalize Above Range (clamped)");
    
    // Test bipolar range
    SimpleTest::assert_float_equal(0.0f, normalize(-5.0f, -5.0f, 5.0f), "Bipolar Min Value");
    SimpleTest::assert_float_equal(0.5f, normalize(0.0f, -5.0f, 5.0f), "Bipolar Center Value");
    SimpleTest::assert_float_equal(1.0f, normalize(5.0f, -5.0f, 5.0f), "Bipolar Max Value");
}

void test_performance() {
    std::cout << "\n--- Testing Performance ---" << std::endl;
    
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
    
    // Should complete in reasonable time (less than 10ms = 10000 microseconds)
    SimpleTest::assert_true(duration.count() < 10000, 
        "Performance Test (1000 operations < 10ms): " + std::to_string(duration.count()) + "μs");
}

void test_json_parsing() {
    std::cout << "\n--- Testing JSON Parsing ---" << std::endl;
    
    // Setup test directory
    std::string testDir = "test_output";
    std::filesystem::create_directories(testDir);
    std::string testConfigPath = testDir + "/test_json.json";
    
    // Create test JSON file
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
    
    std::ofstream file(testConfigPath);
    file << jsonContent;
    file.close();
    
    Config config;
    bool loaded = config.loadFromFile(testConfigPath);
    SimpleTest::assert_true(loaded, "JSON Config Load");
    
    SimpleTest::assert_equal("192.168.1.50", config.getOSCHost(), "JSON OSC Host");
    SimpleTest::assert_equal("7000", config.getOSCPort(), "JSON OSC Port");
    SimpleTest::assert_equal("Test Device", config.getAudioDevice(), "JSON Audio Device");
    SimpleTest::assert_true(config.getUpdateIntervalMs() == 50, "JSON Update Interval");
    
    CVRange range0 = config.getCVRange(0);
    SimpleTest::assert_float_equal(-10.0f, range0.min, "JSON CV Range 0 Min");
    SimpleTest::assert_float_equal(10.0f, range0.max, "JSON CV Range 0 Max");
    
    CVRange range1 = config.getCVRange(1);
    SimpleTest::assert_float_equal(0.0f, range1.min, "JSON CV Range 1 Min");
    SimpleTest::assert_float_equal(5.0f, range1.max, "JSON CV Range 1 Max");
    
    // Cleanup
    std::filesystem::remove_all(testDir);
}

void test_malformed_json() {
    std::cout << "\n--- Testing Malformed JSON Handling ---" << std::endl;
    
    // Setup test directory
    std::string testDir = "test_output";
    std::filesystem::create_directories(testDir);
    std::string testConfigPath = testDir + "/malformed.json";
    
    // Create malformed JSON
    std::string malformedJson = R"({
        "osc_host": "192.168.1.50",
        "osc_port": 7000,
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
    bool loaded = config.loadFromFile(testConfigPath);
    SimpleTest::assert_true(!loaded, "Malformed JSON Should Fail");
    
    // Should still have default values
    SimpleTest::assert_equal("127.0.0.1", config.getOSCHost(), "Malformed JSON Default Host");
    SimpleTest::assert_equal("9000", config.getOSCPort(), "Malformed JSON Default Port");
    
    // Cleanup
    std::filesystem::remove_all(testDir);
}

int main() {
    std::cout << "CV to OSC Converter - Automated Test Suite" << std::endl;
    std::cout << std::string(50, '=') << std::endl;
    
    try {
        test_config_defaults();
        test_config_setters();
        test_config_file_operations();
        test_cv_range_edge_cases();
        test_cv_normalization();
        test_performance();
        test_json_parsing();
        test_malformed_json();
        
        SimpleTest::print_summary();
        return SimpleTest::get_exit_code();
        
    } catch (const std::exception& e) {
        std::cerr << "Test execution failed with exception: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "Test execution failed with unknown exception" << std::endl;
        return 1;
    }
}
