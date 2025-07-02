#pragma once

#include <string>
#include <vector>
#include <regex>
#include <set>
#include <chrono>
#include <mutex>

class OSCSecurity {
public:
    struct SecurityConfig {
        bool enableValidation = true;
        bool enableSanitization = true;
        bool enableRateLimiting = true;
        bool enableAddressWhitelist = false;
        bool enableHostWhitelist = false;
        
        // Rate limiting
        size_t maxMessagesPerSecond = 1000;
        size_t maxBundleSize = 100;
        
        // Value limits
        float maxFloatValue = 1000000.0f;
        float minFloatValue = -1000000.0f;
        int maxIntValue = 1000000;
        int minIntValue = -1000000;
        size_t maxStringLength = 1024;
        size_t maxBlobSize = 1024 * 1024; // 1MB
        
        // Whitelists
        std::vector<std::string> allowedAddresses;
        std::regex allowedAddressPattern{std::regex(R"(^/[a-zA-Z0-9/_-]*$)")};
        std::vector<std::string> allowedHosts;
    };

private:
    SecurityConfig config;
    
    // Rate limiting state
    mutable std::chrono::steady_clock::time_point lastReset;
    mutable size_t messageCount = 0;
    mutable std::mutex rateLimitMutex;

public:
    OSCSecurity() : config{} {
        lastReset = std::chrono::steady_clock::now();
    }
    
    OSCSecurity(const SecurityConfig& cfg) : config(cfg) {
        lastReset = std::chrono::steady_clock::now();
    }
    
    // Configuration
    void setConfig(const SecurityConfig& cfg) { config = cfg; }
    const SecurityConfig& getConfig() const { return config; }
    
    // Address validation
    bool isAddressValid(const std::string& address) const;
    std::string sanitizeAddress(const std::string& address) const;
    
    // Value validation and sanitization
    bool isFloatValid(float value) const;
    bool isIntValid(int value) const;
    bool isStringValid(const std::string& value) const;
    bool isBlobValid(const void* data, size_t size) const;
    
    float sanitizeFloat(float value) const;
    int sanitizeInt(int value) const;
    std::string sanitizeString(const std::string& value) const;
    
    // Host validation
    bool isHostAllowed(const std::string& host) const;
    
    // Rate limiting
    bool checkRateLimit() const;
    bool checkBundleSize(size_t bundleSize) const;
    
    // Comprehensive validation
    bool validateMessage(const std::string& address, float value) const;
    bool validateMessage(const std::string& address, int value) const;
    bool validateMessage(const std::string& address, const std::string& value) const;
    bool validateMessage(const std::string& address, const void* data, size_t size) const;
    
    // Security report
    std::string generateSecurityReport() const;
    
private:
    bool matchesPattern(const std::string& address, const std::regex& pattern) const;
    void resetRateLimit() const;
};
