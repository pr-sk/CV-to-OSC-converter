#pragma once

#include "AudioDeviceManager.h"
#include "OSCMixerTypes.h"
#include "RealAudioStream.h"
#include <functional>
#include <memory>
#ifdef __APPLE__
#include "../platform/MacOSPermissions.h"
#endif

class AudioDeviceIntegration {
public:
    AudioDeviceIntegration();
    ~AudioDeviceIntegration();
    
    // Initialize with audio device manager
    bool initialize(AudioDeviceManager* deviceManager);
    void shutdown();
    
    // Audio device enumeration for OSC mixer
    std::vector<OSCDeviceConfig> getAvailableInputDevices() const;
    std::vector<OSCDeviceConfig> getAvailableOutputDevices() const;
    
    // Convert audio devices to OSC device configs
    OSCDeviceConfig createInputDeviceConfig(const AudioDeviceInfo& audioDevice) const;
    OSCDeviceConfig createOutputDeviceConfig(const AudioDeviceInfo& audioDevice) const;
    
    // Audio device callbacks
    void setDeviceChangeCallback(std::function<void(const std::vector<OSCDeviceConfig>&)> callback);
    
    // Validate that an OSC device config corresponds to a real audio device
    bool validateAudioDevice(const OSCDeviceConfig& device) const;
    
    // Get audio device info from OSC device config
    AudioDeviceInfo getAudioDeviceInfo(const OSCDeviceConfig& device) const;
    
    // Get input sample from audio device
    float getInputSample(const std::string& deviceId) const;
    
    // Send output sample to audio device
    bool sendOutputSample(const std::string& deviceId, float sample);
    
    // Create audio stream for a device
    bool createAudioInputStream(const std::string& deviceId, int deviceIndex);
    bool createAudioOutputStream(const std::string& deviceId, int deviceIndex);
    
    // Create duplex stream for routing audio from input to output
    bool createAudioRouting(const std::string& inputDeviceId, const std::string& outputDeviceId);
    
    // Remove audio stream for a device
    void removeAudioStream(const std::string& deviceId);
    
private:
    // Signal processing and auto-detection
    float processAndConvertAudioSignal(float rawSample, const std::string& deviceId) const;
    AudioDeviceManager* audioDeviceManager_;
    std::function<void(const std::vector<OSCDeviceConfig>&)> deviceChangeCallback_;
    bool initialized_;
    std::unique_ptr<RealAudioStreamManager> streamManager_;
    
    // Internal helpers
    std::string generateDeviceId(const AudioDeviceInfo& device, bool isInput) const;
    OSCDeviceConfig createDeviceConfig(const AudioDeviceInfo& device, bool isInput) const;
    void onAudioDeviceChange(const std::vector<AudioDeviceInfo>& devices);
};
