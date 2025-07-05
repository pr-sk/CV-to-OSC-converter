#pragma once

#include <memory>
#include <vector>
#include <string>
#include <map>
#include <functional>
#include <thread>
#include <atomic>
#include <mutex>

// Forward declarations
class OSCSender;
class OSCReceiver;

/**
 * @brief Types of supported devices
 */
enum class DeviceType {
    BLUETOOTH,
    WIFI,
    USB,
    MIDI,
    UNKNOWN
};

/**
 * @brief Device connection status
 */
enum class DeviceConnectionState {
    DISCONNECTED,
    CONNECTING,
    CONNECTED,
    ERROR,
    TIMEOUT
};
/**
 * @brief Device information structure
 */
struct DeviceInfo {
    std::string id;
    std::string name;
    DeviceType type;
    DeviceConnectionState status;
    std::string address;  // IP, MAC, USB path, etc.
    int port = 0;
    std::map<std::string, std::string> properties;
    
    // OSC routing
    std::string oscInputAddress = "";   // Where to send OSC data from this device
    std::string oscOutputAddress = "";  // Where to receive OSC data for this device
    bool bidirectional = true;
    
    DeviceInfo() : type(DeviceType::UNKNOWN), status(DeviceConnectionState::DISCONNECTED), port(0), bidirectional(true) {}
    
    DeviceInfo(const std::string& deviceId, const std::string& deviceName, DeviceType deviceType)
        : id(deviceId), name(deviceName), type(deviceType), status(DeviceConnectionState::DISCONNECTED), port(0), bidirectional(true) {}
};

/**
 * @brief Base class for device handlers
 */
class DeviceHandler {
public:
    virtual ~DeviceHandler() = default;
    
    // Device lifecycle
    virtual bool initialize() = 0;
    virtual bool connect(const DeviceInfo& device) = 0;
    virtual bool disconnect(const std::string& deviceId) = 0;
    virtual void shutdown() = 0;
    
    // Device discovery
    virtual std::vector<DeviceInfo> scanForDevices() = 0;
    virtual bool isDeviceAvailable(const std::string& deviceId) = 0;
    
    // Data handling
    virtual bool sendData(const std::string& deviceId, const std::vector<uint8_t>& data) = 0;
    virtual void setDataCallback(std::function<void(const std::string&, const std::vector<uint8_t>&)> callback) = 0;
    
    // OSC integration
    virtual bool sendOSCMessage(const std::string& deviceId, const std::string& address, float value) = 0;
    virtual void setOSCCallback(std::function<void(const std::string&, const std::string&, float)> callback) = 0;
    
    // Status
    virtual DeviceConnectionState getDeviceStatus(const std::string& deviceId) = 0;
    virtual std::string getLastError() const = 0;
    
protected:
    std::function<void(const std::string&, const std::vector<uint8_t>&)> dataCallback_;
    std::function<void(const std::string&, const std::string&, float)> oscCallback_;
    std::string lastError_;
};

/**
 * @brief Main device manager class
 */
class DeviceManager {
public:
    DeviceManager();
    ~DeviceManager();
    
    // Initialization
    bool initialize();
    void shutdown();
    
    // Device discovery and connection
    std::vector<DeviceInfo> scanAllDevices();
    std::vector<DeviceInfo> getConnectedDevices() const;
    std::vector<DeviceInfo> getAvailableDevices() const;
    
    bool connectDevice(const std::string& deviceId);
    bool disconnectDevice(const std::string& deviceId);
    bool isDeviceConnected(const std::string& deviceId) const;
    
    // Device configuration
    bool configureDevice(const std::string& deviceId, const std::map<std::string, std::string>& config);
    DeviceInfo getDeviceInfo(const std::string& deviceId) const;
    bool updateDeviceInfo(const DeviceInfo& device);
    
    // OSC routing
    bool setDeviceOSCRouting(const std::string& deviceId, const std::string& inputAddress, const std::string& outputAddress);
    bool sendOSCToDevice(const std::string& deviceId, const std::string& address, float value);
    bool sendOSCToAllDevices(const std::string& address, float value);
    
    // Data communication
    bool sendDataToDevice(const std::string& deviceId, const std::vector<uint8_t>& data);
    bool broadcastData(const std::vector<uint8_t>& data);
    
    // Callbacks
    void setDeviceEventCallback(std::function<void(const std::string&, DeviceConnectionState)> callback);
    void setDataReceivedCallback(std::function<void(const std::string&, const std::vector<uint8_t>&)> callback);
    void setOSCReceivedCallback(std::function<void(const std::string&, const std::string&, float)> callback);
    
    // Status and monitoring
    DeviceConnectionState getDeviceStatus(const std::string& deviceId) const;
    std::map<std::string, DeviceConnectionState> getAllDeviceStatuses() const;
    std::string getLastError() const;
    
    // Auto-discovery and reconnection
    void enableAutoDiscovery(bool enable);
    void enableAutoReconnect(bool enable);
    void setDiscoveryInterval(int intervalMs);
    
private:
    // Device handlers for each type
    std::unique_ptr<DeviceHandler> bluetoothHandler_;
    std::unique_ptr<DeviceHandler> wifiHandler_;
    std::unique_ptr<DeviceHandler> usbHandler_;
#ifdef __APPLE__
    std::unique_ptr<DeviceHandler> midiHandler_;
#endif
    
    // Device registry
    std::map<std::string, DeviceInfo> devices_;
    std::map<std::string, DeviceInfo> connectedDevices_;
    mutable std::mutex devicesMutex_;
    
    // OSC integration
    std::unique_ptr<OSCSender> oscSender_;
    std::unique_ptr<OSCReceiver> oscReceiver_;
    
    // Auto-discovery
    std::atomic<bool> autoDiscoveryEnabled_;
    std::atomic<bool> autoReconnectEnabled_;
    std::atomic<bool> running_;
    std::thread discoveryThread_;
    int discoveryInterval_;
    
    // Callbacks
    std::function<void(const std::string&, DeviceConnectionState)> deviceEventCallback_;
    std::function<void(const std::string&, const std::vector<uint8_t>&)> dataReceivedCallback_;
    std::function<void(const std::string&, const std::string&, float)> oscReceivedCallback_;
    
    std::string lastError_;
    
    // Internal methods
    void discoveryLoop();
    void handleDeviceData(const std::string& deviceId, const std::vector<uint8_t>& data);
    void handleOSCData(const std::string& deviceId, const std::string& address, float value);
    void updateDeviceStatus(const std::string& deviceId, DeviceConnectionState status);
    DeviceHandler* getHandlerForType(DeviceType type);
    DeviceType getDeviceType(const std::string& deviceId) const;
};
