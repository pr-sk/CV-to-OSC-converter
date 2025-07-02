#pragma once

#include <string>
#include <vector>
#include <functional>
#include <memory>
#include <chrono>
#include <fstream>
#include <mutex>
#include <atomic>

enum class ErrorSeverity {
    DEBUG = 0,
    INFO = 1,
    WARNING = 2,
    ERROR = 3,
    CRITICAL = 4
};

enum class ErrorCategory {
    SYSTEM = 0,
    AUDIO = 1,
    NETWORK = 2,
    CONFIG = 3,
    USER_INPUT = 4,
    PERFORMANCE = 5,
    HARDWARE = 6
};

struct ErrorInfo {
    ErrorSeverity severity;
    ErrorCategory category;
    std::string message;
    std::string details;
    std::string function;
    std::string file;
    int line;
    std::chrono::system_clock::time_point timestamp;
    size_t errorCode;
    bool recoverable;
    std::string suggestedAction;
    
    ErrorInfo() : severity(ErrorSeverity::INFO), category(ErrorCategory::SYSTEM), 
                 line(0), timestamp(std::chrono::system_clock::now()), 
                 errorCode(0), recoverable(true) {}
};

class ErrorHandler {
private:
    static std::unique_ptr<ErrorHandler> instance;
    static std::mutex instanceMutex;
    
    std::vector<ErrorInfo> errorHistory;
    std::vector<std::function<void(const ErrorInfo&)>> errorCallbacks;
    mutable std::mutex errorMutex;
    std::ofstream logFile;
    ErrorSeverity logLevel;
    size_t maxHistorySize;
    std::atomic<size_t> errorCounter;
    bool consoleOutput;
    bool fileOutput;
    std::string logFileName;
    
    // Recovery mechanisms
    std::atomic<int> audioRecoveryAttempts;
    std::atomic<int> networkRecoveryAttempts;
    std::atomic<bool> recoveryInProgress;
    std::chrono::system_clock::time_point lastRecoveryAttempt;
    
public:
    static ErrorHandler& getInstance();
    
    // Core error reporting
    void reportError(ErrorSeverity severity, ErrorCategory category, 
                    const std::string& message, const std::string& details = "",
                    const std::string& function = "", const std::string& file = "", 
                    int line = 0, bool recoverable = true, 
                    const std::string& suggestedAction = "");
    
    // Convenience methods for different severities
    void logDebug(const std::string& message, const std::string& details = "");
    void logInfo(const std::string& message, const std::string& details = "");
    void logWarning(const std::string& message, const std::string& details = "", 
                   const std::string& suggestedAction = "");
    void logError(const std::string& message, const std::string& details = "",
                 const std::string& suggestedAction = "", bool recoverable = true);
    void logCritical(const std::string& message, const std::string& details = "",
                    const std::string& suggestedAction = "");
    
    // Category-specific error methods
    void reportAudioError(const std::string& message, const std::string& details = "",
                         bool recoverable = true, const std::string& suggestedAction = "");
    void reportNetworkError(const std::string& message, const std::string& details = "",
                           bool recoverable = true, const std::string& suggestedAction = "");
    void reportConfigError(const std::string& message, const std::string& details = "",
                          bool recoverable = true, const std::string& suggestedAction = "");
    void reportPerformanceWarning(const std::string& message, const std::string& details = "",
                                 const std::string& suggestedAction = "");
    
    // Configuration
    void setLogLevel(ErrorSeverity level) { logLevel = level; }
    void setConsoleOutput(bool enabled) { consoleOutput = enabled; }
    void setFileOutput(bool enabled, const std::string& filename = "");
    void setMaxHistorySize(size_t size) { maxHistorySize = size; }
    
    // Error history and analysis
    std::vector<ErrorInfo> getErrorHistory() const;
    std::vector<ErrorInfo> getErrorsByCategory(ErrorCategory category) const;
    std::vector<ErrorInfo> getErrorsBySeverity(ErrorSeverity severity) const;
    std::vector<ErrorInfo> getRecentErrors(std::chrono::minutes duration) const;
    
    // Statistics and reporting
    size_t getErrorCount() const { return errorCounter.load(); }
    size_t getErrorCountByCategory(ErrorCategory category) const;
    size_t getErrorCountBySeverity(ErrorSeverity severity) const;
    std::string generateErrorReport() const;
    std::string generateHealthStatus() const;
    
    // Recovery mechanisms
    bool attemptAudioRecovery();
    bool attemptNetworkRecovery();
    bool attemptConfigRecovery();
    void resetRecoveryCounters();
    bool isRecoveryInProgress() const { return recoveryInProgress.load(); }
    
    // Callbacks for error notifications
    void addErrorCallback(std::function<void(const ErrorInfo&)> callback);
    void removeAllCallbacks();
    
    // Utility functions
    static std::string severityToString(ErrorSeverity severity);
    static std::string categoryToString(ErrorCategory category);
    static ErrorSeverity stringToSeverity(const std::string& str);
    static std::string formatTimestamp(const std::chrono::system_clock::time_point& timestamp);
    
    // Cleanup
    void clearHistory();
    void closeLogFile();
    
private:
    ErrorHandler();
    
public:
    ~ErrorHandler();
    
    void writeToLog(const ErrorInfo& error);
    void writeToConsole(const ErrorInfo& error);
    void trimHistory();
    void notifyCallbacks(const ErrorInfo& error);
    bool shouldAttemptRecovery(ErrorCategory category) const;
    std::string getColorForSeverity(ErrorSeverity severity) const;
};

// Convenience macros for error reporting with automatic file/line/function info
#define ERROR_DEBUG(msg, details) \
    ErrorHandler::getInstance().reportError(ErrorSeverity::DEBUG, ErrorCategory::SYSTEM, \
                                          msg, details, __FUNCTION__, __FILE__, __LINE__)

#define ERROR_INFO(msg, details) \
    ErrorHandler::getInstance().reportError(ErrorSeverity::INFO, ErrorCategory::SYSTEM, \
                                          msg, details, __FUNCTION__, __FILE__, __LINE__)

#define ERROR_WARNING(msg, details, action) \
    ErrorHandler::getInstance().reportError(ErrorSeverity::WARNING, ErrorCategory::SYSTEM, \
                                          msg, details, __FUNCTION__, __FILE__, __LINE__, true, action)

#define ERROR_ERROR(msg, details, action, recoverable) \
    ErrorHandler::getInstance().reportError(ErrorSeverity::ERROR, ErrorCategory::SYSTEM, \
                                          msg, details, __FUNCTION__, __FILE__, __LINE__, recoverable, action)

#define ERROR_CRITICAL(msg, details, action) \
    ErrorHandler::getInstance().reportError(ErrorSeverity::CRITICAL, ErrorCategory::SYSTEM, \
                                          msg, details, __FUNCTION__, __FILE__, __LINE__, false, action)

// Category-specific macros
#define AUDIO_ERROR(msg, details, recoverable, action) \
    ErrorHandler::getInstance().reportError(ErrorSeverity::ERROR, ErrorCategory::AUDIO, \
                                          msg, details, __FUNCTION__, __FILE__, __LINE__, recoverable, action)

#define NETWORK_ERROR(msg, details, recoverable, action) \
    ErrorHandler::getInstance().reportError(ErrorSeverity::ERROR, ErrorCategory::NETWORK, \
                                          msg, details, __FUNCTION__, __FILE__, __LINE__, recoverable, action)

#define CONFIG_ERROR(msg, details, recoverable, action) \
    ErrorHandler::getInstance().reportError(ErrorSeverity::ERROR, ErrorCategory::CONFIG, \
                                          msg, details, __FUNCTION__, __FILE__, __LINE__, recoverable, action)

#define PERFORMANCE_WARNING(msg, details, action) \
    ErrorHandler::getInstance().reportError(ErrorSeverity::WARNING, ErrorCategory::PERFORMANCE, \
                                          msg, details, __FUNCTION__, __FILE__, __LINE__, true, action)
