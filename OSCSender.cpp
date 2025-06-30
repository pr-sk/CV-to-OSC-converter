#include "OSCSender.h"
#include "ErrorHandler.h"
#include <stdexcept>

OSCSender::OSCSender(const std::string& host, const std::string& port) 
    : host(host), port(port) {
    
    // Create liblo address
    target = lo_address_new(host.c_str(), port.c_str());
    
    if (!target) {
        std::string errorMsg = "Failed to create OSC target address";
        std::string details = "Host: " + host + ", Port: " + port;
        NETWORK_ERROR(errorMsg, details, true, "Check OSC target host and port settings");
        throw std::runtime_error(errorMsg);
    }
    
    // Set error handler - commented out as function may not be available
    // lo_address_set_error_handler(target, staticErrorHandler);
    
    ErrorHandler::getInstance().logInfo("OSC sender initialized", "Target: " + host + ":" + port);
    std::cout << "OSC sender initialized - target: " << host << ":" << port << std::endl;
}

OSCSender::~OSCSender() {
    if (target) {
        lo_address_free(target);
    }
}

bool OSCSender::sendFloat(const std::string& address, float value) {
    if (!target) {
        NETWORK_ERROR("OSC target not available", "Cannot send float message", true, "Reinitialize OSC sender");
        return false;
    }
    
    int result = lo_send(target, address.c_str(), "f", value);
    if (result < 0) {
        std::string errorMsg = "OSC message transmission failed";
        std::string details = "Address: " + address + ", Value: " + std::to_string(value) + 
                             ", Result: " + std::to_string(result);
        NETWORK_ERROR(errorMsg, details, true, "Check network connectivity and OSC target availability");
        return false;
    }
    
    return true;
}

bool OSCSender::sendInt(const std::string& address, int value) {
    if (!target) return false;
    
    int result = lo_send(target, address.c_str(), "i", value);
    return result >= 0;
}

bool OSCSender::sendString(const std::string& address, const std::string& value) {
    if (!target) return false;
    
    int result = lo_send(target, address.c_str(), "s", value.c_str());
    return result >= 0;
}

bool OSCSender::sendFloatArray(const std::string& address, const std::vector<float>& values) {
    if (!target || values.empty()) return false;
    
    // Create message
    lo_message message = lo_message_new();
    
    for (float value : values) {
        lo_message_add_float(message, value);
    }
    
    int result = lo_send_message(target, address.c_str(), message);
    lo_message_free(message);
    
    return result >= 0;
}

void OSCSender::setTarget(const std::string& newHost, const std::string& newPort) {
    if (target) {
        lo_address_free(target);
    }
    
    host = newHost;
    port = newPort;
    target = lo_address_new(host.c_str(), port.c_str());
    
    if (target) {
        // lo_address_set_error_handler(target, staticErrorHandler);
        std::cout << "OSC target updated: " << host << ":" << port << std::endl;
    }
}

void OSCSender::errorHandler(int num, const char* msg, const char* path) {
    std::cerr << "OSC Error " << num << " in path " << (path ? path : "unknown") 
              << ": " << (msg ? msg : "unknown error") << std::endl;
}

bool OSCSender::sendFloatBatch(const std::vector<std::string>& addresses, const std::vector<float>& values) {
    if (!target || addresses.size() != values.size()) return false;
    
    // Create a bundle for better performance and timing
    lo_bundle bundle = lo_bundle_new(LO_TT_IMMEDIATE);
    
    for (size_t i = 0; i < addresses.size(); ++i) {
        lo_message message = lo_message_new();
        lo_message_add_float(message, values[i]);
        lo_bundle_add_message(bundle, addresses[i].c_str(), message);
    }
    
    int result = lo_send_bundle(target, bundle);
    lo_bundle_free_recursive(bundle);
    
    return result >= 0;
}

void OSCSender::staticErrorHandler(int num, const char* msg, const char* path) {
    // This is a static callback, so we can't access instance members
    std::cerr << "OSC Error " << num << " in path " << (path ? path : "unknown") 
              << ": " << (msg ? msg : "unknown error") << std::endl;
}
