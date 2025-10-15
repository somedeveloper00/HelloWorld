#pragma once

#include "logFormats/glmLogFormats.hpp" // it's needed for formatting!
#include "thread.hpp"
#include <cstdio>
#include <format>
#include <mutex>
#include <stdio.h>
#include <string>

namespace engine
{
class log final
{
  public:
    using logHandler = void (*)(const std::string &);

    // current info log handler in use (thread-safe by design)
    static inline logHandler infoLogHandle = nullptr;
    // current info log handler in use (thread-safe by design)
    static inline logHandler warningLogHandle = nullptr;
    // current info log handler in use (thread-safe by design)
    static inline logHandler errorLogHandle = nullptr;

    template <typename... Args>
    static inline void logInfo(const std::string &formatStr, Args &&...args)
    {
        s_loggingMutex.lock();
        infoLogHandle("[ENGINE INFO t:" + threadInfo::getNameAsString() + "] " + std::vformat(formatStr, std::make_format_args(args...)));
        s_loggingMutex.unlock();
    }

    template <typename... Args>
    static inline void logWarning(const std::string &formatStr, Args &&...args)
    {
        s_loggingMutex.lock();
        warningLogHandle("[ENGINE WARNING t:" + threadInfo::getNameAsString() + "] " + std::vformat(formatStr, std::make_format_args(args...)));
        s_loggingMutex.unlock();
    }

    template <typename... Args>
    static inline void logError(const std::string &formatStr, Args &&...args)
    {
        s_loggingMutex.lock();
        errorLogHandle("[ENGINE ERROR t:" + threadInfo::getNameAsString() + "] " + std::vformat(formatStr, std::make_format_args(args...)));
        s_loggingMutex.unlock();
    }

    static inline void initialize()
    {
        static bool s_initialized = false;
        if (s_initialized)
        {
            logError("log already initialized");
            return;
        }
        infoLogHandle = defaultInfoLogHandle;
        warningLogHandle = defaultWarningLogHandle;
        errorLogHandle = defaultErrorLogHandle;
        s_initialized = true;
    }

  private:
    static inline std::mutex s_loggingMutex;

    static inline auto defaultInfoLogHandle = [](const std::string &msg) {
        fputs("\033[1;32m", stdout);
        fputs(msg.c_str(), stdout);
        fputs("\033[1;0m\n", stdout);
    };
    static inline auto defaultWarningLogHandle = [](const std::string &msg) {
        fputs("\033[1;33m", stdout);
        fputs(msg.c_str(), stdout);
        fputs("\033[1;0m\n", stdout);
    };
    static inline auto defaultErrorLogHandle = [](const std::string &msg) {
        fputs("\033[1;31m", stdout);
        fputs(msg.c_str(), stdout);
        fputs("\033[1;0m\n", stdout);
    };
};
} // namespace engine