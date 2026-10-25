#pragma once
#include <cstdint>

// Lets GCC/Clang type-check printf-style arguments at every call site.
#if defined(__GNUC__) || defined(__clang__)
#define ROJ_PRINTF_LIKE(fmtIdx, argIdx) __attribute__((format(printf, fmtIdx, argIdx)))
#else
#define ROJ_PRINTF_LIKE(fmtIdx, argIdx)
#endif

namespace roj {

    enum LogLevel : uint8_t { LogInfo = 0, LogWarn, LogError };

    // A sink receives the formatted message body (no tag, no newline). It is called
    // with the logger's lock held, so it must not call logWrite itself.
    using LogSink = void (*)(LogLevel level, const char* message, void* user);

    // Thread-safe. Messages below the minimum level are dropped before formatting.
    void logWrite(LogLevel level, const char* fmt, ...) ROJ_PRINTF_LIKE(2, 3);

    void logSetMinLevel(LogLevel level);

    // nullptr restores the default sink (stderr, plus the debugger output on Windows).
    void logSetSink(LogSink sink, void* user);

}

#define ROJ_LOG_INFO(...)  ::roj::logWrite(::roj::LogInfo,  __VA_ARGS__)
#define ROJ_LOG_WARN(...)  ::roj::logWrite(::roj::LogWarn,  __VA_ARGS__)
#define ROJ_LOG_ERROR(...) ::roj::logWrite(::roj::LogError, __VA_ARGS__)