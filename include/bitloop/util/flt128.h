#pragma once
#include <cmath>
#include <limits>
#include <string>
#include <iomanip>
#include <sstream>
#include <type_traits>

#include <bitloop/platform/platform_macros.h>
#include <bitloop/core/debug.h>

namespace bl {

#if defined(__FMA__) || defined(__FMA4__) || defined(_MSC_VER) || defined(__clang__) || defined(__GNUC__)
#define FMA_AVAILABLE 
#endif

BL_PUSH_PRECISE
FAST_INLINE constexpr void two_sum_precise(double a, double b, double& s, double& e)
{
    s = a + b;
    double bv = s - a;
    e = (a - (s - bv)) + (b - bv);
}
BL_POP_PRECISE

#ifdef FMA_AVAILABLE

static FAST_INLINE double fma1(double a, double b, double c)
{
    return std::fma(a, b, c);
}

FAST_INLINE void two_prod_precise_fma(double a, double b, double& p, double& err)
{
    p = a * b;
    err = fma1(a, b, -p);
}

#endif

FAST_INLINE constexpr void two_prod_precise_dekker(double a, double b, double& p, double& err)
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
    FAST_INLINE f128& operator*=(f128 rhs) { *this = *this * rhs; return *this; }
    FAST_INLINE f128& operator/=(f128 rhs) { *this = *this / rhs; return *this; }
    #else
    FAST_INLINE constexpr f128& operator*=(f128 rhs) { *this = *this * rhs; return *this; }
    FAST_INLINE constexpr f128& operator/=(f128 rhs) { *this = *this / rhs; return *this; }
    #endif

    constexpr f128() noexcept = default;
    constexpr f128(double hi_, double lo_) noexcept : hi(hi_), lo(lo_) {}
    constexpr f128(float  x) noexcept : hi((double)x), lo(0.0) {}
    constexpr f128(double x) noexcept : hi(x), lo(0.0) {}

    //constexpr void renormalize()
    //{
    //    two_sum_precise(hi, lo, hi, lo);
    //}

    constexpr f128(int64_t  v) noexcept { *this = static_cast<int64_t>(v); }
    constexpr f128(uint64_t u) noexcept { *this = static_cast<uint64_t>(u); }
    constexpr f128(int32_t  v) noexcept : f128((int64_t)v) {}
    constexpr f128(uint32_t u) noexcept : f128((int64_t)u) {}

    //FAST_INLINE constexpr f128(int v) { f128(int64_t(v)); }

    FAST_INLINE constexpr f128& operator=(double x) noexcept {
        hi = x; lo = 0.0; return *this;
    }
    FAST_INLINE constexpr f128& operator=(float x) noexcept {
        hi = static_cast<double>(x); lo = 0.0; return *this;
    }

    FAST_INLINE constexpr f128& operator=(uint64_t u) noexcept;
    FAST_INLINE constexpr f128& operator=(int64_t v) noexcept;

    template<class T, std::enable_if_t<std::is_integral_v<T>&& std::is_signed_v<T> && (sizeof(T) < 8), int> = 0>
    FAST_INLINE constexpr f128& operator=(T v) noexcept {
        return (*this = static_cast<int64_t>(v));
    }
    template<class T, std::enable_if_t<std::is_integral_v<T>&& std::is_unsigned_v<T> && (sizeof(T) < 8), int> = 0>
    FAST_INLINE constexpr f128& operator=(T v) noexcept {
        return (*this = static_cast<uint64_t>(v));
    }

    FAST_INLINE constexpr f128& operator+=(f128 rhs) { *this = *this + rhs; return *this; }
    FAST_INLINE constexpr f128& operator-=(f128 rhs) { *this = *this - rhs; return *this; }


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
        static constexpr f128 infinity()       noexcept { return { bl_infinity<double>(), 0.0}; }
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

static constexpr double DD_PI = 3.141592653589793238462643383279502884;
static constexpr double DD_PI2 = 1.570796326794896619231321691639751442;

/// ======== Helpers ========

FAST_INLINE constexpr f128 renorm(double hi, double lo)
{
    double s, e;
    two_sum_precise(hi, lo, s, e);
    return { s, e };
}
FAST_INLINE constexpr f128 quick_two_sum(double a, double b)
{
    double s = a + b;
    double err = b - (s - a);
    return { s, err };
}
FAST_INLINE constexpr f128 recip(f128 b)
{
    f128 y = f128(1.0 / b.hi);
    f128 one = f128(1.0);
    f128 e = one - b * y;

    y += y * e;
    e = one - b * y;
    y += y * e;

    return y;
}

FAST_INLINE constexpr f128& f128::operator=(uint64_t u) noexcept {
    // same limb path you already use in f128(u)
    uint64_t hi32 = u >> 32, lo32 = u & 0xFFFFFFFFull;
    double a = static_cast<double>(hi32) * 4294967296.0; // 2^32
    double b = static_cast<double>(lo32);
    double s, e; two_sum_precise(a, b, s, e);
    f128 r = renorm(s, e);
    hi = r.hi; lo = r.lo; return *this;
}
FAST_INLINE constexpr f128& f128::operator=(int64_t v) noexcept {
    uint64_t u = (v < 0) ? uint64_t(0) - uint64_t(v) : uint64_t(v);
    f128 r; r = u;                       // reuse uint64_t path
    if (v < 0) { r.hi = -r.hi; r.lo = -r.lo; }
    hi = r.hi; lo = r.lo; return *this;
}

// ------------------ f128 <=> f128 ------------------

FAST_INLINE constexpr bool operator <(const f128& a, const f128& b) { return (a.hi < b.hi) || (a.hi == b.hi && a.lo < b.lo); }
FAST_INLINE constexpr bool operator >(const f128& a, const f128& b) { return b < a; }
FAST_INLINE constexpr bool operator<=(const f128& a, const f128& b) { return (a < b) || (a.hi == b.hi && a.lo == b.lo); }
FAST_INLINE constexpr bool operator>=(const f128& a, const f128& b) { return b <= a; }
FAST_INLINE constexpr bool operator==(const f128& a, const f128& b) { return a.hi == b.hi && a.lo == b.lo; }
FAST_INLINE constexpr bool operator!=(const f128& a, const f128& b) { return !(a == b); }

// ------------------ double <=> f128 ------------------

FAST_INLINE constexpr bool operator<(const f128& a, double b)  { return a < f128(b); }
FAST_INLINE constexpr bool operator<(double a, const f128& b)  { return f128(a) < b; }
FAST_INLINE constexpr bool operator>(const f128& a, double b)  { return b < a; }
FAST_INLINE constexpr bool operator>(double a, const f128& b)  { return b < a; }
FAST_INLINE constexpr bool operator<=(const f128& a, double b) { return !(b < a); }
FAST_INLINE constexpr bool operator<=(double a, const f128& b) { return !(b < a); }
FAST_INLINE constexpr bool operator>=(const f128& a, double b) { return !(a < b); }
FAST_INLINE constexpr bool operator>=(double a, const f128& b) { return !(a < b); }
FAST_INLINE constexpr bool operator==(const f128& a, double b) { return a == f128(b); }
FAST_INLINE constexpr bool operator==(double a, const f128& b) { return f128(a) == b; }
FAST_INLINE constexpr bool operator!=(const f128& a, double b) { return !(a == b); }
FAST_INLINE constexpr bool operator!=(double a, const f128& b) { return !(a == b); }

// ------------------ float <=> f128 ------------------

FAST_INLINE constexpr bool operator<(const f128& a, float b) { return a < f128(b); }
FAST_INLINE constexpr bool operator<(float a, const f128& b) { return f128(a) < b; }
FAST_INLINE constexpr bool operator>(const f128& a, float b) { return b < a; }
FAST_INLINE constexpr bool operator>(float a, const f128& b) { return b < a; }
FAST_INLINE constexpr bool operator<=(const f128& a, float b) { return !(b < a); }
FAST_INLINE constexpr bool operator<=(float a, const f128& b) { return !(b < a); }
FAST_INLINE constexpr bool operator>=(const f128& a, float b) { return !(a < b); }
FAST_INLINE constexpr bool operator>=(float a, const f128& b) { return !(a < b); }
FAST_INLINE constexpr bool operator==(const f128& a, float b) { return a == f128(b); }
FAST_INLINE constexpr bool operator==(float a, const f128& b) { return f128(a) == b; }
FAST_INLINE constexpr bool operator!=(const f128& a, float b) { return !(a == b); }
FAST_INLINE constexpr bool operator!=(float a, const f128& b) { return !(a == b); }

// --------------- ints <=> f128 ---------------

FAST_INLINE constexpr bool operator<(const f128& a, int32_t b) { return a < f128(b); }
FAST_INLINE constexpr bool operator<(int32_t a, const f128& b) { return f128(a) < b; }
FAST_INLINE constexpr bool operator>(const f128& a, int32_t b) { return b < a; }
FAST_INLINE constexpr bool operator>(int32_t a, const f128& b) { return b < a; }
FAST_INLINE constexpr bool operator<=(const f128& a, int32_t b) { return !(b < a); }
FAST_INLINE constexpr bool operator<=(int32_t a, const f128& b) { return !(b < a); }
FAST_INLINE constexpr bool operator>=(const f128& a, int32_t b) { return !(a < b); }
FAST_INLINE constexpr bool operator>=(int32_t a, const f128& b) { return !(a < b); }
FAST_INLINE constexpr bool operator==(const f128& a, int32_t b) { return a == f128(b); }
FAST_INLINE constexpr bool operator==(int32_t a, const f128& b) { return f128(a) == b; }
FAST_INLINE constexpr bool operator!=(const f128& a, int32_t b) { return !(a == b); }
FAST_INLINE constexpr bool operator!=(int32_t a, const f128& b) { return !(a == b); }

FAST_INLINE constexpr bool operator<(const f128& a, uint32_t b) { return a < f128(b); }
FAST_INLINE constexpr bool operator<(uint32_t a, const f128& b) { return f128(a) < b; }
FAST_INLINE constexpr bool operator>(const f128& a, uint32_t b) { return b < a; }
FAST_INLINE constexpr bool operator>(uint32_t a, const f128& b) { return b < a; }
FAST_INLINE constexpr bool operator<=(const f128& a, uint32_t b) { return !(b < a); }
FAST_INLINE constexpr bool operator<=(uint32_t a, const f128& b) { return !(b < a); }
FAST_INLINE constexpr bool operator>=(const f128& a, uint32_t b) { return !(a < b); }
FAST_INLINE constexpr bool operator>=(uint32_t a, const f128& b) { return !(a < b); }
FAST_INLINE constexpr bool operator==(const f128& a, uint32_t b) { return a == f128(b); }
FAST_INLINE constexpr bool operator==(uint32_t a, const f128& b) { return f128(a) == b; }
FAST_INLINE constexpr bool operator!=(const f128& a, uint32_t b) { return !(a == b); }
FAST_INLINE constexpr bool operator!=(uint32_t a, const f128& b) { return !(a == b); }

FAST_INLINE constexpr bool operator<(const f128& a, int64_t b) { return a < f128(b); }
FAST_INLINE constexpr bool operator<(int64_t a, const f128& b) { return f128(a) < b; }
FAST_INLINE constexpr bool operator>(const f128& a, int64_t b) { return b < a; }
FAST_INLINE constexpr bool operator>(int64_t a, const f128& b) { return b < a; }
FAST_INLINE constexpr bool operator<=(const f128& a, int64_t b) { return !(b < a); }
FAST_INLINE constexpr bool operator<=(int64_t a, const f128& b) { return !(b < a); }
FAST_INLINE constexpr bool operator>=(const f128& a, int64_t b) { return !(a < b); }
FAST_INLINE constexpr bool operator>=(int64_t a, const f128& b) { return !(a < b); }
FAST_INLINE constexpr bool operator==(const f128& a, int64_t b) { return a == f128(b); }
FAST_INLINE constexpr bool operator==(int64_t a, const f128& b) { return f128(a) == b; }
FAST_INLINE constexpr bool operator!=(const f128& a, int64_t b) { return !(a == b); }
FAST_INLINE constexpr bool operator!=(int64_t a, const f128& b) { return !(a == b); }

FAST_INLINE constexpr bool operator<(const f128& a, uint64_t b) { return a < f128(b); }
FAST_INLINE constexpr bool operator<(uint64_t a, const f128& b) { return f128(a) < b; }
FAST_INLINE constexpr bool operator>(const f128& a, uint64_t b) { return b < a; }
FAST_INLINE constexpr bool operator>(uint64_t a, const f128& b) { return b < a; }
FAST_INLINE constexpr bool operator<=(const f128& a, uint64_t b) { return !(b < a); }
FAST_INLINE constexpr bool operator<=(uint64_t a, const f128& b) { return !(b < a); }
FAST_INLINE constexpr bool operator>=(const f128& a, uint64_t b) { return !(a < b); }
FAST_INLINE constexpr bool operator>=(uint64_t a, const f128& b) { return !(a < b); }
FAST_INLINE constexpr bool operator==(const f128& a, uint64_t b) { return a == f128(b); }
FAST_INLINE constexpr bool operator==(uint64_t a, const f128& b) { return f128(a) == b; }
FAST_INLINE constexpr bool operator!=(const f128& a, uint64_t b) { return !(a == b); }
FAST_INLINE constexpr bool operator!=(uint64_t a, const f128& b) { return !(a == b); }


/// ======== Arithmetic operators ========
FAST_INLINE constexpr f128 operator+(const f128& a, const f128& b)
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
FAST_INLINE constexpr f128 operator-(const f128& a, const f128& b)
{
    return operator+(a, f128{ -b.hi, -b.lo });
}
#ifdef FMA_AVAILABLE
FAST_INLINE           f128 operator*(const f128& a, const f128& b) {
    double p, e;
    two_prod_precise_fma(a.hi, b.hi, p, e);
    e += a.hi * b.lo + a.lo * b.hi;
    return renorm(p, e);
}
FAST_INLINE           f128 operator/(const f128& a, const f128& b) { return a * recip(b); }
#else
FAST_INLINE constexpr f128 operator*(const f128& a, const f128& b) {
    double p, e;
    two_prod_precise_dekker(a.hi, b.hi, p, e);
    e += a.hi * b.lo + a.lo * b.hi;
    return renorm(p, e);
}
FAST_INLINE constexpr f128 operator/(const f128& a, const f128& b) { return a * recip(b); }
#endif

/// 'double' support
#ifdef FMA_AVAILABLE
FAST_INLINE           f128 operator*(const f128& a, double b) { return a * f128{ b }; }
FAST_INLINE           f128 operator*(double a, const f128& b) { return f128{ a } * b; }
FAST_INLINE           f128 operator/(const f128& a, double b) { return a / f128{ b }; }
FAST_INLINE           f128 operator/(double a, const f128& b) { return f128{ a } / b; }
#else
FAST_INLINE constexpr f128 operator*(const f128& a, double b) { return a / f128{ b }; }
FAST_INLINE constexpr f128 operator*(double a, const f128& b) { return f128{ a } / b; }
FAST_INLINE constexpr f128 operator/(const f128& a, double b) { return a / f128{ b }; }
FAST_INLINE constexpr f128 operator/(double a, const f128& b) { return f128{ a } / b; }
#endif
FAST_INLINE constexpr f128 operator+(const f128& a, double b) { return a + f128{ b }; }
FAST_INLINE constexpr f128 operator+(double a, const f128& b) { return f128{ a } + b; }
FAST_INLINE constexpr f128 operator-(const f128& a, double b) { return a - f128{ b }; }
FAST_INLINE constexpr f128 operator-(double a, const f128& b) { return f128{ a } - b; }

FAST_INLINE bool isnan(const f128& x) { return std::isnan(x.hi); }
FAST_INLINE bool isinf(const f128& x) { return std::isinf(x.hi); }
FAST_INLINE bool isfinite(const f128& x) { return std::isfinite(x.hi); }
FAST_INLINE bool iszero(const f128& x) { return x.hi == 0.0 && x.lo == 0.0; }

FAST_INLINE double log_as_double(float a)
{
    return std::log(a);
}
FAST_INLINE double log_as_double(double a)
{
    return std::log(a);
}
FAST_INLINE double log_as_double(f128 a)
{
    return std::log(a.hi) + std::log1p(a.lo / a.hi);
}
FAST_INLINE f128 log(const f128& a)
{
    double log_hi = std::log(a.hi); // first guess
    f128 exp_log_hi(std::exp(log_hi)); // exp(guess)
    f128 r = (a - exp_log_hi) / exp_log_hi; // (a - e^g) / e^g ≈ error
    return f128(log_hi) + r; // refined log
}
FAST_INLINE f128 log2(const f128& a)
{
    constexpr f128 LOG2_RECIPROCAL(1.442695040888963407359924681001892137); // 1/log(2)
    return log(a) * LOG2_RECIPROCAL;
}
FAST_INLINE f128 log10(const f128& x)
{
    // 1 / ln(10) to full-double accuracy
    static constexpr double INV_LN10 = 0.434294481903251827651128918916605082;
    return log(x) * f128(INV_LN10);   // log(x) already returns float128
}
FAST_INLINE f128 clamp(const f128& v, const f128& lo, const f128& hi)
{
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}
FAST_INLINE f128 abs(const f128& a)
{
    return (a.hi < 0.0) ? -a : a;
}
FAST_INLINE f128 fabs(const f128& a) 
{
    return (a.hi < 0.0) ? -a : a;
}
FAST_INLINE f128 floor(const f128& a)
{
    double f = std::floor(a.hi);
    /*     hi already integral ?  trailing part < 0  →  round one below   */
    if (f == a.hi && a.lo < 0.0) f -= 1.0;
    return { f, 0.0 };
}
FAST_INLINE f128 ceil(const f128& a)
{
    double c = std::ceil(a.hi);
    if (c == a.hi && a.lo > 0.0) c += 1.0;
    return { c, 0.0 };
}
FAST_INLINE f128 trunc(const f128& a)
{
    return (a.hi < 0.0) ? ceil(a) : floor(a);
}
FAST_INLINE f128 round(const f128& a)
{
    f128 t = floor(a + f128(0.5));
    /* halfway cases: round to even like std::round */
    if (((t.hi - a.hi) == 0.5) && (std::fmod(t.hi, 2.0) != 0.0)) t -= f128(1.0);
    return t;
}
FAST_INLINE f128 fmod(const f128& x, const f128& y)
{
    if (y.hi == 0.0 && y.lo == 0.0)
        return std::numeric_limits<f128>::quiet_NaN();
    return x - trunc(x / y) * y;
}
FAST_INLINE f128 remainder(const f128& x, const f128& y)
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

#ifdef FMA_AVAILABLE
FAST_INLINE f128 sqrt(f128 a)
{
    double approx = std::sqrt(a.hi);
    f128 y{ approx };
    f128 y_sq = y * y;
    f128 r = a - y_sq;
    f128 half{ 0.5 };
    // y = y + r*(0.5/y)
    double t = 0.5 / y.hi;
    // fold hi updates with fma; lo correction comes via renorm in ops
    y.hi = fma1(r.hi, t, y.hi);
    // keep the original structure for lo through normal ops
    return y;
}
FAST_INLINE f128 sin(f128 a)
{
    double s = std::sin(a.hi);
    double c = std::cos(a.hi);
    // s + c * a.lo with single rounding in the leading component
    double hi = fma1(c, a.lo, s);
    return f128(hi); // trailing error is small; DD normalize occurs in ops
}
FAST_INLINE f128 cos(f128 a)
{
    double c = std::cos(a.hi);
    double s = std::sin(a.hi);
    double hi = fma1(-s, a.lo, c); // c - s * a.lo
    return f128(hi);
}
FAST_INLINE f128 exp(const f128& a)
{
    double yh = std::exp(a.hi);
    double hi = fma1(yh, a.lo, yh); // yh*(1 + a.lo)
    return f128{ hi };
}
FAST_INLINE f128 atan2(const f128& y, const f128& x)
{
    if (x.hi == 0.0 && x.lo == 0.0) {
        if (y.hi == 0.0 && y.lo == 0.0) return f128(std::numeric_limits<double>::quiet_NaN());
        return (y.hi > 0.0 || (y.hi == 0.0 && y.lo > 0.0)) ? f128(DD_PI2) : f128(-DD_PI2);
    }
    f128 v{ std::atan2(y.hi, x.hi) };
    f128 sv = sin(v);
    f128 cv = cos(v);

    double f_hi = fma1(x.hi, (double)sv, -(y.hi * (double)cv)); // f = x*sv - y*cv
    double fp_hi = fma1(x.hi, (double)cv, (y.hi * (double)sv)); // fp = x*cv + y*sv

    f128 f{ f_hi }, fp{ fp_hi };
    v = v - f / fp;
    return v;
}
#else
FAST_INLINE f128 sqrt(f128 a)
{
    double approx = std::sqrt(a.hi);
    f128 y{ approx };
    f128 y_sq = y * y;
    f128 r = a - y_sq;
    f128 half{ 0.5 };
    y += r * (half / y);
    return y;
}
FAST_INLINE f128 sin(f128 a)
{
    double s = std::sin(a.hi);
    double c = std::cos(a.hi);
    return f128{ s } + f128{ c } * a.lo;
}
FAST_INLINE f128 cos(f128 a)
{
    double c = std::cos(a.hi);
    double s = std::sin(a.hi);
    return f128{ c } - f128{ s } * a.lo;
}
FAST_INLINE f128 exp(const f128& a)
{
    //  exp(a.hi + a.lo) ≈ exp(a.hi) * (1 + a.lo)   (|a.lo| < 1 ulp) 
    f128 y(std::exp(a.hi));
    return y * (f128(1.0) + f128(a.lo));
}
FAST_INLINE f128 atan2(const f128& y, const f128& x)
{
    /*  Special cases first (match IEEE / std::atan2)  */
    if (x.hi == 0.0 && x.lo == 0.0) {
        if (y.hi == 0.0 && y.lo == 0.0)
            return f128(std::numeric_limits<double>::quiet_NaN());   // undefined
        return (y.hi > 0.0 || (y.hi == 0.0 && y.lo > 0.0))
            ? f128(DD_PI2) : f128(-DD_PI2);
    }

    /* 1) bootstrap with the regular double result (gives correct quadrant) */
    f128 v(std::atan2(y.hi, x.hi));

    /* 2) one Newton step on  f(v)=x·sin v − y·cos v  */
    f128 sv = sin(v);
    f128 cv = cos(v);
    f128  f = x * sv - y * cv;          // residual
    f128 fp = x * cv + y * sv;          // f'
    v = v - f / fp;                         // refined solution

    return v;
}
#endif

FAST_INLINE f128 tan(const f128& a) { return sin(a) / cos(a); }
FAST_INLINE f128 pow(const f128& x, const f128& y) { return exp(y * log(x)); }
FAST_INLINE f128 atan(const f128& x) { return atan2(x, 1); }


/*------------ precise DP rounding -------------------------------------------*/

FAST_INLINE f128 round_to_decimals(f128 v, int prec)
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

FAST_INLINE f128 pow10_128(int k)
{
    f128 r = 1, ten = 10;
    int n = (k >= 0) ? k : -k;
    for (int i = 0; i < n; ++i) r = r * ten;
    return (k >= 0) ? r : (1 / r);
}
FAST_INLINE void normalize10(const f128& x, f128& m, int& exp10)
{
    // x = m * 10^exp10 with m in [1,10)   (seed from binary exponent, then correct)
    if (x.hi == 0.0) { m = 0; exp10 = 0; return; }

    f128 ax = abs(x);

    int e2 = 0;
    (void)std::frexp(ax.hi, &e2);             // ax.hi = f * 2^(e2-1)
    int e10 = (int)std::floor((e2 - 1) * 0.30102999566398114); // ≈ log10(2)

    m = ax * pow10_128(-e10);
    while (m >= 10) { m = m / 10; ++e10; }
    while (m < 1) { m = m * 10; --e10; }
    exp10 = e10;
}

FAST_INLINE f128 round_scaled(bl::f128 x, int prec) noexcept {
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
FAST_INLINE void emit_uint_rev(std::string& out, f128 n) {
    /// emits integer 'n' (n >= 0) in base-10 into buffer (reversed)

    // n is a non-negative integer in f128
    const f128 base = f128(1000000000.0); // 1e9

    // Fast path for small values
    if (n < 10) {
        int d = (int)floor(n).hi;
        if (d < 0) d = 0; if (d > 9) d = 9;      // safety clamp
        out.push_back(char('0' + d));
        return;
    }

    // Extract 9-digit chunks in base 1e9
    while (n >= base) {
        bl::f128 q = floor(n / base);
        bl::f128 r = n - q * base;            // should be in [0, 1e9)

        // Convert r to an integer chunk
        long long chunk = (long long)floor(r).hi;

        // Correct occasional rounding to 1e9
        if (chunk >= 1000000000LL) {
            chunk -= 1000000000LL;
            q = q + 1;
        }
        if (chunk < 0) chunk = 0;               // guard against tiny negative drift

        // Emit this chunk as exactly 9 digits, reversed
        for (int i = 0; i < 9; ++i) {
            int d = int(chunk % 10);
            out.push_back(char('0' + d));
            chunk /= 10;
        }

        n = q;
    }

    // Final (most significant) chunk — no zero padding here
    long long last = (long long)floor(n).hi;
    if (last == 0) {
        out.push_back('0');
    }
    else {
        while (last > 0) {
            int d = int(last % 10);
            out.push_back(char('0' + d));
            last /= 10;
        }
    }
}

FAST_INLINE void emit_scientific(std::ostream& os, const f128& x, std::streamsize prec, bool strip_trailing_zeros)
{
    if (x.hi == 0.0) { os << "0"; return; }
    if (prec < 1) prec = 1; // total significant digits

    bool neg = x.hi < 0.0;
    f128 v = neg ? f128(-x.hi, -x.lo) : x;
    f128 m; int e = 0;
    normalize10(v, m, e);

    const f128 ten = 10;

    // Collect digits: d[0] is the first significant digit, then fractional digits.
    // We compute prec digits + 1 lookahead digit for rounding.
    const int sig = (int)prec;
    unsigned char dbuf[128]; // supports quite large 'prec'
    const int max_sig = (int)std::min<int>(sig + 1, (int)(sizeof(dbuf)));

    f128 t = m;
    for (int i = 0; i < max_sig; ++i) {
        int di = (int)floor(t).hi;
        if (di < 0) di = 0; else if (di > 9) di = 9;
        dbuf[i] = (unsigned char)di;
        t = (t - f128(di)) * ten;
    }

    // Round using the lookahead digit if we have it
    bool carry = (max_sig >= sig + 1) && (dbuf[sig] >= 5);

    // Propagate carry across the first 'sig' digits (right to left)
    for (int i = sig - 1; i >= 0 && carry; --i) {
        int vdig = (int)dbuf[i] + 1;
        if (vdig == 10) {
            dbuf[i] = 0;
            carry = true;
        }
        else {
            dbuf[i] = (unsigned char)vdig;
            carry = false;
        }
    }

    // 9.999… → 10.000…
    if (carry) {
        for (int i = sig - 1; i >= 1; --i) dbuf[i] = 0;
        dbuf[0] = 1;
        ++e;
    }

    if (neg) os.put('-');

    // First digit
    os.put(char('0' + dbuf[0]));

    // Fractional digits (with optional stripping)
    if (sig > 1) {
        int last = sig - 1;
        if (strip_trailing_zeros) {
            while (last >= 1 && dbuf[last] == 0) --last; // find last non-zero fractional digit
        }
        if (last >= 1) {
            os.put('.');
            for (int i = 1; i <= last; ++i)
                os.put(char('0' + dbuf[i]));
        }
        // else: all fractional digits were zero → omit decimal point entirely
    }

    os.put('e');
    os << e;
}
FAST_INLINE void emit_fixed_dec(std::ostream& os, f128 x, int prec, bool strip_trailing_zeros)
{
    bool neg = (x.hi < 0.0);
    if (neg) x = f128{ -x.hi, -x.lo };

    // Split into integer and fractional parts without large scaling
    f128 ip = floor(x);
    f128 fp = x - ip;

    // ---- emit integer part ----
    std::string rev; rev.reserve(64);
    emit_uint_rev(rev, ip);
    std::string out; out.reserve(rev.size() + 2 + (prec > 0 ? prec : 0));
    if (neg) out.push_back('-');
    for (int i = int(rev.size()) - 1; i >= 0; --i) out.push_back(rev[i]);

    if (prec > 0)
    {
        out.push_back('.');

        // Generate 'prec' digits with one extra look-ahead for rounding
        const f128 ten{ 10.0 };
        const f128 half{ 0.5 };

        // Collect digits into a small buffer
        std::string digits; digits.resize(size_t(prec), '0');

        f128 frac = fp;
        for (int i = 0; i < prec; ++i) {
            frac = frac * ten;                // stays in [0,10)
            int di = (int)floor(frac).hi;     // safe 0..9
            if (di < 0) di = 0; if (di > 9) di = 9;
            digits[size_t(i)] = char('0' + di);
            frac = frac - f128(di);
        }

        // Look-ahead digit to decide rounding
        f128 nextv = frac * ten;        // in [0,10)
        int next = (int)floor(nextv).hi;      // 0..9
        if (next < 0) next = 0; if (next > 9) next = 9;
        f128 rem = nextv - f128(next); // remainder after next digit

        // Round-half-even on the last printed digit
        bool round_up = false;
        if (next > 5) round_up = true;
        else if (next < 5) round_up = false;
        else { // next == 5
            if (rem.hi > 0.0 || rem.lo > 0.0) {
                round_up = true;              // strictly greater than half
            }
            else {
                // exact tie: round to even (last printed digit even stays)
                int last_digit = digits.empty() ? 0 : (digits.back() - '0');
                round_up = (last_digit % 2 != 0);
            }
        }

        if (round_up) {
            // propagate carry across fractional digits, then into integer if needed
            int i = prec - 1;
            for (; i >= 0; --i) {
                if (digits[size_t(i)] == '9') {
                    digits[size_t(i)] = '0';
                }
                else {
                    digits[size_t(i)]++;
                    break;
                }
            }
            if (i < 0) {
                // carry into integer part, increment integer part 'out' (string) from the right, skipping sign
                int j = int(out.size()) - 1; // currently points at '.'
                // move to last integer digit
                while (j >= 0 && out[size_t(j)] != '.') --j;
                int k = j - 1; // index of last integer digit
                for (; k >= (neg ? 1 : 0); --k) {
                    char c = out[size_t(k)];
                    if (c == '-') break;
                    if (c == '9') out[size_t(k)] = '0';
                    else { out[size_t(k)] = char(c + 1); break; }
                }
                if (k < (neg ? 1 : 0)) {
                    // need to insert '1' at the beginning (after '-')
                    if (neg) out.insert(out.begin() + 1, '1');
                    else     out.insert(out.begin(), '1');
                    ++j; // decimal point index shifted right by one
                }
            }
        }

        // Append (and optionally strip) fractional digits
        if (strip_trailing_zeros) {
            int end = prec - 1;
            while (end >= 0 && digits[size_t(end)] == '0') --end;
            if (end >= 0) {
                out.append(digits.data(), size_t(end + 1));
            }
            else {
                // all zeros: drop the dot
                out.pop_back();
            }
        }
        else {
            out.append(digits);
        }
    }

    if (out == "-0") out = "0";
    os << out;
}

FAST_INLINE std::string to_string(const f128& x, int precision,
    bool fixed = false, bool scientific = false,
    bool strip_trailing_zeros = false)
{
    if (precision < 0) precision = 0;

    std::stringstream ss;

    if (fixed && !scientific) {
        emit_fixed_dec(ss, x, precision, strip_trailing_zeros);
        return ss.str();
    }

    if (scientific && !fixed) {
        emit_scientific(ss, x, precision, strip_trailing_zeros);
        return ss.str();
    }

    // default: choose fixed vs scientific by exponent
    if (x.hi == 0.0) { ss << "0"; return ss.str(); }

    f128 ax = (x.hi < 0.0) ? f128(-x.hi, -x.lo) : x;
    f128 m; int e10 = 0;
    normalize10(ax, m, e10);

    if (e10 >= -4 && e10 < precision) {
        int frac = (e10 >= 0)
            ? precision
            : std::max(0, precision - 1 - e10);
        emit_fixed_dec(ss, x, frac, strip_trailing_zeros);
    }
    else {
        emit_scientific(ss, x, precision, strip_trailing_zeros);
    }

    return ss.str();
}

FAST_INLINE bool   valid_flt128_string(const char* s)
{
    for (char* p = (char*)s; *p; ++p) {
        if (!(isdigit(*p) || *p == '.' || *p == 'e' || *p == 'E' || *p=='-'))
            return false;
    }
    return true;
}
FAST_INLINE bool   parse_flt128(const char* s, f128& out, const char** endptr = nullptr)
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
        out = neg ? -std::numeric_limits<f128>::infinity() : std::numeric_limits<f128>::infinity();
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
FAST_INLINE f128 from_string(const char* s)
{
    f128 ret;
    if (parse_flt128(s, ret))
        return ret;
    return 0;
}

//FAST_INLINE constexpr f128 f128(const char *s) { return from_string(s); }

FAST_INLINE std::ostream& operator<<(std::ostream& os, const f128& x)
{
    int prec = (int)os.precision();
    if (prec <= 0) prec = 6;

    const auto f = os.flags();
    const bool fixed = (f & std::ios_base::fixed) != 0;
    const bool scientific = (f & std::ios_base::scientific) != 0;
    const bool showpoint = (f & std::ios_base::showpoint) != 0;

    if (fixed && !scientific) {
        emit_fixed_dec(os, x, prec, !showpoint);
        return os;
    }

    if (scientific && !fixed) {
        emit_scientific(os, x, prec + 1, !showpoint);
        return os;
    }

    if (x.hi == 0.0) { os << (showpoint ? "0.0" : "0"); return os; }

    f128 ax = (x.hi < 0.0) ? f128{-x.hi, -x.lo} : x;
    f128 m; int e10 = 0;
    normalize10(ax, m, e10);

    if (e10 >= -4 && e10 < prec) {
        const int frac = std::max(0, prec - 1 - e10);
        emit_fixed_dec(os, x, frac, !showpoint);
    }
    else {
        emit_scientific(os, x, prec, !showpoint);
    }
    return os;
}

//
//FAST_INLINE constexpr f128 operator"" _f128(unsigned long long v) noexcept {
//    return f128(static_cast<uint64_t>(v));
//}
//
//FAST_INLINE constexpr f128 operator"" _f128(long double v) noexcept {
//    return f128(static_cast<double>(v));
//}
//FAST_INLINE constexpr f128 operator"" _f128(const char* s, std::size_t) {
//    return f128(s);
//}

} // end bl
