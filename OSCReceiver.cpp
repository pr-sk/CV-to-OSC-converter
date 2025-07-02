#include "OSCSender.h"
#include "ErrorHandler.h"
#include <iostream>

OSCReceiver::OSCReceiver(const std::string& port, std::shared_ptr<OSCFormatManager> formatManager)
    : server(nullptr), port(port), formatManager(formatManager), running(false) {
}

OSCReceiver::~OSCReceiver() {
    stop();
}

bool OSCReceiver::start() {
    if (running) {
        return true; // Already running
    }
    
    try {
        server = lo_server_thread_new(port.c_str(), errorHandler);
        if (!server) {
            ERROR_ERROR("Failed to create OSC server", "Port: " + port, 
                       "Check if port is available", false);
            return false;
        }
        
        setupHandlers();
        
        if (lo_server_thread_start(server) < 0) {
            ERROR_ERROR("Failed to start OSC server thread", "Port: " + port,
                       "Check system resources", false);
            lo_server_thread_free(server);
            server = nullptr;
            return false;
        }
        
        running = true;
        ERROR_INFO("OSC receiver started", "Listening on port " + port);
        return true;
        
    } catch (const std::exception& e) {
        ERROR_ERROR("Exception starting OSC receiver", e.what(),
                   "Check liblo installation", false);
        return false;
    }
}

void OSCReceiver::stop() {
    if (!running || !server) {
        return;
    }
    
    lo_server_thread_stop(server);
    lo_server_thread_free(server);
    server = nullptr;
    running = false;
    
    ERROR_INFO("OSC receiver stopped", "Port " + port + " released");
}

void OSCReceiver::setMessageCallback(std::function<void(const std::string&, const std::vector<float>&)> callback) {
    messageCallback = callback;
}

void OSCReceiver::setStringCallback(std::function<void(const std::string&, const std::string&)> callback) {
    stringCallback = callback;
}

void OSCReceiver::setIntCallback(std::function<void(const std::string&, int)> callback) {
    intCallback = callback;
}

void OSCReceiver::enableLearning(bool enable) {
    if (formatManager) {
        formatManager->setLearningMode(enable);
    }
}

std::string OSCReceiver::getURL() const {
    if (server) {
        char* url = lo_server_thread_get_url(server);
        std::string result(url);
        free(url);
        return result;
    }
    return "osc://localhost:" + port + "/";
}

void OSCReceiver::setupHandlers() {
    if (!server) return;
    
    // Add generic handlers for different data types
    lo_server_thread_add_method(server, nullptr, "f", floatHandler, this);
    lo_server_thread_add_method(server, nullptr, "i", intHandler, this);
    lo_server_thread_add_method(server, nullptr, "s", stringHandler, this);
    lo_server_thread_add_method(server, nullptr, nullptr, genericHandler, this);
}

int OSCReceiver::floatHandler(const char* path, const char* types, lo_arg** argv, 
                             int argc, lo_message msg, void* user_data) {
    (void)msg; // Suppress unused parameter warning
    OSCReceiver* receiver = static_cast<OSCReceiver*>(user_data);
    
    std::vector<float> values;
    for (int i = 0; i < argc; ++i) {
        if (types[i] == 'f') {
            values.push_back(argv[i]->f);
        } else if (types[i] == 'i') {
            values.push_back(static_cast<float>(argv[i]->i));
        }
    }
    
    // Forward to learning system if enabled
    if (receiver->formatManager && receiver->formatManager->isLearningMode()) {
        receiver->formatManager->learnOSCMessage(path, values);
    }
    
    // Forward to callback
    if (receiver->messageCallback && !values.empty()) {
        receiver->messageCallback(path, values);
    }
    
    receiver->formatManager->recordMessageReceived(path);
    return 0;
}

int OSCReceiver::stringHandler(const char* path, const char* types, lo_arg** argv, 
                              int argc, lo_message msg, void* user_data) {
    (void)msg; // Suppress unused parameter warning
    OSCReceiver* receiver = static_cast<OSCReceiver*>(user_data);
    
    if (argc > 0 && types[0] == 's') {
        std::string value(&argv[0]->s);
        if (receiver->stringCallback) {
            receiver->stringCallback(path, value);
        }
    }
    
    if (receiver->formatManager) {
        receiver->formatManager->recordMessageReceived(path);
    }
    return 0;
}

int OSCReceiver::intHandler(const char* path, const char* types, lo_arg** argv, 
                           int argc, lo_message msg, void* user_data) {
    (void)msg; // Suppress unused parameter warning
    OSCReceiver* receiver = static_cast<OSCReceiver*>(user_data);
    
    if (argc > 0 && types[0] == 'i') {
        int value = argv[0]->i;
        if (receiver->intCallback) {
            receiver->intCallback(path, value);
        }
    }
    
    if (receiver->formatManager) {
        receiver->formatManager->recordMessageReceived(path);
    }
    return 0;
}

int OSCReceiver::genericHandler(const char* path, const char* types, lo_arg** argv, 
                               int argc, lo_message msg, void* user_data) {
    (void)msg; // Suppress unused parameter warning
    OSCReceiver* receiver = static_cast<OSCReceiver*>(user_data);
    
    // Handle mixed-type messages
    std::vector<float> floatValues;
    for (int i = 0; i < argc; ++i) {
        switch (types[i]) {
            case 'f':
                floatValues.push_back(argv[i]->f);
                break;
            case 'i':
                floatValues.push_back(static_cast<float>(argv[i]->i));
                break;
            case 'd':
                floatValues.push_back(static_cast<float>(argv[i]->d));
                break;
        }
    }
    
    // Forward to learning system if enabled
    if (receiver->formatManager && receiver->formatManager->isLearningMode() && !floatValues.empty()) {
        receiver->formatManager->learnOSCMessage(path, floatValues);
    }
    
    // Forward to callback
    if (receiver->messageCallback && !floatValues.empty()) {
        receiver->messageCallback(path, floatValues);
    }
    
    if (receiver->formatManager) {
        receiver->formatManager->recordMessageReceived(path);
    }
    return 0;
}

void OSCReceiver::errorHandler(int num, const char* msg, const char* path) {
    std::string errorMsg = "OSC server error " + std::to_string(num) + ": " + std::string(msg);
    if (path) {
        errorMsg += " (path: " + std::string(path) + ")";
    }
    ERROR_ERROR("OSC receiver error", errorMsg, "Check OSC messages format", true);
}
