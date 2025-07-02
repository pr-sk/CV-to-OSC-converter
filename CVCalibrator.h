#pragma once

#include <vector>
#include <string>
#include <chrono>
#include <memory>
#include <atomic>
#include <functional>
#include <nlohmann/json.hpp>

struct CalibrationPoint {
    float inputVoltage;
    float measuredValue;
    std::chrono::system_clock::time_point timestamp;
    
    CalibrationPoint(float input = 0.0f, float measured = 0.0f) 
        : inputVoltage(input), measuredValue(measured), 
          timestamp(std::chrono::system_clock::now()) {}
};

struct CalibrationResult {
    float actualMin, actualMax;
    float offset, scale;
    float accuracy;  // R-squared value
    bool isValid;
    std::vector<CalibrationPoint> points;
    std::chrono::system_clock::time_point calibrationTime;
    
    CalibrationResult() : actualMin(0.0f), actualMax(10.0f), offset(0.0f), 
                         scale(1.0f), accuracy(0.0f), isValid(false),
                         calibrationTime(std::chrono::system_clock::now()) {}
};

class CVCalibrator {
private:
    std::vector<CalibrationResult> channelCalibrations;
    std::string calibrationFile;
    std::atomic<bool> calibrationInProgress{false};
    std::function<std::vector<float>()> dataProvider;
    
    // Auto-calibration parameters
    struct AutoCalibrationConfig {
        float minVoltage = 0.0f;
        float maxVoltage = 10.0f;
        int samplesPerPoint = 100;
        std::chrono::milliseconds dwellTime{1000};
        float tolerance = 0.01f;  // 1% tolerance
        bool enabled = false;
    } autoConfig;
    
public:
    CVCalibrator(int channelCount = 8, const std::string& calibFile = "calibration.json");
    ~CVCalibrator();
    
    // Manual calibration
    void startCalibration(int channel);
    void addCalibrationPoint(int channel, float expectedVoltage);
    CalibrationResult finishCalibration(int channel);
    
    // Auto-calibration
    void setAutoCalibrationConfig(const AutoCalibrationConfig& config);
    void startAutoCalibration(int channel, const std::vector<float>& testVoltages);
    bool isAutoCalibrationSupported() const;
    
    // Data provider for reading current values
    void setDataProvider(std::function<std::vector<float>()> provider);
    
    // Apply calibration
    float applyCalibration(int channel, float rawValue) const;
    std::vector<float> applyCalibration(const std::vector<float>& rawValues) const;
    
    // Validation and diagnostics
    bool validateCalibration(int channel) const;
    float getCalibrationAccuracy(int channel) const;
    std::string getCalibrationReport(int channel) const;
    std::string getSystemCalibrationReport() const;
    
    // Persistence
    bool saveCalibration(const std::string& filename = "");
    bool loadCalibration(const std::string& filename = "");
    void resetCalibration(int channel = -1);  // -1 = all channels
    
    // Status and info
    bool isChannelCalibrated(int channel) const;
    bool isCalibrationInProgress() const { return calibrationInProgress.load(); }
    CalibrationResult getCalibrationResult(int channel) const;
    std::chrono::system_clock::time_point getLastCalibrationTime(int channel) const;
    
    // Statistics
    struct CalibrationStats {
        int totalChannels;
        int calibratedChannels;
        float averageAccuracy;
        std::chrono::system_clock::time_point oldestCalibration;
        std::chrono::system_clock::time_point newestCalibration;
    };
    CalibrationStats getCalibrationStats() const;
    
private:
    // Internal calibration algorithms
    CalibrationResult calculateLinearCalibration(const std::vector<CalibrationPoint>& points);
    float calculateRSquared(const std::vector<CalibrationPoint>& points, float offset, float scale);
    bool isValidCalibrationPoint(float expected, float measured, float tolerance = 0.1f);
    
    // JSON serialization helpers
    nlohmann::json calibrationToJson(const CalibrationResult& calibration);
    CalibrationResult calibrationFromJson(const nlohmann::json& j);
};

// Factory for creating calibration configs
class CalibrationConfigFactory {
public:
    static CVCalibrator::AutoCalibrationConfig createEurorackConfig() {
        CVCalibrator::AutoCalibrationConfig config;
        config.minVoltage = 0.0f;
        config.maxVoltage = 10.0f;
        config.samplesPerPoint = 200;
        config.dwellTime = std::chrono::milliseconds(2000);
        config.tolerance = 0.005f;  // 0.5% for high precision
        return config;
    }
    
    static CVCalibrator::AutoCalibrationConfig createBipolarConfig() {
        CVCalibrator::AutoCalibrationConfig config;
        config.minVoltage = -5.0f;
        config.maxVoltage = 5.0f;
        config.samplesPerPoint = 150;
        config.dwellTime = std::chrono::milliseconds(1500);
        config.tolerance = 0.01f;
        return config;
    }
    
    static CVCalibrator::AutoCalibrationConfig createAudioRateConfig() {
        CVCalibrator::AutoCalibrationConfig config;
        config.minVoltage = -1.0f;
        config.maxVoltage = 1.0f;
        config.samplesPerPoint = 500;  // More samples for audio rate
        config.dwellTime = std::chrono::milliseconds(500);
        config.tolerance = 0.02f;  // Looser tolerance for audio rate
        return config;
    }
};
