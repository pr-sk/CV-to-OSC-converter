#pragma once

#include <string>
#include <vector>
#include <regex>
#include <set>
#include <chrono>
#include <mutex>
#include <map>
#include <random>
#include <nlohmann/json.hpp>
// Simplified crypto implementation without OpenSSL dependencies

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

// Enhanced OSC Security with Encryption and Authentication
class OSCSecurityAdvanced {
public:
    enum class EncryptionMode {
        NONE,
        AES_256_GCM,
        CHACHA20_POLY1305
    };
    
    enum class AuthMode {
        NONE,
        HMAC_SHA256,
        RSA_SIGNATURE
    };
    
    struct SecurityProfile {
        EncryptionMode encryption = EncryptionMode::AES_256_GCM;
        AuthMode authentication = AuthMode::HMAC_SHA256;
        std::string sharedSecret;
        std::string publicKey;
        std::string privateKey;
        bool requireTimestamp = true;
        uint32_t timestampTolerance = 30; // seconds
        bool enableNonceValidation = true;
        size_t nonceWindowSize = 1000;
    };
    
    struct EncryptedMessage {
        std::vector<uint8_t> ciphertext;
        std::vector<uint8_t> nonce;
        std::vector<uint8_t> tag;
        std::vector<uint8_t> signature;
        uint64_t timestamp;
        std::string sender;
    };
    
private:
    SecurityProfile profile;
    std::set<std::vector<uint8_t>> usedNonces;
    std::mutex nonceMutex;
    mutable std::mt19937 rng;
    
public:
    OSCSecurityAdvanced();
    explicit OSCSecurityAdvanced(const SecurityProfile& profile);
    
    // Configuration
    void setSecurityProfile(const SecurityProfile& profile) { this->profile = profile; }
    const SecurityProfile& getSecurityProfile() const { return profile; }
    
    // Key management
    bool generateKeyPair();
    bool setSharedSecret(const std::string& secret);
    bool loadKeysFromFile(const std::string& keyFile);
    bool saveKeysToFile(const std::string& keyFile) const;
    
    // Encryption/Decryption
    bool encryptMessage(const std::vector<uint8_t>& plaintext, EncryptedMessage& encrypted) const;
    bool decryptMessage(const EncryptedMessage& encrypted, std::vector<uint8_t>& plaintext) const;
    
    // Authentication
    bool signMessage(const std::vector<uint8_t>& message, std::vector<uint8_t>& signature) const;
    bool verifySignature(const std::vector<uint8_t>& message, const std::vector<uint8_t>& signature) const;
    
    // High-level OSC message security
    bool secureOSCMessage(const std::string& address, const std::vector<float>& args, 
                         std::vector<uint8_t>& secureData) const;
    bool verifyOSCMessage(const std::vector<uint8_t>& secureData, 
                         std::string& address, std::vector<float>& args) const;
    
    // Nonce management
    std::vector<uint8_t> generateNonce() const;
    bool validateNonce(const std::vector<uint8_t>& nonce);
    void cleanupOldNonces();
    
    // Timestamp validation
    uint64_t getCurrentTimestamp() const;
    bool validateTimestamp(uint64_t timestamp) const;
    
    // Security audit
    std::string generateSecurityAudit() const;
    
private:
    bool initializeCrypto();
    bool encryptAES256GCM(const std::vector<uint8_t>& plaintext, 
                         const std::vector<uint8_t>& key,
                         const std::vector<uint8_t>& nonce,
                         std::vector<uint8_t>& ciphertext,
                         std::vector<uint8_t>& tag) const;
    bool decryptAES256GCM(const std::vector<uint8_t>& ciphertext,
                         const std::vector<uint8_t>& key,
                         const std::vector<uint8_t>& nonce,
                         const std::vector<uint8_t>& tag,
                         std::vector<uint8_t>& plaintext) const;
    
    std::vector<uint8_t> deriveKey(const std::string& secret, const std::vector<uint8_t>& salt) const;
    std::vector<uint8_t> hmacSHA256(const std::vector<uint8_t>& data, const std::vector<uint8_t>& key) const;
};

// Pattern Matching Engine for OSC routing
class OSCPatternMatcher {
public:
    enum class MatchType {
        EXACT,
        PREFIX,
        SUFFIX,
        CONTAINS,
        REGEX,
        WILDCARD,
        OSC_PATTERN // Standard OSC pattern matching with *, ?, [], {}
    };
    
    struct RouteRule {
        std::string pattern;
        MatchType matchType;
        std::string targetAddress;
        std::string targetHost;
        std::string targetPort;
        int priority = 0;
        bool enabled = true;
        std::map<std::string, std::string> transformations;
    };
    
    struct MatchResult {
        bool matched = false;
        std::string targetAddress;
        std::string targetHost;
        std::string targetPort;
        std::map<std::string, std::string> capturedGroups;
        std::vector<std::string> transformedArgs;
    };
    
private:
    std::vector<RouteRule> routes;
    std::map<std::string, std::regex> compiledRegexes;
    
public:
    OSCPatternMatcher() = default;
    
    // Route management
    void addRoute(const RouteRule& rule);
    void removeRoute(const std::string& pattern);
    void updateRoute(const std::string& pattern, const RouteRule& rule);
    const std::vector<RouteRule>& getRoutes() const { return routes; }
    
    // Pattern matching
    std::vector<MatchResult> matchPattern(const std::string& address) const;
    MatchResult findBestMatch(const std::string& address) const;
    bool isMatch(const std::string& address, const RouteRule& rule) const;
    
    // OSC standard pattern matching
    bool matchOSCPattern(const std::string& pattern, const std::string& address) const;
    
    // Transformation
    std::string transformAddress(const std::string& address, const RouteRule& rule, 
                               const MatchResult& match) const;
    std::vector<std::string> transformArguments(const std::vector<std::string>& args,
                                              const RouteRule& rule,
                                              const MatchResult& match) const;
    
    // Validation and testing
    bool validateRule(const RouteRule& rule) const;
    std::vector<std::string> testPattern(const std::string& pattern, 
                                       const std::vector<std::string>& testAddresses) const;
    
    // Configuration
    void loadRoutesFromFile(const std::string& filename);
    void saveRoutesToFile(const std::string& filename) const;
    nlohmann::json exportRoutes() const;
    void importRoutes(const nlohmann::json& config);
    
private:
    bool matchExact(const std::string& pattern, const std::string& address) const;
    bool matchPrefix(const std::string& pattern, const std::string& address) const;
    bool matchSuffix(const std::string& pattern, const std::string& address) const;
    bool matchContains(const std::string& pattern, const std::string& address) const;
    bool matchRegex(const std::string& pattern, const std::string& address) const;
    bool matchWildcard(const std::string& pattern, const std::string& address) const;
    
    std::regex compileRegex(const std::string& pattern) const;
    std::string wildcardToRegex(const std::string& pattern) const;
    
    // OSC pattern matching helpers
    bool matchOSCChar(char pattern, char address) const;
    bool matchOSCBrackets(const std::string& pattern, size_t& pos, char address) const;
    bool matchOSCBraces(const std::string& pattern, size_t& pos, const std::string& address, size_t& addrPos) const;
};
