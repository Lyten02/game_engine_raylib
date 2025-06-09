#pragma once

#include <string>
#include <unordered_map>
#include <chrono>
#include <spdlog/spdlog.h>
#include <spdlog/fmt/fmt.h>

namespace GameEngine {

/**
 * Utility class to limit repetitive log messages
 * Prevents spam by tracking message frequency and suppressing duplicates
 */
class LogLimiter {
public:
    struct MessageInfo {
        int count = 0;
        std::chrono::steady_clock::time_point lastLogged;
        std::chrono::steady_clock::time_point firstSeen;
    };

private:
    static inline std::unordered_map<std::string, MessageInfo> messageTracker;
    static inline int maxMessagesPerKey = 5;  // Maximum times to log the same message
    static inline std::chrono::seconds timeWindow{60};  // Time window for rate limiting
    static inline bool enabled = true;  // Global enable/disable

public:
    /**
     * Configure the log limiter
     * @param maxMessages Maximum number of times to log the same message
     * @param windowSeconds Time window in seconds for rate limiting
     * @param enable Enable or disable log limiting
     */
    static void configure(int maxMessages, int windowSeconds, bool enable = true) {
        maxMessagesPerKey = maxMessages;
        timeWindow = std::chrono::seconds(windowSeconds);
        enabled = enable;
    }

    /**
     * Check if a message should be logged
     * @param key Unique key for the message (usually the message itself or a category)
     * @return true if the message should be logged, false if it should be suppressed
     */
    static bool shouldLog(const std::string& key) {
        if (!enabled) return true;

        auto now = std::chrono::steady_clock::now();
        auto& info = messageTracker[key];

        // First time seeing this message
        if (info.count == 0) {
            info.firstSeen = now;
            info.lastLogged = now;
            info.count = 1;
            return true;
        }

        // Check if we're still within the time window
        auto timeSinceFirst = std::chrono::duration_cast<std::chrono::seconds>(now - info.firstSeen);
        if (timeSinceFirst > timeWindow) {
            // Reset the counter for this message
            info.count = 1;
            info.firstSeen = now;
            info.lastLogged = now;
            return true;
        }

        // Check if we've exceeded the max count
        if (info.count >= maxMessagesPerKey) {
            // Log a summary message once when limit is reached
            if (info.count == maxMessagesPerKey) {
                info.count++;
                spdlog::debug("Message '{}' suppressed after {} occurrences", key, maxMessagesPerKey);
            }
            return false;
        }

        info.count++;
        info.lastLogged = now;
        return true;
    }

    /**
     * Log with rate limiting
     * @param level Log level
     * @param key Message key for tracking
     * @param format Format string
     * @param args Format arguments
     */
    template<typename... Args>
    static void log(spdlog::level::level_enum level, const std::string& key, const std::string& format, Args&&... args) {
        if (shouldLog(key)) {
            spdlog::log(level, fmt::runtime(format), std::forward<Args>(args)...);
        }
    }

    /**
     * Convenience methods for different log levels
     */
    template<typename... Args>
    static void info(const std::string& key, const std::string& format, Args&&... args) {
        log(spdlog::level::info, key, format, std::forward<Args>(args)...);
    }

    template<typename... Args>
    static void warn(const std::string& key, const std::string& format, Args&&... args) {
        log(spdlog::level::warn, key, format, std::forward<Args>(args)...);
    }

    template<typename... Args>
    static void error(const std::string& key, const std::string& format, Args&&... args) {
        log(spdlog::level::err, key, format, std::forward<Args>(args)...);
    }

    /**
     * Clear all tracked messages
     */
    static void clear() {
        messageTracker.clear();
    }

    /**
     * Get statistics about suppressed messages
     */
    static std::unordered_map<std::string, MessageInfo> getStats() {
        return messageTracker;
    }
};

} // namespace GameEngine