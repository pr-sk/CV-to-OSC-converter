#include "CVCalibrator.h"
#include "ErrorHandler.h"
#include <fstream>
#include <algorithm>
#include <numeric>
#include <cmath>
#include <sstream>

CVCalibrator::CVCalibrator(int channelCount, const std::string& calibFile) 
    : calibrationFile(calibFile) {
    channelCalibrations.resize(channelCount);
    
    // Try to load existing calibration
    if (!loadCalibration(calibFile)) {
        ErrorHandler::getInstance().logInfo("No existing calibration found", 
                                          "Starting with default calibration values");
    }
}

CVCalibrator::~CVCalibrator() {
    if (calibrationInProgress.load()) {
        ErrorHandler::getInstance().logWarning("Calibration in progress during destructor", 
                                             "Calibration will be incomplete");
    }
}

void CVCalibrator::startCalibration(int channel) {
    if (channel < 0 || static_cast<size_t>(channel) >= channelCalibrations.size()) {
        ERROR_ERROR("Invalid channel for calibration", 
                   "Channel: " + std::to_string(channel), 
                   "Use valid channel index", false);
        return;
    }
    
    calibrationInProgress = true;
    channelCalibrations[channel] = CalibrationResult(); // Reset
    channelCalibrations[channel].points.clear();
    
    ErrorHandler::getInstance().logInfo("Started calibration for channel " + std::to_string(channel + 1),
                                      "Ready to accept calibration points");
}

void CVCalibrator::addCalibrationPoint(int channel, float expectedVoltage) {
    if (channel < 0 || static_cast<size_t>(channel) >= channelCalibrations.size()) {
        ERROR_ERROR("Invalid channel for calibration point", 
                   "Channel: " + std::to_string(channel), 
                   "Use valid channel index", false);
        return;
    }
    
    if (!dataProvider) {
        ERROR_ERROR("No data provider set", 
                   "Cannot read current CV values", 
                   "Set data provider before calibration", false);
        return;
    }
    
    // Read current values
    auto currentValues = dataProvider();
    if (static_cast<size_t>(channel) >= currentValues.size()) {
        ERROR_ERROR("Channel index out of range in data", 
                   "Channel: " + std::to_string(channel), 
                   "Check data provider implementation", false);
        return;
    }
    
    float measuredValue = currentValues[channel];
    
    // Validate the calibration point
    if (!isValidCalibrationPoint(expectedVoltage, measuredValue)) {
        ERROR_WARNING("Suspicious calibration point", 
                     "Expected: " + std::to_string(expectedVoltage) + 
                     "V, Measured: " + std::to_string(measuredValue),
                     "Check connections and try again");
        return;
    }
    
    CalibrationPoint point(expectedVoltage, measuredValue);
    channelCalibrations[channel].points.push_back(point);
    
    std::string details = "Point " + std::to_string(channelCalibrations[channel].points.size()) + 
                         ": " + std::to_string(expectedVoltage) + "V -> " + 
                         std::to_string(measuredValue);
    ErrorHandler::getInstance().logInfo("Added calibration point", details);
}

CalibrationResult CVCalibrator::finishCalibration(int channel) {
    if (channel < 0 || static_cast<size_t>(channel) >= channelCalibrations.size()) {
        ERROR_ERROR("Invalid channel for calibration finish", 
                   "Channel: " + std::to_string(channel), 
                   "Use valid channel index", false);
        return CalibrationResult();
    }
    
    auto& calibration = channelCalibrations[channel];
    
    if (calibration.points.size() < 2) {
        ERROR_ERROR("Insufficient calibration points", 
                   "Points: " + std::to_string(calibration.points.size()) + " (minimum 2 required)", 
                   "Add more calibration points", false);
        calibration.isValid = false;
        calibrationInProgress = false;
        return calibration;
    }
    
    // Calculate linear calibration
    calibration = calculateLinearCalibration(calibration.points);
    calibration.calibrationTime = std::chrono::system_clock::now();
    
    calibrationInProgress = false;
    
    // Auto-save calibration
    saveCalibration();
    
    std::string details = "Offset: " + std::to_string(calibration.offset) + 
                         ", Scale: " + std::to_string(calibration.scale) + 
                         ", Accuracy: " + std::to_string(calibration.accuracy * 100) + "%";
    ErrorHandler::getInstance().logInfo("Calibration completed for channel " + std::to_string(channel + 1), 
                                      details);
    
    return calibration;
}

void CVCalibrator::setDataProvider(std::function<std::vector<float>()> provider) {
    dataProvider = provider;
    ErrorHandler::getInstance().logInfo("Data provider set for calibration", 
                                      "Calibration system ready");
}

float CVCalibrator::applyCalibration(int channel, float rawValue) const {
    if (channel < 0 || static_cast<size_t>(channel) >= channelCalibrations.size()) {
        return rawValue; // Return uncalibrated value for invalid channels
    }
    
    const auto& calibration = channelCalibrations[channel];
    if (!calibration.isValid) {
        return rawValue; // Return uncalibrated value if calibration is invalid
    }
    
    return rawValue * calibration.scale + calibration.offset;
}

std::vector<float> CVCalibrator::applyCalibration(const std::vector<float>& rawValues) const {
    std::vector<float> calibratedValues;
    calibratedValues.reserve(rawValues.size());
    
    for (size_t i = 0; i < rawValues.size(); ++i) {
        calibratedValues.push_back(applyCalibration(static_cast<int>(i), rawValues[i]));
    }
    
    return calibratedValues;
}

bool CVCalibrator::validateCalibration(int channel) const {
    if (channel < 0 || static_cast<size_t>(channel) >= channelCalibrations.size()) {
        return false;
    }
    
    const auto& calibration = channelCalibrations[channel];
    
    // Check if calibration is valid and recent (less than 30 days old)
    auto now = std::chrono::system_clock::now();
    auto age = now - calibration.calibrationTime;
    auto thirtyDays = std::chrono::hours(24 * 30);
    
    return calibration.isValid && 
           calibration.accuracy > 0.9f && // At least 90% accuracy
           age < thirtyDays;
}

float CVCalibrator::getCalibrationAccuracy(int channel) const {
    if (channel < 0 || static_cast<size_t>(channel) >= channelCalibrations.size()) {
        return 0.0f;
    }
    
    return channelCalibrations[channel].accuracy;
}

std::string CVCalibrator::getCalibrationReport(int channel) const {
    if (channel < 0 || static_cast<size_t>(channel) >= channelCalibrations.size()) {
        return "Invalid channel";
    }
    
    const auto& calibration = channelCalibrations[channel];
    std::ostringstream report;
    
    report << "Channel " << (channel + 1) << " Calibration Report:\n";
    report << "  Status: " << (calibration.isValid ? "Valid" : "Invalid") << "\n";
    report << "  Accuracy: " << (calibration.accuracy * 100) << "%\n";
    report << "  Offset: " << calibration.offset << "\n";
    report << "  Scale: " << calibration.scale << "\n";
    report << "  Range: " << calibration.actualMin << "V to " << calibration.actualMax << "V\n";
    report << "  Calibration Points: " << calibration.points.size() << "\n";
    
    // Format calibration time
    auto time_t = std::chrono::system_clock::to_time_t(calibration.calibrationTime);
    report << "  Calibrated: " << std::ctime(&time_t);
    
    return report.str();
}

bool CVCalibrator::saveCalibration(const std::string& filename) {
    std::string file = filename.empty() ? calibrationFile : filename;
    
    try {
        nlohmann::json j;
        j["version"] = "1.0";
        j["timestamp"] = std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        
        nlohmann::json channels = nlohmann::json::array();
        for (size_t i = 0; i < channelCalibrations.size(); ++i) {
            channels.push_back(calibrationToJson(channelCalibrations[i]));
        }
        j["channels"] = channels;
        
        std::ofstream outFile(file);
        outFile << j.dump(4);
        
        ErrorHandler::getInstance().logInfo("Calibration saved", "File: " + file);
        return true;
        
    } catch (const std::exception& e) {
        ERROR_ERROR("Failed to save calibration", e.what(), 
                   "Check file permissions and disk space", true);
        return false;
    }
}

bool CVCalibrator::loadCalibration(const std::string& filename) {
    std::string file = filename.empty() ? calibrationFile : filename;
    
    try {
        std::ifstream inFile(file);
        if (!inFile.is_open()) {
            return false; // File doesn't exist, not an error
        }
        
        nlohmann::json j;
        inFile >> j;
        
        if (j.contains("channels") && j["channels"].is_array()) {
            auto channels = j["channels"];
            for (size_t i = 0; i < channels.size() && i < channelCalibrations.size(); ++i) {
                channelCalibrations[i] = calibrationFromJson(channels[i]);
            }
        }
        
        ErrorHandler::getInstance().logInfo("Calibration loaded", "File: " + file);
        return true;
        
    } catch (const std::exception& e) {
        ERROR_ERROR("Failed to load calibration", e.what(), 
                   "Check file format and permissions", true);
        return false;
    }
}

bool CVCalibrator::isChannelCalibrated(int channel) const {
    if (channel < 0 || static_cast<size_t>(channel) >= channelCalibrations.size()) {
        return false;
    }
    
    return channelCalibrations[channel].isValid;
}

CalibrationResult CVCalibrator::calculateLinearCalibration(const std::vector<CalibrationPoint>& points) {
    CalibrationResult result;
    
    if (points.size() < 2) {
        result.isValid = false;
        return result;
    }
    
    // Calculate linear regression: measured = scale * expected + offset
    float sumX = 0, sumY = 0, sumXY = 0, sumXX = 0;
    int n = points.size();
    
    for (const auto& point : points) {
        sumX += point.inputVoltage;
        sumY += point.measuredValue;
        sumXY += point.inputVoltage * point.measuredValue;
        sumXX += point.inputVoltage * point.inputVoltage;
    }
    
    float denominator = n * sumXX - sumX * sumX;
    if (std::abs(denominator) < 1e-10f) {
        result.isValid = false;
        return result;
    }
    
    result.scale = (n * sumXY - sumX * sumY) / denominator;
    result.offset = (sumY - result.scale * sumX) / n;
    
    // Calculate accuracy (R-squared)
    result.accuracy = calculateRSquared(points, result.offset, result.scale);
    
    // Calculate actual range
    auto minMaxInputs = std::minmax_element(points.begin(), points.end(),
        [](const CalibrationPoint& a, const CalibrationPoint& b) {
            return a.inputVoltage < b.inputVoltage;
        });
    
    result.actualMin = minMaxInputs.first->inputVoltage;
    result.actualMax = minMaxInputs.second->inputVoltage;
    
    // Mark as valid if accuracy is reasonable
    result.isValid = result.accuracy > 0.8f; // At least 80% correlation
    result.points = points;
    
    return result;
}

float CVCalibrator::calculateRSquared(const std::vector<CalibrationPoint>& points, float offset, float scale) {
    if (points.empty()) return 0.0f;
    
    // Calculate mean of measured values
    float meanY = 0;
    for (const auto& point : points) {
        meanY += point.measuredValue;
    }
    meanY /= points.size();
    
    // Calculate R-squared
    float ssRes = 0, ssTot = 0;
    for (const auto& point : points) {
        float predicted = scale * point.inputVoltage + offset;
        ssRes += (point.measuredValue - predicted) * (point.measuredValue - predicted);
        ssTot += (point.measuredValue - meanY) * (point.measuredValue - meanY);
    }
    
    if (ssTot < 1e-10f) return 0.0f;
    
    return 1.0f - (ssRes / ssTot);
}

bool CVCalibrator::isValidCalibrationPoint(float expected, float measured, float tolerance) {
    // Check for reasonable correlation
    float ratio = std::abs(measured / expected);
    return ratio > (1.0f - tolerance) && ratio < (1.0f + tolerance);
}

nlohmann::json CVCalibrator::calibrationToJson(const CalibrationResult& calibration) {
    nlohmann::json j;
    j["isValid"] = calibration.isValid;
    j["actualMin"] = calibration.actualMin;
    j["actualMax"] = calibration.actualMax;
    j["offset"] = calibration.offset;
    j["scale"] = calibration.scale;
    j["accuracy"] = calibration.accuracy;
    j["calibrationTime"] = std::chrono::duration_cast<std::chrono::seconds>(
        calibration.calibrationTime.time_since_epoch()).count();
    
    nlohmann::json pointsArray = nlohmann::json::array();
    for (const auto& point : calibration.points) {
        nlohmann::json pointJson;
        pointJson["inputVoltage"] = point.inputVoltage;
        pointJson["measuredValue"] = point.measuredValue;
        pointJson["timestamp"] = std::chrono::duration_cast<std::chrono::seconds>(
            point.timestamp.time_since_epoch()).count();
        pointsArray.push_back(pointJson);
    }
    j["points"] = pointsArray;
    
    return j;
}

CalibrationResult CVCalibrator::calibrationFromJson(const nlohmann::json& j) {
    CalibrationResult result;
    
    try {
        result.isValid = j.value("isValid", false);
        result.actualMin = j.value("actualMin", 0.0f);
        result.actualMax = j.value("actualMax", 10.0f);
        result.offset = j.value("offset", 0.0f);
        result.scale = j.value("scale", 1.0f);
        result.accuracy = j.value("accuracy", 0.0f);
        
        if (j.contains("calibrationTime")) {
            auto timestamp = std::chrono::seconds(j["calibrationTime"].get<int64_t>());
            result.calibrationTime = std::chrono::system_clock::time_point(timestamp);
        }
        
        if (j.contains("points") && j["points"].is_array()) {
            for (const auto& pointJson : j["points"]) {
                CalibrationPoint point;
                point.inputVoltage = pointJson.value("inputVoltage", 0.0f);
                point.measuredValue = pointJson.value("measuredValue", 0.0f);
                
                if (pointJson.contains("timestamp")) {
                    auto timestamp = std::chrono::seconds(pointJson["timestamp"].get<int64_t>());
                    point.timestamp = std::chrono::system_clock::time_point(timestamp);
                }
                
                result.points.push_back(point);
            }
        }
        
    } catch (const std::exception& e) {
        ERROR_ERROR("Error parsing calibration JSON", e.what(), 
                   "Using default calibration values", true);
        result = CalibrationResult(); // Reset to defaults
    }
    
    return result;
}
