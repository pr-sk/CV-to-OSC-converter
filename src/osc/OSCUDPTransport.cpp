#include "OSCUDPTransport.h"
#include <iostream>
#include <sstream>

OSCUDPTransport::OSCUDPTransport() : target_(nullptr) {
}

OSCUDPTransport::~OSCUDPTransport() {
    disconnect();
}

bool OSCUDPTransport::connect(const std::string& host, const std::string& port) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (target_) {
        lo_address_free(target_);
        target_ = nullptr;
    }
    
    host_ = host;
    port_ = port;
    
    // Create UDP address
    target_ = lo_address_new(host.c_str(), port.c_str());
    
    if (!target_) {
        std::stringstream ss;
        ss << "Failed to create UDP OSC target: " << host << ":" << port;
        reportError(ss.str());
        return false;
    }
    
    return true;
}

bool OSCUDPTransport::disconnect() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (target_) {
        lo_address_free(target_);
        target_ = nullptr;
    }
    
    host_.clear();
    port_.clear();
    
    return true;
}

bool OSCUDPTransport::isConnected() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return (target_ != nullptr);
}

bool OSCUDPTransport::sendMessage(const std::string& address, void* msg) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!target_) {
        reportError("Not connected");
        return false;
    }
    
    lo_message message = static_cast<lo_message>(msg);
    int result = lo_send_message(target_, address.c_str(), message);
    
    if (result == -1) {
        reportError("Failed to send message to " + address);
        return false;
    }
    
    return true;
}

bool OSCUDPTransport::sendBundle(void* bundle) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!target_) {
        reportError("Not connected");
        return false;
    }
    
    lo_bundle bndl = static_cast<lo_bundle>(bundle);
    int result = lo_send_bundle(target_, bndl);
    
    if (result == -1) {
        reportError("Failed to send bundle");
        return false;
    }
    
    return true;
}

bool OSCUDPTransport::sendMessage(const std::string& address, const std::vector<float>& values) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!target_) {
        reportError("UDP transport not connected");
        return false;
    }
    
    if (values.empty()) {
        int result = lo_send(target_, address.c_str(), "");
        return result >= 0;
    }
    
    // Create message
    lo_message msg = lo_message_new();
    for (float value : values) {
        lo_message_add_float(msg, value);
    }
    
    int result = lo_send_message(target_, address.c_str(), msg);
    lo_message_free(msg);
    
    if (result < 0) {
        std::stringstream ss;
        ss << "Failed to send UDP message to " << address;
        reportError(ss.str());
        return false;
    }
    
    return true;
}

bool OSCUDPTransport::sendMessage(const std::string& address, const std::vector<int>& values) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!target_) {
        reportError("UDP transport not connected");
        return false;
    }
    
    if (values.empty()) {
        int result = lo_send(target_, address.c_str(), "");
        return result >= 0;
    }
    
    // Create message
    lo_message msg = lo_message_new();
    for (int value : values) {
        lo_message_add_int32(msg, value);
    }
    
    int result = lo_send_message(target_, address.c_str(), msg);
    lo_message_free(msg);
    
    if (result < 0) {
        std::stringstream ss;
        ss << "Failed to send UDP message to " << address;
        reportError(ss.str());
        return false;
    }
    
    return true;
}

bool OSCUDPTransport::sendMessage(const std::string& address, const std::string& value) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!target_) {
        reportError("UDP transport not connected");
        return false;
    }
    
    int result = lo_send(target_, address.c_str(), "s", value.c_str());
    
    if (result < 0) {
        std::stringstream ss;
        ss << "Failed to send UDP message to " << address;
        reportError(ss.str());
        return false;
    }
    
    return true;
}

bool OSCUDPTransport::sendBundle(const std::vector<std::pair<std::string, std::vector<float>>>& messages) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!target_) {
        reportError("UDP transport not connected");
        return false;
    }
    
    // Create bundle
    lo_bundle bundle = lo_bundle_new(LO_TT_IMMEDIATE);
    
    for (const auto& [address, values] : messages) {
        lo_message msg = lo_message_new();
        for (float value : values) {
            lo_message_add_float(msg, value);
        }
        lo_bundle_add_message(bundle, address.c_str(), msg);
    }
    
    int result = lo_send_bundle(target_, bundle);
    lo_bundle_free_recursive(bundle);
    
    if (result < 0) {
        reportError("Failed to send UDP bundle");
        return false;
    }
    
    return true;
}

void OSCUDPTransport::errorHandler(int num, const char* msg, const char* path) {
    std::cerr << "OSC UDP Error " << num << " in path " << (path ? path : "unknown") 
              << ": " << (msg ? msg : "unknown error") << std::endl;
}
