#pragma once

#include <vector>
#include <string>
#include <memory>
#include <iostream>
#include <mutex>
#include <atomic>
#include <portaudio.h>

class CVReader {
private:
    PaStream* stream;
    std::vector<float> latestValues;
    std::mutex valuesMutex;
    int numChannels;
    int maxChannels;
    double sampleRate;
    std::string deviceName;
    std::atomic<bool> initialized;
    static constexpr int FRAMES_PER_BUFFER = 64;
    static constexpr int DEFAULT_CHANNELS = 2;  // Start with stereo, auto-detect max

public:
    CVReader(const std::string& deviceName = "");
    ~CVReader();
    
    bool initialize();
    void close();
    std::vector<float> readChannels();
    void readChannels(std::vector<float>& output);  // Zero-copy version
    int getChannelCount() const { return numChannels; }
    
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
