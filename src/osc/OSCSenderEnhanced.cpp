#include "OSCSenderEnhanced.h"
#include "OSCTCPTransport.h"
#include <chrono>
#include <sstream>
#include <lo/lo.h>

OSCSenderEnhanced::OSCSenderEnhanced() 
    : currentProtocol_(OSCTransport::Protocol::UDP)
    , stats_{}
{
    stats_.lastActivity = std::chrono::steady_clock::now();
}

OSCSenderEnhanced::~OSCSenderEnhanced() {
    disconnect();
}

bool OSCSenderEnhanced::connect(const std::string& host, const std::string& port, OSCTransport::Protocol protocol) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Disconnect existing connection
    if (transport_ && transport_->isConnected()) {
        transport_->disconnect();
    }
    
    host_ = host;
    port_ = port;
    currentProtocol_ = protocol;
    
    // Create new transport
    if (!createTransport(protocol)) {
        return false;
    }
    
    // Connect
    bool result = transport_->connect(host, port);
    if (!result) {
        updateStats(false);
        if (errorCallback_) {
            errorCallback_("Failed to connect to " + host + ":" + port + " using " + getProtocolName());
        }
    }
    
    return result;
}

bool OSCSenderEnhanced::disconnect() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (transport_) {
        return transport_->disconnect();
    }
    return true;
}

bool OSCSenderEnhanced::isConnected() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return transport_ && transport_->isConnected();
}

void OSCSenderEnhanced::setProtocol(OSCTransport::Protocol protocol) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (protocol == currentProtocol_) {
        return;
    }
    
    currentProtocol_ = protocol;
    
    // If connected, reconnect with new protocol
    if (transport_ && transport_->isConnected()) {
        transport_->disconnect();
        createTransport(protocol);
        transport_->connect(host_, port_);
    }
}

OSCTransport::Protocol OSCSenderEnhanced::getProtocol() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return currentProtocol_;
}

std::string OSCSenderEnhanced::getProtocolName() const {
    std::lock_guard<std::mutex> lock(mutex_);
    switch (currentProtocol_) {
        case OSCTransport::Protocol::UDP:
            return "UDP";
        case OSCTransport::Protocol::TCP:
            return "TCP";
        default:
            return "Unknown";
    }
}

bool OSCSenderEnhanced::sendFloat(const std::string& address, float value) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!transport_ || !transport_->isConnected()) {
        updateStats(false);
        if (errorCallback_) {
            errorCallback_("Not connected");
        }
        return false;
    }
    
    // Create OSC message
    lo_message msg = lo_message_new();
    lo_message_add_float(msg, value);
    
    bool result = transport_->sendMessage(address, static_cast<void*>(msg));
    
    lo_message_free(msg);
    
    // Estimate size: address + type tag + float (4 bytes)
    size_t estimatedSize = address.length() + 1 + 4;
    updateStats(result, estimatedSize);
    
    if (!result && errorCallback_) {
        errorCallback_("Failed to send float to " + address);
    }
    
    return result;
}

bool OSCSenderEnhanced::sendInt(const std::string& address, int value) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!transport_ || !transport_->isConnected()) {
        updateStats(false);
        if (errorCallback_) {
            errorCallback_("Not connected");
        }
        return false;
    }
    
    lo_message msg = lo_message_new();
    lo_message_add_int32(msg, value);
    
    bool result = transport_->sendMessage(address, static_cast<void*>(msg));
    
    lo_message_free(msg);
    
    // Estimate size: address + type tag + int32 (4 bytes)
    size_t estimatedSize = address.length() + 1 + 4;
    updateStats(result, estimatedSize);
    
    if (!result && errorCallback_) {
        errorCallback_("Failed to send int to " + address);
    }
    
    return result;
}

bool OSCSenderEnhanced::sendString(const std::string& address, const std::string& value) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!transport_ || !transport_->isConnected()) {
        updateStats(false);
        if (errorCallback_) {
            errorCallback_("Not connected");
        }
        return false;
    }
    
    lo_message msg = lo_message_new();
    lo_message_add_string(msg, value.c_str());
    
    bool result = transport_->sendMessage(address, static_cast<void*>(msg));
    
    lo_message_free(msg);
    
    // Estimate size: address + type tag + string length + padding
    size_t estimatedSize = address.length() + 1 + value.length() + 4;
    updateStats(result, estimatedSize);
    
    if (!result && errorCallback_) {
        errorCallback_("Failed to send string to " + address);
    }
    
    return result;
}

bool OSCSenderEnhanced::sendFloatArray(const std::string& address, const std::vector<float>& values) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!transport_ || !transport_->isConnected()) {
        updateStats(false);
        if (errorCallback_) {
            errorCallback_("Not connected");
        }
        return false;
    }
    
    lo_message msg = lo_message_new();
    
    for (float value : values) {
        lo_message_add_float(msg, value);
    }
    
    bool result = transport_->sendMessage(address, static_cast<void*>(msg));
    
    lo_message_free(msg);
    
    // Estimate size: address + type tags + floats
    size_t estimatedSize = address.length() + values.size() + (values.size() * 4);
    updateStats(result, estimatedSize);
    
    if (!result && errorCallback_) {
        errorCallback_("Failed to send float array to " + address);
    }
    
    return result;
}

bool OSCSenderEnhanced::sendFloatBatch(const std::vector<std::string>& addresses, const std::vector<float>& values) {
    if (addresses.size() != values.size()) {
        if (errorCallback_) {
            errorCallback_("Batch send: addresses and values count mismatch");
        }
        return false;
    }
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!transport_ || !transport_->isConnected()) {
        updateStats(false);
        if (errorCallback_) {
            errorCallback_("Not connected");
        }
        return false;
    }
    
    // Create bundle for atomic batch sending
    lo_bundle bundle = lo_bundle_new(LO_TT_IMMEDIATE);
    
    size_t totalSize = 0;
    for (size_t i = 0; i < addresses.size(); ++i) {
        lo_message msg = lo_message_new();
        lo_message_add_float(msg, values[i]);
        lo_bundle_add_message(bundle, addresses[i].c_str(), msg);
        totalSize += addresses[i].length() + 1 + 4;
    }
    
    bool result = transport_->sendBundle(static_cast<void*>(bundle));
    
    lo_bundle_free_recursive(bundle);
    
    updateStats(result, totalSize);
    
    if (!result && errorCallback_) {
        errorCallback_("Failed to send float batch");
    }
    
    return result;
}

void OSCSenderEnhanced::setAutoReconnect(bool enable) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (transport_) {
        auto tcp = dynamic_cast<OSCTCPTransport*>(transport_.get());
        if (tcp) {
            tcp->setAutoReconnect(enable);
        }
    }
}

void OSCSenderEnhanced::setReconnectDelay(int seconds) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (transport_) {
        auto tcp = dynamic_cast<OSCTCPTransport*>(transport_.get());
        if (tcp) {
            tcp->setReconnectDelay(seconds);
        }
    }
}

void OSCSenderEnhanced::setConnectionTimeout(int seconds) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (transport_) {
        auto tcp = dynamic_cast<OSCTCPTransport*>(transport_.get());
        if (tcp) {
            tcp->setConnectionTimeout(seconds);
        }
    }
}

std::string OSCSenderEnhanced::getLastError() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (transport_) {
        return transport_->getLastError();
    }
    return "No transport initialized";
}

void OSCSenderEnhanced::setErrorCallback(std::function<void(const std::string&)> callback) {
    std::lock_guard<std::mutex> lock(mutex_);
    errorCallback_ = callback;
    
    if (transport_) {
        transport_->setErrorCallback(callback);
    }
}

void OSCSenderEnhanced::resetStatistics() {
    std::lock_guard<std::mutex> lock(mutex_);
    stats_ = Statistics{};
    stats_.lastActivity = std::chrono::steady_clock::now();
}

bool OSCSenderEnhanced::createTransport(OSCTransport::Protocol protocol) {
    transport_ = OSCTransportFactory::create(protocol);
    
    if (!transport_) {
        if (errorCallback_) {
            errorCallback_("Failed to create transport for protocol: " + getProtocolName());
        }
        return false;
    }
    
    // Pass error callback to transport
    if (errorCallback_) {
        transport_->setErrorCallback(errorCallback_);
    }
    
    return true;
}

void OSCSenderEnhanced::updateStats(bool success, size_t bytesEstimate) {
    if (success) {
        stats_.messagesSent++;
        stats_.bytesSent += bytesEstimate;
    } else {
        stats_.errors++;
    }
    
    stats_.lastActivity = std::chrono::steady_clock::now();
    
    // Simple moving average for latency (placeholder - actual implementation would measure round-trip)
    // This is a simplified version - real latency measurement would require timestamps
}
