#include <app/clock.h>

#include <algorithm>
#include <chrono>

namespace roj {

    namespace {

        uint64_t nowNs() {
            using namespace std::chrono;
            return uint64_t(duration_cast<nanoseconds>(steady_clock::now().time_since_epoch()).count());
        }

    } // anonymous namespace

    void clockInit(Clock& c, const ClockInit& init) {
        c = Clock{};
        c.lastNs = nowNs();
        c.dtMax = init.dtMax > 0.0f ? init.dtMax : 0.1f;
        c.fixedDt = init.fixedHz > 0.0f ? 1.0f / init.fixedHz : 1.0f / 60.0f;
        c.maxFixedSteps = init.maxFixedSteps > 0 ? init.maxFixedSteps : 1;
    }

    void clockTick(Clock& c) {
        const uint64_t now = nowNs();
        double real = double(now - c.lastNs) * 1e-9;
        c.lastNs = now;

        if (c.frame == 0)
            real = 0.0;

        clockAdvance(c, real);
    }

    void clockAdvance(Clock& c, double realSeconds) {
        const float real = std::clamp(float(realSeconds), 0.0f, c.dtMax);
        const float scaled = c.paused ? 0.0f : real * std::max(c.timeScale, 0.0f);

        c.dtReal = real;
        c.dt = scaled;
        c.realTime += real;
        c.time += scaled;
        c.frame += 1;
        if (real > 0.0f)
            c.dtSmooth += (real - c.dtSmooth) * 0.1f;

        c.accumulator = std::min(c.accumulator + scaled, double(c.fixedDt) * double(c.maxFixedSteps));
    }

    bool clockFixedStep(Clock& c) {
        if (c.accumulator < double(c.fixedDt))
            return false;
        c.accumulator -= double(c.fixedDt);
        c.fixedTick += 1;
        return true;
    }

}