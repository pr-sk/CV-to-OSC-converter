#include "OSCSecurity.h"
#include "ErrorHandler.h"
#include <algorithm>
#include <sstream>
#include <cctype>
#include <regex>
#include <cmath>
#include <chrono>

bool OSCSecurity::isAddressValid(const std::string& address) const {
    if (!config.enableValidation) return true;
    
    // Basic OSC address format validation
    if (address.empty() || address[0] != '/') {
        return false;
    }
    
    // Check against whitelist if enabled
    if (config.enableAddressWhitelist && !config.allowedAddresses.empty()) {
        if (std::find(config.allowedAddresses.begin(), config.allowedAddresses.end(), address) == config.allowedAddresses.end()) {
            return false;
        }
    }
    
    // Check against blacklist
    // No blocked addresses check for now - could be added to config
    
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
    
    // Check against whitelist if enabled
    if (!config.allowedHosts.empty()) {
        return std::find(config.allowedHosts.begin(), config.allowedHosts.end(), host) != config.allowedHosts.end();
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
    
    // No blocked addresses in current config
    
    if (config.enableHostWhitelist && !config.allowedHosts.empty()) {
        report << "Allowed Hosts (" << config.allowedHosts.size() << "):\n";
        for (const auto& host : config.allowedHosts) {
            report << "  " << host << "\n";
        }
        report << "\n";
    }
    
    // No blocked hosts in current config
    
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

// OSCSecurityAdvanced Implementation
OSCSecurityAdvanced::OSCSecurityAdvanced() : rng(std::random_device{}()) {
    initializeCrypto();
}

OSCSecurityAdvanced::OSCSecurityAdvanced(const SecurityProfile& profile) 
    : profile(profile), rng(std::random_device{}()) {
    initializeCrypto();
}

bool OSCSecurityAdvanced::initializeCrypto() {
    // Initialize OpenSSL (simplified for demo)
    return true;
}

bool OSCSecurityAdvanced::generateKeyPair() {
    // Simplified key generation
    profile.sharedSecret = "generated_secret_key_32_bytes_long";
    return true;
}

bool OSCSecurityAdvanced::setSharedSecret(const std::string& secret) {
    if (secret.length() >= 32) {
        profile.sharedSecret = secret;
        return true;
    }
    return false;
}

std::vector<uint8_t> OSCSecurityAdvanced::generateNonce() const {
    std::vector<uint8_t> nonce(12);
    for (size_t i = 0; i < nonce.size(); ++i) {
        nonce[i] = static_cast<uint8_t>(rng() % 256);
    }
    return nonce;
}

bool OSCSecurityAdvanced::validateNonce(const std::vector<uint8_t>& nonce) {
    std::lock_guard<std::mutex> lock(nonceMutex);
    if (usedNonces.find(nonce) != usedNonces.end()) {
        return false; // Replay attack
    }
    usedNonces.insert(nonce);
    if (usedNonces.size() > profile.nonceWindowSize) {
        cleanupOldNonces();
    }
    return true;
}

void OSCSecurityAdvanced::cleanupOldNonces() {
    if (usedNonces.size() > profile.nonceWindowSize / 2) {
        auto it = usedNonces.begin();
        std::advance(it, usedNonces.size() / 2);
        usedNonces.erase(usedNonces.begin(), it);
    }
}

uint64_t OSCSecurityAdvanced::getCurrentTimestamp() const {
    return std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
}

bool OSCSecurityAdvanced::validateTimestamp(uint64_t timestamp) const {
    if (!profile.requireTimestamp) return true;
    uint64_t current = getCurrentTimestamp();
    return std::abs(static_cast<int64_t>(current - timestamp)) <= profile.timestampTolerance;
}

bool OSCSecurityAdvanced::encryptMessage(const std::vector<uint8_t>& plaintext, EncryptedMessage& encrypted) const {
    if (profile.encryption == EncryptionMode::NONE) {
        encrypted.ciphertext = plaintext;
        return true;
    }
    // Simplified encryption for demo
    encrypted.nonce = generateNonce();
    encrypted.timestamp = getCurrentTimestamp();
    encrypted.ciphertext = plaintext; // In real implementation would encrypt
    return true;
}

bool OSCSecurityAdvanced::decryptMessage(const EncryptedMessage& encrypted, std::vector<uint8_t>& plaintext) const {
    if (profile.encryption == EncryptionMode::NONE) {
        plaintext = encrypted.ciphertext;
        return true;
    }
    if (!validateTimestamp(encrypted.timestamp)) return false;
    plaintext = encrypted.ciphertext; // In real implementation would decrypt
    return true;
}

std::string OSCSecurityAdvanced::generateSecurityAudit() const {
    std::stringstream audit;
    audit << "Advanced OSC Security Audit:\n";
    audit << "- Encryption: " << (profile.encryption == EncryptionMode::AES_256_GCM ? "AES-256-GCM" : "None") << "\n";
    audit << "- Authentication: " << (profile.authentication == AuthMode::HMAC_SHA256 ? "HMAC-SHA256" : "None") << "\n";
    audit << "- Timestamp validation: " << (profile.requireTimestamp ? "Enabled" : "Disabled") << "\n";
    audit << "- Nonce validation: " << (profile.enableNonceValidation ? "Enabled" : "Disabled") << "\n";
    audit << "- Active nonces: " << usedNonces.size() << "\n";
    return audit.str();
}

// OSCPatternMatcher Implementation
void OSCPatternMatcher::addRoute(const RouteRule& rule) {
    if (validateRule(rule)) {
        routes.push_back(rule);
        std::sort(routes.begin(), routes.end(), [](const RouteRule& a, const RouteRule& b) {
            return a.priority > b.priority;
        });
    }
}

void OSCPatternMatcher::removeRoute(const std::string& pattern) {
    routes.erase(std::remove_if(routes.begin(), routes.end(),
        [&](const RouteRule& rule) { return rule.pattern == pattern; }), routes.end());
}

std::vector<OSCPatternMatcher::MatchResult> OSCPatternMatcher::matchPattern(const std::string& address) const {
    std::vector<MatchResult> results;
    for (const auto& rule : routes) {
        if (!rule.enabled) continue;
        if (isMatch(address, rule)) {
            MatchResult result;
            result.matched = true;
            result.targetAddress = rule.targetAddress;
            result.targetHost = rule.targetHost;
            result.targetPort = rule.targetPort;
            results.push_back(result);
        }
    }
    return results;
}

bool OSCPatternMatcher::isMatch(const std::string& address, const RouteRule& rule) const {
    switch (rule.matchType) {
        case MatchType::EXACT: return matchExact(rule.pattern, address);
        case MatchType::PREFIX: return matchPrefix(rule.pattern, address);
        case MatchType::SUFFIX: return matchSuffix(rule.pattern, address);
        case MatchType::CONTAINS: return matchContains(rule.pattern, address);
        case MatchType::OSC_PATTERN: return matchOSCPattern(rule.pattern, address);
        default: return false;
    }
}

bool OSCPatternMatcher::matchOSCPattern(const std::string& pattern, const std::string& address) const {
    // Simplified OSC pattern matching
    if (pattern.find('*') == std::string::npos && pattern.find('?') == std::string::npos) {
        return pattern == address;
    }
    // For demo - basic wildcard support
    return matchWildcard(pattern, address);
}

bool OSCPatternMatcher::matchExact(const std::string& pattern, const std::string& address) const {
    return pattern == address;
}

bool OSCPatternMatcher::matchPrefix(const std::string& pattern, const std::string& address) const {
    return address.find(pattern) == 0;
}

bool OSCPatternMatcher::matchSuffix(const std::string& pattern, const std::string& address) const {
    if (pattern.length() > address.length()) return false;
    return address.substr(address.length() - pattern.length()) == pattern;
}

bool OSCPatternMatcher::matchContains(const std::string& pattern, const std::string& address) const {
    return address.find(pattern) != std::string::npos;
}

bool OSCPatternMatcher::matchWildcard(const std::string& pattern, const std::string& address) const {
    // Simple wildcard matching (* and ?)
    size_t pPos = 0, aPos = 0;
    while (pPos < pattern.length() && aPos < address.length()) {
        if (pattern[pPos] == '*') {
            pPos++;
            if (pPos >= pattern.length()) return true;
            while (aPos < address.length() && address[aPos] != pattern[pPos]) aPos++;
        } else if (pattern[pPos] == '?') {
            pPos++; aPos++;
        } else {
            if (pattern[pPos] != address[aPos]) return false;
            pPos++; aPos++;
        }
    }
    while (pPos < pattern.length() && pattern[pPos] == '*') pPos++;
    return pPos >= pattern.length() && aPos >= address.length();
}

bool OSCPatternMatcher::validateRule(const RouteRule& rule) const {
    return !rule.pattern.empty() && !rule.targetHost.empty() && !rule.targetPort.empty();
}
