#pragma once

#include <string>
#include <vector>
#include <functional>
#include <lo/lo.h>
#include "OSCFormatManager.h"

/**
 * @brief OSC Receiver class for handling incoming OSC messages
 */
class OSCReceiver {
public:
    OSCReceiver(const std::string& port, std::shared_ptr<OSCFormatManager> formatManager = nullptr);
    ~OSCReceiver();
    
    // Server lifecycle
    bool start();
    void stop();
    bool isRunning() const { return running; }
    
    // Callbacks
    void setMessageCallback(std::function<void(const std::string&, const std::vector<float>&)> callback);
    void setStringCallback(std::function<void(const std::string&, const std::string&)> callback);
    void setIntCallback(std::function<void(const std::string&, int)> callback);
    
    // Learning mode
    void enableLearning(bool enable);
    
    // Status
    std::string getURL() const;
    std::string getPort() const { return port; }
    
private:
    lo_server_thread server;
    std::string port;
    std::shared_ptr<OSCFormatManager> formatManager;
    bool running;
    
    // Callbacks
    std::function<void(const std::string&, const std::vector<float>&)> messageCallback;
    std::function<void(const std::string&, const std::string&)> stringCallback;
    std::function<void(const std::string&, int)> intCallback;
    
    void setupHandlers();
    
    // Static callback functions for liblo
    static int floatHandler(const char* path, const char* types, lo_arg** argv, 
                           int argc, lo_message msg, void* user_data);
    static int stringHandler(const char* path, const char* types, lo_arg** argv,
                            int argc, lo_message msg, void* user_data);
    static int intHandler(const char* path, const char* types, lo_arg** argv,
                         int argc, lo_message msg, void* user_data);
    static int genericHandler(const char* path, const char* types, lo_arg** argv,
                             int argc, lo_message msg, void* user_data);
    static void errorHandler(int num, const char* msg, const char* path);
};
