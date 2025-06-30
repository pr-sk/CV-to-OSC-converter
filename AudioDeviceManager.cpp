#include "AudioDeviceManager.h"
#include <iostream>
#include <algorithm>
#include <iomanip>
#include <sstream>
#include <thread>
#include <chrono>

AudioDeviceManager::AudioDeviceManager() 
    : initialized(false), lastDefaultInputDevice(paNoDevice) {
}

AudioDeviceManager::~AudioDeviceManager() {
    cleanup();
}

bool AudioDeviceManager::initialize() {
    if (initialized) {
        return true;
    }
    
    PaError err = Pa_Initialize();
    if (err != paNoError) {
        std::cerr << "Failed to initialize PortAudio: " << Pa_GetErrorText(err) << std::endl;
        return false;
    }
    
    initialized = true;
    refreshDeviceList();
    lastDefaultInputDevice = Pa_GetDefaultInputDevice();
    
    return true;
}

void AudioDeviceManager::cleanup() {
    if (initialized) {
        removeAllCallbacks();
        Pa_Terminate();
        initialized = false;
    }
}

void AudioDeviceManager::refreshDeviceList() {
    if (!initialized) {
        return;
    }
    
    devices.clear();
    int deviceCount = Pa_GetDeviceCount();
    
    if (deviceCount < 0) {
        std::cerr << "Error getting device count: " << Pa_GetErrorText(deviceCount) << std::endl;
        return;
    }
    
    PaDeviceIndex defaultInput = Pa_GetDefaultInputDevice();
    PaDeviceIndex defaultOutput = Pa_GetDefaultOutputDevice();
    
    for (int i = 0; i < deviceCount; ++i) {
        AudioDeviceInfo info;
        populateDeviceInfo(info, i);
        info.isDefaultInput = (i == defaultInput);
        info.isDefaultOutput = (i == defaultOutput);
        devices.push_back(info);
    }
}

void AudioDeviceManager::populateDeviceInfo(AudioDeviceInfo& info, PaDeviceIndex index) {
    const PaDeviceInfo* deviceInfo = Pa_GetDeviceInfo(index);
    if (!deviceInfo) {
        return;
    }
    
    info.index = index;
    info.name = deviceInfo->name ? deviceInfo->name : "Unknown Device";
    info.maxInputChannels = deviceInfo->maxInputChannels;
    info.maxOutputChannels = deviceInfo->maxOutputChannels;
    info.defaultSampleRate = deviceInfo->defaultSampleRate;
    info.defaultLowInputLatency = deviceInfo->defaultLowInputLatency;
    info.defaultHighInputLatency = deviceInfo->defaultHighInputLatency;
    info.hostApi = getHostApiName(deviceInfo->hostApi);
    info.isCurrentlyAvailable = testDevice(index, 1, 44100.0); // Test with minimal requirements
}

std::vector<AudioDeviceInfo> AudioDeviceManager::getInputDevices() const {
    std::vector<AudioDeviceInfo> inputDevices;
    for (const auto& device : devices) {
        if (device.maxInputChannels > 0) {
            inputDevices.push_back(device);
        }
    }
    return inputDevices;
}

std::vector<AudioDeviceInfo> AudioDeviceManager::getOutputDevices() const {
    std::vector<AudioDeviceInfo> outputDevices;
    for (const auto& device : devices) {
        if (device.maxOutputChannels > 0) {
            outputDevices.push_back(device);
        }
    }
    return outputDevices;
}

AudioDeviceInfo AudioDeviceManager::getDefaultInputDevice() const {
    PaDeviceIndex defaultIndex = Pa_GetDefaultInputDevice();
    return findDeviceByIndex(defaultIndex);
}

AudioDeviceInfo AudioDeviceManager::getDefaultOutputDevice() const {
    PaDeviceIndex defaultIndex = Pa_GetDefaultOutputDevice();
    return findDeviceByIndex(defaultIndex);
}

AudioDeviceInfo AudioDeviceManager::findDeviceByName(const std::string& name) const {
    for (const auto& device : devices) {
        if (isDeviceNameMatch(device.name, name)) {
            return device;
        }
    }
    return AudioDeviceInfo(); // Return invalid device if not found
}

AudioDeviceInfo AudioDeviceManager::findDeviceByIndex(PaDeviceIndex index) const {
    for (const auto& device : devices) {
        if (device.index == index) {
            return device;
        }
    }
    return AudioDeviceInfo(); // Return invalid device if not found
}

std::vector<AudioDeviceInfo> AudioDeviceManager::findDevicesContaining(const std::string& searchTerm) const {
    std::vector<AudioDeviceInfo> matchingDevices;
    std::string lowerSearchTerm = searchTerm;
    std::transform(lowerSearchTerm.begin(), lowerSearchTerm.end(), lowerSearchTerm.begin(), ::tolower);
    
    for (const auto& device : devices) {
        std::string lowerDeviceName = device.name;
        std::transform(lowerDeviceName.begin(), lowerDeviceName.end(), lowerDeviceName.begin(), ::tolower);
        
        if (lowerDeviceName.find(lowerSearchTerm) != std::string::npos) {
            matchingDevices.push_back(device);
        }
    }
    return matchingDevices;
}

bool AudioDeviceManager::isDeviceValid(PaDeviceIndex index) const {
    if (index < 0 || index >= static_cast<PaDeviceIndex>(devices.size())) {
        return false;
    }
    
    const PaDeviceInfo* deviceInfo = Pa_GetDeviceInfo(index);
    return deviceInfo != nullptr;
}

bool AudioDeviceManager::canDeviceHandleFormat(PaDeviceIndex index, int channelCount, double sampleRate) const {
    if (!isDeviceValid(index)) {
        return false;
    }
    
    const PaDeviceInfo* deviceInfo = Pa_GetDeviceInfo(index);
    if (!deviceInfo) {
        return false;
    }
    
    // Check if device has enough input channels
    if (channelCount > deviceInfo->maxInputChannels) {
        return false;
    }
    
    // Test if the format is supported
    PaStreamParameters inputParameters;
    inputParameters.device = index;
    inputParameters.channelCount = channelCount;
    inputParameters.sampleFormat = paFloat32;
    inputParameters.suggestedLatency = deviceInfo->defaultLowInputLatency;
    inputParameters.hostApiSpecificStreamInfo = nullptr;
    
    PaError err = Pa_IsFormatSupported(&inputParameters, nullptr, sampleRate);
    return err == paFormatIsSupported;
}

bool AudioDeviceManager::testDevice(PaDeviceIndex index, int channelCount, double sampleRate) const {
    if (!isDeviceValid(index)) {
        return false;
    }
    
    const PaDeviceInfo* deviceInfo = Pa_GetDeviceInfo(index);
    if (!deviceInfo || deviceInfo->maxInputChannels < channelCount) {
        return false;
    }
    
    // Try to open and immediately close a stream to test device availability
    PaStreamParameters inputParameters;
    inputParameters.device = index;
    inputParameters.channelCount = channelCount;
    inputParameters.sampleFormat = paFloat32;
    inputParameters.suggestedLatency = deviceInfo->defaultLowInputLatency;
    inputParameters.hostApiSpecificStreamInfo = nullptr;
    
    PaStream* testStream = nullptr;
    PaError err = Pa_OpenStream(&testStream, &inputParameters, nullptr, sampleRate, 64, paClipOff, nullptr, nullptr);
    
    if (err == paNoError && testStream) {
        Pa_CloseStream(testStream);
        return true;
    }
    
    return false;
}

void AudioDeviceManager::addDeviceChangeCallback(std::function<void(const std::vector<AudioDeviceInfo>&)> callback) {
    deviceChangeCallbacks.push_back(callback);
}

void AudioDeviceManager::removeAllCallbacks() {
    deviceChangeCallbacks.clear();
}

bool AudioDeviceManager::detectDeviceChanges() {
    if (!initialized) {
        return false;
    }
    
    std::vector<AudioDeviceInfo> oldDevices = devices;
    PaDeviceIndex currentDefaultInput = Pa_GetDefaultInputDevice();
    
    refreshDeviceList();
    
    bool hasChanges = !compareDeviceLists(oldDevices, devices) || 
                     (currentDefaultInput != lastDefaultInputDevice);
    
    if (hasChanges) {
        lastDefaultInputDevice = currentDefaultInput;
        notifyDeviceChange();
    }
    
    return hasChanges;
}

void AudioDeviceManager::notifyDeviceChange() {
    for (const auto& callback : deviceChangeCallbacks) {
        callback(devices);
    }
}

bool AudioDeviceManager::compareDeviceLists(const std::vector<AudioDeviceInfo>& oldList, 
                                           const std::vector<AudioDeviceInfo>& newList) const {
    if (oldList.size() != newList.size()) {
        return false;
    }
    
    for (size_t i = 0; i < oldList.size(); ++i) {
        if (oldList[i].index != newList[i].index ||
            oldList[i].name != newList[i].name ||
            oldList[i].isCurrentlyAvailable != newList[i].isCurrentlyAvailable) {
            return false;
        }
    }
    
    return true;
}

void AudioDeviceManager::printDeviceList() const {
    std::cout << "\n" << std::string(80, '=') << std::endl;
    std::cout << "AUDIO DEVICE LIST" << std::endl;
    std::cout << std::string(80, '=') << std::endl;
    
    if (devices.empty()) {
        std::cout << "No audio devices found." << std::endl;
        return;
    }
    
    std::cout << std::left << std::setw(4) << "ID" 
              << std::setw(25) << "Device Name"
              << std::setw(15) << "Host API"
              << std::setw(8) << "In Ch"
              << std::setw(8) << "Out Ch"
              << std::setw(10) << "Default"
              << std::setw(10) << "Available" << std::endl;
    std::cout << std::string(80, '-') << std::endl;
    
    for (const auto& device : devices) {
        std::cout << std::left << std::setw(4) << device.index
                  << std::setw(25) << (device.name.length() > 24 ? device.name.substr(0, 21) + "..." : device.name)
                  << std::setw(15) << device.hostApi
                  << std::setw(8) << device.maxInputChannels
                  << std::setw(8) << device.maxOutputChannels
                  << std::setw(10) << (device.isDefaultInput ? "INPUT" : (device.isDefaultOutput ? "OUTPUT" : ""))
                  << std::setw(10) << (device.isCurrentlyAvailable ? "YES" : "NO") << std::endl;
    }
    
    std::cout << std::string(80, '=') << std::endl;
}

void AudioDeviceManager::printDeviceDetails(PaDeviceIndex index) const {
    AudioDeviceInfo device = findDeviceByIndex(index);
    if (device.index == -1) {
        std::cout << "Device with index " << index << " not found." << std::endl;
        return;
    }
    
    std::cout << "\n" << std::string(60, '=') << std::endl;
    std::cout << "DEVICE DETAILS - Index " << index << std::endl;
    std::cout << std::string(60, '=') << std::endl;
    std::cout << "Name: " << device.name << std::endl;
    std::cout << "Host API: " << device.hostApi << std::endl;
    std::cout << "Max Input Channels: " << device.maxInputChannels << std::endl;
    std::cout << "Max Output Channels: " << device.maxOutputChannels << std::endl;
    std::cout << "Default Sample Rate: " << device.defaultSampleRate << " Hz" << std::endl;
    std::cout << "Default Input: " << (device.isDefaultInput ? "YES" : "NO") << std::endl;
    std::cout << "Default Output: " << (device.isDefaultOutput ? "YES" : "NO") << std::endl;
    std::cout << "Low Input Latency: " << formatLatency(device.defaultLowInputLatency) << std::endl;
    std::cout << "High Input Latency: " << formatLatency(device.defaultHighInputLatency) << std::endl;
    std::cout << "Currently Available: " << (device.isCurrentlyAvailable ? "YES" : "NO") << std::endl;
    
    // Test different channel counts
    std::cout << "\nChannel Support Test:" << std::endl;
    for (int channels = 1; channels <= std::min(8, device.maxInputChannels); ++channels) {
        bool supported = canDeviceHandleFormat(index, channels, 44100.0);
        std::cout << "  " << channels << " channel(s): " << (supported ? "SUPPORTED" : "NOT SUPPORTED") << std::endl;
    }
    
    std::cout << std::string(60, '=') << std::endl;
}

std::string AudioDeviceManager::getDeviceStatusReport() const {
    std::ostringstream report;
    
    report << "Audio Device Status Report\n";
    report << "==========================\n";
    report << "Total devices: " << devices.size() << "\n";
    
    auto inputDevices = getInputDevices();
    auto outputDevices = getOutputDevices();
    
    report << "Input devices: " << inputDevices.size() << "\n";
    report << "Output devices: " << outputDevices.size() << "\n";
    
    int availableDevices = 0;
    for (const auto& device : devices) {
        if (device.isCurrentlyAvailable) {
            availableDevices++;
        }
    }
    
    report << "Available devices: " << availableDevices << "\n";
    
    AudioDeviceInfo defaultInput = getDefaultInputDevice();
    if (defaultInput.index != -1) {
        report << "Default input: " << defaultInput.name << " (Index: " << defaultInput.index << ")\n";
    } else {
        report << "Default input: None\n";
    }
    
    AudioDeviceInfo defaultOutput = getDefaultOutputDevice();
    if (defaultOutput.index != -1) {
        report << "Default output: " << defaultOutput.name << " (Index: " << defaultOutput.index << ")\n";
    } else {
        report << "Default output: None\n";
    }
    
    return report.str();
}

std::string AudioDeviceManager::getHostApiName(PaHostApiIndex hostApi) {
    const PaHostApiInfo* hostApiInfo = Pa_GetHostApiInfo(hostApi);
    if (hostApiInfo && hostApiInfo->name) {
        return std::string(hostApiInfo->name);
    }
    return "Unknown";
}

std::string AudioDeviceManager::formatLatency(double latency) {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2) << (latency * 1000.0) << " ms";
    return oss.str();
}

bool AudioDeviceManager::isDeviceNameMatch(const std::string& deviceName, const std::string& searchName) {
    if (deviceName == searchName) {
        return true;
    }
    
    // Case-insensitive partial match
    std::string lowerDeviceName = deviceName;
    std::string lowerSearchName = searchName;
    
    std::transform(lowerDeviceName.begin(), lowerDeviceName.end(), lowerDeviceName.begin(), ::tolower);
    std::transform(lowerSearchName.begin(), lowerSearchName.end(), lowerSearchName.begin(), ::tolower);
    
    return lowerDeviceName.find(lowerSearchName) != std::string::npos;
}
