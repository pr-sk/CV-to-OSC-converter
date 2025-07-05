#include "RealAudioStream.h"
#include <iostream>
#include <cmath>
#include <algorithm>

// RealAudioStream implementation
RealAudioStream::RealAudioStream() 
    : stream_(nullptr)
    , isRunning_(false)
    , currentInputLevel_(0.0f)
    , deviceIndex_(-1)
    , numChannels_(1)
    , sampleRate_(44100.0)
    , audioBuffer_(BUFFER_SIZE)
    , writePosition_(0)
    , readPosition_(0) {
}

RealAudioStream::~RealAudioStream() {
    stop();
}

bool RealAudioStream::startInputStream(int deviceIndex, int channels, double sampleRate) {
    if (isRunning_) {
        stop();
    }
    
    deviceIndex_ = deviceIndex;
    numChannels_ = channels;
    sampleRate_ = sampleRate;
    
    PaStreamParameters inputParameters;
    inputParameters.device = deviceIndex;
    inputParameters.channelCount = channels;
    inputParameters.sampleFormat = paFloat32;
    inputParameters.suggestedLatency = Pa_GetDeviceInfo(deviceIndex)->defaultLowInputLatency;
    inputParameters.hostApiSpecificStreamInfo = nullptr;
    
    PaError err = Pa_OpenStream(
        &stream_,
        &inputParameters,
        nullptr, // No output
        sampleRate,
        256, // frames per buffer
        paClipOff,
        audioCallback,
        this
    );
    
    if (err != paNoError) {
        std::cerr << "Failed to open audio stream: " << Pa_GetErrorText(err) << std::endl;
        return false;
    }
    
    err = Pa_StartStream(stream_);
    if (err != paNoError) {
        std::cerr << "Failed to start audio stream: " << Pa_GetErrorText(err) << std::endl;
        Pa_CloseStream(stream_);
        stream_ = nullptr;
        return false;
    }
    
    isRunning_ = true;
    std::cout << "✅ Started real audio input stream for device " << deviceIndex 
              << " (" << channels << " channels @ " << sampleRate << " Hz)" << std::endl;
    return true;
}

bool RealAudioStream::startOutputStream(int deviceIndex, int channels, double sampleRate) {
    if (isRunning_) {
        stop();
    }
    
    deviceIndex_ = deviceIndex;
    numChannels_ = channels;
    sampleRate_ = sampleRate;
    
    PaStreamParameters outputParameters;
    outputParameters.device = deviceIndex;
    outputParameters.channelCount = channels;
    outputParameters.sampleFormat = paFloat32;
    outputParameters.suggestedLatency = Pa_GetDeviceInfo(deviceIndex)->defaultLowOutputLatency;
    outputParameters.hostApiSpecificStreamInfo = nullptr;
    
    PaError err = Pa_OpenStream(
        &stream_,
        nullptr, // No input
        &outputParameters,
        sampleRate,
        256, // frames per buffer
        paClipOff,
        audioCallback,
        this
    );
    
    if (err != paNoError) {
        std::cerr << "Failed to open audio stream: " << Pa_GetErrorText(err) << std::endl;
        return false;
    }
    
    err = Pa_StartStream(stream_);
    if (err != paNoError) {
        std::cerr << "Failed to start audio stream: " << Pa_GetErrorText(err) << std::endl;
        Pa_CloseStream(stream_);
        stream_ = nullptr;
        return false;
    }
    
    isRunning_ = true;
    std::cout << "✅ Started real audio output stream for device " << deviceIndex 
              << " (" << channels << " channels @ " << sampleRate << " Hz)" << std::endl;
    return true;
}

bool RealAudioStream::startDuplexStream(int inputDeviceIndex, int outputDeviceIndex, int channels, double sampleRate) {
    if (isRunning_) {
        stop();
    }
    
    deviceIndex_ = inputDeviceIndex; // Store input device index
    numChannels_ = channels;
    sampleRate_ = sampleRate;
    
    PaStreamParameters inputParameters;
    inputParameters.device = inputDeviceIndex;
    inputParameters.channelCount = channels;
    inputParameters.sampleFormat = paFloat32;
    inputParameters.suggestedLatency = Pa_GetDeviceInfo(inputDeviceIndex)->defaultLowInputLatency;
    inputParameters.hostApiSpecificStreamInfo = nullptr;
    
    PaStreamParameters outputParameters;
    outputParameters.device = outputDeviceIndex;
    outputParameters.channelCount = channels;
    outputParameters.sampleFormat = paFloat32;
    outputParameters.suggestedLatency = Pa_GetDeviceInfo(outputDeviceIndex)->defaultLowOutputLatency;
    outputParameters.hostApiSpecificStreamInfo = nullptr;
    
    PaError err = Pa_OpenStream(
        &stream_,
        &inputParameters,
        &outputParameters,
        sampleRate,
        256, // frames per buffer
        paClipOff,
        duplexAudioCallback,
        this
    );
    
    if (err != paNoError) {
        std::cerr << "Failed to open duplex audio stream: " << Pa_GetErrorText(err) << std::endl;
        return false;
    }
    
    err = Pa_StartStream(stream_);
    if (err != paNoError) {
        std::cerr << "Failed to start duplex audio stream: " << Pa_GetErrorText(err) << std::endl;
        Pa_CloseStream(stream_);
        stream_ = nullptr;
        return false;
    }
    
    isRunning_ = true;
    std::cout << "✅ Started full-duplex audio stream: input device " << inputDeviceIndex 
              << ", output device " << outputDeviceIndex
              << " (" << channels << " channels @ " << sampleRate << " Hz)" << std::endl;
    return true;
}

void RealAudioStream::stop() {
    if (stream_) {
        isRunning_ = false;
        Pa_StopStream(stream_);
        Pa_CloseStream(stream_);
        stream_ = nullptr;
        std::cout << "Stopped audio stream" << std::endl;
    }
}

float RealAudioStream::getCurrentInputLevel() const {
    return currentInputLevel_.load();
}

void RealAudioStream::setLevelCallback(std::function<void(float)> callback) {
    std::lock_guard<std::mutex> lock(callbackMutex_);
    levelCallback_ = callback;
}

void RealAudioStream::sendAudioData(float level) {
    // Store level for output processing
    currentInputLevel_ = level;
}

int RealAudioStream::audioCallback(const void* inputBuffer, void* outputBuffer,
                                 unsigned long framesPerBuffer,
                                 const PaStreamCallbackTimeInfo* timeInfo,
                                 PaStreamCallbackFlags statusFlags,
                                 void* userData) {
    RealAudioStream* stream = static_cast<RealAudioStream*>(userData);
    
    if (inputBuffer) {
        return stream->processInputAudio(static_cast<const float*>(inputBuffer), framesPerBuffer);
    } else if (outputBuffer) {
        return stream->processOutputAudio(static_cast<float*>(outputBuffer), framesPerBuffer);
    }
    
    return paContinue;
}

int RealAudioStream::processInputAudio(const float* input, unsigned long frameCount) {
    // Calculate RMS and peak levels
    float sum = 0.0f;
    float peak = 0.0f;
    
    // Store audio in circular buffer for passthrough
    size_t writePos = writePosition_.load();
    
    for (unsigned long i = 0; i < frameCount * numChannels_; i += numChannels_) {
        // Mix down to mono if multichannel
        float sample = 0.0f;
        for (int ch = 0; ch < numChannels_; ch++) {
            sample += input[i + ch];
        }
        sample /= numChannels_;
        
        // Store in circular buffer
        audioBuffer_[writePos] = sample;
        writePos = (writePos + 1) % BUFFER_SIZE;
        
        sum += sample * sample;
        peak = std::max(peak, std::abs(sample));
    }
    
    // Update write position
    writePosition_.store(writePos);
    
    float rms = std::sqrt(sum / frameCount);
    
    // Debug logging
    static int callCount = 0;
    callCount++;
    if (callCount % 100 == 0) {
        std::cout << "[RealAudioStream] Input #" << callCount 
                  << ": RMS=" << rms 
                  << ", Peak=" << peak << std::endl;
    }
    
    // Convert RMS to voltage (0-10V range)
    // RMS is typically 0-1 for normalized audio
    // For microphone input, scale it up significantly
    float cvLevel = rms * 50.0f;  // Scale up for typical mic levels
    cvLevel = std::max(0.0f, std::min(10.0f, cvLevel));
    
    // If signal is too low, use peak instead
    if (cvLevel < 0.1f && peak > 0.01f) {
        cvLevel = peak * 10.0f;
        cvLevel = std::max(0.0f, std::min(10.0f, cvLevel));
    }
    
    // Update current level
    currentInputLevel_ = cvLevel;
    
    // Debug log the CV level
    if (callCount % 100 == 0 && cvLevel > 0.01f) {
        std::cout << "  -> CV Level: " << cvLevel << "V" << std::endl;
    }
    
    // Call callback if set
    {
        std::lock_guard<std::mutex> lock(callbackMutex_);
        if (levelCallback_) {
            levelCallback_(cvLevel);
        }
    }
    
    return paContinue;
}

int RealAudioStream::processOutputAudio(float* output, unsigned long frameCount) {
    // Read audio from circular buffer
    size_t readPos = readPosition_.load();
    size_t writePos = writePosition_.load();
    
    // Check if we have enough data in the buffer
    size_t availableSamples = (writePos >= readPos) ? 
                              (writePos - readPos) : 
                              (BUFFER_SIZE - readPos + writePos);
    
    if (availableSamples < frameCount) {
        // Not enough data, output silence
        for (unsigned long i = 0; i < frameCount * numChannels_; i++) {
            output[i] = 0.0f;
        }
        return paContinue;
    }
    
    // Copy audio from buffer to output
    for (unsigned long i = 0; i < frameCount; i++) {
        float sample = audioBuffer_[readPos];
        
        // Write to all channels
        for (int ch = 0; ch < numChannels_; ch++) {
            output[i * numChannels_ + ch] = sample;
        }
        
        readPos = (readPos + 1) % BUFFER_SIZE;
    }
    
    // Update read position
    readPosition_.store(readPos);
    
    return paContinue;
}

int RealAudioStream::duplexAudioCallback(const void* inputBuffer, void* outputBuffer,
                                       unsigned long framesPerBuffer,
                                       const PaStreamCallbackTimeInfo* timeInfo,
                                       PaStreamCallbackFlags statusFlags,
                                       void* userData) {
    RealAudioStream* stream = static_cast<RealAudioStream*>(userData);
    const float* input = static_cast<const float*>(inputBuffer);
    float* output = static_cast<float*>(outputBuffer);
    
    if (!input || !output) {
        return paContinue;
    }
    
    // Process input for level detection and store in buffer
    stream->processInputAudio(input, framesPerBuffer);
    
    // Copy input directly to output (real-time passthrough)
    for (unsigned long i = 0; i < framesPerBuffer * stream->numChannels_; i++) {
        output[i] = input[i];
    }
    
    return paContinue;
}

// RealAudioStreamManager implementation
RealAudioStreamManager::RealAudioStreamManager() 
    : deviceManager_(nullptr) {
}

RealAudioStreamManager::~RealAudioStreamManager() {
    shutdown();
}

bool RealAudioStreamManager::initialize(AudioDeviceManager* deviceManager) {
    if (!deviceManager) {
        return false;
    }
    
    deviceManager_ = deviceManager;
    return true;
}

void RealAudioStreamManager::shutdown() {
    std::lock_guard<std::mutex> lock(streamsMutex_);
    streams_.clear();
}

bool RealAudioStreamManager::createInputStream(const std::string& deviceId, int audioDeviceIndex) {
    std::lock_guard<std::mutex> lock(streamsMutex_);
    
    // Remove existing stream if any
    streams_.erase(deviceId);
    
    auto stream = std::make_unique<RealAudioStream>();
    if (stream->startInputStream(audioDeviceIndex)) {
        streams_[deviceId] = std::move(stream);
        return true;
    }
    
    return false;
}

bool RealAudioStreamManager::createOutputStream(const std::string& deviceId, int audioDeviceIndex) {
    std::lock_guard<std::mutex> lock(streamsMutex_);
    
    // Remove existing stream if any
    streams_.erase(deviceId);
    
    auto stream = std::make_unique<RealAudioStream>();
    if (stream->startOutputStream(audioDeviceIndex)) {
        streams_[deviceId] = std::move(stream);
        return true;
    }
    
    return false;
}

bool RealAudioStreamManager::createDuplexStream(const std::string& deviceId, int inputDeviceIndex, int outputDeviceIndex) {
    std::lock_guard<std::mutex> lock(streamsMutex_);
    
    // Remove existing stream if any
    streams_.erase(deviceId);
    
    auto stream = std::make_unique<RealAudioStream>();
    if (stream->startDuplexStream(inputDeviceIndex, outputDeviceIndex)) {
        streams_[deviceId] = std::move(stream);
        return true;
    }
    
    return false;
}

void RealAudioStreamManager::removeStream(const std::string& deviceId) {
    std::lock_guard<std::mutex> lock(streamsMutex_);
    streams_.erase(deviceId);
}

float RealAudioStreamManager::getInputLevel(const std::string& deviceId) const {
    std::lock_guard<std::mutex> lock(streamsMutex_);
    
    auto it = streams_.find(deviceId);
    if (it != streams_.end() && it->second) {
        return it->second->getCurrentInputLevel();
    }
    
    return 0.0f;
}

void RealAudioStreamManager::sendOutputData(const std::string& deviceId, float level) {
    std::lock_guard<std::mutex> lock(streamsMutex_);
    
    auto it = streams_.find(deviceId);
    if (it != streams_.end() && it->second) {
        it->second->sendAudioData(level);
    }
}

bool RealAudioStreamManager::hasStream(const std::string& deviceId) const {
    std::lock_guard<std::mutex> lock(streamsMutex_);
    return streams_.find(deviceId) != streams_.end();
}

bool RealAudioStreamManager::isStreamRunning(const std::string& deviceId) const {
    std::lock_guard<std::mutex> lock(streamsMutex_);
    
    auto it = streams_.find(deviceId);
    if (it != streams_.end() && it->second) {
        return it->second->isRunning();
    }
    
    return false;
}
