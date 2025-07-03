#include "CVReader.h"
#include "ErrorHandler.h"
#include <iostream>
#include <algorithm>
#include <stdexcept>

CVReader::CVReader(const std::string& deviceName) 
    : stream(nullptr), numChannels(DEFAULT_CHANNELS), maxChannels(8), 
      sampleRate(44100.0), deviceName(deviceName), initialized(false) {
    latestValues.resize(numChannels, 0.0f);
    rawValues.resize(numChannels, 0.0f);
    
    // Initialize calibrator
    calibrator = std::make_unique<CVCalibrator>(8); // Support up to 8 channels
    calibrator->setDataProvider([this]() { return this->readRawChannels(); });
    
    // Initialize filters for each channel
    channelFilters.resize(8);
    for (int i = 0; i < 8; ++i) {
        channelFilters[i] = FilterFactory::createCVFilter();
    }
    
    if (!initialize()) {
        throw std::runtime_error("Failed to initialize CV reader");
    }
}

CVReader::~CVReader() {
    close();
}

void CVReader::startChannelCalibration(int channel) {
    if (calibrator) {
        calibrator->startCalibration(channel);
    }
}

void CVReader::addCalibrationPoint(int channel, float expectedVoltage) {
    if (calibrator) {
        calibrator->addCalibrationPoint(channel, expectedVoltage);
    }
}

CalibrationResult CVReader::finishChannelCalibration(int channel) {
    if (calibrator) {
        return calibrator->finishCalibration(channel);
    }
    return CalibrationResult();
}

bool CVReader::loadCalibration(const std::string& filename) {
    if (calibrator) {
        return calibrator->loadCalibration(filename);
    }
    return false;
}

bool CVReader::saveCalibration(const std::string& filename) {
    if (calibrator) {
        return calibrator->saveCalibration(filename);
    }
    return false;
}

void CVReader::setChannelFilter(int channel, std::unique_ptr<IFilter> filter) {
    if (channel >= 0 && channel < static_cast<int>(channelFilters.size())) {
        channelFilters[channel] = std::move(filter);
    }
}

void CVReader::setAllChannelsFilter(FilterType type, float param1, float param2) {
    for (int i = 0; i < numChannels; ++i) {
        channelFilters[i] = FilterFactory::createFilter(type, param1, param2);
    }
}

void CVReader::clearChannelFilters() {
    for (auto& filter : channelFilters) {
        filter.reset();
    }
}

std::string CVReader::getFilterInfo(int channel) const {
    if (channel >= 0 && channel < static_cast<int>(channelFilters.size()) && channelFilters[channel]) {
        return channelFilters[channel]->getName();
    }
    return "No filter";
}

std::vector<float> CVReader::readRawChannels() {
    std::lock_guard<std::mutex> lock(valuesMutex);
    return rawValues;
}

void CVReader::readRawChannels(std::vector<float>& output) {
    std::lock_guard<std::mutex> lock(valuesMutex);
    output.assign(rawValues.begin(), rawValues.end());
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
    currentDeviceName = deviceInfo->name ? deviceInfo->name : "Unknown Device";
    std::cout << "Using input device: " << currentDeviceName << std::endl;
    
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
    
    // Read raw channel values
    {
        std::lock_guard<std::mutex> lock(valuesMutex);
        for (unsigned long frame = 0; frame < frameCount; ++frame) {
            for (int channel = 0; channel < numChannels; ++channel) {
                rawValues[channel] = input[frame * numChannels + channel];
                if (filteringEnabled && channelFilters[channel]) {
                    rawValues[channel] = channelFilters[channel]->process(rawValues[channel]);
                }
                latestValues[channel] = rawValues[channel];  // Keep for continuity
            }
        }
    }

    // Apply calibration if enabled
    if (calibrationEnabled) {
        latestValues = calibrator->applyCalibration(rawValues);
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
