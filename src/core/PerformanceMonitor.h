#pragma once

#include <chrono>
#include <vector>
#include <memory>
#include <atomic>
#include <mutex>
#include <map>
#include <string>
#include <fstream>
#include <thread>
#include <functional>
#include <deque>
#include <nlohmann/json.hpp>

struct PerformanceMetrics {
    // Timing metrics
    std::chrono::nanoseconds processingTime{0};
    std::chrono::nanoseconds networkLatency{0};
    std::chrono::nanoseconds totalCycleTime{0};
    
    // Throughput metrics
    double actualUpdateRate = 0.0;
    double expectedUpdateRate = 0.0;
    double efficiency = 0.0; // actualUpdateRate / expectedUpdateRate
    
    // Resource usage
    double cpuUsage = 0.0;
    size_t memoryUsage = 0;
    size_t peakMemoryUsage = 0;
    
    // Audio metrics
    int droppedSamples = 0;
    int bufferUnderruns = 0;
    double signalToNoiseRatio = 0.0;
    
    // Network metrics
    int oscMessagesSent = 0;
    int oscMessagesFailed = 0;
    double packetLossRate = 0.0;
    
    // System health
    double systemLoad = 0.0;
    double temperature = 0.0; // CPU temperature if available
    
    // Timestamps
    std::chrono::system_clock::time_point timestamp;
    
    PerformanceMetrics() : timestamp(std::chrono::system_clock::now()) {}
};

struct PerformanceAlert {
    enum class Severity { Info, Warning, Critical };
    enum class Category { CPU, Memory, Network, Audio, Latency, General };
    
    Severity severity;
    Category category;
    std::string message;
    std::string details;
    std::chrono::system_clock::time_point timestamp;
    double value;
    double threshold;
    
    PerformanceAlert(Severity sev, Category cat, const std::string& msg, 
                    double val = 0.0, double thresh = 0.0)
        : severity(sev), category(cat), message(msg), 
          timestamp(std::chrono::system_clock::now()), value(val), threshold(thresh) {}
};

class PerformanceMonitor {
public:
    // Configuration
    struct MonitorConfig {
        bool enabled = true;
        std::chrono::milliseconds updateInterval{1000}; // 1 second
        size_t maxHistorySize = 300; // 5 minutes at 1 second intervals
        bool enableDetailedMetrics = true;
        bool enableAlerts = true;
        bool logToFile = false;
        std::string logFileName = "performance.log";
        
        // Thresholds for alerts
        double cpuThresholdWarning = 70.0;
        double cpuThresholdCritical = 90.0;
        double memoryThresholdWarning = 80.0; // MB
        double memoryThresholdCritical = 150.0; // MB
        double latencyThresholdWarning = 20.0; // ms
        double latencyThresholdCritical = 50.0; // ms
        double efficiencyThresholdWarning = 0.8; // 80%
        double efficiencyThresholdCritical = 0.6; // 60%
        
        // OSC error suppression
        bool suppressOSCWarnings = false;
        std::chrono::seconds oscWarningSuppressDuration{30}; // Suppress for 30 seconds
    };

private:
    MonitorConfig config;
    
    // Metrics storage
    std::deque<PerformanceMetrics> metricsHistory;
    std::vector<PerformanceAlert> activeAlerts;
    mutable std::mutex metricsMutex;
    
    // Monitoring thread
    std::unique_ptr<std::thread> monitorThread;
    std::atomic<bool> monitoring{false};
    
    // Counters and timers
    std::atomic<uint64_t> cycleCounter{0};
    std::atomic<uint64_t> oscSentCounter{0};
    std::atomic<uint64_t> oscFailedCounter{0};
    std::atomic<uint64_t> droppedSamplesCounter{0};
    std::atomic<uint64_t> bufferUnderrunsCounter{0};
    
    // OSC warning suppression
    std::chrono::steady_clock::time_point lastOSCWarningTime;
    std::atomic<bool> oscWarningsCurrentlySuppressed{false};
    
    // Timing measurement
    std::chrono::steady_clock::time_point lastCycleTime;
    std::chrono::steady_clock::time_point startTime;
    
    // Callbacks
    std::vector<std::function<void(const PerformanceMetrics&)>> metricsCallbacks;
    std::vector<std::function<void(const PerformanceAlert&)>> alertCallbacks;
    
    // File logging
    std::unique_ptr<std::ofstream> logFile;
    
public:
    PerformanceMonitor();
    ~PerformanceMonitor();
    
    // Configuration
    void setConfig(const MonitorConfig& cfg);
    MonitorConfig getConfig() const { return config; }
    void setUpdateInterval(std::chrono::milliseconds interval);
    void setMaxHistorySize(size_t size);
    void enableFileLogging(bool enable, const std::string& filename = "");
    void enableOSCWarningSuppression(bool enable, std::chrono::seconds duration = std::chrono::seconds{30});
    
    // Control
    void start();
    void stop();
    bool isMonitoring() const { return monitoring.load(); }
    
    // Metrics recording
    void recordCycleStart();
    void recordCycleEnd();
    void recordProcessingTime(std::chrono::nanoseconds duration);
    void recordNetworkLatency(std::chrono::nanoseconds latency);
    void recordOSCMessageSent();
    void recordOSCMessageFailed();
    void recordDroppedSamples(int count);
    void recordBufferUnderrun();
    
    // Metrics retrieval
    PerformanceMetrics getCurrentMetrics() const;
    PerformanceMetrics getAverageMetrics(std::chrono::minutes duration) const;
    std::vector<PerformanceMetrics> getMetricsHistory() const;
    std::vector<PerformanceMetrics> getMetricsSince(std::chrono::system_clock::time_point since) const;
    
    // Statistics
    struct PerformanceStatistics {
        double avgCpuUsage = 0.0;
        double maxCpuUsage = 0.0;
        double avgMemoryUsage = 0.0;
        double maxMemoryUsage = 0.0;
        double avgLatency = 0.0;
        double maxLatency = 0.0;
        double avgEfficiency = 0.0;
        double minEfficiency = 1.0;
        uint64_t totalCycles = 0;
        uint64_t totalOSCMessages = 0;
        uint64_t totalDroppedSamples = 0;
        uint64_t totalBufferUnderruns = 0;
        std::chrono::minutes uptime{0};
    };
    PerformanceStatistics getStatistics() const;
    
    // Alerts
    std::vector<PerformanceAlert> getActiveAlerts() const;
    void clearAlerts();
    void setAlertThresholds(double cpuWarning, double cpuCritical, 
                           double memoryWarning, double memoryCritical,
                           double latencyWarning, double latencyCritical);
    
    // Callbacks
    void addMetricsCallback(std::function<void(const PerformanceMetrics&)> callback);
    void addAlertCallback(std::function<void(const PerformanceAlert&)> callback);
    void clearCallbacks();
    
    // Reporting
    std::string generateReport() const;
    nlohmann::json exportMetricsAsJson() const;
    bool exportMetricsToCSV(const std::string& filename) const;
    
    // System information
    static double getCurrentCPUUsage();
    static size_t getCurrentMemoryUsage();
    static double getSystemLoad();
    static double getCPUTemperature(); // Platform-specific
    
private:
    // Internal monitoring loop
    void monitoringLoop();
    
    // Metrics calculation
    PerformanceMetrics calculateCurrentMetrics() const;
    void updateCounters();
    void checkThresholds(const PerformanceMetrics& metrics);
    
    // Alert management
    void triggerAlert(PerformanceAlert::Severity severity, 
                     PerformanceAlert::Category category,
                     const std::string& message, double value = 0.0, double threshold = 0.0);
    void clearOldAlerts();
    
    // File operations
    void openLogFile();
    void closeLogFile();
    void writeMetricsToFile(const PerformanceMetrics& metrics);
    
    // Utility functions
    std::string formatDuration(std::chrono::nanoseconds duration) const;
    std::string formatTimestamp(std::chrono::system_clock::time_point timestamp) const;
    
    // Platform-specific implementations
    double getCPUUsageImpl() const;
    size_t getMemoryUsageImpl() const;
    double getSystemLoadImpl() const;
};

// RAII helper for automatic timing measurement
class ScopedTimer {
private:
    std::chrono::steady_clock::time_point startTime;
    std::function<void(std::chrono::nanoseconds)> recordFunction;

public:
    ScopedTimer(PerformanceMonitor&, std::function<void(std::chrono::nanoseconds)> recorder)
        : startTime(std::chrono::steady_clock::now()), recordFunction(recorder) {}
    
    ~ScopedTimer() {
        auto endTime = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(endTime - startTime);
        recordFunction(duration);
    }
};

// Convenience macros for timing measurement
#define MEASURE_PROCESSING_TIME(monitor) \
    ScopedTimer _timer(monitor, [&](std::chrono::nanoseconds d) { monitor.recordProcessingTime(d); })

#define MEASURE_NETWORK_LATENCY(monitor) \
    ScopedTimer _timer(monitor, [&](std::chrono::nanoseconds d) { monitor.recordNetworkLatency(d); })

// Factory for creating common monitoring configurations
class MonitorConfigFactory {
public:
    static PerformanceMonitor::MonitorConfig createHighPerformanceConfig() {
        PerformanceMonitor::MonitorConfig config;
        config.updateInterval = std::chrono::milliseconds(500); // 2 Hz updates
        config.enableDetailedMetrics = true;
        config.enableAlerts = true;
        config.cpuThresholdWarning = 60.0;
        config.cpuThresholdCritical = 80.0;
        config.latencyThresholdWarning = 10.0;
        config.latencyThresholdCritical = 25.0;
        return config;
    }
    
    static PerformanceMonitor::MonitorConfig createProductionConfig() {
        PerformanceMonitor::MonitorConfig config;
        config.updateInterval = std::chrono::milliseconds(5000); // 0.2 Hz updates
        config.enableDetailedMetrics = false;
        config.enableAlerts = true;
        config.logToFile = true;
        config.maxHistorySize = 720; // 1 hour at 5 second intervals
        return config;
    }
    
    static PerformanceMonitor::MonitorConfig createDebugConfig() {
        PerformanceMonitor::MonitorConfig config;
        config.updateInterval = std::chrono::milliseconds(100); // 10 Hz updates
        config.enableDetailedMetrics = true;
        config.enableAlerts = false;
        config.logToFile = true;
        config.maxHistorySize = 6000; // 10 minutes at 100ms intervals
        return config;
    }
};
