#include <app/log.h>

#include <atomic>
#include <cstdarg>
#include <cstdio>
#include <mutex>

#include <bx/debug.h>

namespace roj {

    namespace {

        std::mutex           g_mutex;
        std::atomic<uint8_t> g_minLevel{ LogInfo };
        LogSink              g_sink = nullptr;
        void* g_sinkUser = nullptr;

        const char* levelTag(LogLevel level) {
            constexpr const char* kTags[] = { "info", "warn", "error" };
            const unsigned i = static_cast<unsigned>(level);
            return kTags[i < 3 ? i : 2];
        }

        void defaultSink(LogLevel level, const char* message) {
            char line[1100];
            std::snprintf(line, sizeof(line), "[roj %s] %s\n", levelTag(level), message);
            std::fputs(line, stderr);
#if defined(_WIN32)
            //bx::debugOutput(line);
#endif
        }

    } // anonymous namespace

    void logWrite(LogLevel level, const char* fmt, ...) {
        if (static_cast<uint8_t>(level) < g_minLevel.load(std::memory_order_relaxed))
            return;

        char message[1024];
        va_list args;
        va_start(args, fmt);
        const int n = std::vsnprintf(message, sizeof(message), fmt, args);   // truncates + terminates
        va_end(args);
        if (n < 0)
            message[0] = '\0';

        std::lock_guard<std::mutex> lock(g_mutex);
        if (g_sink)
            g_sink(level, message, g_sinkUser);
        else
            defaultSink(level, message);
    }

    void logSetMinLevel(LogLevel level) {
        g_minLevel.store(static_cast<uint8_t>(level), std::memory_order_relaxed);
    }

    void logSetSink(LogSink sink, void* user) {
        std::lock_guard<std::mutex> lock(g_mutex);
        g_sink = sink;
        g_sinkUser = user;
    }

}