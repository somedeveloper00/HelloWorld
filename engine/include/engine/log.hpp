#pragma once

#include <cstdint>
#include <cstdio>
#include <format>
#include <functional>
#include <stdio.h>
#include <string>

#ifdef _WIN32
#include <io.h>
#else
#error "platform not supported"
#endif

namespace engine
{

enum logType : int8_t
{
    info,
    warning,
    error
};
using logHandler = void (*)(const logType type, const std::string &);

static inline bool isConnectedToTerminal()
{
#ifdef _WIN32
    return _isatty(_fileno(stdout));
#endif
}

// internal log handler storage
inline logHandler &getLogHandler()
{
    static logHandler handler = [](const logType type, const std::string &msg) {
        switch (type)
        {
        case info:
            fputs("\033[1;32m", stdout);
            fputs(msg.c_str(), stdout);
            fputs("\033[0m\n", stdout);
            break;
        case warning:
            fputs("\033[1;33m", stdout);
            fputs(msg.c_str(), stdout);
            fputs("\033[0m\n", stdout);
            break;
        case error:
            fputs("\033[1;31m", stdout);
            fputs(msg.c_str(), stdout);
            fputs("\033[0m\n", stdout);
            break;
        }
    };
    return handler;
}

// set a custom log handler
inline void setLogHandler(logHandler customHandler)
{
    getLogHandler() = customHandler;
}

template <typename... Args> inline void log(const logType type, const std::string &formatStr, Args &&...args)
{
    const std::string msg = std::vformat(formatStr, std::make_format_args(args...));
    getLogHandler()(type, msg);
}
template <typename... Args> inline void logInfo(const std::string &formatStr, Args &&...args)
{
    log(logType::info, "[ENGINE INFO] " + formatStr, std::forward<Args>(args)...);
}

template <typename... Args> inline void logWarning(const std::string &formatStr, Args &&...args)
{
    log(logType::warning, "[ENGINE WARNING] " + formatStr, std::forward<Args>(args)...);
}

template <typename... Args> inline void logError(const std::string &formatStr, Args &&...args)
{
    log(logType::error, "[ENGINE ERROR] " + formatStr, std::forward<Args>(args)...);
}
} // namespace engine