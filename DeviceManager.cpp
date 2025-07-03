#include "DeviceManager.h"
#include "MidiDeviceHandler.h"
#include "WiFiDeviceHandler.h"
#include "OSCSender.h"
#include "OSCReceiver.h"
#include <iostream>
#include <algorithm>
#include <chrono>

DeviceManager::DeviceManager() 
    : autoDiscoveryEnabled_(false), autoReconnectEnabled_(false), running_(false),
      discoveryInterval_(5000) {
}

DeviceManager::~DeviceManager() {
    shutdown();
}

bool DeviceManager::initialize() {
    try {
        // Initialize device handlers
#ifdef __APPLE__
        midiHandler_ = std::make_unique<MidiDeviceHandler>();
#endif
        wifiHandler_ = std::make_unique<WiFiDeviceHandler>();
        
        // Initialize handlers
        bool midiOk = true;
#ifdef __APPLE__
        midiOk = midiHandler_->initialize();
#endif
        bool wifiOk = wifiHandler_->initialize();
        
        if (!midiOk || !wifiOk) {
            lastError_ = "Failed to initialize some device handlers";
            std::cerr << lastError_ << std::endl;
        }
        
        // Set up callbacks
#ifdef __APPLE__
        if (midiHandler_) {
            midiHandler_->setDataCallback([this](const std::string& deviceId, const std::vector<uint8_t>& data) {
                handleDeviceData(deviceId, data);
            });
            
            midiHandler_->setOSCCallback([this](const std::string& deviceId, const std::string& address, float value) {
                handleOSCData(deviceId, address, value);
            });
        }
#endif
        
        if (wifiHandler_) {
            wifiHandler_->setDataCallback([this](const std::string& deviceId, const std::vector<uint8_t>& data) {
                handleDeviceData(deviceId, data);
            });
            
            wifiHandler_->setOSCCallback([this](const std::string& deviceId, const std::string& address, float value) {
                handleOSCData(deviceId, address, value);
            });
        }
        
        // Initialize OSC communication
        oscSender_ = std::make_unique<OSCSender>("127.0.0.1", "9001");
        
        running_ = true;
        std::cout << "DeviceManager initialized successfully" << std::endl;
        
        return true;
        
    } catch (const std::exception& e) {
        lastError_ = "DeviceManager initialization failed: " + std::string(e.what());
        std::cerr << lastError_ << std::endl;
        return false;
    }
}

void DeviceManager::shutdown() {
    running_ = false;
    
    // Stop discovery thread
    if (discoveryThread_.joinable()) {
        discoveryThread_.join();
    }
    
    // Disconnect all devices
    {
        std::lock_guard<std::mutex> lock(devicesMutex_);
        for (auto& [deviceId, device] : connectedDevices_) {
            auto* handler = getHandlerForType(device.type);
            if (handler) {
                handler->disconnect(deviceId);
            }
        }
        connectedDevices_.clear();
        devices_.clear();
    }
    
    // Shutdown handlers
#ifdef __APPLE__
    if (midiHandler_) {
        midiHandler_->shutdown();
    }
#endif
    if (wifiHandler_) {
        wifiHandler_->shutdown();
    }
    
    oscSender_.reset();
    oscReceiver_.reset();
    
    std::cout << "DeviceManager shutdown complete" << std::endl;
}

std::vector<DeviceInfo> DeviceManager::scanAllDevices() {
    std::vector<DeviceInfo> allDevices;
    
    // Scan MIDI devices
#ifdef __APPLE__
    if (midiHandler_) {
        try {
            auto midiDevices = midiHandler_->scanForDevices();
            allDevices.insert(allDevices.end(), midiDevices.begin(), midiDevices.end());
        } catch (const std::exception& e) {
            std::cerr << "Error scanning MIDI devices: " << e.what() << std::endl;
        }
    }
#endif
    
    // Scan WiFi devices
    if (wifiHandler_) {
        try {
            auto wifiDevices = wifiHandler_->scanForDevices();
            allDevices.insert(allDevices.end(), wifiDevices.begin(), wifiDevices.end());
        } catch (const std::exception& e) {
            std::cerr << "Error scanning WiFi devices: " << e.what() << std::endl;
        }
    }
    
    // Update device registry
    {
        std::lock_guard<std::mutex> lock(devicesMutex_);
        for (const auto& device : allDevices) {
            devices_[device.id] = device;
        }
    }
    
    return allDevices;
}

std::vector<DeviceInfo> DeviceManager::getConnectedDevices() const {
    std::lock_guard<std::mutex> lock(devicesMutex_);
    std::vector<DeviceInfo> connectedList;
    connectedList.reserve(connectedDevices_.size());
    
    for (const auto& [deviceId, device] : connectedDevices_) {
        connectedList.push_back(device);
    }
    
    return connectedList;
}

std::vector<DeviceInfo> DeviceManager::getAvailableDevices() const {
    std::lock_guard<std::mutex> lock(devicesMutex_);
    std::vector<DeviceInfo> availableList;
    availableList.reserve(devices_.size());
    
    for (const auto& [deviceId, device] : devices_) {
        availableList.push_back(device);
    }
    
    return availableList;
}

bool DeviceManager::connectDevice(const std::string& deviceId) {
    std::lock_guard<std::mutex> lock(devicesMutex_);
    
    auto deviceIt = devices_.find(deviceId);
    if (deviceIt == devices_.end()) {
        lastError_ = "Device not found: " + deviceId;
        return false;
    }
    
    // Check if already connected
    if (connectedDevices_.find(deviceId) != connectedDevices_.end()) {
        return true; // Already connected
    }
    
    auto& device = deviceIt->second;
    auto* handler = getHandlerForType(device.type);
    if (!handler) {
        lastError_ = "No handler available for device type";
        return false;
    }
    
    device.status = DeviceStatus::CONNECTING;
    updateDeviceStatus(deviceId, DeviceStatus::CONNECTING);
    
    if (handler->connect(device)) {
        device.status = DeviceStatus::CONNECTED;
        connectedDevices_[deviceId] = device;
        updateDeviceStatus(deviceId, DeviceStatus::CONNECTED);
        
        std::cout << "Device connected: " << device.name << " (" << deviceId << ")" << std::endl;
        return true;
    } else {
        device.status = DeviceStatus::ERROR;
        updateDeviceStatus(deviceId, DeviceStatus::ERROR);
        lastError_ = handler->getLastError();
        return false;
    }
}

bool DeviceManager::disconnectDevice(const std::string& deviceId) {
    std::lock_guard<std::mutex> lock(devicesMutex_);
    
    auto connectedIt = connectedDevices_.find(deviceId);
    if (connectedIt == connectedDevices_.end()) {
        return true; // Not connected
    }
    
    auto& device = connectedIt->second;
    auto* handler = getHandlerForType(device.type);
    if (handler) {
        handler->disconnect(deviceId);
    }
    
    device.status = DeviceStatus::DISCONNECTED;
    connectedDevices_.erase(connectedIt);
    updateDeviceStatus(deviceId, DeviceStatus::DISCONNECTED);
    
    std::cout << "Device disconnected: " << device.name << " (" << deviceId << ")" << std::endl;
    return true;
}

bool DeviceManager::isDeviceConnected(const std::string& deviceId) const {
    std::lock_guard<std::mutex> lock(devicesMutex_);
    return connectedDevices_.find(deviceId) != connectedDevices_.end();
}

bool DeviceManager::sendOSCToDevice(const std::string& deviceId, const std::string& address, float value) {
    std::lock_guard<std::mutex> lock(devicesMutex_);
    
    auto connectedIt = connectedDevices_.find(deviceId);
    if (connectedIt == connectedDevices_.end()) {
        lastError_ = "Device not connected: " + deviceId;
        return false;
    }
    
    auto& device = connectedIt->second;
    auto* handler = getHandlerForType(device.type);
    if (!handler) {
        lastError_ = "No handler available for device";
        return false;
    }
    
    return handler->sendOSCMessage(deviceId, address, value);
}

bool DeviceManager::sendOSCToAllDevices(const std::string& address, float value) {
    std::lock_guard<std::mutex> lock(devicesMutex_);
    
    bool allSuccess = true;
    for (const auto& [deviceId, device] : connectedDevices_) {
        auto* handler = getHandlerForType(device.type);
        if (handler) {
            if (!handler->sendOSCMessage(deviceId, address, value)) {
                allSuccess = false;
            }
        }
    }
    
    return allSuccess;
}

void DeviceManager::enableAutoDiscovery(bool enable) {
    autoDiscoveryEnabled_ = enable;
    
    if (enable && !discoveryThread_.joinable() && running_) {
        discoveryThread_ = std::thread(&DeviceManager::discoveryLoop, this);
    }
}

void DeviceManager::setDeviceEventCallback(std::function<void(const std::string&, DeviceStatus)> callback) {
    deviceEventCallback_ = callback;
}

void DeviceManager::setDataReceivedCallback(std::function<void(const std::string&, const std::vector<uint8_t>&)> callback) {
    dataReceivedCallback_ = callback;
}

void DeviceManager::setOSCReceivedCallback(std::function<void(const std::string&, const std::string&, float)> callback) {
    oscReceivedCallback_ = callback;
}

DeviceStatus DeviceManager::getDeviceStatus(const std::string& deviceId) const {
    std::lock_guard<std::mutex> lock(devicesMutex_);
    
    auto connectedIt = connectedDevices_.find(deviceId);
    if (connectedIt != connectedDevices_.end()) {
        return connectedIt->second.status;
    }
    
    auto deviceIt = devices_.find(deviceId);
    if (deviceIt != devices_.end()) {
        return deviceIt->second.status;
    }
    
    return DeviceStatus::DISCONNECTED;
}

void DeviceManager::discoveryLoop() {
    while (running_ && autoDiscoveryEnabled_) {
        try {
            // Scan for new devices
            scanAllDevices();
            
            // Auto-reconnect if enabled
            if (autoReconnectEnabled_) {
                std::lock_guard<std::mutex> lock(devicesMutex_);
                for (auto& [deviceId, device] : devices_) {
                    if (device.status == DeviceStatus::ERROR || device.status == DeviceStatus::DISCONNECTED) {
                        auto* handler = getHandlerForType(device.type);
                        if (handler && handler->isDeviceAvailable(deviceId)) {
                            // Try to reconnect
                            connectDevice(deviceId);
                        }
                    }
                }
            }
            
        } catch (const std::exception& e) {
            std::cerr << "Error in discovery loop: " << e.what() << std::endl;
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(discoveryInterval_));
    }
}

void DeviceManager::handleDeviceData(const std::string& deviceId, const std::vector<uint8_t>& data) {
    if (dataReceivedCallback_) {
        dataReceivedCallback_(deviceId, data);
    }
}

void DeviceManager::handleOSCData(const std::string& deviceId, const std::string& address, float value) {
    if (oscReceivedCallback_) {
        oscReceivedCallback_(deviceId, address, value);
    }
    
    // Forward to main OSC sender if available
    if (oscSender_) {
        std::string fullAddress = "/device/" + deviceId + address;
        oscSender_->sendFloat(fullAddress, value);
    }
}

void DeviceManager::updateDeviceStatus(const std::string& deviceId, DeviceStatus status) {
    if (deviceEventCallback_) {
        deviceEventCallback_(deviceId, status);
    }
}

DeviceHandler* DeviceManager::getHandlerForType(DeviceType type) {
    switch (type) {
        case DeviceType::MIDI:
#ifdef __APPLE__
            return midiHandler_.get();
#else
            return nullptr;
#endif
        case DeviceType::WIFI:
            return wifiHandler_.get();
        case DeviceType::BLUETOOTH:
            // TODO: Implement Bluetooth handler
            return nullptr;
        case DeviceType::USB:
            // TODO: Implement USB handler  
            return nullptr;
        default:
            return nullptr;
    }
}

DeviceType DeviceManager::getDeviceType(const std::string& deviceId) const {
    std::lock_guard<std::mutex> lock(devicesMutex_);
    
    auto deviceIt = devices_.find(deviceId);
    if (deviceIt != devices_.end()) {
        return deviceIt->second.type;
    }
    
    return DeviceType::UNKNOWN;
}
