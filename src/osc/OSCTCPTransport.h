#pragma once

#include "OSCTransport.h"
#include <lo/lo.h>
#include <mutex>
#include <thread>
#include <atomic>

/**
 * @brief TCP transport implementation for OSC
 */
class OSCTCPTransport : public OSCTransport {
public:
    OSCTCPTransport();
    ~OSCTCPTransport() override;

    // Connection management
    bool connect(const std::string& host, const std::string& port) override;
    bool disconnect() override;
    bool isConnected() const override;

    // Sending methods for liblo integration
    bool sendMessage(const std::string& address, void* msg) override;
    bool sendBundle(void* bundle) override;
    
    // High-level sending methods
    bool sendMessage(const std::string& address, const std::vector<float>& values) override;
    bool sendMessage(const std::string& address, const std::vector<int>& values) override;
    bool sendMessage(const std::string& address, const std::string& value) override;
    bool sendBundle(const std::vector<std::pair<std::string, std::vector<float>>>& messages) override;

    // Protocol information
    Protocol getProtocol() const override { return Protocol::TCP; }
    std::string getProtocolName() const override { return "TCP"; }

    // Error handling
    std::string getLastError() const override { return lastError_; }
    void setErrorCallback(std::function<void(const std::string&)> callback) override {
        errorCallback_ = callback;
    }

    // TCP-specific options
    void setKeepAlive(bool enable);
    void setNoDelay(bool enable);
    void setConnectionTimeout(int seconds);
    void setReconnectDelay(int seconds);
    void setAutoReconnect(bool enable);

private:
    lo_address target_;
    std::string host_;
    std::string port_;
    mutable std::mutex mutex_;
    
    // TCP-specific state
    std::atomic<bool> connected_;
    std::atomic<bool> autoReconnect_;
    int connectionTimeout_;
    int reconnectDelay_;
    std::thread reconnectThread_;
    std::atomic<bool> stopReconnect_;
    
    // Connection management
    bool establishConnection();
    void reconnectLoop();
    void stopReconnectThread();
    
    static void errorHandler(int num, const char* msg, const char* path);
};
