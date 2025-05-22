#pragma once
#include <cstdio>
#include <cassert>
#include <cmath>
#include <stdexcept>
#include <format>
#if defined (_WIN32)
#define NOMINMAX
#include <windows.h>
#include <stacktrace>
#elif defined(__EMSCRIPTEN__)
#include <emscripten/emscripten.h>
#endif

#include "nlohmann_json.hpp"

/// ======== Config ========

/// ======== Debug Features ========
//#define DEBUG_FINITE_DOUBLE_CHECKS
//#define DEBUG_DISABLE_PRINT
#define DEBUG_INCLUDE_LOG_TABS

/// ======== Platform Simulation ========
//#define DEBUG_SIMULATE_WEB_UI
//#define DEBUG_SIMULATE_MOBILE
//#define DEBUG_SIMULATE_DPR 2.625f


/// ======== Timer filters ========

//#define TIMERS_ENABLED

//-- Timer thresholds -- 
constexpr double TIMER_ELAPSED_LIMIT = 1.0; // ms

// -- Filters --
constexpr bool THREAD_LOGGING  = false;
constexpr bool THREAD_TIMING             = false;


/// ========================= ///
/// ======== Private ======== ///

// Disable debug flags for Release builds
#ifdef NDEBUG
#undef DEBUG_FINITE_DOUBLE_CHECKS
#undef DEBUG_SIMULATE_WEB_UI
#undef DEBUG_SIMULATE_MOBILE
#endif

extern void ImDebugPrint(const char* txt, ...);

#if defined(_WIN32)
/// Windows
#define DebugPrintString(txt) { const char* __buf = txt; OutputDebugStringA(__buf); ImDebugPrint(__buf); }
#define DebugPrint(fmt, ...)                                \
        do {                                                \
            char buf[512];                                  \
            snprintf(buf, sizeof(buf), fmt, ##__VA_ARGS__);   \
            DebugPrintString(buf);                          \
            OutputDebugStringA("\n");                       \
            printf("%s\n", buf);                            \
        } while (0)
#elif defined(__EMSCRIPTEN__)
/// Webassembly
#define DebugPrintString(txt) printf(txt)
#define DebugPrint(fmt, ...) { printf(fmt "\n", ##__VA_ARGS__); ImDebugPrint(fmt, ##__VA_ARGS__); }
//#define DebugPrintEx(cond, fmt, ...) // todo:
#else
/// Linux / Native
#define DebugPrintString(txt) printf(txt)
#define DebugPrint(fmt, ...) { printf(fmt "\n", ##__VA_ARGS__); ImDebugPrint(fmt, ##__VA_ARGS__); }
//#define DebugPrintEx(cond, fmt, ...) // todo:
#endif
            
template<typename... Args>
void DashedDebugPrint(int width=0, const char* fmt=nullptr, Args... args)
{
    width = (width / 2) * 2;
    if (fmt == nullptr || fmt[0] == 0)
    {
        DebugPrintString((std::string(width, '-') + "\n").c_str());
    }
    else
    {
        int len = std::snprintf(nullptr, 0, fmt, args...);
        std::string s(len, '\0');
        std::snprintf(&s[0], s.size() + 1, fmt, args...);
        if (width > len + 2)
        {
            std::string dashes(std::max(0, (width - len) / 2 - 1), '-');
            DebugPrintString((dashes + " " + s + " " + dashes + "\n").c_str());
        }
        else
            DebugPrintString((s + "\n").c_str());
    }
}

#define DebugPrintEx(cond, fmt, ...) if constexpr (cond) DebugPrint(fmt, __VA_ARGS__);

#if defined(_WIN32)
#define DebugGroupBeg(cond, fmt, ...) \
    if constexpr(cond) {\
        DebugPrintString("\n");\
        DashedDebugPrint(100);\
        DashedDebugPrint(100, fmt, __VA_ARGS__);\
    }

#define DebugGroupEnd(cond, fmt, ...) \
    if constexpr(cond) {\
        DashedDebugPrint(100, fmt, __VA_ARGS__);\
        DashedDebugPrint(100);\
        DebugPrintString("\n");\
    }
#else
// todo: Should work in non-windows builds?
#define DebugGroupBeg(cond, fmt, ...)
#define DebugGroupEnd(cond, fmt, ...)
#endif

#ifdef DEBUG_DISABLE_PRINT
#undef DebugPrint
#undef DebugPrintEx
#define DebugPrint(fmt, ...)
#define DebugPrintEx(fmt, ...)
#endif

#if TIMERS_ENABLED
#define T0(name)      auto _timer_##name = std::chrono::steady_clock::now();
#define T1(name, ...) auto waited_##name = std::chrono::steady_clock::now() - _timer_##name;\
                      double dt_##name = std::chrono::duration<double, std::milli>(waited_##name).count();\
                      if (dt_##name >= TIMER_ELAPSED_LIMIT) DebugPrint("Timer (%s): %.4f", #name, dt_##name);
#else
#define T0(name)     
#define T1(name, ...)
#endif

#if defined(_MSC_VER)
#define DebugBreak() __debugbreak()
#elif defined(__GNUC__) || defined(__clang__)
#include <csignal>
#define DebugBreak() std::raise(SIGTRAP)
#else
#define DebugBreak() ((void)0)  // fallback: no-op
#endif

class FiniteDouble
{
    void set(const FiniteDouble& v)
    {
        //break_on_assignment = v.break_on_assignment;
        set(v.value);
    }
    void set(double v) 
    {
        if (v == value)
            return;

        if (break_on_assignment)
        {
            #ifdef _MSC_VER
            std::string stack = std::to_string(std::stacktrace::current());
            #endif
            DebugBreak();
            if (break_on_assignment == 1)
                break_on_assignment = 0;
        }

        if (!std::isfinite(v))
        {
            #ifdef _MSC_VER
            std::string stack = std::to_string(std::stacktrace::current());
            #endif
            DebugBreak();
        }
        
        value = v;
    }

    double value = 0.0;

public:

    FiniteDouble() = default;
    FiniteDouble(const FiniteDouble& v, bool _break_on_assignment = false)
    {
        set(v);
    }
    FiniteDouble(double v, bool _break_on_assignment=false)
    {
        break_on_assignment = _break_on_assignment;
        set(v);
    }
    FiniteDouble& operator=(const FiniteDouble v)
    {
        set(v);
        return *this;
    }
    FiniteDouble& operator=(double v) 
    {
        set(v);
        return *this;
    }
    operator double() const
    {
        return value;
    }
    double* operator&()
    {
        return &value;
    }
    const double* operator&() const
    {
        return &value;
    }

    bool operator==(double other) const { return value == other; }
    bool operator!=(double other) const { return value != other; }
    bool operator<(double other)  const { return value < other; }
    bool operator<=(double other) const { return value <= other; }
    bool operator>(double other)  const { return value > other; }
    bool operator>=(double other) const { return value >= other; }
    bool operator==(const FiniteDouble& other) const { return value == other.value; }
    bool operator!=(const FiniteDouble& other) const { return value != other.value; }

    FiniteDouble operator+(double other) const { return FiniteDouble(value + other); }
    FiniteDouble operator-(double other) const { return FiniteDouble(value - other); }
    FiniteDouble operator*(double other) const { return FiniteDouble(value * other); }
    FiniteDouble operator/(double other) const { return FiniteDouble(value / other); }
    FiniteDouble& operator+=(double other) { set(value + other); return *this;  }
    FiniteDouble& operator-=(double other) { set(value - other); return *this;  }
    FiniteDouble& operator*=(double other) { set(value * other); return *this;  }
    FiniteDouble& operator/=(double other) { set(value / other); return *this;  }

    friend std::ostream& operator<<(std::ostream& os, const FiniteDouble& fd)
    {
        return os << fd.value;
    }

    int break_on_assignment = 0;
    void breakOnAssignment()
    {
        break_on_assignment = 1;
    }
    void breakOnEveryAssignment()
    {
        break_on_assignment = 2;
    }
};

// JSON support must go *after* class definition
inline void to_json(nlohmann::json& j, const FiniteDouble& fd) {
    j = static_cast<double>(fd);  // or fd.get();
}

inline void from_json(const nlohmann::json& j, FiniteDouble& fd) {
    fd = j.get<double>();
}

namespace std {
    template <>
        struct hash<FiniteDouble> {
            size_t operator()(const FiniteDouble& fd) const noexcept {
                return hash<double>()(static_cast<double>(fd));
        }
    };
}

/// --- Debug & Release ---

#if defined DEBUG_SIMULATE_WEB_UI || defined __EMSCRIPTEN__
#define WEB_UI
#endif

#ifdef NDEBUG

/// --- Release Mode ---
#define finite_double double

#else

/// --- Debug Mode ---
#ifdef DEBUG_FINITE_DOUBLE_CHECKS
#define finite_double FiniteDouble
#else
#define finite_double double
#endif

#endif
