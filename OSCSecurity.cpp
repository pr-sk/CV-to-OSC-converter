#include "OSCSecurity.h"
#include "ErrorHandler.h"
#include <algorithm>
#include <sstream>
#include <cctype>

bool OSCSecurity::isAddressValid(const std::string& address) const {
    if (!config.enableValidation) return true;
    
    // Basic OSC address format validation
    if (address.empty() || address[0] != '/') {
        return false;
    }
    
    // Check against whitelist if enabled
    if (config.enableAddressWhitelist && !config.allowedAddresses.empty()) {
        if (config.allowedAddresses.find(address) == config.allowedAddresses.end()) {
            return false;
        }
    }
    
    // Check against blacklist
    if (config.blockedAddresses.find(address) != config.blockedAddresses.end()) {
        return false;
    }
    
    // Pattern matching
    return matchesPattern(address, config.allowedAddressPattern);
}

std::string OSCSecurity::sanitizeAddress(const std::string& address) const {
    if (!config.enableSanitization) return address;
    
    std::string sanitized = address;
    
    // Ensure it starts with '/'
    if (sanitized.empty() || sanitized[0] != '/') {
        sanitized = "/" + sanitized;
    }
    
    // Remove invalid characters (keep only alphanumeric, /, _, -)
    sanitized.erase(
        std::remove_if(sanitized.begin() + 1, sanitized.end(), 
            [](char c) { 
                return !std::isalnum(c) && c != '/' && c != '_' && c != '-'; 
            }), 
        sanitized.end()
    );
    
    // Remove double slashes
    std::regex doubleSlash("//+");
    sanitized = std::regex_replace(sanitized, doubleSlash, "/");
    
    // Ensure it doesn't end with '/' (unless it's just "/")
    if (sanitized.length() > 1 && sanitized.back() == '/') {
        sanitized.pop_back();
    }
    
    return sanitized;
}

bool OSCSecurity::isFloatValid(float value) const {
    if (!config.enableValidation) return true;
    
    // Check for NaN and infinity
    if (std::isnan(value) || std::isinf(value)) {
        return false;
    }
    
    // Check range
    return value >= config.minFloatValue && value <= config.maxFloatValue;
}

bool OSCSecurity::isIntValid(int value) const {
    if (!config.enableValidation) return true;
    
    return value >= config.minIntValue && value <= config.maxIntValue;
}

bool OSCSecurity::isStringValid(const std::string& value) const {
    if (!config.enableValidation) return true;
    
    // Check length
    if (value.length() > config.maxStringLength) {
        return false;
    }
    
    // Check for null bytes and control characters
    for (char c : value) {
        if (c == '\0' || (c > 0 && c < 32 && c != '\t' && c != '\n' && c != '\r')) {
            return false;
        }
    }
    
    return true;
}

bool OSCSecurity::isBlobValid(const void* data, size_t size) const {
    if (!config.enableValidation) return true;
    
    return data != nullptr && size <= config.maxBlobSize;
}

float OSCSecurity::sanitizeFloat(float value) const {
    if (!config.enableSanitization) return value;
    
    // Handle NaN and infinity
    if (std::isnan(value) || std::isinf(value)) {
        return 0.0f;
    }
    
    // Clamp to range
    return std::max(config.minFloatValue, std::min(config.maxFloatValue, value));
}

int OSCSecurity::sanitizeInt(int value) const {
    if (!config.enableSanitization) return value;
    
    // Clamp to range
    return std::max(config.minIntValue, std::min(config.maxIntValue, value));
}

std::string OSCSecurity::sanitizeString(const std::string& value) const {
    if (!config.enableSanitization) return value;
    
    std::string sanitized = value;
    
    // Truncate if too long
    if (sanitized.length() > config.maxStringLength) {
        sanitized = sanitized.substr(0, config.maxStringLength);
    }
    
    // Remove null bytes and control characters
    sanitized.erase(
        std::remove_if(sanitized.begin(), sanitized.end(), 
            [](char c) { 
                return c == '\0' || (c > 0 && c < 32 && c != '\t' && c != '\n' && c != '\r'); 
            }), 
        sanitized.end()
    );
    
    return sanitized;
}

bool OSCSecurity::isHostAllowed(const std::string& host) const {
    if (!config.enableHostWhitelist) return true;
    
    // Check against blacklist first
    if (config.blockedHosts.find(host) != config.blockedHosts.end()) {
        return false;
    }
    
    // Check against whitelist if enabled
    if (!config.allowedHosts.empty()) {
        return config.allowedHosts.find(host) != config.allowedHosts.end();
    }
    
    return true;
}

bool OSCSecurity::checkRateLimit() const {
    if (!config.enableRateLimiting) return true;
    
    std::lock_guard<std::mutex> lock(rateLimitMutex);
    
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - lastReset);
    
    // Reset counter every second
    if (elapsed.count() >= 1) {
        messageCount = 0;
        lastReset = now;
    }
    
    // Check if under limit
    if (messageCount >= config.maxMessagesPerSecond) {
        return false;
    }
    
    messageCount++;
    return true;
}

bool OSCSecurity::checkBundleSize(size_t bundleSize) const {
    if (!config.enableRateLimiting) return true;
    
    return bundleSize <= config.maxBundleSize;
}

bool OSCSecurity::validateMessage(const std::string& address, float value) const {
    return isAddressValid(address) && isFloatValid(value) && checkRateLimit();
}

bool OSCSecurity::validateMessage(const std::string& address, int value) const {
    return isAddressValid(address) && isIntValid(value) && checkRateLimit();
}

bool OSCSecurity::validateMessage(const std::string& address, const std::string& value) const {
    return isAddressValid(address) && isStringValid(value) && checkRateLimit();
}

bool OSCSecurity::validateMessage(const std::string& address, const void* data, size_t size) const {
    return isAddressValid(address) && isBlobValid(data, size) && checkRateLimit();
}

std::string OSCSecurity::generateSecurityReport() const {
    std::ostringstream report;
    
    report << "OSC Security Configuration Report\n";
    report << "==================================\n\n";
    
    report << "Validation: " << (config.enableValidation ? "ENABLED" : "DISABLED") << "\n";
    report << "Sanitization: " << (config.enableSanitization ? "ENABLED" : "DISABLED") << "\n";
    report << "Rate Limiting: " << (config.enableRateLimiting ? "ENABLED" : "DISABLED") << "\n";
    report << "Address Whitelist: " << (config.enableAddressWhitelist ? "ENABLED" : "DISABLED") << "\n";
    report << "Host Whitelist: " << (config.enableHostWhitelist ? "ENABLED" : "DISABLED") << "\n\n";
    
    if (config.enableRateLimiting) {
        report << "Rate Limits:\n";
        report << "  Max Messages/Second: " << config.maxMessagesPerSecond << "\n";
        report << "  Max Bundle Size: " << config.maxBundleSize << "\n\n";
    }
    
    report << "Value Constraints:\n";
    report << "  Float Range: " << config.minFloatValue << " to " << config.maxFloatValue << "\n";
    report << "  Int Range: " << config.minIntValue << " to " << config.maxIntValue << "\n";
    report << "  Max String Length: " << config.maxStringLength << "\n";
    report << "  Max Blob Size: " << config.maxBlobSize << " bytes\n\n";
    
    if (config.enableAddressWhitelist && !config.allowedAddresses.empty()) {
        report << "Allowed Addresses (" << config.allowedAddresses.size() << "):\n";
        for (const auto& addr : config.allowedAddresses) {
            report << "  " << addr << "\n";
        }
        report << "\n";
    }
    
    if (!config.blockedAddresses.empty()) {
        report << "Blocked Addresses (" << config.blockedAddresses.size() << "):\n";
        for (const auto& addr : config.blockedAddresses) {
            report << "  " << addr << "\n";
        }
        report << "\n";
    }
    
    if (config.enableHostWhitelist && !config.allowedHosts.empty()) {
        report << "Allowed Hosts (" << config.allowedHosts.size() << "):\n";
        for (const auto& host : config.allowedHosts) {
            report << "  " << host << "\n";
        }
        report << "\n";
    }
    
    if (!config.blockedHosts.empty()) {
        report << "Blocked Hosts (" << config.blockedHosts.size() << "):\n";
        for (const auto& host : config.blockedHosts) {
            report << "  " << host << "\n";
        }
        report << "\n";
    }
    
    return report.str();
}

bool OSCSecurity::matchesPattern(const std::string& address, const std::regex& pattern) const {
    try {
        return std::regex_match(address, pattern);
    } catch (const std::regex_error& e) {
        ErrorHandler::getInstance().logError("Regex pattern matching failed", e.what());
        return false;
    }
}

void OSCSecurity::resetRateLimit() const {
    std::lock_guard<std::mutex> lock(rateLimitMutex);
    messageCount = 0;
    lastReset = std::chrono::steady_clock::now();
}
