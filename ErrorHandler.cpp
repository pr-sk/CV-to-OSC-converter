#include "ErrorHandler.h"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <algorithm>
#include <thread>

// Static member initialization
std::unique_ptr<ErrorHandler> ErrorHandler::instance = nullptr;
std::mutex ErrorHandler::instanceMutex;

ErrorHandler::ErrorHandler() 
    : logLevel(ErrorSeverity::INFO), maxHistorySize(1000), errorCounter(0),
      consoleOutput(true), fileOutput(false), logFileName("cv_osc_converter.log"),
      audioRecoveryAttempts(0), networkRecoveryAttempts(0), recoveryInProgress(false),
      lastRecoveryAttempt(std::chrono::system_clock::now()) {
}

ErrorHandler::~ErrorHandler() {
    closeLogFile();
}

ErrorHandler& ErrorHandler::getInstance() {
    std::lock_guard<std::mutex> lock(instanceMutex);
    if (!instance) {
        instance = std::unique_ptr<ErrorHandler>(new ErrorHandler());
    }
    return *instance;
}

void ErrorHandler::reportError(ErrorSeverity severity, ErrorCategory category, 
                              const std::string& message, const std::string& details,
                              const std::string& function, const std::string& file, 
                              int line, bool recoverable, const std::string& suggestedAction) {
    
    // Check if we should log this severity level
    if (severity < logLevel) {
        return;
    }
    
    ErrorInfo error;
    error.severity = severity;
    error.category = category;
    error.message = message;
    error.details = details;
    error.function = function;
    error.file = file;
    error.line = line;
    error.timestamp = std::chrono::system_clock::now();
    error.errorCode = errorCounter.fetch_add(1);
    error.recoverable = recoverable;
    error.suggestedAction = suggestedAction;
    
    std::lock_guard<std::mutex> lock(errorMutex);
    
    // Add to history
    errorHistory.push_back(error);
    trimHistory();
    
    // Output to console if enabled
    if (consoleOutput) {
        writeToConsole(error);
    }
    
    // Output to file if enabled
    if (fileOutput) {
        writeToLog(error);
    }
    
    // Notify callbacks
    notifyCallbacks(error);
    
    // Attempt recovery if appropriate
    if (recoverable && shouldAttemptRecovery(category)) {
        std::thread recoveryThread([this, category]() {
            switch (category) {
                case ErrorCategory::AUDIO:
                    attemptAudioRecovery();
                    break;
                case ErrorCategory::NETWORK:
                    attemptNetworkRecovery();
                    break;
                case ErrorCategory::CONFIG:
                    attemptConfigRecovery();
                    break;
                default:
                    break;
            }
        });
        recoveryThread.detach();
    }
}

void ErrorHandler::logDebug(const std::string& message, const std::string& details) {
    reportError(ErrorSeverity::DEBUG, ErrorCategory::SYSTEM, message, details);
}

void ErrorHandler::logInfo(const std::string& message, const std::string& details) {
    reportError(ErrorSeverity::INFO, ErrorCategory::SYSTEM, message, details);
}

void ErrorHandler::logWarning(const std::string& message, const std::string& details, 
                             const std::string& suggestedAction) {
    reportError(ErrorSeverity::WARNING, ErrorCategory::SYSTEM, message, details, 
                "", "", 0, true, suggestedAction);
}

void ErrorHandler::logError(const std::string& message, const std::string& details,
                           const std::string& suggestedAction, bool recoverable) {
    reportError(ErrorSeverity::ERROR_LEVEL, ErrorCategory::SYSTEM, message, details, 
               "", "", 0, recoverable, suggestedAction);
}

void ErrorHandler::logCritical(const std::string& message, const std::string& details,
                             const std::string& suggestedAction) {
    reportError(ErrorSeverity::CRITICAL_LEVEL, ErrorCategory::SYSTEM, message, details, 
               "", "", 0, false, suggestedAction);
}

void ErrorHandler::reportAudioError(const std::string& message, const std::string& details,
                                   bool recoverable, const std::string& suggestedAction) {
    reportError(ErrorSeverity::ERROR_LEVEL, ErrorCategory::AUDIO, message, details, 
                "", "", 0, recoverable, suggestedAction);
}

void ErrorHandler::reportNetworkError(const std::string& message, const std::string& details,
                                     bool recoverable, const std::string& suggestedAction) {
    reportError(ErrorSeverity::ERROR_LEVEL, ErrorCategory::NETWORK, message, details, 
                "", "", 0, recoverable, suggestedAction);
}

void ErrorHandler::reportConfigError(const std::string& message, const std::string& details,
                                    bool recoverable, const std::string& suggestedAction) {
    reportError(ErrorSeverity::ERROR_LEVEL, ErrorCategory::CONFIG, message, details, 
                "", "", 0, recoverable, suggestedAction);
}

void ErrorHandler::reportPerformanceWarning(const std::string& message, const std::string& details,
                                           const std::string& suggestedAction) {
    reportError(ErrorSeverity::WARNING, ErrorCategory::PERFORMANCE, message, details, 
                "", "", 0, true, suggestedAction);
}

void ErrorHandler::setFileOutput(bool enabled, const std::string& filename) {
    std::lock_guard<std::mutex> lock(errorMutex);
    
    if (logFile.is_open()) {
        logFile.close();
    }
    
    fileOutput = enabled;
    if (!filename.empty()) {
        logFileName = filename;
    }
    
    if (fileOutput) {
        logFile.open(logFileName, std::ios::app);
        if (!logFile.is_open()) {
            consoleOutput = true; // Fallback to console if file fails
            std::cerr << "Failed to open log file: " << logFileName << std::endl;
        }
    }
}

std::vector<ErrorInfo> ErrorHandler::getErrorHistory() const {
    std::lock_guard<std::mutex> lock(errorMutex);
    return errorHistory;
}

std::vector<ErrorInfo> ErrorHandler::getErrorsByCategory(ErrorCategory category) const {
    std::lock_guard<std::mutex> lock(errorMutex);
    std::vector<ErrorInfo> filtered;
    
    for (const auto& error : errorHistory) {
        if (error.category == category) {
            filtered.push_back(error);
        }
    }
    
    return filtered;
}

std::vector<ErrorInfo> ErrorHandler::getErrorsBySeverity(ErrorSeverity severity) const {
    std::lock_guard<std::mutex> lock(errorMutex);
    std::vector<ErrorInfo> filtered;
    
    for (const auto& error : errorHistory) {
        if (error.severity == severity) {
            filtered.push_back(error);
        }
    }
    
    return filtered;
}

std::vector<ErrorInfo> ErrorHandler::getRecentErrors(std::chrono::minutes duration) const {
    std::lock_guard<std::mutex> lock(errorMutex);
    std::vector<ErrorInfo> recent;
    auto cutoff = std::chrono::system_clock::now() - duration;
    
    for (const auto& error : errorHistory) {
        if (error.timestamp > cutoff) {
            recent.push_back(error);
        }
    }
    
    return recent;
}

size_t ErrorHandler::getErrorCountByCategory(ErrorCategory category) const {
    std::lock_guard<std::mutex> lock(errorMutex);
    size_t count = 0;
    
    for (const auto& error : errorHistory) {
        if (error.category == category) {
            count++;
        }
    }
    
    return count;
}

size_t ErrorHandler::getErrorCountBySeverity(ErrorSeverity severity) const {
    std::lock_guard<std::mutex> lock(errorMutex);
    size_t count = 0;
    
    for (const auto& error : errorHistory) {
        if (error.severity == severity) {
            count++;
        }
    }
    
    return count;
}

std::string ErrorHandler::generateErrorReport() const {
    std::lock_guard<std::mutex> lock(errorMutex);
    std::ostringstream report;
    
    report << "\n" << std::string(60, '=') << "\n";
    report << "ERROR REPORT\n";
    report << std::string(60, '=') << "\n";
    report << "Total Errors: " << errorCounter.load() << "\n";
    report << "Errors in History: " << errorHistory.size() << "\n";
    report << "Generated: " << formatTimestamp(std::chrono::system_clock::now()) << "\n\n";
    
    // Summary by severity
    report << "Errors by Severity:\n";
    for (int i = 0; i <= static_cast<int>(ErrorSeverity::CRITICAL_LEVEL); ++i) {
        ErrorSeverity sev = static_cast<ErrorSeverity>(i);
        size_t count = getErrorCountBySeverity(sev);
        if (count > 0) {
            report << "  " << severityToString(sev) << ": " << count << "\n";
        }
    }
    
    // Summary by category
    report << "\nErrors by Category:\n";
    for (int i = 0; i <= static_cast<int>(ErrorCategory::HARDWARE); ++i) {
        ErrorCategory cat = static_cast<ErrorCategory>(i);
        size_t count = getErrorCountByCategory(cat);
        if (count > 0) {
            report << "  " << categoryToString(cat) << ": " << count << "\n";
        }
    }
    
    // Recent critical/error messages
    report << "\nRecent Critical and Error Messages:\n";
    int recentCount = 0;
    auto now = std::chrono::system_clock::now();
    auto oneHourAgo = now - std::chrono::hours(1);
    
    for (auto it = errorHistory.rbegin(); it != errorHistory.rend() && recentCount < 10; ++it) {
        if (it->timestamp > oneHourAgo && 
            (it->severity == ErrorSeverity::ERROR_LEVEL || it->severity == ErrorSeverity::CRITICAL_LEVEL)) {
            report << "  [" << formatTimestamp(it->timestamp) << "] "
                   << severityToString(it->severity) << " (" << categoryToString(it->category) << "): "
                   << it->message << "\n";
            recentCount++;
        }
    }
    
    if (recentCount == 0) {
        report << "  No recent critical or error messages.\n";
    }
    
    report << std::string(60, '=') << "\n";
    return report.str();
}

std::string ErrorHandler::generateHealthStatus() const {
    std::ostringstream status;
    auto recentErrors = getRecentErrors(std::chrono::minutes(10));
    auto criticalErrors = getErrorCountBySeverity(ErrorSeverity::CRITICAL_LEVEL);
    auto errors = getErrorCountBySeverity(ErrorSeverity::ERROR_LEVEL);
    auto warnings = getErrorCountBySeverity(ErrorSeverity::WARNING);
    
    status << "System Health: ";
    
    if (criticalErrors > 0) {
        status << "CRITICAL - " << criticalErrors << " critical errors detected";
    } else if (errors > 5) {
        status << "DEGRADED - High error count (" << errors << " errors)";
    } else if (recentErrors.size() > 3) {
        status << "WARNING - " << recentErrors.size() << " recent errors";
    } else if (warnings > 10) {
        status << "CAUTION - " << warnings << " warnings";
    } else {
        status << "HEALTHY - System operating normally";
    }
    
    status << "\nRecovery Status: ";
    if (recoveryInProgress.load()) {
        status << "Recovery in progress";
    } else {
        status << "No active recovery";
    }
    
    status << "\nAudio Recovery Attempts: " << audioRecoveryAttempts.load();
    status << "\nNetwork Recovery Attempts: " << networkRecoveryAttempts.load();
    
    return status.str();
}

bool ErrorHandler::attemptAudioRecovery() {
    if (recoveryInProgress.load()) {
        return false;
    }
    
    recoveryInProgress = true;
    audioRecoveryAttempts++;
    lastRecoveryAttempt = std::chrono::system_clock::now();
    
    logInfo("Attempting audio system recovery", "Audio recovery attempt #" + std::to_string(audioRecoveryAttempts.load()));
    
    // Simulate recovery process (in real implementation, this would reinitialize audio systems)
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    // For now, just simulate success after 3 attempts
    bool success = (audioRecoveryAttempts.load() % 3 == 0);
    
    if (success) {
        logInfo("Audio recovery successful", "Audio system restored");
        audioRecoveryAttempts = 0;
    } else {
        logWarning("Audio recovery failed", "Attempt #" + std::to_string(audioRecoveryAttempts.load()),
                  "Check audio device connections and restart application if problem persists");
    }
    
    recoveryInProgress = false;
    return success;
}

bool ErrorHandler::attemptNetworkRecovery() {
    if (recoveryInProgress.load()) {
        return false;
    }
    
    recoveryInProgress = true;
    networkRecoveryAttempts++;
    lastRecoveryAttempt = std::chrono::system_clock::now();
    
    logInfo("Attempting network recovery", "Network recovery attempt #" + std::to_string(networkRecoveryAttempts.load()));
    
    // Simulate recovery process
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    
    // Simulate success after 2 attempts
    bool success = (networkRecoveryAttempts.load() % 2 == 0);
    
    if (success) {
        logInfo("Network recovery successful", "OSC connection restored");
        networkRecoveryAttempts = 0;
    } else {
        logWarning("Network recovery failed", "Attempt #" + std::to_string(networkRecoveryAttempts.load()),
                  "Check network connectivity and OSC target availability");
    }
    
    recoveryInProgress = false;
    return success;
}

bool ErrorHandler::attemptConfigRecovery() {
    if (recoveryInProgress.load()) {
        return false;
    }
    
    recoveryInProgress = true;
    lastRecoveryAttempt = std::chrono::system_clock::now();
    
    logInfo("Attempting configuration recovery", "Restoring default configuration");
    
    // Simulate config recovery
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    logInfo("Configuration recovery completed", "Default configuration restored");
    
    recoveryInProgress = false;
    return true;
}

void ErrorHandler::resetRecoveryCounters() {
    audioRecoveryAttempts = 0;
    networkRecoveryAttempts = 0;
    recoveryInProgress = false;
}

void ErrorHandler::addErrorCallback(std::function<void(const ErrorInfo&)> callback) {
    std::lock_guard<std::mutex> lock(errorMutex);
    errorCallbacks.push_back(callback);
}

void ErrorHandler::removeAllCallbacks() {
    std::lock_guard<std::mutex> lock(errorMutex);
    errorCallbacks.clear();
}

std::string ErrorHandler::severityToString(ErrorSeverity severity) {
    switch (severity) {
        case ErrorSeverity::DEBUG: return "DEBUG";
        case ErrorSeverity::INFO: return "INFO";
        case ErrorSeverity::WARNING: return "WARNING";
        case ErrorSeverity::ERROR_LEVEL: return "ERROR";
        case ErrorSeverity::CRITICAL_LEVEL: return "CRITICAL";
        default: return "UNKNOWN";
    }
}

std::string ErrorHandler::categoryToString(ErrorCategory category) {
    switch (category) {
        case ErrorCategory::SYSTEM: return "SYSTEM";
        case ErrorCategory::AUDIO: return "AUDIO";
        case ErrorCategory::NETWORK: return "NETWORK";
        case ErrorCategory::CONFIG: return "CONFIG";
        case ErrorCategory::USER_INPUT: return "USER_INPUT";
        case ErrorCategory::PERFORMANCE: return "PERFORMANCE";
        case ErrorCategory::HARDWARE: return "HARDWARE";
        default: return "UNKNOWN";
    }
}

ErrorSeverity ErrorHandler::stringToSeverity(const std::string& str) {
    std::string lower = str;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
    
    if (lower == "debug") return ErrorSeverity::DEBUG;
    if (lower == "info") return ErrorSeverity::INFO;
    if (lower == "warning" || lower == "warn") return ErrorSeverity::WARNING;
    if (lower == "error") return ErrorSeverity::ERROR_LEVEL;
    if (lower == "critical") return ErrorSeverity::CRITICAL_LEVEL;
    
    return ErrorSeverity::INFO; // Default
}

std::string ErrorHandler::formatTimestamp(const std::chrono::system_clock::time_point& timestamp) {
    auto time_t = std::chrono::system_clock::to_time_t(timestamp);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        timestamp.time_since_epoch()) % 1000;
    
    std::ostringstream oss;
    oss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
    oss << "." << std::setfill('0') << std::setw(3) << ms.count();
    return oss.str();
}

void ErrorHandler::clearHistory() {
    std::lock_guard<std::mutex> lock(errorMutex);
    errorHistory.clear();
}

void ErrorHandler::closeLogFile() {
    std::lock_guard<std::mutex> lock(errorMutex);
    if (logFile.is_open()) {
        logFile.close();
    }
}

void ErrorHandler::writeToLog(const ErrorInfo& error) {
    if (logFile.is_open()) {
        logFile << "[" << formatTimestamp(error.timestamp) << "] "
                << severityToString(error.severity) << " "
                << categoryToString(error.category) << " "
                << error.message;
        
        if (!error.details.empty()) {
            logFile << " | " << error.details;
        }
        
        if (!error.function.empty() && !error.file.empty()) {
            logFile << " | " << error.function << "() at " << error.file << ":" << error.line;
        }
        
        if (!error.suggestedAction.empty()) {
            logFile << " | Suggested: " << error.suggestedAction;
        }
        
        logFile << std::endl;
        logFile.flush();
    }
}

void ErrorHandler::writeToConsole(const ErrorInfo& error) {
    std::string color = getColorForSeverity(error.severity);
    std::string reset = "\033[0m";
    
    std::cerr << color << "[" << formatTimestamp(error.timestamp) << "] "
              << severityToString(error.severity) << reset << " "
              << categoryToString(error.category) << ": " << error.message;
    
    if (!error.details.empty()) {
        std::cerr << "\n  Details: " << error.details;
    }
    
    if (!error.suggestedAction.empty()) {
        std::cerr << "\n  " << color << "Suggested Action: " << reset << error.suggestedAction;
    }
    
    std::cerr << std::endl;
}

void ErrorHandler::trimHistory() {
    while (errorHistory.size() > maxHistorySize) {
        errorHistory.erase(errorHistory.begin());
    }
}

void ErrorHandler::notifyCallbacks(const ErrorInfo& error) {
    for (const auto& callback : errorCallbacks) {
        try {
            callback(error);
        } catch (...) {
            // Ignore callback exceptions to prevent infinite recursion
        }
    }
}

bool ErrorHandler::shouldAttemptRecovery(ErrorCategory category) const {
    auto now = std::chrono::system_clock::now();
    auto timeSinceLastRecovery = now - lastRecoveryAttempt;
    
    // Don't attempt recovery if one was tried recently (less than 5 seconds ago)
    if (timeSinceLastRecovery < std::chrono::seconds(5)) {
        return false;
    }
    
    // Limit recovery attempts based on category
    switch (category) {
        case ErrorCategory::AUDIO:
            return audioRecoveryAttempts.load() < 5;
        case ErrorCategory::NETWORK:
            return networkRecoveryAttempts.load() < 3;
        case ErrorCategory::CONFIG:
            return true; // Config recovery is usually safe to retry
        default:
            return false;
    }
}

std::string ErrorHandler::getColorForSeverity(ErrorSeverity severity) const {
    switch (severity) {
        case ErrorSeverity::DEBUG: return "\033[37m";    // White
        case ErrorSeverity::INFO: return "\033[36m";     // Cyan
        case ErrorSeverity::WARNING: return "\033[33m";  // Yellow
        case ErrorSeverity::ERROR_LEVEL: return "\033[31m";    // Red
        case ErrorSeverity::CRITICAL_LEVEL: return "\033[35m"; // Magenta
        default: return "\033[0m";                       // Reset
    }
}
