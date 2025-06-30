#pragma once

#include <vector>
#include <string>
#include <memory>
#include <functional>
#include <portaudio.h>

struct AudioDeviceInfo {
    PaDeviceIndex index;
    std::string name;
    std::string hostApi;
    int maxInputChannels;
    int maxOutputChannels;
    double defaultSampleRate;
    bool isDefaultInput;
    bool isDefaultOutput;
    double defaultLowInputLatency;
    double defaultHighInputLatency;
    bool isCurrentlyAvailable;
    
    AudioDeviceInfo() : index(-1), maxInputChannels(0), maxOutputChannels(0), 
                       defaultSampleRate(44100.0), isDefaultInput(false), 
                       isDefaultOutput(false), defaultLowInputLatency(0.0), 
                       defaultHighInputLatency(0.0), isCurrentlyAvailable(false) {}
};

class AudioDeviceManager {
private:
    std::vector<AudioDeviceInfo> devices;
    std::vector<std::function<void(const std::vector<AudioDeviceInfo>&)>> deviceChangeCallbacks;
    bool initialized;
    PaDeviceIndex lastDefaultInputDevice;
    
public:
    AudioDeviceManager();
    ~AudioDeviceManager();
    
    // Core functionality
    bool initialize();
    void cleanup();
    void refreshDeviceList();
    
    // Device enumeration
    const std::vector<AudioDeviceInfo>& getDevices() const { return devices; }
    std::vector<AudioDeviceInfo> getInputDevices() const;
    std::vector<AudioDeviceInfo> getOutputDevices() const;
    AudioDeviceInfo getDefaultInputDevice() const;
    AudioDeviceInfo getDefaultOutputDevice() const;
    
    // Device search
    AudioDeviceInfo findDeviceByName(const std::string& name) const;
    AudioDeviceInfo findDeviceByIndex(PaDeviceIndex index) const;
    std::vector<AudioDeviceInfo> findDevicesContaining(const std::string& searchTerm) const;
    
    // Device validation
    bool isDeviceValid(PaDeviceIndex index) const;
    bool canDeviceHandleFormat(PaDeviceIndex index, int channelCount, double sampleRate) const;
    bool testDevice(PaDeviceIndex index, int channelCount = 2, double sampleRate = 44100.0) const;
    
    // Monitoring and callbacks
    void addDeviceChangeCallback(std::function<void(const std::vector<AudioDeviceInfo>&)> callback);
    void removeAllCallbacks();
    bool detectDeviceChanges(); // Returns true if changes detected
    
    // Information and diagnostics
    void printDeviceList() const;
    void printDeviceDetails(PaDeviceIndex index) const;
    std::string getDeviceStatusReport() const;
    
    // Utility functions
    static std::string getHostApiName(PaHostApiIndex hostApi);
    static std::string formatLatency(double latency);
    static bool isDeviceNameMatch(const std::string& deviceName, const std::string& searchName);
    
private:
    void populateDeviceInfo(AudioDeviceInfo& info, PaDeviceIndex index);
    void notifyDeviceChange();
    bool compareDeviceLists(const std::vector<AudioDeviceInfo>& oldList, 
                           const std::vector<AudioDeviceInfo>& newList) const;
};
