#include "PerformanceMonitor.h"
#include "ErrorHandler.h"
#include <sstream>
#include <iomanip>
#include <algorithm>

#ifdef __APPLE__
#include <mach/mach.h>
#include <sys/sysctl.h>
#include <mach/host_info.h>
#include <mach/mach_host.h>
#include <mach/task_info.h>
#include <mach/task.h>
#elif __linux__
#include <unistd.h>
#include <sys/resource.h>
#include <fstream>
#include <sstream>
#endif

PerformanceMonitor::PerformanceMonitor() {
    startTime = std::chrono::steady_clock::now();
    lastCycleTime = startTime;
}

PerformanceMonitor::~PerformanceMonitor() {
    stop();
}

void PerformanceMonitor::setConfig(const MonitorConfig& cfg) {
    std::lock_guard<std::mutex> lock(metricsMutex);
    config = cfg;
    
    if (config.logToFile && !logFile) {
        openLogFile();
    } else if (!config.logToFile && logFile) {
        closeLogFile();
    }
    
    // Resize history if needed
    while (metricsHistory.size() > config.maxHistorySize) {
        metricsHistory.pop_front();
    }
}

void PerformanceMonitor::setUpdateInterval(std::chrono::milliseconds interval) {
    config.updateInterval = interval;
}

void PerformanceMonitor::setMaxHistorySize(size_t size) {
    std::lock_guard<std::mutex> lock(metricsMutex);
    config.maxHistorySize = size;
    while (metricsHistory.size() > size) {
        metricsHistory.pop_front();
    }
}

void PerformanceMonitor::enableFileLogging(bool enable, const std::string& filename) {
    config.logToFile = enable;
    if (!filename.empty()) {
        config.logFileName = filename;
    }
    
    if (enable && !logFile) {
        openLogFile();
    } else if (!enable && logFile) {
        closeLogFile();
    }
}

void PerformanceMonitor::start() {
    if (monitoring.load()) {
        return; // Already monitoring
    }
    
    monitoring = true;
    monitorThread = std::make_unique<std::thread>(&PerformanceMonitor::monitoringLoop, this);
    
    ErrorHandler::getInstance().logInfo("Performance monitoring started", 
                                      "Update interval: " + std::to_string(config.updateInterval.count()) + "ms");
}

void PerformanceMonitor::stop() {
    monitoring = false;
    if (monitorThread && monitorThread->joinable()) {
        monitorThread->join();
    }
    
    closeLogFile();
    ErrorHandler::getInstance().logInfo("Performance monitoring stopped", "");
}

void PerformanceMonitor::recordCycleStart() {
    lastCycleTime = std::chrono::steady_clock::now();
}

void PerformanceMonitor::recordCycleEnd() {
    auto now = std::chrono::steady_clock::now();
    auto cycleDuration = now - lastCycleTime;
    (void)cycleDuration; // Suppress unused variable warning
    
    cycleCounter++;
    lastCycleTime = now;
}

void PerformanceMonitor::recordProcessingTime(std::chrono::nanoseconds duration) {
    (void)duration; // Suppress unused parameter warning
    // This will be used by the monitoring loop to calculate averages
    // For now, we'll store the latest value
}

void PerformanceMonitor::recordNetworkLatency(std::chrono::nanoseconds latency) {
    (void)latency; // Suppress unused parameter warning
    // Similar to processing time
}

void PerformanceMonitor::recordOSCMessageSent() {
    oscSentCounter++;
}

void PerformanceMonitor::recordOSCMessageFailed() {
    oscFailedCounter++;
}

void PerformanceMonitor::recordDroppedSamples(int count) {
    droppedSamplesCounter += count;
}

void PerformanceMonitor::recordBufferUnderrun() {
    bufferUnderrunsCounter++;
}

PerformanceMetrics PerformanceMonitor::getCurrentMetrics() const {
    return calculateCurrentMetrics();
}

PerformanceMetrics PerformanceMonitor::getAverageMetrics(std::chrono::minutes duration) const {
    std::lock_guard<std::mutex> lock(metricsMutex);
    
    auto cutoffTime = std::chrono::system_clock::now() - duration;
    PerformanceMetrics avgMetrics;
    
    std::vector<PerformanceMetrics> relevantMetrics;
    for (const auto& metrics : metricsHistory) {
        if (metrics.timestamp >= cutoffTime) {
            relevantMetrics.push_back(metrics);
        }
    }
    
    if (relevantMetrics.empty()) {
        return avgMetrics;
    }
    
    // Calculate averages
    double totalCpu = 0, totalMemory = 0, totalEfficiency = 0;
    uint64_t totalOscSent = 0, totalOscFailed = 0, totalDropped = 0, totalUnderruns = 0;
    
    for (const auto& metrics : relevantMetrics) {
        totalCpu += metrics.cpuUsage;
        totalMemory += metrics.memoryUsage;
        totalEfficiency += metrics.efficiency;
        totalOscSent += metrics.oscMessagesSent;
        totalOscFailed += metrics.oscMessagesFailed;
        totalDropped += metrics.droppedSamples;
        totalUnderruns += metrics.bufferUnderruns;
    }
    
    size_t count = relevantMetrics.size();
    avgMetrics.cpuUsage = totalCpu / count;
    avgMetrics.memoryUsage = static_cast<size_t>(totalMemory / count);
    avgMetrics.efficiency = totalEfficiency / count;
    avgMetrics.oscMessagesSent = static_cast<int>(totalOscSent / count);
    avgMetrics.oscMessagesFailed = static_cast<int>(totalOscFailed / count);
    avgMetrics.droppedSamples = static_cast<int>(totalDropped / count);
    avgMetrics.bufferUnderruns = static_cast<int>(totalUnderruns / count);
    
    return avgMetrics;
}

std::vector<PerformanceMetrics> PerformanceMonitor::getMetricsHistory() const {
    std::lock_guard<std::mutex> lock(metricsMutex);
    return std::vector<PerformanceMetrics>(metricsHistory.begin(), metricsHistory.end());
}

PerformanceMonitor::PerformanceStatistics PerformanceMonitor::getStatistics() const {
    std::lock_guard<std::mutex> lock(metricsMutex);
    
    PerformanceStatistics stats;
    
    if (metricsHistory.empty()) {
        return stats;
    }
    
    // Calculate averages and extremes
    double totalCpu = 0, totalMemory = 0, totalLatency = 0, totalEfficiency = 0;
    stats.maxCpuUsage = 0;
    stats.maxMemoryUsage = 0;
    stats.maxLatency = 0;
    stats.minEfficiency = 1.0;
    
    for (const auto& metrics : metricsHistory) {
        totalCpu += metrics.cpuUsage;
        totalMemory += metrics.memoryUsage;
        totalLatency += std::chrono::duration<double, std::milli>(metrics.networkLatency).count();
        totalEfficiency += metrics.efficiency;
        
        stats.maxCpuUsage = std::max(stats.maxCpuUsage, metrics.cpuUsage);
        stats.maxMemoryUsage = std::max(stats.maxMemoryUsage, static_cast<double>(metrics.memoryUsage));
        stats.maxLatency = std::max(stats.maxLatency, 
                                   std::chrono::duration<double, std::milli>(metrics.networkLatency).count());
        stats.minEfficiency = std::min(stats.minEfficiency, metrics.efficiency);
        
        stats.totalOSCMessages += metrics.oscMessagesSent;
        stats.totalDroppedSamples += metrics.droppedSamples;
        stats.totalBufferUnderruns += metrics.bufferUnderruns;
    }
    
    size_t count = metricsHistory.size();
    stats.avgCpuUsage = totalCpu / count;
    stats.avgMemoryUsage = totalMemory / count;
    stats.avgLatency = totalLatency / count;
    stats.avgEfficiency = totalEfficiency / count;
    
    stats.totalCycles = cycleCounter.load();
    
    // Calculate uptime
    auto now = std::chrono::steady_clock::now();
    auto uptime = std::chrono::duration_cast<std::chrono::minutes>(now - startTime);
    stats.uptime = uptime;
    
    return stats;
}

std::vector<PerformanceAlert> PerformanceMonitor::getActiveAlerts() const {
    std::lock_guard<std::mutex> lock(metricsMutex);
    return activeAlerts;
}

void PerformanceMonitor::clearAlerts() {
    std::lock_guard<std::mutex> lock(metricsMutex);
    activeAlerts.clear();
}

void PerformanceMonitor::addMetricsCallback(std::function<void(const PerformanceMetrics&)> callback) {
    metricsCallbacks.push_back(callback);
}

void PerformanceMonitor::addAlertCallback(std::function<void(const PerformanceAlert&)> callback) {
    alertCallbacks.push_back(callback);
}

void PerformanceMonitor::clearCallbacks() {
    metricsCallbacks.clear();
    alertCallbacks.clear();
}

std::string PerformanceMonitor::generateReport() const {
    auto stats = getStatistics();
    std::ostringstream report;
    
    report << "Performance Report\n";
    report << "==================\n\n";
    
    report << "System Statistics:\n";
    report << "  Uptime: " << stats.uptime.count() << " minutes\n";
    report << "  Total Cycles: " << stats.totalCycles << "\n";
    report << "  Total OSC Messages: " << stats.totalOSCMessages << "\n";
    report << "  Total Dropped Samples: " << stats.totalDroppedSamples << "\n";
    report << "  Total Buffer Underruns: " << stats.totalBufferUnderruns << "\n\n";
    
    report << "Resource Usage:\n";
    report << "  Average CPU: " << std::fixed << std::setprecision(1) << stats.avgCpuUsage << "%\n";
    report << "  Peak CPU: " << std::fixed << std::setprecision(1) << stats.maxCpuUsage << "%\n";
    report << "  Average Memory: " << std::fixed << std::setprecision(1) << stats.avgMemoryUsage << " MB\n";
    report << "  Peak Memory: " << std::fixed << std::setprecision(1) << stats.maxMemoryUsage << " MB\n\n";
    
    report << "Performance Metrics:\n";
    report << "  Average Latency: " << std::fixed << std::setprecision(2) << stats.avgLatency << " ms\n";
    report << "  Peak Latency: " << std::fixed << std::setprecision(2) << stats.maxLatency << " ms\n";
    report << "  Average Efficiency: " << std::fixed << std::setprecision(1) << (stats.avgEfficiency * 100) << "%\n";
    report << "  Minimum Efficiency: " << std::fixed << std::setprecision(1) << (stats.minEfficiency * 100) << "%\n\n";
    
    // Active alerts
    auto alerts = getActiveAlerts();
    if (!alerts.empty()) {
        report << "Active Alerts (" << alerts.size() << "):\n";
        for (const auto& alert : alerts) {
            report << "  [" << (alert.severity == PerformanceAlert::Severity::Critical ? "CRITICAL" : 
                               alert.severity == PerformanceAlert::Severity::Warning ? "WARNING" : "INFO")
                   << "] " << alert.message << "\n";
        }
    }
    
    return report.str();
}

void PerformanceMonitor::monitoringLoop() {
    while (monitoring.load()) {
        auto metrics = calculateCurrentMetrics();
        
        // Store metrics
        {
            std::lock_guard<std::mutex> lock(metricsMutex);
            metricsHistory.push_back(metrics);
            
            // Trim history if needed
            while (metricsHistory.size() > config.maxHistorySize) {
                metricsHistory.pop_front();
            }
        }
        
        // Check thresholds and trigger alerts
        if (config.enableAlerts) {
            checkThresholds(metrics);
        }
        
        // Write to log file
        if (config.logToFile && logFile) {
            writeMetricsToFile(metrics);
        }
        
        // Call callbacks
        for (const auto& callback : metricsCallbacks) {
            try {
                callback(metrics);
            } catch (const std::exception& e) {
                ErrorHandler::getInstance().logError("Metrics callback failed", e.what());
            }
        }
        
        std::this_thread::sleep_for(config.updateInterval);
    }
}

PerformanceMetrics PerformanceMonitor::calculateCurrentMetrics() const {
    PerformanceMetrics metrics;
    
    // Get system metrics
    metrics.cpuUsage = getCPUUsageImpl();
    metrics.memoryUsage = getMemoryUsageImpl();
    metrics.systemLoad = getSystemLoadImpl();
    
    // Calculate rates
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - startTime);
    
    if (elapsed.count() > 0) {
        metrics.actualUpdateRate = static_cast<double>(cycleCounter.load()) / elapsed.count();
        metrics.expectedUpdateRate = 1000.0 / config.updateInterval.count();
        metrics.efficiency = metrics.expectedUpdateRate > 0 ? 
                           (metrics.actualUpdateRate / metrics.expectedUpdateRate) : 0.0;
    }
    
    // Network metrics
    uint64_t totalSent = oscSentCounter.load();
    uint64_t totalFailed = oscFailedCounter.load();
    
    metrics.oscMessagesSent = static_cast<int>(totalSent);
    metrics.oscMessagesFailed = static_cast<int>(totalFailed);
    
    if (totalSent > 0) {
        metrics.packetLossRate = static_cast<double>(totalFailed) / (totalSent + totalFailed);
    }
    
    // Audio metrics
    metrics.droppedSamples = static_cast<int>(droppedSamplesCounter.load());
    metrics.bufferUnderruns = static_cast<int>(bufferUnderrunsCounter.load());
    
    return metrics;
}

void PerformanceMonitor::checkThresholds(const PerformanceMetrics& metrics) {
    // CPU usage alerts
    if (metrics.cpuUsage > config.cpuThresholdCritical) {
        triggerAlert(PerformanceAlert::Severity::Critical, PerformanceAlert::Category::CPU,
                    "Critical CPU usage", metrics.cpuUsage, config.cpuThresholdCritical);
    } else if (metrics.cpuUsage > config.cpuThresholdWarning) {
        triggerAlert(PerformanceAlert::Severity::Warning, PerformanceAlert::Category::CPU,
                    "High CPU usage", metrics.cpuUsage, config.cpuThresholdWarning);
    }
    
    // Memory usage alerts
    double memoryMB = static_cast<double>(metrics.memoryUsage);
    if (memoryMB > config.memoryThresholdCritical) {
        triggerAlert(PerformanceAlert::Severity::Critical, PerformanceAlert::Category::Memory,
                    "Critical memory usage", memoryMB, config.memoryThresholdCritical);
    } else if (memoryMB > config.memoryThresholdWarning) {
        triggerAlert(PerformanceAlert::Severity::Warning, PerformanceAlert::Category::Memory,
                    "High memory usage", memoryMB, config.memoryThresholdWarning);
    }
    
    // Efficiency alerts
    if (metrics.efficiency < config.efficiencyThresholdCritical) {
        triggerAlert(PerformanceAlert::Severity::Critical, PerformanceAlert::Category::General,
                    "Critical performance degradation", metrics.efficiency, config.efficiencyThresholdCritical);
    } else if (metrics.efficiency < config.efficiencyThresholdWarning) {
        triggerAlert(PerformanceAlert::Severity::Warning, PerformanceAlert::Category::General,
                    "Performance degradation", metrics.efficiency, config.efficiencyThresholdWarning);
    }
    
    // Network alerts
    if (metrics.packetLossRate > 0.1) { // 10% packet loss
        triggerAlert(PerformanceAlert::Severity::Warning, PerformanceAlert::Category::Network,
                    "High packet loss rate", metrics.packetLossRate, 0.1);
    }
    
    // Audio alerts
    if (metrics.bufferUnderruns > 0) {
        triggerAlert(PerformanceAlert::Severity::Warning, PerformanceAlert::Category::Audio,
                    "Audio buffer underruns detected", metrics.bufferUnderruns, 0);
    }
}

void PerformanceMonitor::triggerAlert(PerformanceAlert::Severity severity, 
                                     PerformanceAlert::Category category,
                                     const std::string& message, double value, double threshold) {
    PerformanceAlert alert(severity, category, message, value, threshold);
    
    {
        std::lock_guard<std::mutex> lock(metricsMutex);
        
        // Check if similar alert already exists
        auto it = std::find_if(activeAlerts.begin(), activeAlerts.end(),
            [&](const PerformanceAlert& existing) {
                return existing.category == category && existing.message == message;
            });
        
        if (it != activeAlerts.end()) {
            // Update existing alert
            *it = alert;
        } else {
            // Add new alert
            activeAlerts.push_back(alert);
        }
        
        // Limit number of active alerts
        if (activeAlerts.size() > 50) {
            activeAlerts.erase(activeAlerts.begin());
        }
    }
    
    // Call alert callbacks
    for (const auto& callback : alertCallbacks) {
        try {
            callback(alert);
        } catch (const std::exception& e) {
            ErrorHandler::getInstance().logError("Alert callback failed", e.what());
        }
    }
}

void PerformanceMonitor::openLogFile() {
    try {
        logFile = std::make_unique<std::ofstream>(config.logFileName, std::ios::app);
        if (logFile->is_open()) {
            *logFile << "# Performance Log Started: " << formatTimestamp(std::chrono::system_clock::now()) << "\n";
            *logFile << "timestamp,cpu_usage,memory_usage,efficiency,osc_sent,osc_failed,dropped_samples,underruns\n";
        }
    } catch (const std::exception& e) {
        ErrorHandler::getInstance().logError("Failed to open performance log file", e.what());
        logFile.reset();
    }
}

void PerformanceMonitor::closeLogFile() {
    if (logFile) {
        logFile->close();
        logFile.reset();
    }
}

void PerformanceMonitor::writeMetricsToFile(const PerformanceMetrics& metrics) {
    if (!logFile || !logFile->is_open()) {
        return;
    }
    
    try {
        *logFile << formatTimestamp(metrics.timestamp) << ","
                 << metrics.cpuUsage << ","
                 << metrics.memoryUsage << ","
                 << metrics.efficiency << ","
                 << metrics.oscMessagesSent << ","
                 << metrics.oscMessagesFailed << ","
                 << metrics.droppedSamples << ","
                 << metrics.bufferUnderruns << "\n";
        logFile->flush();
    } catch (const std::exception& e) {
        ErrorHandler::getInstance().logError("Failed to write performance metrics to file", e.what());
    }
}

std::string PerformanceMonitor::formatTimestamp(std::chrono::system_clock::time_point timestamp) const {
    auto time_t = std::chrono::system_clock::to_time_t(timestamp);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        timestamp.time_since_epoch()) % 1000;
    
    std::ostringstream oss;
    oss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
    oss << '.' << std::setfill('0') << std::setw(3) << ms.count();
    return oss.str();
}

// Platform-specific implementations
#ifdef __APPLE__
double PerformanceMonitor::getCPUUsageImpl() const {
    host_cpu_load_info_data_t cpuinfo;
    mach_msg_type_number_t count = HOST_CPU_LOAD_INFO_COUNT;
    
    if (host_statistics(mach_host_self(), HOST_CPU_LOAD_INFO, 
                       (host_info_t)&cpuinfo, &count) == KERN_SUCCESS) {
        
        static unsigned int lastTotalTicks = 0;
        static unsigned int lastIdleTicks = 0;
        
        unsigned int totalTicks = cpuinfo.cpu_ticks[CPU_STATE_USER] + 
                                 cpuinfo.cpu_ticks[CPU_STATE_SYSTEM] + 
                                 cpuinfo.cpu_ticks[CPU_STATE_IDLE] + 
                                 cpuinfo.cpu_ticks[CPU_STATE_NICE];
        
        unsigned int idleTicks = cpuinfo.cpu_ticks[CPU_STATE_IDLE];
        
        unsigned int totalTicksSinceLastTime = totalTicks - lastTotalTicks;
        unsigned int idleTicksSinceLastTime = idleTicks - lastIdleTicks;
        
        lastTotalTicks = totalTicks;
        lastIdleTicks = idleTicks;
        
        if (totalTicksSinceLastTime > 0) {
            double usage = 100.0 * (1.0 - (double)idleTicksSinceLastTime / totalTicksSinceLastTime);
            return std::max(0.0, std::min(100.0, usage));
        }
    }
    
    return 0.0;
}

size_t PerformanceMonitor::getMemoryUsageImpl() const {
    struct task_basic_info info;
    mach_msg_type_number_t size = sizeof(info);
    
    if (task_info(mach_task_self(), TASK_BASIC_INFO, (task_info_t)&info, &size) == KERN_SUCCESS) {
        return info.resident_size / (1024 * 1024); // Convert to MB
    }
    
    return 0;
}

double PerformanceMonitor::getSystemLoadImpl() const {
    double loadavg[3];
    if (getloadavg(loadavg, 3) != -1) {
        return loadavg[0]; // 1-minute load average
    }
    return 0.0;
}

#elif __linux__
double PerformanceMonitor::getCPUUsageImpl() const {
    std::ifstream file("/proc/stat");
    if (!file.is_open()) return 0.0;
    
    std::string line;
    std::getline(file, line);
    
    std::istringstream ss(line);
    std::string cpu;
    long user, nice, system, idle, iowait, irq, softirq, steal;
    
    ss >> cpu >> user >> nice >> system >> idle >> iowait >> irq >> softirq >> steal;
    
    static long lastIdle = 0, lastTotal = 0;
    
    long totalIdle = idle + iowait;
    long total = user + nice + system + idle + iowait + irq + softirq + steal;
    
    long totald = total - lastTotal;
    long idled = totalIdle - lastIdle;
    
    lastTotal = total;
    lastIdle = totalIdle;
    
    if (totald > 0) {
        return 100.0 * (1.0 - (double)idled / totald);
    }
    
    return 0.0;
}

size_t PerformanceMonitor::getMemoryUsageImpl() const {
    std::ifstream file("/proc/self/status");
    if (!file.is_open()) return 0;
    
    std::string line;
    while (std::getline(file, line)) {
        if (line.substr(0, 6) == "VmRSS:") {
            std::istringstream ss(line.substr(6));
            size_t memory_kb;
            ss >> memory_kb;
            return memory_kb / 1024; // Convert to MB
        }
    }
    
    return 0;
}

double PerformanceMonitor::getSystemLoadImpl() const {
    std::ifstream file("/proc/loadavg");
    if (!file.is_open()) return 0.0;
    
    double load;
    file >> load;
    return load;
}

#else
// Generic fallback implementations
double PerformanceMonitor::getCPUUsageImpl() const { return 0.0; }
size_t PerformanceMonitor::getMemoryUsageImpl() const { return 0; }
double PerformanceMonitor::getSystemLoadImpl() const { return 0.0; }
#endif

// Static methods
double PerformanceMonitor::getCurrentCPUUsage() {
    PerformanceMonitor monitor;
    return monitor.getCPUUsageImpl();
}

size_t PerformanceMonitor::getCurrentMemoryUsage() {
    PerformanceMonitor monitor;
    return monitor.getMemoryUsageImpl();
}

double PerformanceMonitor::getSystemLoad() {
    PerformanceMonitor monitor;
    return monitor.getSystemLoadImpl();
}

double PerformanceMonitor::getCPUTemperature() {
    // Platform-specific temperature reading would go here
    // This is complex and varies by system
    return 0.0;
}
