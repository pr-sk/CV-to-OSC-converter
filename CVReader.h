#pragma once

#include <vector>
#include <string>
#include <memory>
#include <iostream>
#include <mutex>
#include <atomic>
#include <portaudio.h>
#include "CVCalibrator.h"
#include "SignalFilter.h"

class CVReader {
private:
    PaStream* stream;
    std::vector<float> latestValues;
    std::vector<float> rawValues;  // Store uncalibrated values
    std::mutex valuesMutex;
    int numChannels;
    int maxChannels;
    double sampleRate;
    std::string deviceName;
    std::string currentDeviceName;  // Actual device name from PortAudio
    std::atomic<bool> initialized;
    static constexpr int FRAMES_PER_BUFFER = 64;
    static constexpr int DEFAULT_CHANNELS = 2;  // Start with stereo, auto-detect max
    
    // Calibration and filtering
    std::unique_ptr<CVCalibrator> calibrator;
    std::vector<std::unique_ptr<IFilter>> channelFilters;
    bool calibrationEnabled = true;
    bool filteringEnabled = true;

public:
    CVReader(const std::string& deviceName = "");
    ~CVReader();
    
    bool initialize();
    void close();
    std::vector<float> readChannels();
    void readChannels(std::vector<float>& output);  // Zero-copy version
    int getChannelCount() const { return numChannels; }
    int getMaxChannels() const { return maxChannels; }
    double getSampleRate() const { return sampleRate; }
    std::string getCurrentDeviceName() const { return currentDeviceName; }
    bool isInitialized() const { return initialized; }
    
    // Calibration methods
    void enableCalibration(bool enable) { calibrationEnabled = enable; }
    bool isCalibrationEnabled() const { return calibrationEnabled; }
    CVCalibrator* getCalibrator() { return calibrator.get(); }
    void startChannelCalibration(int channel);
    void addCalibrationPoint(int channel, float expectedVoltage);
    CalibrationResult finishChannelCalibration(int channel);
    bool loadCalibration(const std::string& filename = "");
    bool saveCalibration(const std::string& filename = "");
    
    // Filtering methods
    void enableFiltering(bool enable) { filteringEnabled = enable; }
    bool isFilteringEnabled() const { return filteringEnabled; }
    void setChannelFilter(int channel, std::unique_ptr<IFilter> filter);
    void setAllChannelsFilter(FilterType type, float param1 = 0.0f, float param2 = 0.0f);
    void clearChannelFilters();
    std::string getFilterInfo(int channel) const;
    
    // Raw data access (uncalibrated/unfiltered)
    std::vector<float> readRawChannels();
    void readRawChannels(std::vector<float>& output);
    
    // Static callback for PortAudio
    static int audioCallback(const void* inputBuffer, void* outputBuffer,
                           unsigned long framesPerBuffer,
                           const PaStreamCallbackTimeInfo* timeInfo,
                           PaStreamCallbackFlags statusFlags,
                           void* userData);

private:
    int processAudio(const float* input, unsigned long frameCount);
    PaDeviceIndex findDevice(const std::string& deviceName);
};
