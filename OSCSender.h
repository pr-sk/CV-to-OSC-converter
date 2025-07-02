#pragma once

#include <string>
#include <memory>
#include <vector>
#include <iostream>
#include <lo/lo.h>

// OSC Message formatting options
struct OSCMessageFormat {
    std::string addressPattern = "/cv/channel/{channel}";
    std::string dataType = "float"; // "float", "int", "string", "blob"
    float scale = 1.0f;
    float offset = 0.0f;
    int precision = 6; // for string conversion
    bool bundleMessages = true;
    std::string stringFormat = "{value:.3f}"; // for string type
};

class OSCSender {
private:
    lo_address target;
    std::string host;
    std::string port;
    OSCMessageFormat messageFormat;
    
public:
    OSCSender(const std::string& host, const std::string& port);
    ~OSCSender();
    
    // Basic sending methods
    bool sendFloat(const std::string& address, float value);
    bool sendInt(const std::string& address, int value);
    bool sendString(const std::string& address, const std::string& value);
    bool sendBlob(const std::string& address, const void* data, size_t size);
    
    // Enhanced sending with custom formatting
    bool sendValue(int channel, float value); // Uses current format
    bool sendFormattedValue(const std::string& addressTemplate, float value, const OSCMessageFormat& format);
    
    // Send multiple values at once
    bool sendFloatArray(const std::string& address, const std::vector<float>& values);
    bool sendMixedArray(const std::string& address, const std::vector<float>& floats, 
                       const std::vector<int>& ints, const std::vector<std::string>& strings);
    
    // Batch sending for better performance
    bool sendFloatBatch(const std::vector<std::string>& addresses, const std::vector<float>& values);
    bool sendFormattedBatch(const std::vector<float>& values); // Uses current format for all channels
    
    // Configuration
    void setTarget(const std::string& host, const std::string& port);
    void setMessageFormat(const OSCMessageFormat& format) { messageFormat = format; }
    const OSCMessageFormat& getMessageFormat() const { return messageFormat; }
    
    // Utility methods
    std::string formatAddress(int channel) const;
    std::string formatValue(float value, const std::string& format) const;
    
private:
    void errorHandler(int num, const char* msg, const char* path);
    static void staticErrorHandler(int num, const char* msg, const char* path);
};
