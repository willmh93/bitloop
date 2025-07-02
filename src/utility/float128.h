#pragma once
///#ifdef _MSC_VER
///#pragma float_control(precise, off)
///#endif

#include <cmath>
#include <limits>
#include <string>
#include <iomanip>
#include <sstream>
#include "platform_macros.h"

/* ---------- HW-FMA feature check (once per TU) ------------------------- */
#ifndef HAVE_HW_FMA
#if defined(__has_builtin)          /* this guard compiles on MSVC too  */
#if __has_builtin(__builtin_fma)
#define HAVE_HW_FMA 1
#endif
#endif

/* GCC / Clang fallback if target ISA guarantees FMA */
#if !defined(HAVE_HW_FMA) && defined(__FMA__)
#define HAVE_HW_FMA 1
#endif

/* MSVC: /arch:AVX2 or better implies FMA; need /fp:contract or /fp:fast */
#if !defined(HAVE_HW_FMA) && defined(_MSC_VER) && defined(__AVX2__)
#define HAVE_HW_FMA 1
#endif

#ifndef HAVE_HW_FMA
#define HAVE_HW_FMA 0
#endif
#endif

#define BL_HAS_FMA 1

///

BL_PUSH_PRECISE

FAST_INLINE constexpr void two_sum_precise(double a, double b, double& s, double& e) 
{
    s = a + b;
    double bv = s - a;
    e = (a - (s - bv)) + (b - bv);
}

FAST_INLINE constexpr void two_prod_precise(double a, double b, double& p, double& err) 
{
    constexpr double split = 134217729.0;

    double a_c = a * split;
    double a_hi = a_c - (a_c - a);
    double a_lo = a - a_hi;

    double b_c = b * split;
    double b_hi = b_c - (b_c - b);
    double b_lo = b - b_hi;

    p = a * b; // rounded product
    err = ((a_hi * b_hi - p) + a_hi * b_lo + a_lo * b_hi) + a_lo * b_lo;
}

BL_POP_PRECISE

//FAST_INLINE void two_sum(double a, double b, double& s, double& e)
//{
//    #if BL_HAS_FMA
//    {
//        s = a + b;
//        e = std::fma(-s, 1.0, a) + b;
//    }
//    #else       
//    {
//        two_sum_precise(a, b, s, e);
//    }
//    #endif
//}
//
//FAST_INLINE void two_prod(double a, double b, double& p, double& err)
//{
//    #if BL_HAS_FMA
//    {
//        p = a * b;
//        err = std::fma(a, b, -p);
//    }
//    #else
//    {
//        two_prod_precise(a, b, p, err);
//    }
//    #endif
//}


class float128 {
public:
    double hi; // leading component
    double lo; // trailing error

    /*------------------------------------------------------------------------
      Constructors
    ------------------------------------------------------------------------*/
    constexpr float128() : hi(0.0), lo(0.0) {}
    constexpr float128(double x) : hi(x), lo(0.0) {}
    constexpr float128(double h, double l) : hi(h), lo(l) {}

    /*------------------------------------------------------------------------
      Basic helpers (error-free transforms)
    ------------------------------------------------------------------------*/

    

    static FAST_INLINE constexpr float128 quick_two_sum(double a, double b)
    {
        double s = a + b;
        double err = b - (s - a);
        return { s, err };
    }

    /*------------------------------------------------------------------------
      Normalisation (ensure |lo| <= 0.5 ulp(hi))
    ------------------------------------------------------------------------*/

    static FAST_INLINE constexpr float128 renorm(double hi, double lo)
    {
        double s, e;
        two_sum_precise(hi, lo, s, e);
        return { s, e };
    }

    /*------------------------------------------------------------------------
      Arithmetic operators
    ------------------------------------------------------------------------*/
    friend FAST_INLINE constexpr float128 operator*(float128 a, double b)
    {
        return a * float128(b);
    }

    friend FAST_INLINE constexpr float128 operator*(double a, float128 b)
    {
        return float128(a) * b;
    }

    friend FAST_INLINE constexpr float128 operator+(float128 a, float128 b)
    {
        double s1, e1;
        two_sum_precise(a.hi, b.hi, s1, e1);
        double s2, e2;
        two_sum_precise(a.lo, b.lo, s2, e2);
        double lo = e1 + s2;
        double result_hi, result_lo;
        two_sum_precise(s1, lo, result_hi, result_lo);
        result_lo += e2;
        return renorm(result_hi, result_lo);
    }

    friend FAST_INLINE constexpr float128 operator-(float128 a, float128 b)
    {
        b.hi = -b.hi;
        b.lo = -b.lo;
        return a + b;
    }

    friend FAST_INLINE constexpr float128 operator*(float128 a, float128 b)
    {
        double p1, e1;
        two_prod_precise(a.hi, b.hi, p1, e1);
        e1 += a.hi * b.lo + a.lo * b.hi;
        return renorm(p1, e1);
    }

    friend FAST_INLINE constexpr float128 operator/(float128 a, float128 b)
    {
        // one Newton step after double quotient
        double q = a.hi / b.hi;                // coarse
        float128 qb = b * q;               // q * b
        float128 r = a - qb;              // remainder
        double q_corr = r.hi / b.hi;         // refine
        return renorm(q + q_corr, 0.0);
    }

    /* compound assignments */
    FAST_INLINE constexpr float128& operator+=(float128 rhs) { *this = *this + rhs; return *this; }
    FAST_INLINE constexpr float128& operator-=(float128 rhs) { *this = *this - rhs; return *this; }
    FAST_INLINE constexpr float128& operator*=(float128 rhs) { *this = *this * rhs; return *this; }
    FAST_INLINE constexpr float128& operator/=(float128 rhs) { *this = *this / rhs; return *this; }

    /*------------------------------------------------------------------------
      Transcendentals (sqrt, sin, cos via Dekker/Bailey style refinement)
    ------------------------------------------------------------------------*/
    friend FAST_INLINE float128 sqrt(float128 a)
    {
        double approx = std::sqrt(a.hi);
        float128 y(approx);                    // y ≈ sqrt(a)
        float128 y_sq = y * y;                 // y^2
        float128 r = a - y_sq;                 // residual
        float128 half = 0.5;
        y += r * (half / y);                       // one Newton iteration
        return y;
    }

    friend FAST_INLINE float128 sin(float128 a)
    {
        // Reduce to double first, then correct with one term of series
        double s = std::sin(a.hi);
        double c = std::cos(a.hi);
        return float128(s) + float128(c) * a.lo;
    }

    friend FAST_INLINE float128 cos(float128 a)
    {
        double c = std::cos(a.hi);
        double s = std::sin(a.hi);
        return float128(c) - float128(s) * a.lo;
    }

    /*------------------------------------------------------------------------
      Conversions
    ------------------------------------------------------------------------*/
    explicit constexpr operator double() const { return hi + lo; }
    explicit constexpr operator float() const { return static_cast<float>(hi + lo); }

    constexpr float128 operator -() const
    {
        return (*this) * -1;
    }

    /*------------------------------------------------------------------------
      Utility
    ------------------------------------------------------------------------*/
    static constexpr float128 eps()
    {
        return { std::numeric_limits<double>::epsilon(), 0.0 };
    }
};



namespace std {


    template <>
    struct numeric_limits<float128> {
        static constexpr bool is_specialized = true;

        static constexpr float128 min() noexcept {
            return { std::numeric_limits<double>::min(), 0.0 };
        }

        static constexpr float128 max() noexcept {
            return { std::numeric_limits<double>::max(), -std::numeric_limits<double>::epsilon() };
        }

        static constexpr float128 lowest() noexcept {
            return { -std::numeric_limits<double>::max(), std::numeric_limits<double>::epsilon() };
        }

        static constexpr float128 epsilon() noexcept {
            // ~2^-106, conservatively a single ulp of double-double
            return { 1.232595164407831e-32, 0.0 };
        }

        static constexpr float128 round_error() noexcept {
            return { 0.5, 0.0 };
        }

        static constexpr float128 infinity() noexcept {
            return { bl_infinity<double>(), 0.0};
            //return { std::numeric_limits<double>::infinity(), 0.0 };
        }

        static constexpr float128 quiet_NaN() noexcept {
            return { std::numeric_limits<double>::quiet_NaN(), 0.0 };
        }

        static constexpr float128 signaling_NaN() noexcept {
            return { std::numeric_limits<double>::signaling_NaN(), 0.0 };
        }

        static constexpr float128 denorm_min() noexcept {
            return { std::numeric_limits<double>::denorm_min(), 0.0 };
        }

        static constexpr int digits = 106;  // ~53 bits * 2
        static constexpr int digits10 = 31;   // log10(2^106) ≈ 31.9
        static constexpr int max_digits10 = 35;

        static constexpr bool is_signed = true;
        static constexpr bool is_integer = false;
        static constexpr bool is_exact = false;
        static constexpr int radix = 2;

        static constexpr int min_exponent = std::numeric_limits<double>::min_exponent;
        static constexpr int min_exponent10 = std::numeric_limits<double>::min_exponent10;
        static constexpr int max_exponent = std::numeric_limits<double>::max_exponent;
        static constexpr int max_exponent10 = std::numeric_limits<double>::max_exponent10;

        static constexpr bool has_infinity = true;
        static constexpr bool has_quiet_NaN = true;
        static constexpr bool has_signaling_NaN = true;

        static constexpr bool is_iec559 = false; // not IEEE-754 compliant
        static constexpr bool is_bounded = true;
        static constexpr bool is_modulo = false;

        static constexpr bool traps = false;
        static constexpr bool tinyness_before = false;
        static constexpr float_round_style round_style = round_to_nearest;
    };
}

// comparisons
constexpr FAST_INLINE bool operator<(const float128& a, const float128& b)
{
    return (a.hi < b.hi) || (a.hi == b.hi && a.lo < b.lo);
}

constexpr FAST_INLINE bool operator <=(const float128& a, const float128& b)
{
    return (a < b) || (a.hi == b.hi && a.lo == b.lo);
}

constexpr FAST_INLINE bool operator>(const float128& a, const float128& b) { return b < a; }
constexpr FAST_INLINE bool operator>=(const float128& a, const float128& b) { return b <= a; }
constexpr FAST_INLINE bool operator==(const float128& a, const float128& b) { return a.hi == b.hi && a.lo == b.lo; }
constexpr FAST_INLINE bool operator!=(const float128& a, const float128& b) { return !(a == b); }

inline float128 log(const float128& a)
{
    double log_hi = std::log(a.hi); // first guess
    float128 exp_log_hi(std::exp(log_hi)); // exp(guess)
    float128 r = (a - exp_log_hi) / exp_log_hi; // (a - e^g) / e^g ≈ error
    return float128(log_hi) + r; // refined log
}

inline float128 log2(const float128& a)
{
    constexpr float128 LOG2_RECIPROCAL(1.442695040888963407359924681001892137); // 1/log(2)
    return log(a) * LOG2_RECIPROCAL;
}

inline std::string to_string(const float128& x)
{
    if (x.hi == 0.0) return "0";

    // --- 1. get sign and magnitude ----------------------------------------
    float128 v = x;
    bool neg = false;
    if (v.hi < 0) { neg = true; v.hi = -v.hi; v.lo = -v.lo; }

    // --- 2. scale into [1,10) and find exponent ---------------------------
    int exp10 = static_cast<int>(std::floor(std::log10(v.hi)));
    float128 scale = std::pow(10.0, -exp10);   // 10^(-exp10)
    v *= scale;                                    // now 1 ≤ v < 10

    // --- 3. generate 32 digits (safely > 31) ------------------------------
    std::ostringstream oss;
    oss << std::setprecision(32) << std::fixed;
    for (int i = 0; i < 32; ++i) {
        int d = static_cast<int>(v.hi);
        oss << d;
        v -= float128(d);              // remove integer part
        v *= 10.0;                         // shift left
    }

    std::string s = oss.str();

    // --- 4. insert decimal point after first digit ------------------------
    s.insert(1, ".");
    if (neg) s.insert(0, "-");

    // --- 5. add exponent ---------------------------------------------------
    s += "e";
    s += std::to_string(exp10);

    return s;
}

inline std::ostream& operator<<(std::ostream& os, const float128& x)
{
    return os << to_string(x);
}


