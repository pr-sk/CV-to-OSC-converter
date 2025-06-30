#pragma once

#include <string>
#include <memory>
#include <vector>
#include <iostream>
#include <lo/lo.h>

class OSCSender {
private:
    lo_address target;
    std::string host;
    std::string port;

public:
    OSCSender(const std::string& host, const std::string& port);
    ~OSCSender();
    
    bool sendFloat(const std::string& address, float value);
    bool sendInt(const std::string& address, int value);
    bool sendString(const std::string& address, const std::string& value);
    
    // Send multiple values at once
    bool sendFloatArray(const std::string& address, const std::vector<float>& values);
    
    // Batch sending for better performance
    bool sendFloatBatch(const std::vector<std::string>& addresses, const std::vector<float>& values);
    
    void setTarget(const std::string& host, const std::string& port);
    
private:
    void errorHandler(int num, const char* msg, const char* path);
    static void staticErrorHandler(int num, const char* msg, const char* path);
};
