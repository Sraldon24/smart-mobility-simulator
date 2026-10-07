#pragma once

#include <string>
#include <iostream>
#include <chrono>
#include <iomanip>

namespace utils {

enum class LogLevel {
    Debug,
    Info,
    Warn,
    Error
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
            case LogLevel::Debug: levelStr = "DEBUG"; break;
            case LogLevel::Info:  levelStr = "INFO "; break;
            case LogLevel::Warn:  levelStr = "WARN "; break;
            case LogLevel::Error: levelStr = "ERROR"; break;
        }

        std::ostream& out = (level == LogLevel::Error) ? std::cerr : std::cout;
        
        out << "[" << std::put_time(std::localtime(&in_time_t), "%Y-%m-%d %H:%M:%S") << "] "
            << "[" << levelStr << "] "
            << "[" << prefix << "] "
            << message << "\n";
    }

private:
    static inline LogLevel currentLevel = LogLevel::Info;
};

} // namespace utils

#define LOG_DEBUG(prefix, msg) utils::Logger::log(utils::LogLevel::Debug, prefix, msg)
#define LOG_INFO(prefix, msg)  utils::Logger::log(utils::LogLevel::Info, prefix, msg)
#define LOG_WARN(prefix, msg)  utils::Logger::log(utils::LogLevel::Warn, prefix, msg)
#define LOG_ERROR(prefix, msg) utils::Logger::log(utils::LogLevel::Error, prefix, msg)
