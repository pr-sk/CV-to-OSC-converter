#pragma once

#include "OSCTransport.h"
#include <lo/lo.h>
#include <mutex>

/**
 * @brief UDP transport implementation for OSC
 */
class OSCUDPTransport : public OSCTransport {
public:
    OSCUDPTransport();
    ~OSCUDPTransport() override;

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
    Protocol getProtocol() const override { return Protocol::UDP; }
    std::string getProtocolName() const override { return "UDP"; }

    // Error handling
    std::string getLastError() const override { return lastError_; }
    void setErrorCallback(std::function<void(const std::string&)> callback) override {
        errorCallback_ = callback;
    }

private:
    lo_address target_;
    std::string host_;
    std::string port_;
    mutable std::mutex mutex_;
    
    static void errorHandler(int num, const char* msg, const char* path);
};
