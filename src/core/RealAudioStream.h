#pragma once

#include <portaudio.h>
#include <memory>
#include <atomic>
#include <mutex>
#include <map>
#include <functional>
#include <vector>
#include "AudioDeviceManager.h"

class RealAudioStream {
private:
    PaStream* stream_;
    std::atomic<bool> isRunning_;
    std::atomic<float> currentInputLevel_;
    std::mutex callbackMutex_;
    
    // Device info
    int deviceIndex_;
    int numChannels_;
    double sampleRate_;
    
    // Callback for processed audio data
    std::function<void(float)> levelCallback_;
    
    // Audio buffer for passthrough (circular buffer)
    std::vector<float> audioBuffer_;
    std::atomic<size_t> writePosition_;
    std::atomic<size_t> readPosition_;
    static constexpr size_t BUFFER_SIZE = 44100 * 2; // 2 seconds at 44.1kHz
    
public:
    RealAudioStream();
    ~RealAudioStream();
    
    // Start audio stream for specified device
    bool startInputStream(int deviceIndex, int channels = 1, double sampleRate = 44100.0);
    bool startOutputStream(int deviceIndex, int channels = 1, double sampleRate = 44100.0);
    bool startDuplexStream(int inputDeviceIndex, int outputDeviceIndex, int channels = 1, double sampleRate = 44100.0);
    
    void stop();
    
    // Get current audio level (for input streams)
    float getCurrentInputLevel() const;
    
    // Set callback for processed audio data
    void setLevelCallback(std::function<void(float)> callback);
    
    // Send audio data (for output streams)
    void sendAudioData(float level);
    
    bool isRunning() const { return isRunning_; }
    
private:
    // PortAudio callbacks
    static int audioCallback(const void* inputBuffer, void* outputBuffer,
                           unsigned long framesPerBuffer,
                           const PaStreamCallbackTimeInfo* timeInfo,
                           PaStreamCallbackFlags statusFlags,
                           void* userData);
    
    static int duplexAudioCallback(const void* inputBuffer, void* outputBuffer,
                                 unsigned long framesPerBuffer,
                                 const PaStreamCallbackTimeInfo* timeInfo,
                                 PaStreamCallbackFlags statusFlags,
                                 void* userData);
    
    int processInputAudio(const float* input, unsigned long frameCount);
    int processOutputAudio(float* output, unsigned long frameCount);
};

// Manager for multiple audio streams
class RealAudioStreamManager {
private:
    std::map<std::string, std::unique_ptr<RealAudioStream>> streams_;
    mutable std::mutex streamsMutex_;
    AudioDeviceManager* deviceManager_;
    
public:
    RealAudioStreamManager();
    ~RealAudioStreamManager();
    
    bool initialize(AudioDeviceManager* deviceManager);
    void shutdown();
    
    // Create audio stream for OSC device
    bool createInputStream(const std::string& deviceId, int audioDeviceIndex);
    bool createOutputStream(const std::string& deviceId, int audioDeviceIndex);
    bool createDuplexStream(const std::string& deviceId, int inputDeviceIndex, int outputDeviceIndex);
    
    void removeStream(const std::string& deviceId);
    
    // Get current level from input stream
    float getInputLevel(const std::string& deviceId) const;
    
    // Send audio data to output stream
    void sendOutputData(const std::string& deviceId, float level);
    
    // Check if stream exists and is running
    bool hasStream(const std::string& deviceId) const;
    bool isStreamRunning(const std::string& deviceId) const;
};
