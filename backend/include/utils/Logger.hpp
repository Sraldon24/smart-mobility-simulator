#pragma once

#include <string>
#include <iostream>
#include <chrono>
#include <iomanip>

namespace utils {

enum class LogLevel {
    DEBUG,
    INFO,
    WARN,
    ERROR
};

class Logger {
public:
    static void setLevel(LogLevel level) {
        currentLevel = level;
    }

    static void log(LogLevel level, const std::string& prefix, const std::string& message) {
        if (level < currentLevel) return;

        auto now = std::chrono::system_clock::now();
        auto in_time_t = std::chrono::system_clock::to_time_t(now);

        std::string levelStr;
        switch (level) {
            case LogLevel::DEBUG: levelStr = "DEBUG"; break;
            case LogLevel::INFO:  levelStr = "INFO "; break;
            case LogLevel::WARN:  levelStr = "WARN "; break;
            case LogLevel::ERROR: levelStr = "ERROR"; break;
        }

        std::ostream& out = (level == LogLevel::ERROR) ? std::cerr : std::cout;
        
        out << "[" << std::put_time(std::localtime(&in_time_t), "%Y-%m-%d %H:%M:%S") << "] "
            << "[" << levelStr << "] "
            << "[" << prefix << "] "
            << message << "\n";
    }

private:
    static inline LogLevel currentLevel = LogLevel::INFO;
};

} // namespace utils

#define LOG_DEBUG(prefix, msg) utils::Logger::log(utils::LogLevel::DEBUG, prefix, msg)
#define LOG_INFO(prefix, msg)  utils::Logger::log(utils::LogLevel::INFO, prefix, msg)
#define LOG_WARN(prefix, msg)  utils::Logger::log(utils::LogLevel::WARN, prefix, msg)
#define LOG_ERROR(prefix, msg) utils::Logger::log(utils::LogLevel::ERROR, prefix, msg)

