#pragma once
#include <cmath>
#include <limits>
#include <string>
#include <cstring>
#include <cstdint>
#include <iomanip>
#include <sstream>
#include <type_traits>

/// ======== FORCE_INLINE ========

#ifndef FORCE_INLINE
    #if defined(_MSC_VER)
    #define FORCE_INLINE __forceinline
    #elif defined(__clang__) || defined(__GNUC__)
    #define FORCE_INLINE inline __attribute__((always_inline))
    #else
    #define FORCE_INLINE inline
    #endif
#endif

/// ======== Fast-math ========

#ifndef BL_FAST_MATH
    #if defined(__FAST_MATH__) // GCC/Clang: -ffast-math
    #define BL_FAST_MATH
    #elif defined(_MSC_VER) && defined(_M_FP_FAST)  // MSVC: /fp:fast
    #define BL_FAST_MATH
    #endif
#endif

// avoid inlining precise print helpers into fast-math callers on MSVC
#ifndef BL_PRINT_NOINLINE
    #if defined(_MSC_VER) && defined(BL_FAST_MATH)
        #define BL_PRINT_NOINLINE __declspec(noinline)
    #else
        #define BL_PRINT_NOINLINE
    #endif
#endif


#if defined(_MSC_VER)

    #ifndef BL_PUSH_PRECISE
        #define BL_PUSH_PRECISE  __pragma(float_control(precise, on, push)) \
                                 __pragma(fp_contract(off))
    #endif
    #ifndef BL_POP_PRECISE
        #define BL_POP_PRECISE   __pragma(float_control(pop))
    #endif

#elif defined(__EMSCRIPTEN__)

    #ifndef BL_PUSH_PRECISE
        #define BL_PUSH_PRECISE  _Pragma("clang fp reassociate(off)") \
                                 _Pragma("clang fp contract(off)")
    #endif
    #ifndef BL_POP_PRECISE
        #define BL_POP_PRECISE   _Pragma("clang fp reassociate(on)")  \
                                 _Pragma("clang fp contract(fast)")
    #endif
    // Other Clang (desktop) that supports the richer set
    #elif defined(__clang__)
    #ifndef BL_PUSH_PRECISE
        #define BL_PUSH_PRECISE  _Pragma("clang fp reassociate(off)") \
                                 _Pragma("clang fp contract(off)")
    #endif
    #ifndef BL_POP_PRECISE
        #define BL_POP_PRECISE   _Pragma("clang fp reassociate(on)")  \
                                 _Pragma("clang fp contract(fast)")
    #endif
    // GCC fallback
    #elif defined(__GNUC__)
    #ifndef BL_PUSH_PRECISE
        #define BL_PUSH_PRECISE  _Pragma("GCC push_options")               \
                                 _Pragma("GCC optimize(\"no-fast-math\")") \
                                 _Pragma("STDC FP_CONTRACT OFF")
    #endif
    #ifndef BL_POP_PRECISE
        #define BL_POP_PRECISE   _Pragma("GCC pop_options")
    #endif
#else

    #define BL_PUSH_PRECISE
    #define BL_POP_PRECISE

#endif
namespace bl {

// For now, disabling std::fma seems to offer a 5-6x performance boost on web
#ifndef __EMSCRIPTEN__
    #if defined(__FMA__) || defined(__FMA4__) || defined(_MSC_VER) || defined(__clang__) || defined(__GNUC__)
    #define FMA_AVAILABLE
    #endif
#endif

BL_PUSH_PRECISE
FORCE_INLINE constexpr void two_sum_precise(double a, double b, double& s, double& e)
{
    s = a + b;
    double bv = s - a;
    e = (a - (s - bv)) + (b - bv);
}
BL_POP_PRECISE

#ifdef FMA_AVAILABLE

static FORCE_INLINE double fma1(double a, double b, double c)
{
    return std::fma(a, b, c);
}

FORCE_INLINE void two_prod_precise_fma(double a, double b, double& p, double& err)
{
    p = a * b;
    err = fma1(a, b, -p);
}

#endif

BL_PUSH_PRECISE
FORCE_INLINE constexpr void two_prod_precise_dekker(double a, double b, double& p, double& err)
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

struct f128;

constexpr f128 operator+(const f128& a, const f128& b);
constexpr f128 operator-(const f128& a, const f128& b);
#ifdef FMA_AVAILABLE
f128 operator*(const f128& a, const f128& b);
f128 operator/(const f128& a, const f128& b);
#else
constexpr f128 operator*(const f128& a, const f128& b);
constexpr f128 operator/(const f128& a, const f128& b);
#endif

struct f128
{
    double hi; // leading component
    double lo; // trailing error
    
    #ifdef FMA_AVAILABLE
    /// FMA versions (not constexpr)
    FORCE_INLINE f128& operator*=(f128 rhs) { *this = *this * rhs; return *this; }
    FORCE_INLINE f128& operator/=(f128 rhs) { *this = *this / rhs; return *this; }
    #else
    FORCE_INLINE constexpr f128& operator*=(f128 rhs) { *this = *this * rhs; return *this; }
    FORCE_INLINE constexpr f128& operator/=(f128 rhs) { *this = *this / rhs; return *this; }
    #endif

    constexpr f128() noexcept = default;
    constexpr f128(double hi_, double lo_) noexcept : hi(hi_), lo(lo_) {}
    constexpr f128(float  x) noexcept : hi((double)x), lo(0.0) {}
    constexpr f128(double x) noexcept : hi(x), lo(0.0) {}

    constexpr f128(int64_t  v) noexcept { *this = static_cast<int64_t>(v); }
    constexpr f128(uint64_t u) noexcept { *this = static_cast<uint64_t>(u); }
    constexpr f128(int32_t  v) noexcept : f128((int64_t)v) {}
    constexpr f128(uint32_t u) noexcept : f128((int64_t)u) {}

    //FORCE_INLINE constexpr f128(int v) { f128(int64_t(v)); }

    FORCE_INLINE constexpr f128& operator=(double x) noexcept {
        hi = x; lo = 0.0; return *this;
    }
    FORCE_INLINE constexpr f128& operator=(float x) noexcept {
        hi = static_cast<double>(x); lo = 0.0; return *this;
    }

    FORCE_INLINE constexpr f128& operator=(uint64_t u) noexcept;
    FORCE_INLINE constexpr f128& operator=(int64_t v) noexcept;

    template<class T, std::enable_if_t<std::is_integral_v<T>&& std::is_signed_v<T> && (sizeof(T) < 8), int> = 0>
    FORCE_INLINE constexpr f128& operator=(T v) noexcept {
        return (*this = static_cast<int64_t>(v));
    }
    template<class T, std::enable_if_t<std::is_integral_v<T>&& std::is_unsigned_v<T> && (sizeof(T) < 8), int> = 0>
    FORCE_INLINE constexpr f128& operator=(T v) noexcept {
        return (*this = static_cast<uint64_t>(v));
    }

    FORCE_INLINE constexpr f128& operator+=(f128 rhs) { *this = *this + rhs; return *this; }
    FORCE_INLINE constexpr f128& operator-=(f128 rhs) { *this = *this - rhs; return *this; }


    /// ======== Conversions ========
    explicit constexpr operator double() const { return hi + lo; }
    explicit constexpr operator float() const { return static_cast<float>(hi + lo); }
    explicit constexpr operator int() const { return static_cast<int>(hi + lo); }

    constexpr f128 operator+() const { return *this; }
    constexpr f128 operator-() const noexcept { return f128{ -hi, -lo }; }

    /// ======== Utility ========
    static constexpr f128 eps() { return { 1.232595164407831e-32, 0.0 }; }
};

} // end bl

namespace std
{
    using bl::f128;

    template<> struct numeric_limits<f128>
    {
        static constexpr bool is_specialized = true;

        // limits
        static constexpr f128 min()            noexcept { return {  numeric_limits<double>::min(), 0.0 }; }
        static constexpr f128 max()            noexcept { return {  numeric_limits<double>::max(), -numeric_limits<double>::epsilon() }; }
        static constexpr f128 lowest()         noexcept { return { -numeric_limits<double>::max(),  numeric_limits<double>::epsilon() }; }
        static constexpr f128 highest()        noexcept { return {  numeric_limits<double>::max(), -numeric_limits<double>::epsilon() }; }
                                                 
		// special values                        
        static constexpr f128 epsilon()        noexcept { return { 1.232595164407831e-32, 0.0 }; } // ~2^-106, a single ulp of double-double
        static constexpr f128 round_error()    noexcept { return { 0.5, 0.0 }; }
        static constexpr f128 infinity()       noexcept { return { numeric_limits<double>::max(), 0.0}; }
        static constexpr f128 quiet_NaN()      noexcept { return { numeric_limits<double>::quiet_NaN(), 0.0 }; }
        static constexpr f128 signaling_NaN()  noexcept { return { numeric_limits<double>::signaling_NaN(), 0.0 }; }
        static constexpr f128 denorm_min()     noexcept { return { numeric_limits<double>::denorm_min(), 0.0 }; }
                                                 
        static constexpr bool has_infinity       = true;
        static constexpr bool has_quiet_NaN      = true;
        static constexpr bool has_signaling_NaN  = true;
                                                 
		// properties                            
        static constexpr int  digits             = 106;  // ~53 bits * 2
        static constexpr int  digits10           = 31;   // log10(2^106) ≈ 31.9
        static constexpr int  max_digits10       = 35;
        static constexpr bool is_signed          = true;
        static constexpr bool is_integer         = false;
        static constexpr bool is_exact           = false;
        static constexpr int  radix              = 2;
                                                 
		// exponent range                        
        static constexpr int  min_exponent       = numeric_limits<double>::min_exponent;
        static constexpr int  max_exponent       = numeric_limits<double>::max_exponent;
        static constexpr int  min_exponent10     = numeric_limits<double>::min_exponent10;
        static constexpr int  max_exponent10     = numeric_limits<double>::max_exponent10;
                                                 
		// properties                            
        static constexpr bool is_iec559          = false; // not IEEE-754 compliant
        static constexpr bool is_bounded         = true;
        static constexpr bool is_modulo          = false;
                                                 
		// rounding                              
        static constexpr bool traps              = false;
        static constexpr bool tinyness_before    = false;

        static constexpr float_round_style round_style = round_to_nearest;
    };
}

namespace bl {

/// ======== Helpers ========

FORCE_INLINE constexpr f128 renorm(double hi, double lo)
{
    double s, e;
    two_sum_precise(hi, lo, s, e);
    return { s, e };
}

BL_PUSH_PRECISE
FORCE_INLINE constexpr f128 quick_two_sum(double a, double b)
{
    double s = a + b;
    double err = b - (s - a);
    return { s, err };
}
BL_POP_PRECISE

FORCE_INLINE constexpr f128 recip(f128 b)
{
    constexpr f128 one = f128(1.0);
    f128 y = f128(1.0 / b.hi);
    f128 e = one - b * y;

    y += y * e;
    e = one - b * y;
    y += y * e;

    return y;
}

FORCE_INLINE constexpr f128& f128::operator=(uint64_t u) noexcept {
    // same limb path you already use in f128(u)
    uint64_t hi32 = u >> 32, lo32 = u & 0xFFFFFFFFull;
    double a = static_cast<double>(hi32) * 4294967296.0; // 2^32
    double b = static_cast<double>(lo32);
    double s, e; two_sum_precise(a, b, s, e);
    f128 r = renorm(s, e);
    hi = r.hi; lo = r.lo; return *this;
}
FORCE_INLINE constexpr f128& f128::operator=(int64_t v) noexcept {
    uint64_t u = (v < 0) ? uint64_t(0) - uint64_t(v) : uint64_t(v);
    f128 r; r = u;                       // reuse uint64_t path
    if (v < 0) { r.hi = -r.hi; r.lo = -r.lo; }
    hi = r.hi; lo = r.lo; return *this;
}

// ------------------ f128 <=> f128 ------------------

FORCE_INLINE constexpr bool operator <(const f128& a, const f128& b) { return (a.hi < b.hi) || (a.hi == b.hi && a.lo < b.lo); }
FORCE_INLINE constexpr bool operator >(const f128& a, const f128& b) { return b < a; }
FORCE_INLINE constexpr bool operator<=(const f128& a, const f128& b) { return (a < b) || (a.hi == b.hi && a.lo == b.lo); }
FORCE_INLINE constexpr bool operator>=(const f128& a, const f128& b) { return b <= a; }
FORCE_INLINE constexpr bool operator==(const f128& a, const f128& b) { return a.hi == b.hi && a.lo == b.lo; }
FORCE_INLINE constexpr bool operator!=(const f128& a, const f128& b) { return !(a == b); }

// ------------------ double <=> f128 ------------------

FORCE_INLINE constexpr bool operator<(const f128& a, double b)  { return a < f128(b); }
FORCE_INLINE constexpr bool operator<(double a, const f128& b)  { return f128(a) < b; }
FORCE_INLINE constexpr bool operator>(const f128& a, double b)  { return b < a; }
FORCE_INLINE constexpr bool operator>(double a, const f128& b)  { return b < a; }
FORCE_INLINE constexpr bool operator<=(const f128& a, double b) { return !(b < a); }
FORCE_INLINE constexpr bool operator<=(double a, const f128& b) { return !(b < a); }
FORCE_INLINE constexpr bool operator>=(const f128& a, double b) { return !(a < b); }
FORCE_INLINE constexpr bool operator>=(double a, const f128& b) { return !(a < b); }
FORCE_INLINE constexpr bool operator==(const f128& a, double b) { return a == f128(b); }
FORCE_INLINE constexpr bool operator==(double a, const f128& b) { return f128(a) == b; }
FORCE_INLINE constexpr bool operator!=(const f128& a, double b) { return !(a == b); }
FORCE_INLINE constexpr bool operator!=(double a, const f128& b) { return !(a == b); }

// ------------------ float <=> f128 ------------------

FORCE_INLINE constexpr bool operator<(const f128& a, float b) { return a < f128(b); }
FORCE_INLINE constexpr bool operator<(float a, const f128& b) { return f128(a) < b; }
FORCE_INLINE constexpr bool operator>(const f128& a, float b) { return b < a; }
FORCE_INLINE constexpr bool operator>(float a, const f128& b) { return b < a; }
FORCE_INLINE constexpr bool operator<=(const f128& a, float b) { return !(b < a); }
FORCE_INLINE constexpr bool operator<=(float a, const f128& b) { return !(b < a); }
FORCE_INLINE constexpr bool operator>=(const f128& a, float b) { return !(a < b); }
FORCE_INLINE constexpr bool operator>=(float a, const f128& b) { return !(a < b); }
FORCE_INLINE constexpr bool operator==(const f128& a, float b) { return a == f128(b); }
FORCE_INLINE constexpr bool operator==(float a, const f128& b) { return f128(a) == b; }
FORCE_INLINE constexpr bool operator!=(const f128& a, float b) { return !(a == b); }
FORCE_INLINE constexpr bool operator!=(float a, const f128& b) { return !(a == b); }

// --------------- ints <=> f128 ---------------

FORCE_INLINE constexpr bool operator<(const f128& a, int32_t b) { return a < f128(b); }
FORCE_INLINE constexpr bool operator<(int32_t a, const f128& b) { return f128(a) < b; }
FORCE_INLINE constexpr bool operator>(const f128& a, int32_t b) { return b < a; }
FORCE_INLINE constexpr bool operator>(int32_t a, const f128& b) { return b < a; }
FORCE_INLINE constexpr bool operator<=(const f128& a, int32_t b) { return !(b < a); }
FORCE_INLINE constexpr bool operator<=(int32_t a, const f128& b) { return !(b < a); }
FORCE_INLINE constexpr bool operator>=(const f128& a, int32_t b) { return !(a < b); }
FORCE_INLINE constexpr bool operator>=(int32_t a, const f128& b) { return !(a < b); }
FORCE_INLINE constexpr bool operator==(const f128& a, int32_t b) { return a == f128(b); }
FORCE_INLINE constexpr bool operator==(int32_t a, const f128& b) { return f128(a) == b; }
FORCE_INLINE constexpr bool operator!=(const f128& a, int32_t b) { return !(a == b); }
FORCE_INLINE constexpr bool operator!=(int32_t a, const f128& b) { return !(a == b); }

FORCE_INLINE constexpr bool operator<(const f128& a, uint32_t b) { return a < f128(b); }
FORCE_INLINE constexpr bool operator<(uint32_t a, const f128& b) { return f128(a) < b; }
FORCE_INLINE constexpr bool operator>(const f128& a, uint32_t b) { return b < a; }
FORCE_INLINE constexpr bool operator>(uint32_t a, const f128& b) { return b < a; }
FORCE_INLINE constexpr bool operator<=(const f128& a, uint32_t b) { return !(b < a); }
FORCE_INLINE constexpr bool operator<=(uint32_t a, const f128& b) { return !(b < a); }
FORCE_INLINE constexpr bool operator>=(const f128& a, uint32_t b) { return !(a < b); }
FORCE_INLINE constexpr bool operator>=(uint32_t a, const f128& b) { return !(a < b); }
FORCE_INLINE constexpr bool operator==(const f128& a, uint32_t b) { return a == f128(b); }
FORCE_INLINE constexpr bool operator==(uint32_t a, const f128& b) { return f128(a) == b; }
FORCE_INLINE constexpr bool operator!=(const f128& a, uint32_t b) { return !(a == b); }
FORCE_INLINE constexpr bool operator!=(uint32_t a, const f128& b) { return !(a == b); }

FORCE_INLINE constexpr bool operator<(const f128& a, int64_t b) { return a < f128(b); }
FORCE_INLINE constexpr bool operator<(int64_t a, const f128& b) { return f128(a) < b; }
FORCE_INLINE constexpr bool operator>(const f128& a, int64_t b) { return b < a; }
FORCE_INLINE constexpr bool operator>(int64_t a, const f128& b) { return b < a; }
FORCE_INLINE constexpr bool operator<=(const f128& a, int64_t b) { return !(b < a); }
FORCE_INLINE constexpr bool operator<=(int64_t a, const f128& b) { return !(b < a); }
FORCE_INLINE constexpr bool operator>=(const f128& a, int64_t b) { return !(a < b); }
FORCE_INLINE constexpr bool operator>=(int64_t a, const f128& b) { return !(a < b); }
FORCE_INLINE constexpr bool operator==(const f128& a, int64_t b) { return a == f128(b); }
FORCE_INLINE constexpr bool operator==(int64_t a, const f128& b) { return f128(a) == b; }
FORCE_INLINE constexpr bool operator!=(const f128& a, int64_t b) { return !(a == b); }
FORCE_INLINE constexpr bool operator!=(int64_t a, const f128& b) { return !(a == b); }

FORCE_INLINE constexpr bool operator<(const f128& a, uint64_t b) { return a < f128(b); }
FORCE_INLINE constexpr bool operator<(uint64_t a, const f128& b) { return f128(a) < b; }
FORCE_INLINE constexpr bool operator>(const f128& a, uint64_t b) { return b < a; }
FORCE_INLINE constexpr bool operator>(uint64_t a, const f128& b) { return b < a; }
FORCE_INLINE constexpr bool operator<=(const f128& a, uint64_t b) { return !(b < a); }
FORCE_INLINE constexpr bool operator<=(uint64_t a, const f128& b) { return !(b < a); }
FORCE_INLINE constexpr bool operator>=(const f128& a, uint64_t b) { return !(a < b); }
FORCE_INLINE constexpr bool operator>=(uint64_t a, const f128& b) { return !(a < b); }
FORCE_INLINE constexpr bool operator==(const f128& a, uint64_t b) { return a == f128(b); }
FORCE_INLINE constexpr bool operator==(uint64_t a, const f128& b) { return f128(a) == b; }
FORCE_INLINE constexpr bool operator!=(const f128& a, uint64_t b) { return !(a == b); }
FORCE_INLINE constexpr bool operator!=(uint64_t a, const f128& b) { return !(a == b); }


/// ======== Arithmetic operators ========

BL_PUSH_PRECISE
FORCE_INLINE constexpr f128 operator+(const f128& a, const f128& b)
{
    // accurate sum of the high parts
    double s, e;
    two_sum_precise(a.hi, b.hi, s, e);

    // fold low parts into the error
    double t = a.lo + b.lo;
    e += t;

    // renormalize
    return renorm(s, e);
}
BL_POP_PRECISE

FORCE_INLINE constexpr f128 operator-(const f128& a, const f128& b)
{
    return a + f128{ -b.hi, -b.lo };
}
#ifdef FMA_AVAILABLE
FORCE_INLINE f128 operator*(const f128& a, const f128& b)
{
    double p, e;
    two_prod_precise_fma(a.hi, b.hi, p, e);   // p ≈ a.hi*b.hi, e = exact error

    // Fold in cross terms and low*low
    e = fma1(a.hi, b.lo, e);
    e = fma1(a.lo, b.hi, e);
    e = fma1(a.lo, b.lo, e); // kept for generic high precision

    return renorm(p, e);
}
FORCE_INLINE           f128 operator/(const f128& a, const f128& b) { return a * recip(b); }
#else
FORCE_INLINE constexpr f128 operator*(const f128& a, const f128& b)
{
    double p, e;
    two_prod_precise_dekker(a.hi, b.hi, p, e);
    e += a.hi * b.lo + a.lo * b.hi;
    e += a.lo * b.lo; // restore full dd precision

    return quick_two_sum(p, e);
}
FORCE_INLINE constexpr f128 operator/(const f128& a, const f128& b) { return a * recip(b); }
#endif

/// 'double' support
#ifdef FMA_AVAILABLE
FORCE_INLINE           f128 operator*(const f128& a, double b) { return a * f128{ b }; }
FORCE_INLINE           f128 operator*(double a, const f128& b) { return f128{ a } * b; }
FORCE_INLINE           f128 operator/(const f128& a, double b) { return a / f128{ b }; }
FORCE_INLINE           f128 operator/(double a, const f128& b) { return f128{ a } / b; }
#else
FORCE_INLINE constexpr f128 operator*(const f128& a, double b) { return a * f128{ b }; }
FORCE_INLINE constexpr f128 operator*(double a, const f128& b) { return f128{ a } * b; }
FORCE_INLINE constexpr f128 operator/(const f128& a, double b) { return a / f128{ b }; }
FORCE_INLINE constexpr f128 operator/(double a, const f128& b) { return f128{ a } / b; }
#endif
FORCE_INLINE constexpr f128 operator+(const f128& a, double b) { return a + f128{ b }; }
FORCE_INLINE constexpr f128 operator+(double a, const f128& b) { return f128{ a } + b; }
FORCE_INLINE constexpr f128 operator-(const f128& a, double b) { return a - f128{ b }; }
FORCE_INLINE constexpr f128 operator-(double a, const f128& b) { return f128{ a } - b; }

FORCE_INLINE bool isnan(const f128& x) { return std::isnan(x.hi); }
FORCE_INLINE bool isinf(const f128& x) { return std::isinf(x.hi); }
FORCE_INLINE bool isfinite(const f128& x) { return std::isfinite(x.hi); }
FORCE_INLINE bool iszero(const f128& x) { return x.hi == 0.0 && x.lo == 0.0; }

FORCE_INLINE double log_as_double(float a)
{
    return std::log(a);
}
FORCE_INLINE double log_as_double(double a)
{
    return std::log(a);
}
FORCE_INLINE double log_as_double(f128 a)
{
    return std::log(a.hi) + std::log1p(a.lo / a.hi);
}
FORCE_INLINE f128 log(const f128& a)
{
    double log_hi = std::log(a.hi); // first guess
    f128 exp_log_hi(std::exp(log_hi)); // exp(guess)
    f128 r = (a - exp_log_hi) / exp_log_hi; // (a - e^g) / e^g ≈ error
    return f128(log_hi) + r; // refined log
}
FORCE_INLINE f128 log2(const f128& a)
{
    constexpr f128 LOG2_RECIPROCAL(1.442695040888963407359924681001892137); // 1/log(2)
    return log(a) * LOG2_RECIPROCAL;
}
FORCE_INLINE f128 log10(const f128& x)
{
    // 1 / ln(10) to full-double accuracy
    static constexpr double INV_LN10 = 0.434294481903251827651128918916605082;
    return log(x) * f128(INV_LN10);   // log(x) already returns float128
}
FORCE_INLINE f128 clamp(const f128& v, const f128& lo, const f128& hi)
{
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}
FORCE_INLINE f128 abs(const f128& a)
{
    return (a.hi < 0.0) ? -a : a;
}
FORCE_INLINE f128 fabs(const f128& a) 
{
    return (a.hi < 0.0) ? -a : a;
}
FORCE_INLINE f128 floor(const f128& a)
{
    double f = std::floor(a.hi);

    // for integral hi, decide using the rounded-to-double sum to avoid false decrement from tiny lo noise
    if (f == a.hi) {
        const double sum = a.hi + a.lo;
        if (sum < f) f -= 1.0;
    }

    return { f, 0.0 };
}
FORCE_INLINE f128 ceil(const f128& a)
{
    double c = std::ceil(a.hi);

    // for integral hi, decide using the rounded-to-double sum to avoid false increment from tiny lo noise
    if (c == a.hi) {
        const double sum = a.hi + a.lo;
        if (sum > c) c += 1.0;
    }

    return { c, 0.0 };
}
FORCE_INLINE f128 trunc(const f128& a)
{
    return (a.hi < 0.0) ? ceil(a) : floor(a);
}
FORCE_INLINE f128 round(const f128& a)
{
    f128 t = floor(a + f128(0.5));
    /* halfway cases: round to even like std::round */
    if (((t.hi - a.hi) == 0.5) && (std::fmod(t.hi, 2.0) != 0.0)) t -= f128(1.0);
    return t;
}
FORCE_INLINE f128 fmod(const f128& x, const f128& y)
{
    if (y.hi == 0.0 && y.lo == 0.0)
        return std::numeric_limits<f128>::quiet_NaN();
    return x - trunc(x / y) * y;
}
FORCE_INLINE f128 remainder(const f128& x, const f128& y)
{
    // Domain checks (match std::remainder)
    if (isnan(x) || isnan(y)) return std::numeric_limits<f128>::quiet_NaN();
    if (iszero(y))            return std::numeric_limits<f128>::quiet_NaN();
    if (isinf(x))             return std::numeric_limits<f128>::quiet_NaN();
    if (isinf(y))             return x;

    // n = nearest integer to q = x/y, ties to even
    const f128 q = x / y;
    f128 n = trunc(q);
    f128 rfrac = q - n;                // fractional part with sign of q
    const f128 half = f128(0.5);
    const f128 one = 1;

    if (abs(rfrac) > half) {
        n += (rfrac.hi >= 0.0 ? one : -one);
    }
    else if (abs(rfrac) == half) {
        // tie: choose even n
        const f128 n_mod2 = fmod(n, 2);
        if (n_mod2 != 0)
            n += (rfrac.hi >= 0.0 ? one : -one);
    }

    f128 r = x - n * y;

    // If result is zero, sign should match x (std::remainder semantics)
    if (iszero(r))
        return f128(std::signbit(x.hi) ? -0.0 : 0.0);

    return r;
}

/*------------ transcendentals -------------------------------------------*/

// high-precision constants
static constexpr f128 F128_PIO2 = renorm(1.5707963267948966, 6.1232339957367660e-17);
static constexpr f128 F128_INV_PIO2 = renorm(0.6366197723675814, -3.9357353350364970e-17);
static constexpr f128 F128_LN2 = renorm(0.6931471805599453, 2.3190468138462996e-17);
static constexpr f128 F128_INV_LN2 = renorm(1.4426950408889634, 2.0355273740931033e-17);
static constexpr double F128_PIO4_HI = 0.7853981633974483;

FORCE_INLINE f128 f128_ldexp(const f128& x, int e)
{
    // Scale by a power of two (preserves the double exponent range behavior)
    return f128(std::ldexp(x.hi, e), std::ldexp(x.lo, e));
}
FORCE_INLINE bool f128_rem_pio2(const f128& x, long long& n_out, f128& r_out)
{
    // Reduce x = n*(pi/2) + r with r in roughly [-pi/4, pi/4].
    // Returns false if n cannot be represented robustly

    double ax = std::fabs(x.hi);
    if (!std::isfinite(ax)) return false;

    // If x exceeds 2^52, a double cannot represent every integer and remainder accuracy collapses
    if (ax > 7.0e15) return false;

    f128 t = x * F128_INV_PIO2;
    double qd = std::nearbyint((double)t);

    // Guard cast to int64
    if (!std::isfinite(qd) ||
        qd < (double)std::numeric_limits<long long>::min() ||
        qd >(double)std::numeric_limits<long long>::max())
        return false;

    long long n = (long long)qd;
    n_out = n;
    r_out = x - f128((double)n) * F128_PIO2;
    return true;
}
FORCE_INLINE void f128_sincos_kernel_pio4(const f128& x, f128& s_out, f128& c_out)
{
    // Taylor kernels on |x| <= pi/4. Degrees chosen to push truncation error below ~1e-34.
    f128 t = x * x;

    // sin(x) = x + x^3 * P(t)
    f128 ps = f128(1.13099628864477159e-31);      //  1/29!
    ps = ps * t + f128(-9.18368986379554601e-29); // -1/27!
    ps = ps * t + f128(6.44695028438447359e-26);  //  1/25!
    ps = ps * t + f128(-3.86817017063068413e-23); // -1/23!
    ps = ps * t + f128(1.95729410633912626e-20);  //  1/21!
    ps = ps * t + f128(-8.22063524662432950e-18); // -1/19!
    ps = ps * t + f128(2.81145725434552060e-15);  //  1/17!
    ps = ps * t + f128(-7.64716373181981641e-13); // -1/15!
    ps = ps * t + f128(1.60590438368216133e-10);  //  1/13!
    ps = ps * t + f128(-2.50521083854417202e-08); // -1/11!
    ps = ps * t + f128(2.75573192239858925e-06);  //  1/9!
    ps = ps * t + f128(-1.98412698412698413e-04); // -1/7!
    ps = ps * t + f128(8.33333333333333322e-03);  //  1/5!
    ps = ps * t + f128(-1.66666666666666657e-01); // -1/3!
    s_out = x + x * t * ps;

    // cos(x) = 1 + x^2 * Q(t)
    f128 pc = f128(3.27988923706983776e-30);       //  1/28!
    pc = pc * t + f128(-2.47959626322479759e-27);  // -1/26!
    pc = pc * t + f128(1.61173757109611839e-24);   //  1/24!
    pc = pc * t + f128(-8.89679139245057408e-22);  // -1/22!
    pc = pc * t + f128(4.11031762331216484e-19);   //  1/20!
    pc = pc * t + f128(-1.56192069685862253e-16);  // -1/18!
    pc = pc * t + f128(4.77947733238738525e-14);   //  1/16!
    pc = pc * t + f128(-1.14707455977297245e-11);  // -1/14!
    pc = pc * t + f128(2.08767569878681002e-09);   //  1/12!
    pc = pc * t + f128(-2.75573192239858883e-07);  // -1/10!
    pc = pc * t + f128(2.48015873015873016e-05);   //  1/8!
    pc = pc * t + f128(-1.38888888888888894e-03);  // -1/6!
    pc = pc * t + f128(4.16666666666666644e-02);   //  1/4!
    pc = pc * t + f128(-5.00000000000000000e-01);  // -1/2!
    c_out = f128(1.0) + t * pc;
}
FORCE_INLINE bool f128_sincos(const f128& x, f128& s_out, f128& c_out)
{
    // Full sin/cos with quadrant handling; uses kernels above
    double ax = std::fabs(x.hi);
    if (!std::isfinite(ax))
    {
        s_out = f128(std::numeric_limits<double>::quiet_NaN());
        c_out = s_out;
        return false;
    }

    // Fast path: already small enough.
    if (ax <= F128_PIO4_HI)
    {
        f128_sincos_kernel_pio4(x, s_out, c_out);
        return true;
    }

    long long n = 0;
    f128 r;
    if (!f128_rem_pio2(x, n, r))
        return false;

    f128 sr, cr;
    f128_sincos_kernel_pio4(r, sr, cr);

    switch ((int)(n & 3))
    {
    case 0: s_out = sr;  c_out = cr;  break;
    case 1: s_out = cr;  c_out = -sr; break;
    case 2: s_out = -sr; c_out = -cr; break;
    default: s_out = -cr; c_out = sr; break;
    }
    return true;
}
FORCE_INLINE f128 f128_exp_kernel_ln2_half(const f128& r)
{
    // exp(r) kernel for |r| <= ln2/2 via truncated Taylor series (degree 22)
    f128 p =    f128(8.89679139245057408e-22); // 1/22!
    p = p * r + f128(1.95729410633912626e-20); // 1/21!
    p = p * r + f128(4.11031762331216484e-19); // 1/20!
    p = p * r + f128(8.22063524662432950e-18); // 1/19!
    p = p * r + f128(1.56192069685862253e-16); // 1/18!
    p = p * r + f128(2.81145725434552060e-15); // 1/17!
    p = p * r + f128(4.77947733238738525e-14); // 1/16!
    p = p * r + f128(7.64716373181981641e-13); // 1/15!
    p = p * r + f128(1.14707455977297245e-11); // 1/14!
    p = p * r + f128(1.60590438368216133e-10); // 1/13!
    p = p * r + f128(2.08767569878681002e-09); // 1/12!
    p = p * r + f128(2.50521083854417202e-08); // 1/11!
    p = p * r + f128(2.75573192239858883e-07); // 1/10!
    p = p * r + f128(2.75573192239858925e-06); // 1/9!
    p = p * r + f128(2.48015873015873016e-05); // 1/8!
    p = p * r + f128(1.98412698412698413e-04); // 1/7!
    p = p * r + f128(1.38888888888888894e-03); // 1/6!
    p = p * r + f128(8.33333333333333322e-03); // 1/5!
    p = p * r + f128(4.16666666666666644e-02); // 1/4!
    p = p * r + f128(1.66666666666666657e-01); // 1/3!
    p = p * r + f128(5.00000000000000000e-01); // 1/2!
    p = p * r + f128(1.0);                     // 1/1!
    return (p * r) + f128(1.0);                //+1/0!
}
FORCE_INLINE f128 f128_sin_kernel_pio4(const f128& x)
{
    // sin kernel for |x| <= pi/4.
    f128 t = x * x;

    // sin(x) = x + x*t*P(t)
    f128 ps = f128(-9.18368986379554615e-29);
    ps = ps * t + f128(6.4469502843844734e-26);
    ps = ps * t + f128(-3.86817017063068404e-23);
    ps = ps * t + f128(1.95729410633912612e-20);
    ps = ps * t + f128(-8.22063524662432972e-18);
    ps = ps * t + f128(2.81145725434552076e-15);
    ps = ps * t + f128(-7.64716373181981648e-13);
    ps = ps * t + f128(1.60590438368216146e-10);
    ps = ps * t + f128(-2.50521083854417188e-8);
    ps = ps * t + f128(2.75573192239858907e-6);
    ps = ps * t + f128(-1.98412698412698413e-4);
    ps = ps * t + f128(8.33333333333333333e-3);
    ps = ps * t + f128(-1.66666666666666667e-1);

    return x + x * t * ps;
}
FORCE_INLINE f128 f128_cos_kernel_pio4(const f128& x)
{
    // cos kernel for |x| <= pi/4
    f128 t = x * x;

    // cos(x) = 1 + t*Q(t)
    f128 pc = f128(3.27988923706983791e-30);
    pc = pc * t + f128(-2.47959626322479746e-27);
    pc = pc * t + f128(1.61173757109611835e-24);
    pc = pc * t + f128(-8.89679139245057329e-22);
    pc = pc * t + f128(4.11031762331216486e-19);
    pc = pc * t + f128(-1.56192069685862265e-16);
    pc = pc * t + f128(4.7794773323873853e-14);
    pc = pc * t + f128(-1.14707455977297247e-11);
    pc = pc * t + f128(2.0876756987868099e-9);
    pc = pc * t + f128(-2.75573192239858907e-7);
    pc = pc * t + f128(2.48015873015873016e-5);
    pc = pc * t + f128(-1.38888888888888889e-3);
    pc = pc * t + f128(4.16666666666666667e-2);
    pc = pc * t + f128(-5.0e-1);

    return f128(1.0) + t * pc;
}

FORCE_INLINE f128 sin(f128 a)
{
    double ax = std::fabs(a.hi);
    if (!std::isfinite(ax))
        return f128(std::sin((double)a));

    // Small fast path: no range reduction
    if (ax <= F128_PIO4_HI)
        return f128_sin_kernel_pio4(a);

    long long n = 0;
    f128 r;
    if (!f128_rem_pio2(a, n, r))
        return f128(std::sin((double)a));

    // sin(x) uses sin(r) or cos(r) depending on quadrant.
    switch ((int)(n & 3))
    {
    case 0:  return  f128_sin_kernel_pio4(r);
    case 1:  return  f128_cos_kernel_pio4(r);
    case 2:  return -f128_sin_kernel_pio4(r);
    default: return -f128_cos_kernel_pio4(r);
    }
}
FORCE_INLINE f128 cos(f128 a)
{
    double ax = std::fabs(a.hi);
    if (!std::isfinite(ax))
        return f128(std::cos((double)a));

    // Small fast path: no range reduction.
    if (ax <= F128_PIO4_HI)
        return f128_cos_kernel_pio4(a);

    long long n = 0;
    f128 r;
    if (!f128_rem_pio2(a, n, r))
        return f128(std::cos((double)a));

    // cos(x) uses cos(r) or sin(r) depending on quadrant.
    switch ((int)(n & 3))
    {
    case 0:  return  f128_cos_kernel_pio4(r);
    case 1:  return -f128_sin_kernel_pio4(r);
    case 2:  return -f128_cos_kernel_pio4(r);
    default: return  f128_sin_kernel_pio4(r);
    }
}
FORCE_INLINE f128 sqrt(f128 a)
{
    // Match std semantics for negative / zero quickly.
    if (a.hi <= 0.0)
    {
        if (a.hi == 0.0 && a.lo == 0.0) return f128(0.0);
        return f128(std::numeric_limits<double>::quiet_NaN());
    }

    double y0 = std::sqrt(a.hi);
    f128 y(y0);

    // Two Newton refinements
    y = y + (a - y * y) / (y + y);
    y = y + (a - y * y) / (y + y);
    return y;
}
FORCE_INLINE f128 exp(const f128& a)
{
    // Preserve NaN/Inf behavior in the leading component.
    if (!std::isfinite(a.hi)) return f128(std::exp(a.hi));

    // Hard overflow/underflow limits for double exponent range.
    if (a.hi > 709.782712893384) return f128(std::numeric_limits<double>::max());
    if (a.hi < -745.133219101941) return f128(0.0);

    f128 t = a * F128_INV_LN2;
    double kd = std::nearbyint((double)t);

    // Keep k within exact-in-double integer range (prevents remainder blowups).
    if (!std::isfinite(kd) || std::fabs(kd) > 9.0e15)
        return f128(std::exp((double)a));

    int k = (int)kd;
    f128 r = a - f128((double)k) * F128_LN2;

    // if reduction misbehaved, fall back rather than returning nonsense.
    if (std::fabs(r.hi) > 0.40)
        return f128(std::exp((double)a));

    f128 er = f128_exp_kernel_ln2_half(r);
    return f128_ldexp(er, k);
}
FORCE_INLINE f128 atan2(const f128& y, const f128& x)
{
    static constexpr double DD_PI2 = 1.570796326794896619231321691639751442;

    // Special cases first (match IEEE / std::atan2 as closely as possible).
    if (x.hi == 0.0 && x.lo == 0.0)
    {
        if (y.hi == 0.0 && y.lo == 0.0)
            return f128(std::numeric_limits<double>::quiet_NaN());
        return (y.hi > 0.0 || (y.hi == 0.0 && y.lo > 0.0)) ? f128(DD_PI2) : f128(-DD_PI2);
    }

    // Seed from libm (correct quadrant), then refine with Newton on f(v)=x*sin v - y*cos v.
    f128 v(std::atan2(y.hi, x.hi));

    for (int i = 0; i < 2; ++i)
    {
        f128 sv, cv;
        if (!f128_sincos(v, sv, cv))
        {
            double vd = (double)v;
            sv = f128(std::sin(vd));
            cv = f128(std::cos(vd));
        }

        f128 f = x * sv - y * cv;
        f128 fp = x * cv + y * sv;

        v = v - f / fp;
    }
    return v;
}
FORCE_INLINE f128 tan(const f128& a) { return sin(a) / cos(a); }
FORCE_INLINE f128 pow(const f128& x, const f128& y) { return exp(y * log(x)); }
FORCE_INLINE f128 atan(const f128& x) { return atan2(x, 1); }


/*------------ precise DP rounding -------------------------------------------*/

FORCE_INLINE f128 round_to_decimals(f128 v, int prec)
{
    if (prec <= 0) return v;

    static constexpr f128 INV10_DD{
        0.1000000000000000055511151231257827021181583404541015625,  // hi (double rounded)
       -0.0000000000000000055511151231257827021181583404541015625   // lo = 0.1 - hi
    };

    // Sign
    const bool neg = v < 0.0;
    if (neg) v = -v;

    // Split
    f128 ip = floor(v);
    f128 frac = v - ip;

    // Extract digits with one look-ahead
    std::string dig; dig.reserve((size_t)prec);
    f128 w = frac;
    for (int i = 0; i < prec; ++i)
    {
        w = w * 10.0;
        int di = (int)floor(w).hi;
        if (di < 0) di = 0; else if (di > 9) di = 9;
        dig.push_back(char('0' + di));
        w = w - f128(di);
    }

    // Look-ahead digit
    f128 la = w * 10.0;
    int next = (int)floor(la).hi;
    if (next < 0) next = 0; else if (next > 9) next = 9;
    f128 rem = la - f128(next);

    // ties-to-even on last printed digit
    const int last = dig.empty() ? 0 : (dig.back() - '0');
    const bool round_up =
        (next > 5) ||
        (next == 5 && (rem.hi > 0.0 || rem.lo > 0.0 || (last & 1)));

    if (round_up) {
        // propagate carry over fractional digits; if overflow, bump integer part
        int i = prec - 1;
        for (; i >= 0; --i) {
            if (dig[(size_t)i] == '9') dig[(size_t)i] = '0';
            else { ++dig[(size_t)i]; break; }
        }
        if (i < 0) ip = ip + 1;
    }

    // Rebuild fractional value backward
    f128 frac_val{ 0.0, 0.0 };
    for (int i = prec - 1; i >= 0; --i) {
        frac_val = frac_val + f128(dig[(size_t)i] - '0');
        frac_val = frac_val * INV10_DD;
    }

    f128 out = ip + frac_val;
    return neg ? -out : out;
}


/*------------ printing --------------------------------------------------*/

FORCE_INLINE f128 pow10_128(int k)
{
    if (k == 0) return f128(1.0);

    int n = (k >= 0) ? k : -k;

    // fast small-exponent path
    if (n <= 16) {
        f128 r = f128(1.0);
        const f128 ten = f128(10.0);
        for (int i = 0; i < n; ++i) r = r * ten;
        return (k >= 0) ? r : (f128(1.0) / r);
    }

    f128 r = f128(1.0);
    f128 base = f128(10.0);

    while (n) {
        if (n & 1) r = r * base;
        n >>= 1;
        if (n) base = base * base;
    }

    return (k >= 0) ? r : (f128(1.0) / r);
}

FORCE_INLINE void normalize10(const f128& x, f128& m, int& exp10)
{
    if (x.hi == 0.0 && x.lo == 0.0) { m = f128(0.0); exp10 = 0; return; }

    f128 ax = abs(x);

    int e2 = 0;
    (void)std::frexp(ax.hi, &e2); // ax.hi = f * 2^(e2-1)
    int e10 = (int)std::floor((e2 - 1) * 0.30102999566398114); // ≈ log10(2)

    m = ax * pow10_128(-e10);
    while (m >= f128(10.0)) { m = m / f128(10.0); ++e10; }
    while (m <  f128(1.0))  { m = m * f128(10.0); --e10; }
    exp10 = e10;
}

FORCE_INLINE f128 round_scaled(f128 x, int prec) noexcept
{
    /// round x to an integer at scale = 10^prec (ties-to-even)
    if (prec <= 0) return x;
    const f128 scale = pow10_128(prec);
    f128 y = x * scale;

    f128 n = floor(y); // integer below
    f128 f = y - n;    // fraction in [0,1]

    const f128 half = f128(0.5);
    bool tie = (f == half);
    if (f > half || (tie && fmod(n, 2) != 0))
        n = n + 1;

    return n;
}

BL_PUSH_PRECISE
BL_PRINT_NOINLINE FORCE_INLINE f128 mul_by_double_print(f128 a, double b) noexcept
{
    double p, err;
#ifdef FMA_AVAILABLE
    two_prod_precise_fma(a.hi, b, p, err);
#else
    two_prod_precise_dekker(a.hi, b, p, err);
#endif
    err += a.lo * b;

    double s, e;
    two_sum_precise(p, err, s, e);
    return f128{s, e};
}

BL_PRINT_NOINLINE FORCE_INLINE f128 sub_by_double_print(f128 a, double b) noexcept
{
    double s, e;
    two_sum_precise(a.hi, -b, s, e);
    e += a.lo;

    double ss, ee;
    two_sum_precise(s, e, ss, ee);
    return f128{ss, ee};
}
BL_POP_PRECISE

struct f128_chars_result
{
    char* ptr = nullptr;
    bool ok = false;
};

FORCE_INLINE int emit_uint_rev_buf(char* dst, f128 n)
{
    // n is a non-negative integer in f128
    const f128 base = f128(1000000000.0); // 1e9

    int len = 0;

    if (n < f128(10.0)) {
        int d = (int)n.hi;
        if (d < 0) d = 0; else if (d > 9) d = 9;
        dst[len++] = char('0' + d);
        return len;
    }

    while (n >= base) {
        f128 q = floor(n / base);
        f128 r = n - q * base;

        long long chunk = (long long)std::floor(r.hi);
        if (chunk >= 1000000000LL) { chunk -= 1000000000LL; q = q + f128(1.0); }
        if (chunk < 0) chunk = 0;

        for (int i = 0; i < 9; ++i) {
            int d = int(chunk % 10);
            dst[len++] = char('0' + d);
            chunk /= 10;
        }

        n = q;
    }

    long long last = (long long)std::floor(n.hi);
    if (last == 0) {
        dst[len++] = '0';
    } else {
        while (last > 0) {
            int d = int(last % 10);
            dst[len++] = char('0' + d);
            last /= 10;
        }
    }

    return len;
}

FORCE_INLINE void emit_uint_rev(std::string& out, f128 n)
{
    char tmp[320];
    int len = emit_uint_rev_buf(tmp, n);
    out.assign(tmp, tmp + len);
}

FORCE_INLINE f128_chars_result append_exp10_to_chars(char* p, char* end, int e10) noexcept
{
    if (p >= end) return {p, false};
    *p++ = 'e';

    if (p >= end) return {p, false};
    if (e10 < 0) { *p++ = '-'; e10 = -e10; }
    else         { *p++ = '+'; }

    char buf[8];
    int n = 0;
    do {
        buf[n++] = char('0' + (e10 % 10));
        e10 /= 10;
    } while (e10);

    if (n < 2) buf[n++] = '0';

    if (p + n > end) return {p, false};
    for (int i = n - 1; i >= 0; --i) *p++ = buf[i];

    return {p, true};
}

FORCE_INLINE f128_chars_result emit_fixed_dec_to_chars(char* first, char* last, f128 x, int prec, bool strip_trailing_zeros) noexcept
{
    if (x.hi == 0.0 && x.lo == 0.0) {
        if (first >= last) return {first, false};
        *first = '0';
        return {first + 1, true};
    }

    if (prec < 0) prec = 0;

    const bool neg = (x.hi < 0.0);
    if (neg) x = f128{-x.hi, -x.lo};
    x = renorm(x.hi, x.lo);

    f128 ip = floor(x);
    f128 fp = sub_by_double_print(x, ip.hi);

    // compensate for non-canonical hi/lo splits where floor based on hi underestimates the integer part
    if (fp >= f128(1.0)) { fp = fp - f128(1.0); ip = ip + f128(1.0); }
    else if (fp < f128(0.0)) { fp = f128(0.0); }

    // fractional digits scratch (rounded in-place)
    constexpr int kFracStack = 2048;
    char frac_stack[kFracStack];
    char* frac = frac_stack;

    std::string frac_dyn;
    if (prec > kFracStack) {
        frac_dyn.resize((size_t)prec);
        frac = frac_dyn.data();
    }

    int frac_len = (prec > 0) ? prec : 0;

    if (prec > 0) {
        static constexpr double kPow10[10] = {
            1.0, 10.0, 100.0, 1000.0, 10000.0, 100000.0,
            1000000.0, 10000000.0, 100000000.0, 1000000000.0
        };

        int written = 0;
        const int full = prec / 9;
        const int rem  = prec - full * 9;

        for (int c = 0; c < full; ++c) {
            fp = mul_by_double_print(fp, kPow10[9]);
            uint32_t chunk = (uint32_t)fp.hi;
            fp = sub_by_double_print(fp, (double)chunk);

            for (int i = 8; i >= 0; --i) {
                frac[written + i] = char('0' + (chunk % 10u));
                chunk /= 10u;
            }
            written += 9;
        }

        if (rem > 0) {
            fp = mul_by_double_print(fp, kPow10[rem]);
            uint32_t chunk = (uint32_t)fp.hi;
            fp = sub_by_double_print(fp, (double)chunk);

            for (int i = rem - 1; i >= 0; --i) {
                frac[written + i] = char('0' + (chunk % 10u));
                chunk /= 10u;
            }
            written += rem;
        }

        // look-ahead digit for ties-to-even
        f128 la = mul_by_double_print(fp, 10.0);
        int next = (int)la.hi;
        if (next < 0) next = 0; else if (next > 9) next = 9;
        f128 remv = sub_by_double_print(la, (double)next);

        const int last_digit = frac[prec - 1] - '0';
        bool round_up = false;
        if (next > 5) round_up = true;
        else if (next < 5) round_up = false;
        else {
            const bool gt_half = (remv.hi > 0.0) || (remv.lo > 0.0);
            round_up = gt_half || ((last_digit & 1) != 0);
        }

        if (round_up) {
            int i = prec - 1;
            for (; i >= 0; --i) {
                char& c = frac[i];
                if (c == '9') c = '0';
                else { c = char(c + 1); break; }
            }
            if (i < 0) {
                ip = ip + f128(1.0);
                for (int j = 0; j < prec; ++j) frac[j] = '0';
            }
        }

        if (strip_trailing_zeros) {
            while (frac_len > 0 && frac[frac_len - 1] == '0') --frac_len;
        }
    }

    char int_rev[320];
    int int_len = emit_uint_rev_buf(int_rev, ip);

    if (neg && int_len == 1 && int_rev[0] == '0' && frac_len == 0) {
        if (first >= last) return {first, false};
        *first = '0';
        return {first + 1, true};
    }

    const size_t needed = (size_t)(neg ? 1 : 0) + (size_t)int_len + (frac_len ? (size_t)(1 + frac_len) : 0u);
    if ((size_t)(last - first) < needed) return {first, false};

    char* p = first;
    if (neg) *p++ = '-';

    for (int i = int_len - 1; i >= 0; --i) *p++ = int_rev[i];

    if (frac_len > 0) {
        *p++ = '.';
        std::memcpy(p, frac, (size_t)frac_len);
        p += frac_len;
    }

    return {p, true};
}

FORCE_INLINE f128_chars_result emit_scientific_to_chars(char* first, char* last, const f128& x, std::streamsize prec, bool strip_trailing_zeros) noexcept
{
    if (x.hi == 0.0 && x.lo == 0.0) {
        if (first >= last) return {first, false};
        *first = '0';
        return {first + 1, true};
    }

    if (prec < 1) prec = 1; // total significant digits

    const bool neg = (x.hi < 0.0);
    f128 v = neg ? f128(-x.hi, -x.lo) : x;

    f128 m; int e = 0;
    normalize10(v, m, e);

    const int sig = (int)prec;
    unsigned char dbuf[128];
    const int max_sig = (int)std::min<int>(sig + 1, (int)sizeof(dbuf));

    f128 t = m;
    for (int i = 0; i < max_sig; ++i) {
        int di = (int)t.hi;
        if (di < 0) di = 0; else if (di > 9) di = 9;
        dbuf[i] = (unsigned char)di;
        t = mul_by_double_print(sub_by_double_print(t, (double)di), 10.0);
    }

    bool carry = (max_sig >= sig + 1) && (dbuf[sig] >= 5);

    for (int i = sig - 1; i >= 0 && carry; --i) {
        int vdig = (int)dbuf[i] + 1;
        if (vdig == 10) { dbuf[i] = 0; carry = true; }
        else            { dbuf[i] = (unsigned char)vdig; carry = false; }
    }

    if (carry) {
        for (int i = sig - 1; i >= 1; --i) dbuf[i] = 0;
        dbuf[0] = 1;
        ++e;
    }

    int last_frac = sig - 1;
    if (sig > 1 && strip_trailing_zeros) {
        while (last_frac >= 1 && dbuf[last_frac] == 0) --last_frac;
    }

    // compute exponent part first (for bounds)
    char exp_buf[16];
    {
        char* ep = exp_buf;
        char* eend = exp_buf + sizeof(exp_buf);
        auto er = append_exp10_to_chars(ep, eend, e);
        if (!er.ok) return {first, false};
        int exp_len = (int)(er.ptr - ep);

        const bool has_frac = (sig > 1) && (last_frac >= 1);
        const size_t needed = (size_t)(neg ? 1 : 0) + 1u + (has_frac ? (size_t)(1 + last_frac) : 0u) + (size_t)exp_len;
        if ((size_t)(last - first) < needed) return {first, false};

        char* p = first;
        if (neg) *p++ = '-';
        *p++ = char('0' + dbuf[0]);

        if (has_frac) {
            *p++ = '.';
            for (int i = 1; i <= last_frac; ++i) *p++ = char('0' + dbuf[i]);
        }

        std::memcpy(p, exp_buf, (size_t)exp_len);
        p += exp_len;

        return {p, true};
    }
}

FORCE_INLINE f128_chars_result to_chars(char* first, char* last, const f128& x, int precision,
    bool fixed = false, bool scientific = false,
    bool strip_trailing_zeros = false) noexcept
{
    if (precision < 0) precision = 0;

    if (fixed && !scientific) {
        return emit_fixed_dec_to_chars(first, last, x, precision, strip_trailing_zeros);
    }

    if (scientific && !fixed) {
        return emit_scientific_to_chars(first, last, x, precision, strip_trailing_zeros);
    }

    if (x.hi == 0.0 && x.lo == 0.0) {
        if (first >= last) return {first, false};
        *first = '0';
        return {first + 1, true};
    }

    f128 ax = (x.hi < 0.0) ? f128(-x.hi, -x.lo) : x;
    f128 m; int e10 = 0;
    normalize10(ax, m, e10);

    if (e10 >= -4 && e10 < precision) {
        const int frac = (e10 >= 0)
            ? precision
            : std::max(0, precision - 1 - e10);
        return emit_fixed_dec_to_chars(first, last, x, frac, strip_trailing_zeros);
    }

    return emit_scientific_to_chars(first, last, x, precision, strip_trailing_zeros);
}

FORCE_INLINE void to_string_into(std::string& out, const f128& x, int precision,
    bool fixed = false, bool scientific = false,
    bool strip_trailing_zeros = false)
{
    if (precision < 0) precision = 0;

    // avoids reallocation in the hot loop
    const size_t cap_fixed = 1u + 309u + 1u + (size_t)precision + 32u;
    const size_t cap_sci   = 1u + 1u   + 1u + (size_t)precision + 32u;
    const size_t cap = (cap_fixed > cap_sci) ? cap_fixed : cap_sci;

    out.resize(cap);

    char* first = out.data();
    char* last  = out.data() + out.size();

    auto r = to_chars(first, last, x, precision, fixed, scientific, strip_trailing_zeros);
    if (!r.ok) { out.clear(); return; }

    out.resize((size_t)(r.ptr - first));
}

FORCE_INLINE void emit_scientific(std::string& os, const f128& x, std::streamsize prec, bool strip_trailing_zeros)
{
    to_string_into(os, x, (int)prec, false, true, strip_trailing_zeros);
}

FORCE_INLINE void emit_fixed_dec(std::string& os, f128 x, int prec, bool strip_trailing_zeros)
{
    to_string_into(os, x, prec, true, false, strip_trailing_zeros);
}

FORCE_INLINE std::string to_string(const f128& x, int precision,
    bool fixed = false, bool scientific = false,
    bool strip_trailing_zeros = false)
{
    std::string out;
    to_string_into(out, x, precision, fixed, scientific, strip_trailing_zeros);
    return out;
}
FORCE_INLINE bool   valid_flt128_string(const char* s)
{
    for (char* p = (char*)s; *p; ++p) {
        if (!(isdigit(*p) || *p == '.' || *p == 'e' || *p == 'E' || *p=='-'))
            return false;
    }
    return true;
}
FORCE_INLINE bool   parse_flt128(const char* s, f128& out, const char** endptr = nullptr)
{
    const char* p = s;

    // 1) skip spaces
    while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r' || *p == '\f' || *p == '\v') ++p;

    // 2) sign
    bool neg = false;
    if (*p == '+' || *p == '-') { neg = (*p == '-'); ++p; }

    // 3) specials: nan/inf
    auto ci = [](char c) { return (unsigned char)((c >= 'A' && c <= 'Z') ? (c | 0x20) : c); }; // cheap lower
    const char* ps = p;
    if ((ci(p[0]) == 'n' && ci(p[1]) == 'a' && ci(p[2]) == 'n'))
    {
        out = std::numeric_limits<f128>::quiet_NaN();
        p += 3; if (endptr) *endptr = p; return true;
    }
    if ((ci(p[0]) == 'i' && ci(p[1]) == 'n' && ci(p[2]) == 'f'))
    {
        p += 3;
        if (ci(p[0]) == 'i' && ci(p[1]) == 'n' && ci(p[2]) == 'i' && ci(p[3]) == 't' && ci(p[4]) == 'y') p += 5;
        out = neg ? -std::numeric_limits<f128>::max() : std::numeric_limits<f128>::max();
        if (endptr) *endptr = p; return true;
    }
    p = ps; // reset if not special

    // 4) digits (integer part)
    auto isdig = [](char c)->bool { return (c >= '0' && c <= '9'); };

    f128 int_part = 0;
    f128 frac_part = 0;
    int    frac_digits = 0;
    bool   any_digit = false;

    const f128 base1e9 = f128(1000000000.0);
    auto mul_pow10_small = [](f128 v, int n)->f128 {
        // multiply by 10^n, 0<=n<=9
        static const int POWS[10] = { 1,10,100,1000,10000,100000,1000000,10000000,100000000,1000000000 };
        return v * f128(POWS[n]); // constant fits in double exactly for n<=9
    };

    // integer: chunked base 1e9 accumulation
    {
        unsigned int chunk = 0; int clen = 0;
        while (isdig(*p)) {
            chunk = chunk * 10u + unsigned(*p - '0'); ++clen; ++p; any_digit = true;
            if (clen == 9) { int_part = int_part * base1e9 + f128((double)chunk); chunk = 0; clen = 0; }
        }
        if (clen > 0)
            int_part = int_part * mul_pow10_small(1, clen) + f128((double)chunk);
    }

    // fractional: '.' then digits
    if (*p == '.')
    {
        ++p;
        unsigned int chunk = 0; int clen = 0;
        int local_digits = 0;
        while (isdig(*p)) {
            chunk = chunk * 10u + unsigned(*p - '0'); ++clen; ++p; ++local_digits; any_digit = true;
            if (clen == 9) { frac_part = frac_part * base1e9 + f128((double)chunk); chunk = 0; clen = 0; }
        }
        if (clen > 0)
            frac_part = frac_part * mul_pow10_small(1, clen) + f128((double)chunk);
        frac_digits = local_digits;
    }

    if (!any_digit) { if (endptr) *endptr = s; return false; }

    // 5) exponent: e[+/-]digits
    int exp10 = 0;
    if (*p == 'e' || *p == 'E')
    {
        const char* pe = p + 1;
        bool eneg = false;
        if (*pe == '+' || *pe == '-') { eneg = (*pe == '-'); ++pe; }
        if (!isdig(*pe)) { /* malformed exponent: stop before 'e' */ }
        else {
            int eacc = 0;
            while (isdig(*pe)) {
                int d = *pe - '0';
                if (eacc < 100000000) eacc = eacc * 10 + d; // avoid int overflow
                ++pe;
            }
            exp10 = eneg ? -eacc : eacc;
            p = pe;
        }
    }

    // 6) compose value = (int_part + frac_part / 10^frac_digits) * 10^exp10
    f128 value = int_part;
    if (frac_digits > 0) {
        // compute 10^frac_digits
        f128 pow_frac = 1;
        int fd = frac_digits;
        while (fd >= 9) { pow_frac = pow_frac * base1e9; fd -= 9; }
        if (fd > 0) {
            static const int POWS[10] = { 1,10,100,1000,10000,100000,1000000,10000000,100000000,1000000000 };
            pow_frac = pow_frac * f128((double)POWS[fd]);
        }
        value = value + (frac_part / pow_frac);
    }

    if (exp10 != 0) {
        // scale by 10^exp10 (do it in 1e9 blocks to reduce ops)
        int e = exp10;
        if (e > 0) {
            while (e >= 9) { value = value * base1e9; e -= 9; }
            if (e > 0) {
                static const int POWS[10] = { 1,10,100,1000,10000,100000,1000000,10000000,100000000,1000000000 };
                value = value * f128((double)POWS[e]);
            }
        }
        else {
            e = -e;
            while (e >= 9) { value = value / base1e9; e -= 9; }
            if (e > 0) {
                static const int POWS[10] = { 1,10,100,1000,10000,100000,1000000,10000000,100000000,1000000000 };
                value = value / f128((double)POWS[e]);
            }
        }
    }

    if (neg) value = -value;

    out = value;
    if (endptr) *endptr = p;
    return true;
}
FORCE_INLINE f128 from_string(const char* s)
{
    f128 ret;
    if (parse_flt128(s, ret))
        return ret;
    return 0;
}

FORCE_INLINE std::ostream& operator<<(std::ostream& os, const f128& x)
{
    int prec = (int)os.precision();
    if (prec <= 0) prec = 6;

    const auto f = os.flags();
    const bool fixed = (f & std::ios_base::fixed) != 0;
    const bool scientific = (f & std::ios_base::scientific) != 0;
    const bool showpoint = (f & std::ios_base::showpoint) != 0;

    std::string s;

    if (fixed && !scientific) {
        emit_fixed_dec(s, x, prec, !showpoint);
        os << s;
        return os;
    }

    if (scientific && !fixed) {
        emit_scientific(s, x, prec + 1, !showpoint);
        os << s;
        return os;
    }

    if (x.hi == 0.0 && x.lo == 0.0) { os << (showpoint ? "0.0" : "0"); return os; }

    f128 ax = (x.hi < 0.0) ? f128{-x.hi, -x.lo} : x;
    f128 m; int e10 = 0;
    normalize10(ax, m, e10);

    if (e10 >= -4 && e10 < prec) {
        const int frac = std::max(0, prec - 1 - e10);
        emit_fixed_dec(s, x, frac, !showpoint);
    }
    else {
        emit_scientific(s, x, prec, !showpoint);
    }
    os << s;

    return os;
}

//
//FORCE_INLINE constexpr f128 operator"" _f128(unsigned long long v) noexcept {
//    return f128(static_cast<uint64_t>(v));
//}
//
//FORCE_INLINE constexpr f128 operator"" _f128(long double v) noexcept {
//    return f128(static_cast<double>(v));
//}
//FORCE_INLINE constexpr f128 operator"" _f128(const char* s, std::size_t) {
//    return f128(s);
//}

} // end bl
