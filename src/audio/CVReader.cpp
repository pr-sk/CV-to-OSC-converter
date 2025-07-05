#include "CVReader.h"
#include "ErrorHandler.h"
#include <iostream>
#include <algorithm>
#include <stdexcept>
#include <cmath>
#include <iomanip>
#include <string>

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
    
    // Initialize signal analysis structures
    channelAnalysis.resize(8);
    channelSignalTypes.resize(8, SignalType::AUTO_DETECT);
    signalHistory.resize(8);
    for (int i = 0; i < 8; ++i) {
        signalHistory[i].reserve(ANALYSIS_HISTORY_SIZE);
        channelAnalysis[i] = {}; // Zero-initialize
        channelAnalysis[i].detectedType = SignalType::UNKNOWN;
    }
    
    // Try to detect device type based on name
    if (isDeviceCV(deviceName)) {
        globalSignalType = SignalType::CV_SIGNAL;
        std::cout << "Device detected as CV interface: " << deviceName << std::endl;
    } else if (isDeviceAudio(deviceName)) {
        globalSignalType = SignalType::AUDIO_SIGNAL;
        std::cout << "Device detected as audio interface: " << deviceName << std::endl;
    } else {
        globalSignalType = SignalType::AUTO_DETECT;
        std::cout << "Device type unknown, will auto-detect: " << deviceName << std::endl;
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
        
        // Try to restart with different parameters if first attempt fails
        std::cout << "Attempting to restart stream with fallback parameters..." << std::endl;
        Pa_CloseStream(stream);
        stream = nullptr;
        
        // Try with minimal parameters
        inputParameters.channelCount = 1; // Minimal channels
        inputParameters.suggestedLatency = deviceInfo->defaultHighInputLatency; // Higher latency
        
        err = Pa_OpenStream(&stream,
                           &inputParameters,
                           nullptr, // no output
                           sampleRate,
                           FRAMES_PER_BUFFER * 2, // Larger buffer
                           paClipOff,
                           audioCallback,
                           this);
        
        if (err == paNoError) {
            err = Pa_StartStream(stream);
            if (err == paNoError) {
                numChannels = 1; // Update channel count
                latestValues.resize(numChannels, 0.0f);
                rawValues.resize(numChannels, 0.0f);
                std::cout << "Audio stream started with fallback parameters" << std::endl;
            } else {
                return false;
            }
        } else {
            return false;
        }
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
    
    // Debug: Log every 100th call to avoid spam
    static int callCount = 0;
    callCount++;
    if (callCount % 100 == 0) {
        std::cout << "[CVReader::processAudio] Call #" << callCount 
                  << ", frameCount=" << frameCount 
                  << ", numChannels=" << numChannels << std::endl;
    }
    
    std::lock_guard<std::mutex> lock(valuesMutex);
    
    // Process samples for each channel
    std::vector<std::vector<float>> channelSamples(numChannels);
    
    // Extract and filter samples for each channel
    for (unsigned long frame = 0; frame < frameCount; ++frame) {
        for (int channel = 0; channel < numChannels; ++channel) {
            float sample = input[frame * numChannels + channel];
            
            // Apply filtering if enabled
            if (filteringEnabled && channelFilters[channel]) {
                sample = channelFilters[channel]->process(sample);
            }
            
            channelSamples[channel].push_back(sample);
        }
    }
    
    // Process each channel based on its signal type
    for (int channel = 0; channel < numChannels; ++channel) {
        if (channelSamples[channel].empty()) continue;
        
        // Update signal history for analysis
        updateSignalHistory(channel, channelSamples[channel]);
        
        // Determine signal type for this channel
        SignalType channelType = channelSignalTypes[channel];
        if (channelType == SignalType::AUTO_DETECT && autoDetectionEnabled) {
            analyzeSignal(channel, signalHistory[channel]);
            channelType = channelAnalysis[channel].detectedType;
            channelSignalTypes[channel] = channelType;
        } else if (globalSignalType != SignalType::AUTO_DETECT) {
            channelType = globalSignalType;
        }
        
        // Process signal based on detected/configured type
        if (channelType == SignalType::CV_SIGNAL) {
            // CV processing: use DC component (average)
            float sum = 0.0f;
            for (float sample : channelSamples[channel]) {
                sum += sample;
            }
            rawValues[channel] = sum / channelSamples[channel].size();
        } else {
            // Audio processing: use RMS
            float sumSquares = 0.0f;
            for (float sample : channelSamples[channel]) {
                sumSquares += sample * sample;
            }
            rawValues[channel] = std::sqrt(sumSquares / channelSamples[channel].size());
        }
        
        latestValues[channel] = rawValues[channel];
        
        // Debug: Log channel values every 100th call
        if (callCount % 100 == 0) {
            std::cout << "[CVReader] Channel " << channel 
                      << ": raw=" << rawValues[channel] 
                      << ", latest=" << latestValues[channel]
                      << ", type=" << signalTypeToString(channelType) << std::endl;
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

// Signal analysis method implementations
void CVReader::updateSignalHistory(int channel, const std::vector<float>& newSamples) {
    if (channel < 0 || channel >= static_cast<int>(signalHistory.size())) return;
    
    auto& history = signalHistory[channel];
    
    // Add new samples to history
    for (float sample : newSamples) {
        history.push_back(sample);
        
        // Keep history size limited
        if (history.size() > ANALYSIS_HISTORY_SIZE) {
            history.erase(history.begin(), history.begin() + (history.size() - ANALYSIS_HISTORY_SIZE));
        }
    }
}

void CVReader::analyzeSignal(int channel, const std::vector<float>& samples) {
    if (channel < 0 || channel >= static_cast<int>(channelAnalysis.size()) || samples.empty()) return;
    
    auto& analysis = channelAnalysis[channel];
    
    // Calculate signal characteristics
    analysis.dcComponent = calculateDC(samples);
    analysis.acComponent = calculateAC(samples, analysis.dcComponent);
    analysis.peakToPeak = calculatePeakToPeak(samples);
    analysis.changeRate = calculateChangeRate(samples);
    
    // Detect signal type
    analysis.detectedType = detectSignalType(analysis);
    
    // Calculate confidence based on signal characteristics
    if (analysis.detectedType == SignalType::CV_SIGNAL) {
        // High confidence if signal is stable with low AC component
        analysis.confidence = std::min(1.0f, 1.0f - (analysis.acComponent / 0.1f));
    } else if (analysis.detectedType == SignalType::AUDIO_SIGNAL) {
        // High confidence if signal has significant AC component
        analysis.confidence = std::min(1.0f, analysis.acComponent / 0.1f);
    } else {
        analysis.confidence = 0.0f;
    }
    
    // Track consecutive stable readings for CV detection
    if (analysis.detectedType == SignalType::CV_SIGNAL && analysis.changeRate < CV_STABILITY_THRESHOLD) {
        analysis.consecutiveStable++;
    } else {
        analysis.consecutiveStable = 0;
    }
}

SignalType CVReader::detectSignalType(const SignalAnalysis& analysis) const {
    // CV signals typically have:
    // - Low AC component (stable DC)
    // - Low change rate
    // - Small peak-to-peak variation relative to DC level
    
    if (analysis.acComponent < CV_STABILITY_THRESHOLD && 
        analysis.changeRate < CV_STABILITY_THRESHOLD &&
        analysis.peakToPeak < std::abs(analysis.dcComponent) * 0.1f) {
        return SignalType::CV_SIGNAL;
    }
    
    // Audio signals typically have:
    // - Significant AC component
    // - High change rate
    // - Large peak-to-peak variation
    
    if (analysis.acComponent > AUDIO_AC_THRESHOLD ||
        analysis.changeRate > AUDIO_AC_THRESHOLD) {
        return SignalType::AUDIO_SIGNAL;
    }
    
    return SignalType::UNKNOWN;
}

float CVReader::calculateDC(const std::vector<float>& samples) const {
    if (samples.empty()) return 0.0f;
    
    float sum = 0.0f;
    for (float sample : samples) {
        sum += sample;
    }
    return sum / samples.size();
}

float CVReader::calculateAC(const std::vector<float>& samples, float dcLevel) const {
    if (samples.empty()) return 0.0f;
    
    float sumSquares = 0.0f;
    for (float sample : samples) {
        float acComponent = sample - dcLevel;
        sumSquares += acComponent * acComponent;
    }
    return std::sqrt(sumSquares / samples.size());
}

float CVReader::calculatePeakToPeak(const std::vector<float>& samples) const {
    if (samples.empty()) return 0.0f;
    
    auto minMax = std::minmax_element(samples.begin(), samples.end());
    return *minMax.second - *minMax.first;
}

float CVReader::calculateChangeRate(const std::vector<float>& samples) const {
    if (samples.size() < 2) return 0.0f;
    
    float totalChange = 0.0f;
    for (size_t i = 1; i < samples.size(); ++i) {
        totalChange += std::abs(samples[i] - samples[i-1]);
    }
    return totalChange / (samples.size() - 1);
}

bool CVReader::isDeviceCV(const std::string& deviceName) const {
    // Check for common CV interface names
    std::string lowerName = deviceName;
    std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);
    
    return lowerName.find("cv") != std::string::npos ||
           lowerName.find("control voltage") != std::string::npos ||
           lowerName.find("eurorack") != std::string::npos ||
           lowerName.find("modular") != std::string::npos ||
           lowerName.find("voltage") != std::string::npos ||
           lowerName.find("gate") != std::string::npos ||
           lowerName.find("trigger") != std::string::npos;
}

bool CVReader::isDeviceAudio(const std::string& deviceName) const {
    // Check for common audio interface names
    std::string lowerName = deviceName;
    std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);
    
    return lowerName.find("микрофон") != std::string::npos ||
           lowerName.find("microphone") != std::string::npos ||
           lowerName.find("mic") != std::string::npos ||
           lowerName.find("audio") != std::string::npos ||
           lowerName.find("звук") != std::string::npos ||
           lowerName.find("speaker") != std::string::npos ||
           lowerName.find("headphone") != std::string::npos;
}

// Public method implementations
SignalType CVReader::getChannelSignalType(int channel) const {
    if (channel < 0 || channel >= static_cast<int>(channelSignalTypes.size())) {
        return SignalType::UNKNOWN;
    }
    return channelSignalTypes[channel];
}

SignalAnalysis CVReader::getChannelAnalysis(int channel) const {
    if (channel < 0 || channel >= static_cast<int>(channelAnalysis.size())) {
        return SignalAnalysis{};
    }
    return channelAnalysis[channel];
}

void CVReader::forceChannelSignalType(int channel, SignalType type) {
    if (channel >= 0 && channel < static_cast<int>(channelSignalTypes.size())) {
        channelSignalTypes[channel] = type;
        std::cout << "Channel " << channel << " signal type forced to: " << signalTypeToString(type) << std::endl;
    }
}

std::string CVReader::signalTypeToString(SignalType type) const {
    switch (type) {
        case SignalType::CV_SIGNAL: return "CV Signal";
        case SignalType::AUDIO_SIGNAL: return "Audio Signal";
        case SignalType::AUTO_DETECT: return "Auto-Detect";
        case SignalType::UNKNOWN: return "Unknown";
        default: return "Invalid";
    }
}

void CVReader::printSignalAnalysis() const {
    std::cout << "\n=== Signal Analysis Report ===" << std::endl;
    std::cout << "Global Signal Type: " << signalTypeToString(globalSignalType) << std::endl;
    std::cout << "Auto-Detection: " << (autoDetectionEnabled ? "Enabled" : "Disabled") << std::endl;
    
    for (int i = 0; i < numChannels; ++i) {
        const auto& analysis = channelAnalysis[i];
        std::cout << "\nChannel " << i << ":" << std::endl;
        std::cout << "  Detected Type: " << signalTypeToString(analysis.detectedType) 
                  << " (confidence: " << std::fixed << std::setprecision(2) << analysis.confidence << ")" << std::endl;
        std::cout << "  DC Component: " << analysis.dcComponent << "V" << std::endl;
        std::cout << "  AC Component: " << analysis.acComponent << "V" << std::endl;
        std::cout << "  Peak-to-Peak: " << analysis.peakToPeak << "V" << std::endl;
        std::cout << "  Change Rate: " << analysis.changeRate << "V/sample" << std::endl;
        std::cout << "  Stable Count: " << analysis.consecutiveStable << std::endl;
    }
    std::cout << "===============================\n" << std::endl;
}
