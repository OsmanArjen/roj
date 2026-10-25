#pragma once
#include <cstdint>

namespace roj {

    struct ClockInit {
        float    fixedHz = 60.0f;
        float    dtMax = 0.1f;    // longest frame the clock will report (spiral-of-death guard)
        uint32_t maxFixedSteps = 8;       // fixed steps allowed to run per frame
    };

    struct Clock {
        uint64_t lastNs = 0;            // steady_clock stamp of the previous tick
        uint64_t frame = 0;
        double   time = 0.0;            // scaled (game) time; frozen while paused
        double   realTime = 0.0;        // unscaled wall time, clamped per frame
        float    dt = 0.0f;             // scaled delta: 0 while paused
        float    dtReal = 0.0f;         // unscaled delta
        float    dtSmooth = 1.0f / 60.0f;   // exponentially smoothed real delta (for fps)
        float    dtMax = 0.1f;
        float    timeScale = 1.0f;
        bool     paused = false;

        float    fixedDt = 1.0f / 60.0f;
        uint32_t maxFixedSteps = 8;
        double   accumulator = 0.0;
        uint64_t fixedTick = 0;
    };

    void clockInit(Clock& c, const ClockInit& init);
    void clockTick(Clock& c);
    void clockAdvance(Clock& c, double realSeconds);

    bool clockFixedStep(Clock& c);

    inline void clockSetPaused(Clock& c, bool paused) { c.paused = paused; }
    inline void clockTogglePause(Clock& c) { c.paused = !c.paused; }

    inline float clockFixedAlpha(const Clock& c) { return float(c.accumulator / double(c.fixedDt)); }
    inline float clockFps(const Clock& c) { return c.dtSmooth > 0.0f ? 1.0f / c.dtSmooth : 0.0f; }

}