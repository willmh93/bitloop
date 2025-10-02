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


struct flt128;

constexpr flt128 operator+(const flt128& a, const flt128& b);
constexpr flt128 operator-(const flt128& a, const flt128& b);
#ifdef FMA_AVAILABLE
flt128 operator*(const flt128& a, const flt128& b);
flt128 operator/(const flt128& a, const flt128& b);
#else
constexpr flt128 operator*(const flt128& a, const flt128& b);
constexpr flt128 operator/(const flt128& a, const flt128& b);
#endif

struct flt128
{
    double hi; // leading component
    double lo; // trailing error
    
    #ifdef FMA_AVAILABLE
    /// FMA versions (not constexpr)
    FAST_INLINE flt128& operator*=(flt128 rhs) { *this = *this * rhs; return *this; }
    FAST_INLINE flt128& operator/=(flt128 rhs) { *this = *this / rhs; return *this; }
    #else
    FAST_INLINE constexpr flt128& operator*=(flt128 rhs) { *this = *this * rhs; return *this; }
    FAST_INLINE constexpr flt128& operator/=(flt128 rhs) { *this = *this / rhs; return *this; }
    #endif


    FAST_INLINE constexpr flt128& operator=(double x) noexcept {
        hi = x; lo = 0.0; return *this;
    }
    FAST_INLINE constexpr flt128& operator=(float x) noexcept {
        hi = static_cast<double>(x); lo = 0.0; return *this;
    }

    FAST_INLINE constexpr flt128& operator=(uint64_t u) noexcept;
    FAST_INLINE constexpr flt128& operator=(int64_t v) noexcept;

    template<class T, std::enable_if_t<std::is_integral_v<T>&& std::is_signed_v<T> && (sizeof(T) < 8), int> = 0>
    FAST_INLINE constexpr flt128& operator=(T v) noexcept {
        return (*this = static_cast<int64_t>(v));
    }
    template<class T, std::enable_if_t<std::is_integral_v<T>&& std::is_unsigned_v<T> && (sizeof(T) < 8), int> = 0>
    FAST_INLINE constexpr flt128& operator=(T v) noexcept {
        return (*this = static_cast<uint64_t>(v));
    }

    FAST_INLINE constexpr flt128& operator+=(flt128 rhs) { *this = *this + rhs; return *this; }
    FAST_INLINE constexpr flt128& operator-=(flt128 rhs) { *this = *this - rhs; return *this; }


    /// ======== Conversions ========
    explicit constexpr operator double() const { return hi + lo; }
    explicit constexpr operator float() const { return static_cast<float>(hi + lo); }
    explicit constexpr operator int() const { return static_cast<int>(hi + lo); }

    constexpr flt128 operator+() const { return *this; }
    constexpr flt128 operator-() const noexcept { return flt128{ -hi, -lo }; }

    /// ======== Utility ========
    static constexpr flt128 eps()
    {
        return { std::numeric_limits<double>::epsilon(), 0.0 };
    }
};

} // end bl

namespace std
{
    using bl::flt128;

    template<> struct numeric_limits<flt128>
    {
        static constexpr bool is_specialized = true;

        // limits
        static constexpr flt128 min()            noexcept { return {  numeric_limits<double>::min(), 0.0 }; }
        static constexpr flt128 max()            noexcept { return {  numeric_limits<double>::max(), -numeric_limits<double>::epsilon() }; }
        static constexpr flt128 lowest()         noexcept { return { -numeric_limits<double>::max(),  numeric_limits<double>::epsilon() }; }
        static constexpr flt128 highest()        noexcept { return {  numeric_limits<double>::max(), -numeric_limits<double>::epsilon() }; }
                                                 
		// special values                        
        static constexpr flt128 epsilon()        noexcept { return { 1.232595164407831e-32, 0.0 }; } // ~2^-106, a single ulp of double-double
        static constexpr flt128 round_error()    noexcept { return { 0.5, 0.0 }; }
        static constexpr flt128 infinity()       noexcept { return { bl_infinity<double>(), 0.0}; }
        static constexpr flt128 quiet_NaN()      noexcept { return { numeric_limits<double>::quiet_NaN(), 0.0 }; }
        static constexpr flt128 signaling_NaN()  noexcept { return { numeric_limits<double>::signaling_NaN(), 0.0 }; }
        static constexpr flt128 denorm_min()     noexcept { return { numeric_limits<double>::denorm_min(), 0.0 }; }
                                                 
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

FAST_INLINE constexpr flt128 renorm(double hi, double lo)
{
    double s, e;
    two_sum_precise(hi, lo, s, e);
    return { s, e };
}
FAST_INLINE constexpr flt128 quick_two_sum(double a, double b)
{
    double s = a + b;
    double err = b - (s - a);
    return { s, err };
}
FAST_INLINE constexpr flt128 recip(flt128 b)
{
    flt128 y = flt128(1.0 / b.hi);
    flt128 one = flt128(1.0);
    flt128 e = one - b * y;

    y += y * e;
    e = one - b * y;
    y += y * e;

    return y;
}

FAST_INLINE constexpr flt128& flt128::operator=(uint64_t u) noexcept {
    // same limb path you already use in f128(u)
    uint64_t hi32 = u >> 32, lo32 = u & 0xFFFFFFFFull;
    double a = static_cast<double>(hi32) * 4294967296.0; // 2^32
    double b = static_cast<double>(lo32);
    double s, e; two_sum_precise(a, b, s, e);
    flt128 r = renorm(s, e);
    hi = r.hi; lo = r.lo; return *this;
}
FAST_INLINE constexpr flt128& flt128::operator=(int64_t v) noexcept {
    uint64_t u = (v < 0) ? uint64_t(0) - uint64_t(v) : uint64_t(v);
    flt128 r; r = u;                       // reuse uint64_t path
    if (v < 0) { r.hi = -r.hi; r.lo = -r.lo; }
    hi = r.hi; lo = r.lo; return *this;
}

// ------------------ flt128 constructors (outside flt128 to keep trivially constructible) ------------------

FAST_INLINE constexpr flt128 f128(double x) noexcept { return { x, 0.0 }; }
FAST_INLINE constexpr flt128 f128(float x) { return flt128{ static_cast<double>(x) }; }
FAST_INLINE constexpr flt128 f128(uint64_t u) {
    // u = (hi<<32) + lo
    uint64_t hi32 = u >> 32;
    uint64_t lo32 = u & 0xFFFFFFFFull;

    double a = static_cast<double>(hi32) * 4294967296.0; // 2^32
    double b = static_cast<double>(lo32);

    double s, e;
    two_sum_precise(a, b, s, e);
    return renorm(s, e);
}
FAST_INLINE constexpr flt128 f128(int64_t v) {
    uint64_t u = v < 0 ? uint64_t(0) - uint64_t(v) : uint64_t(v);
    flt128 r = f128(u);
    return (v < 0) ? flt128{ -r.hi, -r.lo } : r;
}
FAST_INLINE constexpr flt128 f128(int v) { return f128(int64_t(v)); }

// ------------------ flt128 <=> flt128 ------------------

FAST_INLINE constexpr bool operator <(const flt128& a, const flt128& b) { return (a.hi < b.hi) || (a.hi == b.hi && a.lo < b.lo); }
FAST_INLINE constexpr bool operator >(const flt128& a, const flt128& b) { return b < a; }
FAST_INLINE constexpr bool operator<=(const flt128& a, const flt128& b) { return (a < b) || (a.hi == b.hi && a.lo == b.lo); }
FAST_INLINE constexpr bool operator>=(const flt128& a, const flt128& b) { return b <= a; }
FAST_INLINE constexpr bool operator==(const flt128& a, const flt128& b) { return a.hi == b.hi && a.lo == b.lo; }
FAST_INLINE constexpr bool operator!=(const flt128& a, const flt128& b) { return !(a == b); }

// ------------------ double <=> flt128 ------------------

FAST_INLINE constexpr bool operator<(const flt128& a, double b) { return a < f128(b); }
FAST_INLINE constexpr bool operator<(double a, const flt128& b) { return f128(a) < b; }
FAST_INLINE constexpr bool operator>(const flt128& a, double b) { return b < a; }
FAST_INLINE constexpr bool operator>(double a, const flt128& b) { return b < a; }
FAST_INLINE constexpr bool operator<=(const flt128& a, double b) { return !(b < a); }
FAST_INLINE constexpr bool operator<=(double a, const flt128& b) { return !(b < a); }
FAST_INLINE constexpr bool operator>=(const flt128& a, double b) { return !(a < b); }
FAST_INLINE constexpr bool operator>=(double a, const flt128& b) { return !(a < b); }
FAST_INLINE constexpr bool operator==(const flt128& a, double b) { return a == f128(b); }
FAST_INLINE constexpr bool operator==(double a, const flt128& b) { return f128(a) == b; }
FAST_INLINE constexpr bool operator!=(const flt128& a, double b) { return !(a == b); }
FAST_INLINE constexpr bool operator!=(double a, const flt128& b) { return !(a == b); }

// ------------------ float <=> flt128 ------------------

FAST_INLINE constexpr bool operator<(const flt128& a, float b) { return a < f128(b); }
FAST_INLINE constexpr bool operator<(float a, const flt128& b) { return f128(a) < b; }
FAST_INLINE constexpr bool operator>(const flt128& a, float b) { return b < a; }
FAST_INLINE constexpr bool operator>(float a, const flt128& b) { return b < a; }
FAST_INLINE constexpr bool operator<=(const flt128& a, float b) { return !(b < a); }
FAST_INLINE constexpr bool operator<=(float a, const flt128& b) { return !(b < a); }
FAST_INLINE constexpr bool operator>=(const flt128& a, float b) { return !(a < b); }
FAST_INLINE constexpr bool operator>=(float a, const flt128& b) { return !(a < b); }
FAST_INLINE constexpr bool operator==(const flt128& a, float b) { return a == f128(b); }
FAST_INLINE constexpr bool operator==(float a, const flt128& b) { return f128(a) == b; }
FAST_INLINE constexpr bool operator!=(const flt128& a, float b) { return !(a == b); }
FAST_INLINE constexpr bool operator!=(float a, const flt128& b) { return !(a == b); }

// --------------- int64_t / uint64_t <=> flt128 ---------------

FAST_INLINE constexpr bool operator<(const flt128& a, int64_t b) { return a < f128(b); }
FAST_INLINE constexpr bool operator<(int64_t a, const flt128& b) { return f128(a) < b; }
FAST_INLINE constexpr bool operator>(const flt128& a, int64_t b) { return b < a; }
FAST_INLINE constexpr bool operator>(int64_t a, const flt128& b) { return b < a; }
FAST_INLINE constexpr bool operator<=(const flt128& a, int64_t b) { return !(b < a); }
FAST_INLINE constexpr bool operator<=(int64_t a, const flt128& b) { return !(b < a); }
FAST_INLINE constexpr bool operator>=(const flt128& a, int64_t b) { return !(a < b); }
FAST_INLINE constexpr bool operator>=(int64_t a, const flt128& b) { return !(a < b); }
FAST_INLINE constexpr bool operator==(const flt128& a, int64_t b) { return a == f128(b); }
FAST_INLINE constexpr bool operator==(int64_t a, const flt128& b) { return f128(a) == b; }
FAST_INLINE constexpr bool operator!=(const flt128& a, int64_t b) { return !(a == b); }
FAST_INLINE constexpr bool operator!=(int64_t a, const flt128& b) { return !(a == b); }
FAST_INLINE constexpr bool operator<(const flt128& a, uint64_t b) { return a < f128(b); }
FAST_INLINE constexpr bool operator<(uint64_t a, const flt128& b) { return f128(a) < b; }
FAST_INLINE constexpr bool operator>(const flt128& a, uint64_t b) { return b < a; }
FAST_INLINE constexpr bool operator>(uint64_t a, const flt128& b) { return b < a; }
FAST_INLINE constexpr bool operator<=(const flt128& a, uint64_t b) { return !(b < a); }
FAST_INLINE constexpr bool operator<=(uint64_t a, const flt128& b) { return !(b < a); }
FAST_INLINE constexpr bool operator>=(const flt128& a, uint64_t b) { return !(a < b); }
FAST_INLINE constexpr bool operator>=(uint64_t a, const flt128& b) { return !(a < b); }
FAST_INLINE constexpr bool operator==(const flt128& a, uint64_t b) { return a == f128(b); }
FAST_INLINE constexpr bool operator==(uint64_t a, const flt128& b) { return f128(a) == b; }
FAST_INLINE constexpr bool operator!=(const flt128& a, uint64_t b) { return !(a == b); }
FAST_INLINE constexpr bool operator!=(uint64_t a, const flt128& b) { return !(a == b); }


/// ======== Arithmetic operators ========
FAST_INLINE constexpr flt128 operator+(const flt128& a, const flt128& b)
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
FAST_INLINE constexpr flt128 operator-(const flt128& a, const flt128& b)
{
    return operator+(a, flt128{ -b.hi, -b.lo });
}
#ifdef FMA_AVAILABLE
FAST_INLINE           flt128 operator*(const flt128& a, const flt128& b) {
    double p, e;
    two_prod_precise_fma(a.hi, b.hi, p, e);
    e += a.hi * b.lo + a.lo * b.hi;
    return renorm(p, e);
}
FAST_INLINE           flt128 operator/(const flt128& a, const flt128& b) { return a * recip(b); }
#else
FAST_INLINE constexpr flt128 operator*(const flt128& a, const flt128& b) {
    double p, e;
    two_prod_precise_dekker(a.hi, b.hi, p, e);
    e += a.hi * b.lo + a.lo * b.hi;
    return renorm(p, e);
}
FAST_INLINE constexpr flt128 operator/(const flt128& a, const flt128& b) { return a * recip(b); }
#endif

/// 'double' support
#ifdef FMA_AVAILABLE
FAST_INLINE           flt128 operator*(const flt128& a, double b) { return a * flt128{ b }; }
FAST_INLINE           flt128 operator*(double a, const flt128& b) { return flt128{ a } * b; }
FAST_INLINE           flt128 operator/(const flt128& a, double b) { return a / flt128{ b }; }
FAST_INLINE           flt128 operator/(double a, const flt128& b) { return flt128{ a } / b; }
#else
FAST_INLINE constexpr flt128 operator*(const flt128& a, double b) { return a / flt128{ b }; }
FAST_INLINE constexpr flt128 operator*(double a, const flt128& b) { return flt128{ a } / b; }
FAST_INLINE constexpr flt128 operator/(const flt128& a, double b) { return a / flt128{ b }; }
FAST_INLINE constexpr flt128 operator/(double a, const flt128& b) { return flt128{ a } / b; }
#endif
FAST_INLINE constexpr flt128 operator+(const flt128& a, double b) { return a + flt128{ b }; }
FAST_INLINE constexpr flt128 operator+(double a, const flt128& b) { return flt128{ a } + b; }
FAST_INLINE constexpr flt128 operator-(const flt128& a, double b) { return a - flt128{ b }; }
FAST_INLINE constexpr flt128 operator-(double a, const flt128& b) { return flt128{ a } - b; }

FAST_INLINE bool isnan(const flt128& x) { return std::isnan(x.hi); }
FAST_INLINE bool isinf(const flt128& x) { return std::isinf(x.hi); }
FAST_INLINE bool iszero(const flt128& x) { return x.hi == 0.0 && x.lo == 0.0; }

FAST_INLINE double log_as_double(float a)
{
    return std::log(a);
}
FAST_INLINE double log_as_double(double a)
{
    return std::log(a);
}
FAST_INLINE double log_as_double(flt128 a)
{
    return std::log(a.hi) + std::log1p(a.lo / a.hi);
}
FAST_INLINE flt128 log(const flt128& a)
{
    double log_hi = std::log(a.hi); // first guess
    flt128 exp_log_hi(std::exp(log_hi)); // exp(guess)
    flt128 r = (a - exp_log_hi) / exp_log_hi; // (a - e^g) / e^g ≈ error
    return flt128(log_hi) + r; // refined log
}
FAST_INLINE flt128 log2(const flt128& a)
{
    constexpr flt128 LOG2_RECIPROCAL(1.442695040888963407359924681001892137); // 1/log(2)
    return log(a) * LOG2_RECIPROCAL;
}
FAST_INLINE flt128 log10(const flt128& x)
{
    // 1 / ln(10) to full-double accuracy
    static constexpr double INV_LN10 = 0.434294481903251827651128918916605082;
    return log(x) * flt128(INV_LN10);   // log(x) already returns float128
}
FAST_INLINE flt128 clamp(const flt128& v, const flt128& lo, const flt128& hi)
{
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}
FAST_INLINE flt128 abs(const flt128& a)
{
    return (a.hi < 0.0) ? -a : a;
}
FAST_INLINE flt128 fabs(const flt128& a) 
{
    return (a.hi < 0.0) ? -a : a;
}
FAST_INLINE flt128 floor(const flt128& a)
{
    double f = std::floor(a.hi);
    /*     hi already integral ?  trailing part < 0  →  round one below   */
    if (f == a.hi && a.lo < 0.0) f -= 1.0;
    return { f, 0.0 };
}
FAST_INLINE flt128 ceil(const flt128& a)
{
    double c = std::ceil(a.hi);
    if (c == a.hi && a.lo > 0.0) c += 1.0;
    return { c, 0.0 };
}
FAST_INLINE flt128 trunc(const flt128& a)
{
    return (a.hi < 0.0) ? ceil(a) : floor(a);
}
FAST_INLINE flt128 round(const flt128& a)
{
    flt128 t = floor(a + flt128(0.5));
    /* halfway cases: round to even like std::round */
    if (((t.hi - a.hi) == 0.5) && (std::fmod(t.hi, 2.0) != 0.0)) t -= flt128(1.0);
    return t;
}
FAST_INLINE flt128 fmod(const flt128& x, const flt128& y)
{
    if (y.hi == 0.0 && y.lo == 0.0)
        return std::numeric_limits<flt128>::quiet_NaN();
    return x - trunc(x / y) * y;
}
FAST_INLINE flt128 remainder(const flt128& x, const flt128& y)
{
    // Domain checks (match std::remainder)
    if (isnan(x) || isnan(y)) return std::numeric_limits<flt128>::quiet_NaN();
    if (iszero(y))            return std::numeric_limits<flt128>::quiet_NaN();
    if (isinf(x))             return std::numeric_limits<flt128>::quiet_NaN();
    if (isinf(y))             return x;

    // n = nearest integer to q = x/y, ties to even
    const flt128 q = x / y;
    flt128 n = trunc(q);
    flt128 rfrac = q - n;                // fractional part with sign of q
    const flt128 half = flt128(0.5);
    const flt128 one = flt128(1);

    if (abs(rfrac) > half) {
        n += (rfrac.hi >= 0.0 ? one : -one);
    }
    else if (abs(rfrac) == half) {
        // tie: choose even n
        const flt128 n_mod2 = fmod(n, flt128(2));
        if (n_mod2 != flt128(0))
            n += (rfrac.hi >= 0.0 ? one : -one);
    }

    flt128 r = x - n * y;

    // If result is zero, sign should match x (std::remainder semantics)
    if (iszero(r))
        return flt128(std::signbit(x.hi) ? -0.0 : 0.0);

    return r;
}

/*------------ transcendentals -------------------------------------------*/

#ifdef FMA_AVAILABLE
FAST_INLINE flt128 sqrt(flt128 a)
{
    double approx = std::sqrt(a.hi);
    flt128 y{ approx };
    flt128 y_sq = y * y;
    flt128 r = a - y_sq;
    flt128 half{ 0.5 };
    // y = y + r*(0.5/y)
    double t = 0.5 / y.hi;
    // fold hi updates with fma; lo correction comes via renorm in ops
    y.hi = fma1(r.hi, t, y.hi);
    // keep the original structure for lo through normal ops
    return y;
}
FAST_INLINE flt128 sin(flt128 a)
{
    double s = std::sin(a.hi);
    double c = std::cos(a.hi);
    // s + c * a.lo with single rounding in the leading component
    double hi = fma1(c, a.lo, s);
    return flt128(hi); // trailing error is small; DD normalize occurs in ops
}
FAST_INLINE flt128 cos(flt128 a)
{
    double c = std::cos(a.hi);
    double s = std::sin(a.hi);
    double hi = fma1(-s, a.lo, c); // c - s * a.lo
    return flt128(hi);
}
FAST_INLINE flt128 exp(const flt128& a)
{
    double yh = std::exp(a.hi);
    double hi = fma1(yh, a.lo, yh); // yh*(1 + a.lo)
    return flt128{ hi };
}
FAST_INLINE flt128 atan2(const flt128& y, const flt128& x)
{
    if (x.hi == 0.0 && x.lo == 0.0) {
        if (y.hi == 0.0 && y.lo == 0.0) return flt128(std::numeric_limits<double>::quiet_NaN());
        return (y.hi > 0.0 || (y.hi == 0.0 && y.lo > 0.0)) ? flt128(DD_PI2) : flt128(-DD_PI2);
    }
    flt128 v{ std::atan2(y.hi, x.hi) };
    flt128 sv = sin(v);
    flt128 cv = cos(v);

    double f_hi = fma1(x.hi, (double)sv, -(y.hi * (double)cv)); // f = x*sv - y*cv
    double fp_hi = fma1(x.hi, (double)cv, (y.hi * (double)sv)); // fp = x*cv + y*sv

    flt128 f{ f_hi }, fp{ fp_hi };
    v = v - f / fp;
    return v;
}
#else
FAST_INLINE flt128 sqrt(flt128 a)
{
    double approx = std::sqrt(a.hi);
    flt128 y{ approx };
    flt128 y_sq = y * y;
    flt128 r = a - y_sq;
    flt128 half{ 0.5 };
    y += r * (half / y);
    return y;
}
FAST_INLINE flt128 sin(flt128 a)
{
    double s = std::sin(a.hi);
    double c = std::cos(a.hi);
    return flt128{ s } + flt128{ c } * a.lo;
}
FAST_INLINE flt128 cos(flt128 a)
{
    double c = std::cos(a.hi);
    double s = std::sin(a.hi);
    return flt128{ c } - flt128{ s } * a.lo;
}
FAST_INLINE flt128 exp(const flt128& a)
{
    //  exp(a.hi + a.lo) ≈ exp(a.hi) * (1 + a.lo)   (|a.lo| < 1 ulp) 
    flt128 y(std::exp(a.hi));
    return y * (flt128(1.0) + f128(a.lo));
}
FAST_INLINE flt128 atan2(const flt128& y, const flt128& x)
{
    /*  Special cases first (match IEEE / std::atan2)  */
    if (x.hi == 0.0 && x.lo == 0.0) {
        if (y.hi == 0.0 && y.lo == 0.0)
            return flt128(std::numeric_limits<double>::quiet_NaN());   // undefined
        return (y.hi > 0.0 || (y.hi == 0.0 && y.lo > 0.0))
            ? flt128(DD_PI2) : flt128(-DD_PI2);
    }

    /* 1) bootstrap with the regular double result (gives correct quadrant) */
    flt128 v(std::atan2(y.hi, x.hi));

    /* 2) one Newton step on  f(v)=x·sin v − y·cos v  */
    flt128 sv = sin(v);
    flt128 cv = cos(v);
    flt128  f = x * sv - y * cv;          // residual
    flt128 fp = x * cv + y * sv;          // f'
    v = v - f / fp;                         // refined solution

    return v;
}
#endif

FAST_INLINE flt128 tan(const flt128& a) { return sin(a) / cos(a); }
FAST_INLINE flt128 pow(const flt128& x, const flt128& y) { return exp(y * log(x)); }
FAST_INLINE flt128 atan(const flt128& x) { return atan2(x, flt128(1.0)); }

/*------------ printing --------------------------------------------------*/


static FAST_INLINE flt128 pow10_128(int k)
{
    flt128 r = flt128(1), ten = flt128(10);
    int n = (k >= 0) ? k : -k;
    for (int i = 0; i < n; ++i) r = r * ten;
    return (k >= 0) ? r : (flt128(1) / r);
}
static FAST_INLINE void normalize10(const flt128& x, flt128& m, int& exp10)
{
    // x = m * 10^exp10 with m in [1,10)   (seed from binary exponent, then correct)
    if (x.hi == 0.0) { m = flt128(0); exp10 = 0; return; }

    flt128 ax = abs(x);

    int e2 = 0;
    (void)std::frexp(ax.hi, &e2);             // ax.hi = f * 2^(e2-1)
    int e10 = (int)std::floor((e2 - 1) * 0.30102999566398114); // ≈ log10(2)

    m = ax * pow10_128(-e10);
    while (m >= flt128(10)) { m = m / flt128(10); ++e10; }
    while (m < flt128(1)) { m = m * flt128(10); --e10; }
    exp10 = e10;
}

static FAST_INLINE void emit_scientific(std::ostream& os, const flt128& x, std::streamsize prec)
{
    if (x.hi == 0.0) { os << "0"; return; }

    bool neg = x.hi < 0.0;
    flt128 v = neg ? flt128(-x.hi, -x.lo) : x;

    flt128 m; int e;
    normalize10(v, m, e);                 // m in [1,10)

    constexpr flt128 ten = flt128(10);

    // first digit
    int d0 = (int)floor(m).hi;            // safe 0..9
    flt128 frac = m - flt128(d0);

    if (neg) os.put('-');
    os.put(char('0' + d0));

    if (prec > 1)
    {
        os.put('.');
        for (int i = 0; i < prec - 1; ++i)
        {
            frac = frac * ten;

            // next digit = floor(frac) (handles tiny negative drift)
            int di = (int)floor(frac).hi;
            if (di < 0) di = 0; if (di > 9) di = 9;
            os.put(char('0' + di));
            frac = frac - flt128(di);
        }
    }

    os.put('e');
    os << e;
}

// round x to an integer at scale = 10^prec (ties-to-even)
static inline bl::flt128 round_scaled(bl::flt128 x, int prec) noexcept {
    if (prec <= 0) return x;
    const bl::flt128 scale = pow10_128(prec);
    bl::flt128 y = x * scale;

    bl::flt128 n = floor(y);          // integer below
    bl::flt128 f = y - n;             // fraction in [0,1)

    const bl::flt128 half(0.5);
    bool tie = (f == half);
    if (f > half || (tie && fmod(n, bl::flt128(2)) != bl::flt128(0)))
        n = n + bl::flt128(1);

    return n; // integer at the scaled grid
}

// emit integer 'n' (n >= 0) in base-10 into buffer (reversed)
static inline void emit_uint_rev(std::string& out, bl::flt128 n) {
    const bl::flt128 ten(10);
    if (n < bl::flt128(10)) {
        out.push_back(char('0' + int(floor(n).hi)));
        return;
    }
    while (n >= bl::flt128(10)) {
        bl::flt128 q = floor(n / ten);
        bl::flt128 r = n - q * ten;                 // exact digit 0..9
        int d = (int)floor(r).hi;
        out.push_back(char('0' + d));
        n = q;
    }
    out.push_back(char('0' + int(floor(n).hi)));
}

// print x with exactly 'prec' decimals after rounding to grid.
// if strip_trailing_zeros==true, remove trailing zeros in the fraction
// and drop the '.' if nothing remains. Guarantees "-0" -> "0".
static inline void emit_fixed_dec(std::ostream& os,
    bl::flt128 x,
    int prec,
    bool strip_trailing_zeros)
{
    // handle sign and -0
    bool neg = (x.hi < 0.0);
    if (neg) x = bl::flt128(-x.hi, -x.lo);

    // scale-and-round to an integer
    bl::flt128 n = round_scaled(x, prec); // integer at 10^prec grid

    // split into integer/fractional by decimal placement
    // we emit all digits of n, then insert dot prec-from-right
    std::string rev; rev.reserve(64);
    emit_uint_rev(rev, n); // digits in reverse order

    // ensure we have at least prec+1 digits to place the dot
    if (prec > 0 && (int)rev.size() <= prec) {
        // pad leading zeros on the left of integer part
        int need = prec + 1 - (int)rev.size();
        rev.append(need, '0');
    }

    // build forward string
    std::string s; s.reserve(rev.size() + 2);
    if (neg) s.push_back('-');

    if (prec <= 0) {
        // just the integer
        for (auto it = rev.rbegin(); it != rev.rend(); ++it) s.push_back(*it);
    }
    else {
        // integer part: everything beyond last 'prec' digits
        int total = (int)rev.size();

        // write integer part
        for (int i = total - 1; i >= prec; --i) s.push_back(rev[i]);

        // decimal point + fractional part
        s.push_back('.');
        for (int i = prec - 1; i >= 0; --i) s.push_back(rev[i]);

        if (strip_trailing_zeros) {
            // strip trailing zeros in fraction
            int end = (int)s.size() - 1;
            while (end >= 0 && s[end] == '0') --end;

            if (end >= 0 && s[end] == '.') {
                // remove '.' too
                --end;
            }

            s.resize(size_t(std::max(end + 1, neg ? 1 : 0)));
            // if all digits stripped after '-', ensure we still have at least "0"
            if (s.empty() || (neg && s.size() == 1))
                s = neg ? "-0" : "0";
        }
    }

    // normalize "-0" to "0"
    if (s == "-0") s = "0";

    os << s;
}

// ---------- the ostream operator (uses stream flags/precision) ----------

static inline std::string to_string(const flt128& x, int precision, bool scientific=true)
{
    std::stringstream ss;
    if (scientific)
        emit_scientific(ss, x, precision);
    else
        emit_fixed_dec(ss, x, precision, true);

    return ss.str();
}
static inline bool   parse_flt128(const char* s, flt128& out, const char** endptr = nullptr)
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
        out = std::numeric_limits<flt128>::quiet_NaN();
        p += 3; if (endptr) *endptr = p; return true;
    }
    if ((ci(p[0]) == 'i' && ci(p[1]) == 'n' && ci(p[2]) == 'f'))
    {
        p += 3;
        if (ci(p[0]) == 'i' && ci(p[1]) == 'n' && ci(p[2]) == 'i' && ci(p[3]) == 't' && ci(p[4]) == 'y') p += 5;
        out = neg ? -std::numeric_limits<flt128>::infinity() : std::numeric_limits<flt128>::infinity();
        if (endptr) *endptr = p; return true;
    }
    p = ps; // reset if not special

    // 4) digits (integer part)
    auto isdig = [](char c)->bool { return (c >= '0' && c <= '9'); };

    flt128 int_part = flt128(0);
    flt128 frac_part = flt128(0);
    int    frac_digits = 0;
    bool   any_digit = false;

    const flt128 base1e9 = flt128(1000000000.0);
    auto mul_pow10_small = [](flt128 v, int n)->flt128 {
        // multiply by 10^n, 0<=n<=9
        static const int POWS[10] = { 1,10,100,1000,10000,100000,1000000,10000000,100000000,1000000000 };
        return v * flt128((double)POWS[n]); // constant fits in double exactly for n<=9
    };

    // integer: chunked base 1e9 accumulation
    {
        unsigned int chunk = 0; int clen = 0;
        while (isdig(*p)) {
            chunk = chunk * 10u + unsigned(*p - '0'); ++clen; ++p; any_digit = true;
            if (clen == 9) { int_part = int_part * base1e9 + flt128((double)chunk); chunk = 0; clen = 0; }
        }
        if (clen > 0)
            int_part = int_part * mul_pow10_small(flt128(1), clen) + flt128((double)chunk);
    }

    // fractional: '.' then digits
    if (*p == '.')
    {
        ++p;
        unsigned int chunk = 0; int clen = 0;
        int local_digits = 0;
        while (isdig(*p)) {
            chunk = chunk * 10u + unsigned(*p - '0'); ++clen; ++p; ++local_digits; any_digit = true;
            if (clen == 9) { frac_part = frac_part * base1e9 + flt128((double)chunk); chunk = 0; clen = 0; }
        }
        if (clen > 0)
            frac_part = frac_part * mul_pow10_small(flt128(1), clen) + flt128((double)chunk);
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
    flt128 value = int_part;
    if (frac_digits > 0) {
        // compute 10^frac_digits
        flt128 pow_frac = flt128(1);
        int fd = frac_digits;
        while (fd >= 9) { pow_frac = pow_frac * base1e9; fd -= 9; }
        if (fd > 0) {
            static const int POWS[10] = { 1,10,100,1000,10000,100000,1000000,10000000,100000000,1000000000 };
            pow_frac = pow_frac * flt128((double)POWS[fd]);
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
                value = value * flt128((double)POWS[e]);
            }
        }
        else {
            e = -e;
            while (e >= 9) { value = value / base1e9; e -= 9; }
            if (e > 0) {
                static const int POWS[10] = { 1,10,100,1000,10000,100000,1000000,10000000,100000000,1000000000 };
                value = value / flt128((double)POWS[e]);
            }
        }
    }

    if (neg) value = -value;

    // 7) done
    out = value;
    if (endptr) *endptr = p;
    return true;
}
static inline flt128 from_string(const char* s)
{
    flt128 ret;
    if (parse_flt128(s, ret))
        return ret;
    return flt128{ 0 };
}

inline std::ostream& operator<<(std::ostream& os, const flt128& x)
{
    int prec = (int)os.precision();
    if (prec <= 0) prec = 6;

    const auto f = os.flags();
    const bool fixed = (f & std::ios_base::fixed) != 0;
    const bool scientific = (f & std::ios_base::scientific) != 0;

    if (fixed && !scientific)
        emit_fixed_dec(os, x, prec, true);
    else
        emit_scientific(os, x, prec);

    return os;
}

} // end bl
