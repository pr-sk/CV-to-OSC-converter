#include "OSCTCPTransport.h"
#include <iostream>
#include <sstream>
#include <chrono>

OSCTCPTransport::OSCTCPTransport() 
    : target_(nullptr)
    , connected_(false)
    , autoReconnect_(false)
    , connectionTimeout_(5)
    , reconnectDelay_(5)
    , stopReconnect_(false) {
}

OSCTCPTransport::~OSCTCPTransport() {
    disconnect();
}

bool OSCTCPTransport::connect(const std::string& host, const std::string& port) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (target_) {
        lo_address_free(target_);
        target_ = nullptr;
    }
    
    host_ = host;
    port_ = port;
    
    // Stop any existing reconnect thread
    stopReconnectThread();
    
    // Attempt connection
    if (establishConnection()) {
        connected_ = true;
        
        // Start reconnect thread if auto-reconnect is enabled
        if (autoReconnect_) {
            stopReconnect_ = false;
            reconnectThread_ = std::thread(&OSCTCPTransport::reconnectLoop, this);
        }
        
        return true;
    }
    
    return false;
}

bool OSCTCPTransport::disconnect() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Stop reconnect thread
    stopReconnectThread();
    
    if (target_) {
        lo_address_free(target_);
        target_ = nullptr;
    }
    
    host_.clear();
    port_.clear();
    connected_ = false;
    
    return true;
}

bool OSCTCPTransport::isConnected() const {
    return connected_.load();
}

bool OSCTCPTransport::sendMessage(const std::string& address, void* msg) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!target_ || !connected_) {
        reportError("Not connected");
        return false;
    }
    
    lo_message message = static_cast<lo_message>(msg);
    int result = lo_send_message(target_, address.c_str(), message);
    
    if (result < 0) {
        connected_ = false;
        reportError("Failed to send message to " + address + " - connection lost");
        return false;
    }
    
    return true;
}

bool OSCTCPTransport::sendBundle(void* bundle) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!target_ || !connected_) {
        reportError("Not connected");
        return false;
    }
    
    lo_bundle bndl = static_cast<lo_bundle>(bundle);
    int result = lo_send_bundle(target_, bndl);
    
    if (result < 0) {
        connected_ = false;
        reportError("Failed to send bundle - connection lost");
        return false;
    }
    
    return true;
}

bool OSCTCPTransport::establishConnection() {
    // Create TCP address with protocol specifier
    std::string tcp_url = "osc.tcp://" + host_ + ":" + port_;
    target_ = lo_address_new_from_url(tcp_url.c_str());
    
    if (!target_) {
        std::stringstream ss;
        ss << "Failed to create TCP OSC target: " << host_ << ":" << port_;
        reportError(ss.str());
        return false;
    }
    
    // TCP protocol is already set by the URL format
    
    // Test connection by sending a ping message
    int result = lo_send(target_, "/ping", "s", "cv_to_osc_tcp_test");
    
    if (result < 0) {
        std::stringstream ss;
        ss << "Failed to establish TCP connection to " << host_ << ":" << port_;
        reportError(ss.str());
        lo_address_free(target_);
        target_ = nullptr;
        return false;
    }
    
    return true;
}

void OSCTCPTransport::reconnectLoop() {
    while (!stopReconnect_) {
        if (!connected_) {
            std::cout << "Attempting TCP reconnection to " << host_ << ":" << port_ << std::endl;
            
            {
                std::lock_guard<std::mutex> lock(mutex_);
                if (establishConnection()) {
                    connected_ = true;
                    std::cout << "TCP reconnection successful" << std::endl;
                }
            }
        }
        
        // Wait before next reconnect attempt
        std::this_thread::sleep_for(std::chrono::seconds(reconnectDelay_));
    }
}

void OSCTCPTransport::stopReconnectThread() {
    stopReconnect_ = true;
    if (reconnectThread_.joinable()) {
        reconnectThread_.join();
    }
}

bool OSCTCPTransport::sendMessage(const std::string& address, const std::vector<float>& values) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!target_ || !connected_) {
        reportError("TCP transport not connected");
        return false;
    }
    
    if (values.empty()) {
        int result = lo_send(target_, address.c_str(), "");
        if (result < 0) {
            connected_ = false;
            reportError("TCP connection lost");
            return false;
        }
        return true;
    }
    
    // Create message
    lo_message msg = lo_message_new();
    for (float value : values) {
        lo_message_add_float(msg, value);
    }
    
    int result = lo_send_message(target_, address.c_str(), msg);
    lo_message_free(msg);
    
    if (result < 0) {
        connected_ = false;
        std::stringstream ss;
        ss << "Failed to send TCP message to " << address << " - connection lost";
        reportError(ss.str());
        return false;
    }
    
    return true;
}

bool OSCTCPTransport::sendMessage(const std::string& address, const std::vector<int>& values) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!target_ || !connected_) {
        reportError("TCP transport not connected");
        return false;
    }
    
    if (values.empty()) {
        int result = lo_send(target_, address.c_str(), "");
        if (result < 0) {
            connected_ = false;
            reportError("TCP connection lost");
            return false;
        }
        return true;
    }
    
    // Create message
    lo_message msg = lo_message_new();
    for (int value : values) {
        lo_message_add_int32(msg, value);
    }
    
    int result = lo_send_message(target_, address.c_str(), msg);
    lo_message_free(msg);
    
    if (result < 0) {
        connected_ = false;
        std::stringstream ss;
        ss << "Failed to send TCP message to " << address << " - connection lost";
        reportError(ss.str());
        return false;
    }
    
    return true;
}

bool OSCTCPTransport::sendMessage(const std::string& address, const std::string& value) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!target_ || !connected_) {
        reportError("TCP transport not connected");
        return false;
    }
    
    int result = lo_send(target_, address.c_str(), "s", value.c_str());
    
    if (result < 0) {
        connected_ = false;
        std::stringstream ss;
        ss << "Failed to send TCP message to " << address << " - connection lost";
        reportError(ss.str());
        return false;
    }
    
    return true;
}

bool OSCTCPTransport::sendBundle(const std::vector<std::pair<std::string, std::vector<float>>>& messages) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!target_ || !connected_) {
        reportError("TCP transport not connected");
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
        connected_ = false;
        reportError("Failed to send TCP bundle - connection lost");
        return false;
    }
    
    return true;
}

void OSCTCPTransport::setKeepAlive(bool enable) {
    // TCP keep-alive would be set on the socket level
    // This requires access to the underlying socket which liblo doesn't expose directly
    // For now, we'll document this as a future enhancement
    (void)enable; // Suppress unused parameter warning
}

void OSCTCPTransport::setNoDelay(bool enable) {
    // TCP_NODELAY would be set on the socket level
    // This requires access to the underlying socket which liblo doesn't expose directly
    // For now, we'll document this as a future enhancement
    (void)enable; // Suppress unused parameter warning
}

void OSCTCPTransport::setConnectionTimeout(int seconds) {
    connectionTimeout_ = seconds;
}

void OSCTCPTransport::setReconnectDelay(int seconds) {
    reconnectDelay_ = seconds;
}

void OSCTCPTransport::setAutoReconnect(bool enable) {
    autoReconnect_ = enable;
    
    if (enable && connected_ && !reconnectThread_.joinable()) {
        stopReconnect_ = false;
        reconnectThread_ = std::thread(&OSCTCPTransport::reconnectLoop, this);
    } else if (!enable) {
        stopReconnectThread();
    }
}

void OSCTCPTransport::errorHandler(int num, const char* msg, const char* path) {
    std::cerr << "OSC TCP Error " << num << " in path " << (path ? path : "unknown") 
              << ": " << (msg ? msg : "unknown error") << std::endl;
}
