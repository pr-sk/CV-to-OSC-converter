#pragma once

#include <iostream>
#include <fstream>
#include <string>
#include <thread>
#include <atomic>
#include <functional>
#include <chrono>
#include <sys/stat.h>
#include <ctime>
#include <nlohmann/json.hpp>
#include "Config.h"

class ConfigWatcher {
private:
    std::string filename;
    std::function<void(const Config &)> callback;
    std::atomic<bool> watching{false};
    std::thread watchThread;
    std::chrono::seconds interval{5}; // Check every 5 seconds
    
public:
    ConfigWatcher(const std::string &file) : filename(file) {}
    ~ConfigWatcher() { stop(); }

    void start(std::function<void(const Config &)> configCallback) {
        callback = configCallback;
        watching = true;
        watchThread = std::thread(&ConfigWatcher::watch, this);
    }

    void stop() {
        watching = false;
        if (watchThread.joinable()) watchThread.join();
    }

    void setInterval(std::chrono::seconds sec) { interval = sec; }

private:
    void watch() {
        std::time_t lastWriteTime = getLastWriteTime();
        while (watching) {
            std::this_thread::sleep_for(interval);
            auto currentWriteTime = getLastWriteTime();
            if (currentWriteTime != lastWriteTime) {
                lastWriteTime = currentWriteTime;
                Config newConfig;
                if (newConfig.loadFromFile(filename)) {
                    callback(newConfig);
                }
            }
        }
    }

    std::time_t getLastWriteTime() {
        struct stat fileStat;
        if (stat(filename.c_str(), &fileStat) == 0) {
            return fileStat.st_mtime;
        }
        return 0; // If the file doesn't exist or can't be accessed
    }
};
