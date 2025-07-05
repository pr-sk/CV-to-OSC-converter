#pragma once

#include <string>
#include <memory>
#include <vector>
#include <mutex>
#include "OSCTransport.h"
#include "OSCFormatManager.h"

/**
 * @brief Enhanced OSC sender with multi-protocol support
 */
class OSCSenderEnhanced {
public:
    OSCSenderEnhanced();
    ~OSCSenderEnhanced();

    // Connection management
    bool connect(const std::string& host, const std::string& port, OSCTransport::Protocol protocol = OSCTransport::Protocol::UDP);
    bool disconnect();
    bool isConnected() const;
    
    // Protocol management
    void setProtocol(OSCTransport::Protocol protocol);
    OSCTransport::Protocol getProtocol() const;
    std::string getProtocolName() const;
    
    // Basic sending methods
    bool sendFloat(const std::string& address, float value);
    bool sendInt(const std::string& address, int value);
    bool sendString(const std::string& address, const std::string& value);
    bool sendFloatArray(const std::string& address, const std::vector<float>& values);
    
    // Batch sending
    bool sendFloatBatch(const std::vector<std::string>& addresses, const std::vector<float>& values);
    
    // TCP-specific options
    void setAutoReconnect(bool enable);
    void setReconnectDelay(int seconds);
    void setConnectionTimeout(int seconds);
    
    // Error handling
    std::string getLastError() const;
    void setErrorCallback(std::function<void(const std::string&)> callback);
    
    // Statistics
    struct Statistics {
        uint64_t messagesSent = 0;
        uint64_t bytesSent = 0;
        uint64_t errors = 0;
        float averageLatency = 0.0f;
        std::chrono::steady_clock::time_point lastActivity;
    };
    
    Statistics getStatistics() const { return stats_; }
    void resetStatistics();

private:
    std::unique_ptr<OSCTransport> transport_;
    OSCTransport::Protocol currentProtocol_;
    std::string host_;
    std::string port_;
    
    // Thread safety
    mutable std::mutex mutex_;
    
    // Statistics
    Statistics stats_;
    
    // Error callback
    std::function<void(const std::string&)> errorCallback_;
    
    // Transport creation
    bool createTransport(OSCTransport::Protocol protocol);
    
    // Update statistics
    void updateStats(bool success, size_t bytesEstimate = 0);
};
