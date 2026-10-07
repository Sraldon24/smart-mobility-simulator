#pragma once

#include "Logger.hpp"
#include <chrono>
#include <string>

namespace utils {

class ScopedTimer {
public:
    ScopedTimer(const std::string& modulePrefix, const std::string& operationName)
        : m_prefix(modulePrefix), m_operationName(operationName) {
        m_start = std::chrono::high_resolution_clock::now();
    }

    ~ScopedTimer() {
        auto end = std::chrono::high_resolution_clock::now();
        auto durationMs = std::chrono::duration_cast<std::chrono::milliseconds>(end - m_start).count();
        auto durationUs = std::chrono::duration_cast<std::chrono::microseconds>(end - m_start).count();
        
        if (durationMs > 0) {
            LOG_INFO(m_prefix, m_operationName + " completed in " + std::to_string(durationMs) + " ms");
        } else {
            LOG_INFO(m_prefix, m_operationName + " completed in " + std::to_string(durationUs) + " us");
        }
    }

private:
    std::string m_prefix;
    std::string m_operationName;
    std::chrono::time_point<std::chrono::high_resolution_clock> m_start;
};

} // namespace utils

