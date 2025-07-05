#pragma once

#include "DeviceManager.h"
#include <map>
#include <thread>

#ifdef __APPLE__
#include <CoreMIDI/CoreMIDI.h>
#else
// Placeholder definitions for non-macOS platforms
typedef uint32_t MIDIEndpointRef;
typedef uint32_t MIDIPortRef;
typedef uint32_t MIDIClientRef;
struct MIDIPacketList {};
struct MIDINotification {};
#endif

/**
 * @brief MIDI Device Handler for macOS using CoreMIDI
 */
class MidiDeviceHandler : public DeviceHandler {
public:
    MidiDeviceHandler();
    ~MidiDeviceHandler() override;
    
    // DeviceHandler interface
    bool initialize() override;
    bool connect(const DeviceInfo& device) override;
    bool disconnect(const std::string& deviceId) override;
    void shutdown() override;
    
    std::vector<DeviceInfo> scanForDevices() override;
    bool isDeviceAvailable(const std::string& deviceId) override;
    
    bool sendData(const std::string& deviceId, const std::vector<uint8_t>& data) override;
    void setDataCallback(std::function<void(const std::string&, const std::vector<uint8_t>&)> callback) override;
    
    bool sendOSCMessage(const std::string& deviceId, const std::string& address, float value) override;
    void setOSCCallback(std::function<void(const std::string&, const std::string&, float)> callback) override;
    
    DeviceConnectionState getDeviceStatus(const std::string& deviceId) override;
    std::string getLastError() const override;
    
    // MIDI-specific methods
    bool sendMIDICC(const std::string& deviceId, int channel, int cc, int value);
    bool sendMIDINote(const std::string& deviceId, int channel, int note, int velocity, bool noteOn = true);
    bool sendMIDIPitchBend(const std::string& deviceId, int channel, int value);
    bool sendMIDISysEx(const std::string& deviceId, const std::vector<uint8_t>& sysex);
    
    // MIDI learning mode
    void enableLearningMode(bool enable);
    bool isLearningMode() const { return learningMode_; }
    
private:
    struct MidiDeviceInfo {
        MIDIEndpointRef inputEndpoint;
        MIDIEndpointRef outputEndpoint;
        MIDIPortRef inputPort;
        MIDIPortRef outputPort;
        DeviceConnectionState status;
        std::string name;
    };
    
    MIDIClientRef midiClient_;
    std::map<std::string, MidiDeviceInfo> midiDevices_;
    std::mutex midiMutex_;
    
    bool initialized_;
    bool learningMode_;
    std::string lastError_;
    
    // Callbacks
    std::function<void(const std::string&, const std::vector<uint8_t>&)> dataCallback_;
    std::function<void(const std::string&, const std::string&, float)> oscCallback_;
    
#ifdef __APPLE__
    // macOS-specific callback functions
    static void midiInputCallback(const MIDIPacketList* pktlist, void* readProcRefCon, void* srcConnRefCon);
    static void midiNotifyCallback(const MIDINotification* message, void* refCon);
    
    void handleMidiInput(const MIDIPacketList* pktlist, void* srcConnRefCon);
    void handleMidiNotification(const MIDINotification* message);
    
    // Helper methods
    std::string getEndpointName(MIDIEndpointRef endpoint);
    std::string generateDeviceId(MIDIEndpointRef endpoint);
    bool createMidiPorts(const std::string& deviceId, MIDIEndpointRef inputEndpoint, MIDIEndpointRef outputEndpoint);
    void convertMidiToOSC(const std::string& deviceId, const std::vector<uint8_t>& midiData);
    std::vector<uint8_t> convertOSCToMidi(const std::string& address, float value);
#endif // __APPLE__
};
