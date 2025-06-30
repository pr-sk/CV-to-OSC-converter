#include "CVReader.h"
#include "ErrorHandler.h"
#include <iostream>
#include <algorithm>
#include <stdexcept>

CVReader::CVReader(const std::string& deviceName) 
    : stream(nullptr), numChannels(DEFAULT_CHANNELS), maxChannels(8), 
      sampleRate(44100.0), deviceName(deviceName), initialized(false) {
    latestValues.resize(numChannels, 0.0f);
    
    if (!initialize()) {
        throw std::runtime_error("Failed to initialize CV reader");
    }
}

CVReader::~CVReader() {
    close();
}

bool CVReader::initialize() {
    PaError err = Pa_Initialize();
    if (err != paNoError) {
        std::string errorMsg = "PortAudio initialization failed";
        std::string details = std::string("PortAudio Error: ") + Pa_GetErrorText(err);
        AUDIO_ERROR(errorMsg, details, true, "Check audio drivers and restart application");
        return false;
    }

    PaDeviceIndex deviceIndex = Pa_GetDefaultInputDevice();
    if (!deviceName.empty()) {
        deviceIndex = findDevice(deviceName);
        if (deviceIndex == paNoDevice) {
            std::cout << "Device '" << deviceName << "' not found, using default input device" << std::endl;
            deviceIndex = Pa_GetDefaultInputDevice();
        }
    }

    if (deviceIndex == paNoDevice) {
        AUDIO_ERROR("No input device available", 
                   "System has no available audio input devices", 
                   false, 
                   "Connect an audio interface or enable built-in microphone");
        return false;
    }

    const PaDeviceInfo* deviceInfo = Pa_GetDeviceInfo(deviceIndex);
    std::cout << "Using input device: " << deviceInfo->name << std::endl;
    
    // Auto-detect maximum available channels, but start with a reasonable default
    maxChannels = std::min(static_cast<int>(deviceInfo->maxInputChannels), 8);
    numChannels = std::min(numChannels, maxChannels);
    
    std::cout << "Available channels: " << maxChannels << ", using: " << numChannels << std::endl;
    latestValues.resize(numChannels, 0.0f);

    PaStreamParameters inputParameters;
    inputParameters.device = deviceIndex;
    inputParameters.channelCount = numChannels;
    inputParameters.sampleFormat = paFloat32;
    inputParameters.suggestedLatency = deviceInfo->defaultLowInputLatency;
    inputParameters.hostApiSpecificStreamInfo = nullptr;

    err = Pa_OpenStream(&stream,
                        &inputParameters,
                        nullptr, // no output
                        sampleRate,
                        FRAMES_PER_BUFFER,
                        paClipOff,
                        audioCallback,
                        this);

    if (err != paNoError) {
        std::string errorMsg = "Failed to open audio stream";
        std::string details = std::string("Device: ") + deviceInfo->name + ", Channels: " + 
                             std::to_string(numChannels) + ", Error: " + Pa_GetErrorText(err);
        AUDIO_ERROR(errorMsg, details, true, "Try different audio device or reduce channel count");
        return false;
    }

    err = Pa_StartStream(stream);
    if (err != paNoError) {
        std::string errorMsg = "Failed to start audio stream";
        std::string details = std::string("Error: ") + Pa_GetErrorText(err);
        AUDIO_ERROR(errorMsg, details, true, "Check audio device availability and permissions");
        return false;
    }

    initialized = true;
    std::string msg = "CV Reader initialized successfully";
    std::string details = "Device: " + std::string(deviceInfo->name) + ", Channels: " + 
                         std::to_string(numChannels) + ", Sample Rate: " + 
                         std::to_string(sampleRate) + " Hz";
    ErrorHandler::getInstance().logInfo(msg, details);
    std::cout << "CV Reader initialized successfully with " << numChannels << " channels" << std::endl;
    return true;
}

void CVReader::close() {
    if (stream) {
        Pa_CloseStream(stream);
        stream = nullptr;
    }
    Pa_Terminate();
}

std::vector<float> CVReader::readChannels() {
    std::lock_guard<std::mutex> lock(valuesMutex);
    return latestValues; // Return the most recent values from the audio callback
}

void CVReader::readChannels(std::vector<float>& output) {
    std::lock_guard<std::mutex> lock(valuesMutex);
    output.assign(latestValues.begin(), latestValues.end());
}

int CVReader::audioCallback(const void* inputBuffer, void* /* outputBuffer */,
                           unsigned long framesPerBuffer,
                           const PaStreamCallbackTimeInfo* /* timeInfo */,
                           PaStreamCallbackFlags /* statusFlags */,
                           void* userData) {
    CVReader* reader = static_cast<CVReader*>(userData);
    const float* input = static_cast<const float*>(inputBuffer);
    
    return reader->processAudio(input, framesPerBuffer);
}

int CVReader::processAudio(const float* input, unsigned long frameCount) {
    if (!input || !initialized) return paContinue;
    
    // Use RMS for better CV signal representation (better than simple averaging)
    std::vector<float> channelRMS(numChannels, 0.0f);
    
    for (unsigned long frame = 0; frame < frameCount; ++frame) {
        for (int channel = 0; channel < numChannels; ++channel) {
            float sample = input[frame * numChannels + channel];
            channelRMS[channel] += sample * sample;
        }
    }
    
    // Store the RMS values with thread safety
    {
        std::lock_guard<std::mutex> lock(valuesMutex);
        for (int channel = 0; channel < numChannels; ++channel) {
            latestValues[channel] = std::sqrt(channelRMS[channel] / frameCount);
        }
    }
    
    return paContinue;
}

PaDeviceIndex CVReader::findDevice(const std::string& deviceName) {
    int numDevices = Pa_GetDeviceCount();
    
    for (int i = 0; i < numDevices; ++i) {
        const PaDeviceInfo* deviceInfo = Pa_GetDeviceInfo(i);
        if (deviceInfo && std::string(deviceInfo->name).find(deviceName) != std::string::npos) {
            return i;
        }
    }
    
    return paNoDevice;
}
