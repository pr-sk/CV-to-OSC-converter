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
    
    // Check permissions first (but allow initialization to continue for listing devices)
    if (!checkPermissions()) {
        std::cout << "⚠️  Microphone permission not granted. Audio devices may not be available." << std::endl;
        std::cout << "   Run: ./cv_to_osc_converter --request-permissions" << std::endl;
        // Continue initialization to allow device listing
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
    // Use more aggressive testing for input devices
    if (info.maxInputChannels > 0) {
        // First try format test (safer)
        PaStreamParameters inputParams;
        inputParams.device = index;
        inputParams.channelCount = 1;
        inputParams.sampleFormat = paFloat32;
        inputParams.suggestedLatency = info.defaultLowInputLatency;
        inputParams.hostApiSpecificStreamInfo = nullptr;
        
        PaError formatErr = Pa_IsFormatSupported(&inputParams, nullptr, 44100.0);
        info.isCurrentlyAvailable = (formatErr == paFormatIsSupported);
    } else {
        info.isCurrentlyAvailable = false; // Output-only devices not useful for CV
    }
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
    if (!deviceInfo) {
        return false;
    }
    
    // For input devices, check if we have input channels
    if (channelCount > 0 && deviceInfo->maxInputChannels < channelCount) {
        return false;
    }
    
    // Don't try to open streams for devices with 0 input channels when we need input
    if (channelCount > 0 && deviceInfo->maxInputChannels == 0) {
        return false;
    }
    
    // For devices that should have input channels, try format check first (safer)
    if (deviceInfo->maxInputChannels > 0) {
        PaStreamParameters inputParameters;
        inputParameters.device = index;
        inputParameters.channelCount = std::min(channelCount, deviceInfo->maxInputChannels);
        inputParameters.sampleFormat = paFloat32;
        inputParameters.suggestedLatency = deviceInfo->defaultLowInputLatency;
        inputParameters.hostApiSpecificStreamInfo = nullptr;
        
        // Just check if format is supported (safer than opening stream)
        PaError err = Pa_IsFormatSupported(&inputParameters, nullptr, sampleRate);
        if (err == paFormatIsSupported) {
            return true;
        }
        
        // If format check fails, try with minimal parameters
        inputParameters.channelCount = 1;
        err = Pa_IsFormatSupported(&inputParameters, nullptr, 44100.0);
        return (err == paFormatIsSupported);
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

// Permission management methods
bool AudioDeviceManager::checkPermissions() {
    return MacOSPermissions::checkMicrophonePermission() == PermissionStatus::Granted;
}

void AudioDeviceManager::requestPermissions(std::function<void(bool)> callback) {
    std::cout << "🔐 Requesting microphone permissions..." << std::endl;
    MacOSPermissions::requestMicrophonePermission([this, callback](bool granted) {
        if (granted) {
            std::cout << "✅ Microphone permission granted! You can now use audio devices." << std::endl;
            // Refresh device list now that we have permissions
            if (initialized) {
                refreshDeviceList();
            }
        } else {
            std::cout << "❌ Microphone permission denied. Audio devices will not be available." << std::endl;
            std::cout << "To manually enable:" << std::endl;
            std::cout << "1. Open System Preferences > Security & Privacy > Privacy > Microphone" << std::endl;
            std::cout << "2. Check the box next to this application" << std::endl;
            std::cout << "3. Restart the application" << std::endl;
        }
        
        if (callback) {
            callback(granted);
        }
    });
}

PermissionStatus AudioDeviceManager::getPermissionStatus() {
    return MacOSPermissions::checkMicrophonePermission();
}

std::string AudioDeviceManager::getPermissionStatusMessage() {
    PermissionStatus status = getPermissionStatus();
    
    switch (status) {
        case PermissionStatus::Granted:
            return "✅ Microphone access granted";
        case PermissionStatus::Denied:
            return "❌ Microphone access denied - Enable in System Preferences > Security & Privacy > Privacy > Microphone";
        case PermissionStatus::NotDetermined:
            return "⚠️ Microphone permission not requested yet - Run with --request-permissions";
        case PermissionStatus::Restricted:
            return "🔒 Microphone access restricted by system policy";
        case PermissionStatus::Unknown:
        default:
            return "❓ Unknown microphone permission status";
    }
}

bool AudioDeviceManager::testDeviceWithPermissionCheck(PaDeviceIndex index, int channelCount, double sampleRate) const {
    // Simply test the device directly - if permissions are missing, the test will fail naturally
    return testDevice(index, channelCount, sampleRate);
}

bool AudioDeviceManager::forceTestDevice(PaDeviceIndex index) const {
    if (!isDeviceValid(index)) {
        return false;
    }
    
    const PaDeviceInfo* deviceInfo = Pa_GetDeviceInfo(index);
    if (!deviceInfo) {
        return false;
    }
    
    // For devices with no input channels, they're not useful for CV input
    if (deviceInfo->maxInputChannels == 0) {
        return false;
    }
    
    // Try to actually open a stream with minimal settings
    PaStreamParameters inputParameters;
    inputParameters.device = index;
    inputParameters.channelCount = 1; // Minimal channel count
    inputParameters.sampleFormat = paFloat32;
    inputParameters.suggestedLatency = deviceInfo->defaultLowInputLatency;
    inputParameters.hostApiSpecificStreamInfo = nullptr;
    
    PaStream* testStream = nullptr;
    
    // Try to open the stream
    PaError err = Pa_OpenStream(&testStream, &inputParameters, nullptr, 44100.0, 64, paClipOff, nullptr, nullptr);
    
    bool success = false;
    if (err == paNoError && testStream) {
        // Try to start the stream to really test it
        PaError startErr = Pa_StartStream(testStream);
        if (startErr == paNoError) {
            // Stream started successfully, stop it immediately
            Pa_StopStream(testStream);
            success = true;
        }
        Pa_CloseStream(testStream);
    }
    
    return success;
}

void AudioDeviceManager::runDetailedDiagnostics() const {
    std::cout << "\n" << std::string(80, '=') << std::endl;
    std::cout << "DETAILED AUDIO SYSTEM DIAGNOSTICS" << std::endl;
    std::cout << std::string(80, '=') << std::endl;
    
    // PortAudio info
    std::cout << "\n🔧 PortAudio Information:" << std::endl;
    std::cout << "  Version: " << Pa_GetVersionText() << std::endl;
    std::cout << "  Version Number: " << Pa_GetVersion() << std::endl;
    
    // Host APIs
    std::cout << "\n🖥️  Host APIs:" << std::endl;
    int hostApiCount = Pa_GetHostApiCount();
    for (int i = 0; i < hostApiCount; ++i) {
        const PaHostApiInfo* info = Pa_GetHostApiInfo(i);
        if (info) {
            std::cout << "  [" << i << "] " << info->name 
                      << " (" << info->deviceCount << " devices, default: " 
                      << info->defaultInputDevice << "/" << info->defaultOutputDevice << ")" << std::endl;
        }
    }
    
    // Permissions
    std::cout << "\n🔐 macOS Permissions:" << std::endl;
    PermissionStatus micStatus = MacOSPermissions::checkMicrophonePermission();
    std::cout << "  Microphone: " << MacOSPermissions::permissionStatusToString(micStatus) << std::endl;
    PermissionStatus fileStatus = MacOSPermissions::checkFilePermission();
    std::cout << "  File Access: " << MacOSPermissions::permissionStatusToString(fileStatus) << std::endl;
    
    // Test each device individually
    std::cout << "\n🎤 Device Testing Results:" << std::endl;
    for (const auto& device : devices) {
        if (device.maxInputChannels > 0) {
            std::cout << "\n  Device [" << device.index << "]: " << device.name << std::endl;
            
            // Test format support
            PaStreamParameters inputParams;
            inputParams.device = device.index;
            inputParams.channelCount = 1;
            inputParams.sampleFormat = paFloat32;
            inputParams.suggestedLatency = device.defaultLowInputLatency;
            inputParams.hostApiSpecificStreamInfo = nullptr;
            
            PaError formatErr = Pa_IsFormatSupported(&inputParams, nullptr, 44100.0);
            std::cout << "    Format Support (1ch, 44.1kHz): " 
                      << (formatErr == paFormatIsSupported ? "✅ YES" : "❌ NO") << std::endl;
            
            if (formatErr != paFormatIsSupported) {
                std::cout << "    Error: " << Pa_GetErrorText(formatErr) << std::endl;
            }
            
            // Try to open a stream
            PaStream* testStream = nullptr;
            PaError openErr = Pa_OpenStream(&testStream, &inputParams, nullptr, 44100.0, 64, paClipOff, nullptr, nullptr);
            
            if (openErr == paNoError && testStream) {
                std::cout << "    Stream Open: ✅ SUCCESS" << std::endl;
                
                // Try to start the stream
                PaError startErr = Pa_StartStream(testStream);
                if (startErr == paNoError) {
                    std::cout << "    Stream Start: ✅ SUCCESS" << std::endl;
                    Pa_StopStream(testStream);
                } else {
                    std::cout << "    Stream Start: ❌ FAILED (" << Pa_GetErrorText(startErr) << ")" << std::endl;
                }
                
                Pa_CloseStream(testStream);
            } else {
                std::cout << "    Stream Open: ❌ FAILED (" << Pa_GetErrorText(openErr) << ")" << std::endl;
            }
        }
    }
    
    // System info
    std::cout << "\n💻 System Information:" << std::endl;
    std::cout << "  App Bundle ID: " << MacOSPermissions::getBundleIdentifier() << std::endl;
    std::cout << "  App Name: " << MacOSPermissions::getAppName() << std::endl;
    std::cout << "  Sandboxed: " << (MacOSPermissions::isAppSandboxed() ? "Yes" : "No") << std::endl;
    
    std::cout << "\n" << std::string(80, '=') << std::endl;
    
    // Recommendations
    std::cout << "\n💡 Recommendations:" << std::endl;
    if (micStatus != PermissionStatus::Granted) {
        std::cout << "  ⚠️  Enable microphone permission in System Preferences" << std::endl;
    }
    
    int availableCount = 0;
    for (const auto& device : devices) {
        if (device.isCurrentlyAvailable && device.maxInputChannels > 0) {
            availableCount++;
        }
    }
    
    if (availableCount == 0) {
        std::cout << "  ⚠️  No input devices are currently available for CV reading" << std::endl;
        std::cout << "  🔧 Try: ./cv_to_osc_converter --request-permissions" << std::endl;
    } else {
        std::cout << "  ✅ Found " << availableCount << " available input device(s)" << std::endl;
    }
    
    std::cout << std::string(80, '=') << std::endl;
}
