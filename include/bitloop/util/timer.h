#pragma once

#include <chrono>
#include "math_util.h"

namespace bl {

// millseconds only
struct SimpleTimer
{
    std::chrono::steady_clock::time_point t0 = std::chrono::steady_clock::now();

    void begin()
    {
        t0 = std::chrono::steady_clock::now();
    }

    double tick()
    {
       const auto now = std::chrono::steady_clock::now();
       const auto elapsed = now - t0;
       t0 = now;
       return static_cast<double>(std::chrono::duration_cast<std::chrono::nanoseconds>(elapsed).count()) / 1000000.0;
    }

    [[nodiscard]] double elapsed() const
    {
        const auto elapsed = std::chrono::steady_clock::now() - t0;
        return static_cast<double>(std::chrono::duration_cast<std::chrono::nanoseconds>(elapsed).count()) / 1000000.0;
    }
};

struct AverageTimer
{
    math::SMA<double> avg;
    SimpleTimer timer;

    AverageTimer(int count=1)
    {
        setLength(count);
    }

    void setLength(int count)
    {
        avg.setLength(count);
    }

    double tick()
    {
        return avg.push(timer.tick());
    }

    double average() const
    {
        return avg.average();
    }
};

struct FPSTimer
{
    AverageTimer timer;

    FPSTimer(int count = 1)
    {
        setLength(count);
    }

    void setLength(int length)
    {
        timer.setLength(length);
    }

    void tick()
    {
        timer.tick();
    }

    double getFPS() const
    {
        return 1000.0 / timer.average();
    }
};

}
