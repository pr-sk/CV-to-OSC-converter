#include "OSCSender.h"
#include "ErrorHandler.h"
#include <stdexcept>
#include <sstream>
#include <iomanip>
#include <regex>

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

bool OSCSender::sendBlob(const std::string& address, const void* data, size_t size) {
    if (!target) return false;
    
    lo_blob blob = lo_blob_new(static_cast<int32_t>(size), data);
    int result = lo_send(target, address.c_str(), "b", blob);
    lo_blob_free(blob);
    
    return result >= 0;
}

bool OSCSender::sendValue(int channel, float value) {
    std::string address = formatAddress(channel);
    
    // Apply scale and offset
    float scaledValue = value * messageFormat.scale + messageFormat.offset;
    
    if (messageFormat.dataType == "float") {
        return sendFloat(address, scaledValue);
    } else if (messageFormat.dataType == "int") {
        return sendInt(address, static_cast<int>(scaledValue));
    } else if (messageFormat.dataType == "string") {
        std::string formattedValue = formatValue(scaledValue, messageFormat.stringFormat);
        return sendString(address, formattedValue);
    }
    
    return false;
}

bool OSCSender::sendFormattedValue(const std::string& addressTemplate, float value, const OSCMessageFormat& format) {
    // Apply scale and offset
    float scaledValue = value * format.scale + format.offset;
    
    if (format.dataType == "float") {
        return sendFloat(addressTemplate, scaledValue);
    } else if (format.dataType == "int") {
        return sendInt(addressTemplate, static_cast<int>(scaledValue));
    } else if (format.dataType == "string") {
        std::string formattedValue = formatValue(scaledValue, format.stringFormat);
        return sendString(addressTemplate, formattedValue);
    }
    
    return false;
}

bool OSCSender::sendMixedArray(const std::string& address, const std::vector<float>& floats, 
                              const std::vector<int>& ints, const std::vector<std::string>& strings) {
    if (!target) return false;
    
    lo_message message = lo_message_new();
    
    // Add all floats
    for (float value : floats) {
        lo_message_add_float(message, value);
    }
    
    // Add all ints
    for (int value : ints) {
        lo_message_add_int32(message, value);
    }
    
    // Add all strings
    for (const std::string& value : strings) {
        lo_message_add_string(message, value.c_str());
    }
    
    int result = lo_send_message(target, address.c_str(), message);
    lo_message_free(message);
    
    return result >= 0;
}

bool OSCSender::sendFormattedBatch(const std::vector<float>& values) {
    if (!target || values.empty()) return false;
    
    if (messageFormat.bundleMessages) {
        lo_bundle bundle = lo_bundle_new(LO_TT_IMMEDIATE);
        
        for (size_t i = 0; i < values.size(); ++i) {
            std::string address = formatAddress(static_cast<int>(i));
            float scaledValue = values[i] * messageFormat.scale + messageFormat.offset;
            
            lo_message message = lo_message_new();
            
            if (messageFormat.dataType == "float") {
                lo_message_add_float(message, scaledValue);
            } else if (messageFormat.dataType == "int") {
                lo_message_add_int32(message, static_cast<int>(scaledValue));
            } else if (messageFormat.dataType == "string") {
                std::string formattedValue = formatValue(scaledValue, messageFormat.stringFormat);
                lo_message_add_string(message, formattedValue.c_str());
            }
            
            lo_bundle_add_message(bundle, address.c_str(), message);
        }
        
        int result = lo_send_bundle(target, bundle);
        lo_bundle_free_recursive(bundle);
        
        return result >= 0;
    } else {
        // Send individual messages
        bool allSuccess = true;
        for (size_t i = 0; i < values.size(); ++i) {
            if (!sendValue(static_cast<int>(i), values[i])) {
                allSuccess = false;
            }
        }
        return allSuccess;
    }
}

std::string OSCSender::formatAddress(int channel) const {
    std::string address = messageFormat.addressPattern;
    
    // Replace {channel} with actual channel number (1-based)
    std::regex channelRegex("\\{channel\\}");
    address = std::regex_replace(address, channelRegex, std::to_string(channel + 1));
    
    return address;
}

std::string OSCSender::formatValue(float value, const std::string& format) const {
    std::ostringstream oss;
    
    // Simple format string parsing - supports {value:.Nf} format
    std::regex formatRegex("\\{value:?\\.?(\\d*)f?\\}");
    std::smatch match;
    
    if (std::regex_search(format, match, formatRegex)) {
        int precision = messageFormat.precision;
        if (match[1].length() > 0) {
            precision = std::stoi(match[1]);
        }
        oss << std::fixed << std::setprecision(precision) << value;
    } else {
        // Fallback to simple conversion
        oss << std::fixed << std::setprecision(messageFormat.precision) << value;
    }
    
    return oss.str();
}

void OSCSender::staticErrorHandler(int num, const char* msg, const char* path) {
    // This is a static callback, so we can't access instance members
    std::cerr << "OSC Error " << num << " in path " << (path ? path : "unknown") 
              << ": " << (msg ? msg : "unknown error") << std::endl;
}
