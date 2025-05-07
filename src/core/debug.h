#pragma once
#include <cstdio>
#include <cassert>
#include <cmath>
#include <stdexcept>
#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#else
#include <stacktrace>
#endif
#include "nlohmann_json.hpp"

/// ------------------------------ Flags ------------------------------------- ///

//#define DEBUG_FINITE_DOUBLE_CHECKS
//#define DEBUG_SIMULATE_WEB_UI
//#define DEBUG_SIMULATE_MOBILE
//#define DEBUG_DISABLE_PRINT

/// -------------------------------------------------------------------------- ///

// Disable debug flags for Release builds
#ifdef NDEBUG
#undef DEBUG_FINITE_DOUBLE_CHECKS
#undef DEBUG_SIMULATE_WEB_UI
#undef DEBUG_SIMULATE_MOBILE
#endif

#if defined(_WIN32)
#define NOMINMAX
#include <windows.h>
#define DebugPrint(fmt, ...)                                \
        do {                                                \
            char buf[512];                                  \
            snprintf(buf, sizeof(buf), fmt, ##__VA_ARGS__); \
            OutputDebugStringA(buf);                        \
            OutputDebugStringA("\n");                       \
            printf("%s\n", buf);                            \
        } while (0)
#define DebugPrintEx(cond, fmt, ...) if (cond) DebugPrint(fmt, __VA_ARGS__)
#elif defined(__EMSCRIPTEN__)
#define DebugPrint(fmt, ...) printf(fmt "\n", ##__VA_ARGS__)
#define DebugPrintEx(cond, fmt, ...) // todo:
#else
#define DebugPrint(fmt, ...) printf(fmt "\n", ##__VA_ARGS__)
#define DebugPrintEx(cond, fmt, ...) // todo:
#endif


#ifdef DEBUG_DISABLE_PRINT
#undef DebugPrint
#undef DebugPrintEx
#define DebugPrint(fmt, ...)
#define DebugPrintEx(fmt, ...)
#endif

#if defined(_MSC_VER)
#define DebugBreak() __debugbreak()
#elif defined(__GNUC__) || defined(__clang__)
#include <csignal>
#define DebugBreak() std::raise(SIGTRAP)
#else
#define DebugBreak() ((void)0)  // fallback: no-op
#endif


/*class FiniteDouble {
    double value;
public:
    FiniteDouble() : value(0.0) {}
    FiniteDouble(double v) {
        set(v);
    }

    FiniteDouble& operator=(double v) {
        set(v);
        return *this;
    }

    operator double() const {
        return value;
    }

    // Optional: support arithmetic operations
    FiniteDouble operator+(double other) const { return FiniteDouble(value + other); }
    FiniteDouble operator-(double other) const { return FiniteDouble(value - other); }
    FiniteDouble operator*(double other) const { return FiniteDouble(value * other); }
    FiniteDouble operator/(double other) const { return FiniteDouble(value / other); }

private:
    void set(double v) {
        assert(std::isfinite(v) && "FiniteDouble assigned non-finite value");
        if (!std::isfinite(v)) {
            throw std::runtime_error("FiniteDouble assigned non-finite value");
        }
        value = v;
    }
};*/


class FiniteDouble
{
    void set(const FiniteDouble& v)
    {
        //break_on_assignment = v.break_on_assignment;
        set(v.value);
    }
    void set(double v) 
    {
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

    //friend FiniteDouble operator+(double lhs, const FiniteDouble& rhs) { return FiniteDouble(lhs + rhs.value); }
    //friend FiniteDouble operator-(double lhs, const FiniteDouble& rhs) { return FiniteDouble(lhs - rhs.value); }
    //friend FiniteDouble operator*(double lhs, const FiniteDouble& rhs) { return FiniteDouble(lhs * rhs.value); }
    //friend FiniteDouble operator/(double lhs, const FiniteDouble& rhs) { return FiniteDouble(lhs / rhs.value); }

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
