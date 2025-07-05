#pragma once

#include <string>
#include <vector>
#include <memory>
#include <functional>

/**
 * @brief Base interface for OSC transport protocols
 */
class OSCTransport {
public:
    enum class Protocol {
        UDP,
        TCP,
        MULTICAST,
        BROADCAST
    };

    virtual ~OSCTransport() = default;

    // Connection management
    virtual bool connect(const std::string& host, const std::string& port) = 0;
    virtual bool disconnect() = 0;
    virtual bool isConnected() const = 0;

    // Sending methods for liblo integration
    virtual bool sendMessage(const std::string& address, void* msg) = 0;  // lo_message type
    virtual bool sendBundle(void* bundle) = 0;  // lo_bundle type
    
    // High-level sending methods (optional override)
    virtual bool sendMessage(const std::string& address, const std::vector<float>& values) = 0;
    virtual bool sendMessage(const std::string& address, const std::vector<int>& values) = 0;
    virtual bool sendMessage(const std::string& address, const std::string& value) = 0;
    virtual bool sendBundle(const std::vector<std::pair<std::string, std::vector<float>>>& messages) = 0;

    // Protocol information
    virtual Protocol getProtocol() const = 0;
    virtual std::string getProtocolName() const = 0;

    // Error handling
    virtual std::string getLastError() const = 0;
    virtual void setErrorCallback(std::function<void(const std::string&)> callback) = 0;

protected:
    std::function<void(const std::string&)> errorCallback_;
    std::string lastError_;

    void reportError(const std::string& error) {
        lastError_ = error;
        if (errorCallback_) {
            errorCallback_(error);
        }
    }
};

/**
 * @brief Factory for creating OSC transport instances
 */
class OSCTransportFactory {
public:
    static std::unique_ptr<OSCTransport> create(OSCTransport::Protocol protocol);
    static std::vector<OSCTransport::Protocol> getSupportedProtocols();
};
