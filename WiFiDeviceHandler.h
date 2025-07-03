#pragma once

#include "DeviceManager.h"
#include <map>
#include <thread>
#include <atomic>
#include <chrono>

// Cross-platform networking headers
#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "ws2_32.lib")
    typedef int socklen_t;
#else
    #include <netinet/in.h>
    #include <sys/socket.h>
    #include <arpa/inet.h>
    #include <unistd.h>
    #include <netdb.h>
#endif

/**
 * @brief WiFi Device Handler for network-based devices
 */
class WiFiDeviceHandler : public DeviceHandler {
public:
    WiFiDeviceHandler();
    ~WiFiDeviceHandler() override;
    
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
    
    DeviceStatus getDeviceStatus(const std::string& deviceId) override;
    std::string getLastError() const override;
    
    // WiFi-specific methods
    bool addManualDevice(const std::string& name, const std::string& ipAddress, int port);
    bool removeManualDevice(const std::string& deviceId);
    bool enableMulticast(const std::string& multicastGroup, int port);
    bool disableMulticast();
    
    // Network discovery
    void enableAutoDiscovery(bool enable);
    void setDiscoveryPorts(const std::vector<int>& ports);
    void setDiscoveryTimeout(int timeoutMs);
    
private:
    struct WiFiDeviceInfo {
        int socket;
        struct sockaddr_in address;
        DeviceStatus status;
        std::string name;
        int port;
        bool isManual;
        std::chrono::steady_clock::time_point lastPing;
    };
    
    std::map<std::string, WiFiDeviceInfo> wifiDevices_;
    std::mutex wifiMutex_;
    
    bool initialized_;
    std::atomic<bool> running_;
    std::atomic<bool> autoDiscoveryEnabled_;
    
    // Network discovery
    std::thread discoveryThread_;
    std::thread pingThread_;
    std::vector<int> discoveryPorts_;
    int discoveryTimeout_;
    
    // Multicast support
    bool multicastEnabled_;
    int multicastSocket_;
    std::string multicastGroup_;
    int multicastPort_;
    std::thread multicastReceiveThread_;
    
    // Server socket for incoming connections
    int serverSocket_;
    int serverPort_;
    std::thread serverThread_;
    
    void discoveryLoop();
    void pingLoop();
    void serverLoop();
    void multicastReceiveLoop();
    
    // Network utilities
    bool createSocket(const std::string& deviceId, const std::string& ipAddress, int port);
    bool pingDevice(const std::string& deviceId);
    std::vector<std::string> scanNetworkRange(const std::string& subnet, const std::vector<int>& ports);
    bool isPortOpen(const std::string& ipAddress, int port, int timeoutMs = 1000);
    std::string generateDeviceId(const std::string& ipAddress, int port);
    
    // Protocol handling
    void handleIncomingData(const std::string& deviceId, const std::vector<uint8_t>& data);
    bool sendTCPData(int socket, const std::vector<uint8_t>& data);
    bool sendUDPData(const std::string& deviceId, const std::vector<uint8_t>& data);
    
    // OSC over network
    void convertNetworkToOSC(const std::string& deviceId, const std::vector<uint8_t>& data);
    std::vector<uint8_t> convertOSCToNetwork(const std::string& address, float value);
};
