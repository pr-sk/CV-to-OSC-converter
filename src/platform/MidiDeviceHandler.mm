#include "MidiDeviceHandler.h"
#include <iostream>
#include <sstream>

#ifdef __APPLE__
#include <CoreFoundation/CoreFoundation.h>

MidiDeviceHandler::MidiDeviceHandler() 
    : midiClient_(0), initialized_(false), learningMode_(false) {
}

MidiDeviceHandler::~MidiDeviceHandler() {
    shutdown();
}

bool MidiDeviceHandler::initialize() {
    if (initialized_) {
        return true;
    }
    
    OSStatus result = MIDIClientCreate(CFSTR("CV_OSC_Converter"), midiNotifyCallback, this, &midiClient_);
    if (result != noErr) {
        lastError_ = "Failed to create MIDI client: " + std::to_string(result);
        return false;
    }
    
    initialized_ = true;
    std::cout << "MIDI Device Handler initialized successfully" << std::endl;
    return true;
}

void MidiDeviceHandler::shutdown() {
    if (!initialized_) {
        return;
    }
    
    std::lock_guard<std::mutex> lock(midiMutex_);
    
    // Disconnect all devices
    for (auto& [deviceId, midiDevice] : midiDevices_) {
        if (midiDevice.inputPort != 0) {
            MIDIPortDispose(midiDevice.inputPort);
        }
        if (midiDevice.outputPort != 0) {
            MIDIPortDispose(midiDevice.outputPort);
        }
    }
    midiDevices_.clear();
    
    if (midiClient_ != 0) {
        MIDIClientDispose(midiClient_);
        midiClient_ = 0;
    }
    
    initialized_ = false;
    std::cout << "MIDI Device Handler shutdown complete" << std::endl;
}

std::vector<DeviceInfo> MidiDeviceHandler::scanForDevices() {
    std::vector<DeviceInfo> devices;
    
    if (!initialized_) {
        return devices;
    }
    
    // Scan MIDI sources (input devices)
    ItemCount sourceCount = MIDIGetNumberOfSources();
    for (ItemCount i = 0; i < sourceCount; ++i) {
        MIDIEndpointRef source = MIDIGetSource(i);
        if (source != 0) {
            std::string name = getEndpointName(source);
            std::string deviceId = generateDeviceId(source);
            
            DeviceInfo device(deviceId, name, DeviceType::MIDI);
            device.address = std::to_string(source);
            device.properties["endpoint_type"] = "source";
            device.properties["endpoint_ref"] = std::to_string(source);
            
            devices.push_back(device);
        }
    }
    
    // Scan MIDI destinations (output devices)
    ItemCount destCount = MIDIGetNumberOfDestinations();
    for (ItemCount i = 0; i < destCount; ++i) {
        MIDIEndpointRef dest = MIDIGetDestination(i);
        if (dest != 0) {
            std::string name = getEndpointName(dest);
            std::string deviceId = generateDeviceId(dest);
            
            // Check if we already have this device from sources
            bool found = false;
            for (auto& existingDevice : devices) {
                if (existingDevice.id == deviceId) {
                    existingDevice.properties["has_output"] = "true";
                    existingDevice.properties["output_endpoint"] = std::to_string(dest);
                    found = true;
                    break;
                }
            }
            
            if (!found) {
                DeviceInfo device(deviceId, name, DeviceType::MIDI);
                device.address = std::to_string(dest);
                device.properties["endpoint_type"] = "destination";
                device.properties["endpoint_ref"] = std::to_string(dest);
                
                devices.push_back(device);
            }
        }
    }
    
    return devices;
}

bool MidiDeviceHandler::connect(const DeviceInfo& device) {
    if (!initialized_) {
        lastError_ = "MIDI handler not initialized";
        return false;
    }
    
    std::lock_guard<std::mutex> lock(midiMutex_);
    
    // Check if already connected
    if (midiDevices_.find(device.id) != midiDevices_.end()) {
        return true;
    }
    
    MidiDeviceInfo midiDevice;
    midiDevice.name = device.name;
    midiDevice.status = DeviceConnectionState::CONNECTING;
    
    // Get endpoint references
    MIDIEndpointRef inputEndpoint = 0;
    MIDIEndpointRef outputEndpoint = 0;
    
    auto endpointRefIt = device.properties.find("endpoint_ref");
    if (endpointRefIt != device.properties.end()) {
        MIDIEndpointRef endpoint = std::stoul(endpointRefIt->second);
        
        auto endpointTypeIt = device.properties.find("endpoint_type");
        if (endpointTypeIt != device.properties.end()) {
            if (endpointTypeIt->second == "source") {
                inputEndpoint = endpoint;
            } else if (endpointTypeIt->second == "destination") {
                outputEndpoint = endpoint;
            }
        }
    }
    
    // Check for output endpoint
    auto outputEndpointIt = device.properties.find("output_endpoint");
    if (outputEndpointIt != device.properties.end()) {
        outputEndpoint = std::stoul(outputEndpointIt->second);
    }
    
    // Create MIDI ports
    if (!createMidiPorts(device.id, inputEndpoint, outputEndpoint)) {
        lastError_ = "Failed to create MIDI ports for device: " + device.name;
        return false;
    }
    
    midiDevice.inputEndpoint = inputEndpoint;
    midiDevice.outputEndpoint = outputEndpoint;
    midiDevice.status = DeviceConnectionState::CONNECTED;
    
    midiDevices_[device.id] = midiDevice;
    
    std::cout << "MIDI device connected: " << device.name << " (" << device.id << ")" << std::endl;
    return true;
}

bool MidiDeviceHandler::disconnect(const std::string& deviceId) {
    std::lock_guard<std::mutex> lock(midiMutex_);
    
    auto it = midiDevices_.find(deviceId);
    if (it == midiDevices_.end()) {
        return true; // Not connected
    }
    
    auto& midiDevice = it->second;
    
    if (midiDevice.inputPort != 0) {
        MIDIPortDispose(midiDevice.inputPort);
    }
    if (midiDevice.outputPort != 0) {
        MIDIPortDispose(midiDevice.outputPort);
    }
    
    midiDevices_.erase(it);
    
    std::cout << "MIDI device disconnected: " << deviceId << std::endl;
    return true;
}

bool MidiDeviceHandler::isDeviceAvailable(const std::string& deviceId) {
    // Check if device is still available in the system
    auto devices = scanForDevices();
    for (const auto& device : devices) {
        if (device.id == deviceId) {
            return true;
        }
    }
    return false;
}

bool MidiDeviceHandler::sendData(const std::string& deviceId, const std::vector<uint8_t>& data) {
    std::lock_guard<std::mutex> lock(midiMutex_);
    
    auto it = midiDevices_.find(deviceId);
    if (it == midiDevices_.end() || it->second.outputPort == 0) {
        lastError_ = "Device not connected or no output port: " + deviceId;
        return false;
    }
    
    auto& midiDevice = it->second;
    
    // Create MIDI packet
    MIDIPacketList packetList;
    MIDIPacket* packet = MIDIPacketListInit(&packetList);
    
    packet = MIDIPacketListAdd(&packetList, sizeof(packetList), packet, 0, 
                              data.size(), data.data());
    
    if (packet == nullptr) {
        lastError_ = "Failed to create MIDI packet";
        return false;
    }
    
    OSStatus result = MIDISend(midiDevice.outputPort, midiDevice.outputEndpoint, &packetList);
    if (result != noErr) {
        lastError_ = "Failed to send MIDI data: " + std::to_string(result);
        return false;
    }
    
    return true;
}

bool MidiDeviceHandler::sendOSCMessage(const std::string& deviceId, const std::string& address, float value) {
    // Convert OSC message to MIDI data
    auto midiData = convertOSCToMidi(address, value);
    if (midiData.empty()) {
        lastError_ = "Failed to convert OSC to MIDI";
        return false;
    }
    
    return sendData(deviceId, midiData);
}

DeviceConnectionState MidiDeviceHandler::getDeviceStatus(const std::string& deviceId) {
    std::lock_guard<std::mutex> lock(midiMutex_);
    
    auto it = midiDevices_.find(deviceId);
    if (it != midiDevices_.end()) {
        return it->second.status;
    }
    
    return DeviceConnectionState::DISCONNECTED;
}

void MidiDeviceHandler::setDataCallback(std::function<void(const std::string&, const std::vector<uint8_t>&)> callback) {
    dataCallback_ = callback;
}

void MidiDeviceHandler::setOSCCallback(std::function<void(const std::string&, const std::string&, float)> callback) {
    oscCallback_ = callback;
}

std::string MidiDeviceHandler::getLastError() const {
    return lastError_;
}

// Static callback functions
void MidiDeviceHandler::midiInputCallback(const MIDIPacketList* pktlist, void* readProcRefCon, void* srcConnRefCon) {
    MidiDeviceHandler* handler = static_cast<MidiDeviceHandler*>(readProcRefCon);
    if (handler) {
        handler->handleMidiInput(pktlist, srcConnRefCon);
    }
}

void MidiDeviceHandler::midiNotifyCallback(const MIDINotification* message, void* refCon) {
    MidiDeviceHandler* handler = static_cast<MidiDeviceHandler*>(refCon);
    if (handler) {
        handler->handleMidiNotification(message);
    }
}

void MidiDeviceHandler::handleMidiInput(const MIDIPacketList* pktlist, void* srcConnRefCon) {
    std::lock_guard<std::mutex> lock(midiMutex_);
    
    // Find device ID from source connection
    std::string deviceId;
    for (const auto& [id, midiDevice] : midiDevices_) {
        if (reinterpret_cast<void*>(midiDevice.inputEndpoint) == srcConnRefCon) {
            deviceId = id;
            break;
        }
    }
    
    if (deviceId.empty()) {
        return;
    }
    
    const MIDIPacket* packet = &pktlist->packet[0];
    for (UInt32 i = 0; i < pktlist->numPackets; ++i) {
        std::vector<uint8_t> data(packet->data, packet->data + packet->length);
        
        // Forward raw MIDI data
        if (dataCallback_) {
            dataCallback_(deviceId, data);
        }
        
        // Convert to OSC and forward
        convertMidiToOSC(deviceId, data);
        
        packet = MIDIPacketNext(packet);
    }
}

void MidiDeviceHandler::handleMidiNotification(const MIDINotification* message) {
    // Handle MIDI system notifications (device add/remove, etc.)
    switch (message->messageID) {
        case kMIDIMsgObjectAdded:
        case kMIDIMsgObjectRemoved:
            // Could trigger device re-scan
            break;
        default:
            break;
    }
}

// Helper methods
std::string MidiDeviceHandler::getEndpointName(MIDIEndpointRef endpoint) {
    CFStringRef name = nullptr;
    OSStatus result = MIDIObjectGetStringProperty(endpoint, kMIDIPropertyName, &name);
    
    if (result == noErr && name != nullptr) {
        char buffer[256];
        if (CFStringGetCString(name, buffer, sizeof(buffer), kCFStringEncodingUTF8)) {
            CFRelease(name);
            return std::string(buffer);
        }
        CFRelease(name);
    }
    
    return "Unknown MIDI Device";
}

std::string MidiDeviceHandler::generateDeviceId(MIDIEndpointRef endpoint) {
    return "midi_" + std::to_string(endpoint);
}

bool MidiDeviceHandler::createMidiPorts(const std::string& deviceId, MIDIEndpointRef inputEndpoint, MIDIEndpointRef outputEndpoint) {
    auto& midiDevice = midiDevices_[deviceId];
    
    // Create input port if we have an input endpoint
    if (inputEndpoint != 0) {
        std::string inputPortName = "Input_" + deviceId;
        OSStatus result = MIDIInputPortCreate(midiClient_, 
                                            CFStringCreateWithCString(nullptr, inputPortName.c_str(), kCFStringEncodingUTF8),
                                            midiInputCallback, this, &midiDevice.inputPort);
        
        if (result != noErr) {
            lastError_ = "Failed to create MIDI input port: " + std::to_string(result);
            return false;
        }
        
        // Connect to source
        result = MIDIPortConnectSource(midiDevice.inputPort, inputEndpoint, 
                                     reinterpret_cast<void*>(inputEndpoint));
        if (result != noErr) {
            lastError_ = "Failed to connect to MIDI source: " + std::to_string(result);
            return false;
        }
    }
    
    // Create output port if we have an output endpoint
    if (outputEndpoint != 0) {
        std::string outputPortName = "Output_" + deviceId;
        OSStatus result = MIDIOutputPortCreate(midiClient_,
                                             CFStringCreateWithCString(nullptr, outputPortName.c_str(), kCFStringEncodingUTF8),
                                             &midiDevice.outputPort);
        
        if (result != noErr) {
            lastError_ = "Failed to create MIDI output port: " + std::to_string(result);
            return false;
        }
    }
    
    return true;
}

void MidiDeviceHandler::convertMidiToOSC(const std::string& deviceId, const std::vector<uint8_t>& midiData) {
    if (midiData.empty() || !oscCallback_) {
        return;
    }
    
    // Basic MIDI to OSC conversion
    uint8_t status = midiData[0];
    uint8_t channel = status & 0x0F;
    uint8_t command = status & 0xF0;
    
    std::string address;
    float value = 0.0f;
    
    switch (command) {
        case 0x90: // Note On
            if (midiData.size() >= 3) {
                address = "/midi/note/" + std::to_string(channel + 1);
                value = static_cast<float>(midiData[1]) / 127.0f; // Note number normalized
            }
            break;
            
        case 0x80: // Note Off
            if (midiData.size() >= 3) {
                address = "/midi/note/" + std::to_string(channel + 1);
                value = 0.0f; // Note off
            }
            break;
            
        case 0xB0: // Control Change
            if (midiData.size() >= 3) {
                address = "/midi/cc/" + std::to_string(channel + 1) + "/" + std::to_string(midiData[1]);
                value = static_cast<float>(midiData[2]) / 127.0f;
            }
            break;
            
        case 0xE0: // Pitch Bend
            if (midiData.size() >= 3) {
                address = "/midi/pitchbend/" + std::to_string(channel + 1);
                int pitchValue = (midiData[2] << 7) | midiData[1];
                value = (static_cast<float>(pitchValue) - 8192.0f) / 8192.0f; // Normalized to -1 to +1
            }
            break;
            
        default:
            return; // Unsupported command
    }
    
    if (!address.empty()) {
        oscCallback_(deviceId, address, value);
    }
}

std::vector<uint8_t> MidiDeviceHandler::convertOSCToMidi(const std::string& address, float value) {
    std::vector<uint8_t> midiData;
    
    // Parse OSC address to determine MIDI message type
    if (address.find("/midi/cc/") == 0) {
        // Control Change: /midi/cc/channel/cc_number
        size_t channelStart = address.find('/', 9);
        size_t ccStart = address.find('/', channelStart + 1);
        
        if (channelStart != std::string::npos && ccStart != std::string::npos) {
            int channel = std::stoi(address.substr(9, channelStart - 9)) - 1; // Convert to 0-based
            int ccNumber = std::stoi(address.substr(channelStart + 1, ccStart - channelStart - 1));
            
            if (channel >= 0 && channel < 16 && ccNumber >= 0 && ccNumber < 128) {
                int ccValue = static_cast<int>(value * 127.0f);
                ccValue = std::clamp(ccValue, 0, 127);
                
                midiData.push_back(0xB0 | channel); // Control Change + channel
                midiData.push_back(ccNumber);
                midiData.push_back(ccValue);
            }
        }
    }
    else if (address.find("/midi/note/") == 0) {
        // Note: /midi/note/channel
        int channel = std::stoi(address.substr(11)) - 1; // Convert to 0-based
        
        if (channel >= 0 && channel < 16) {
            int note = static_cast<int>(value * 127.0f);
            note = std::clamp(note, 0, 127);
            
            if (value > 0.0f) {
                midiData.push_back(0x90 | channel); // Note On + channel
                midiData.push_back(note);
                midiData.push_back(100); // Default velocity
            } else {
                midiData.push_back(0x80 | channel); // Note Off + channel
                midiData.push_back(note);
                midiData.push_back(0);
            }
        }
    }
    
    return midiData;
}

#else // Non-Apple platforms

// Stub implementations for non-macOS platforms
MidiDeviceHandler::MidiDeviceHandler() 
    : initialized_(false), learningMode_(false) {
}

MidiDeviceHandler::~MidiDeviceHandler() {
}

bool MidiDeviceHandler::initialize() {
    lastError_ = "MIDI support not available on this platform";
    return false;
}

void MidiDeviceHandler::shutdown() {
}

std::vector<DeviceInfo> MidiDeviceHandler::scanForDevices() {
    return {};
}

bool MidiDeviceHandler::connect(const DeviceInfo& device) {
    lastError_ = "MIDI support not available on this platform";
    return false;
}

bool MidiDeviceHandler::disconnect(const std::string& deviceId) {
    return true;
}

bool MidiDeviceHandler::isDeviceAvailable(const std::string& deviceId) {
    return false;
}

bool MidiDeviceHandler::sendData(const std::string& deviceId, const std::vector<uint8_t>& data) {
    lastError_ = "MIDI support not available on this platform";
    return false;
}

bool MidiDeviceHandler::sendOSCMessage(const std::string& deviceId, const std::string& address, float value) {
    lastError_ = "MIDI support not available on this platform";
    return false;
}

DeviceConnectionState MidiDeviceHandler::getDeviceStatus(const std::string& deviceId) {
    return DeviceConnectionState::DISCONNECTED;
}

void MidiDeviceHandler::setDataCallback(std::function<void(const std::string&, const std::vector<uint8_t>&)> callback) {
    dataCallback_ = callback;
}

void MidiDeviceHandler::setOSCCallback(std::function<void(const std::string&, const std::string&, float)> callback) {
    oscCallback_ = callback;
}

std::string MidiDeviceHandler::getLastError() const {
    return lastError_;
}

// MIDI-specific stub methods
bool MidiDeviceHandler::sendMIDICC(const std::string& deviceId, int channel, int cc, int value) {
    lastError_ = "MIDI support not available on this platform";
    return false;
}

bool MidiDeviceHandler::sendMIDINote(const std::string& deviceId, int channel, int note, int velocity, bool noteOn) {
    lastError_ = "MIDI support not available on this platform";
    return false;
}

bool MidiDeviceHandler::sendMIDIPitchBend(const std::string& deviceId, int channel, int value) {
    lastError_ = "MIDI support not available on this platform";
    return false;
}

bool MidiDeviceHandler::sendMIDISysEx(const std::string& deviceId, const std::vector<uint8_t>& sysex) {
    lastError_ = "MIDI support not available on this platform";
    return false;
}

void MidiDeviceHandler::enableLearningMode(bool enable) {
    learningMode_ = enable;
}

#endif // __APPLE__
