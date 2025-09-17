#pragma once
///#ifdef _MSC_VER
///#pragma float_control(precise, off)
///#endif

#include <cmath>
#include <limits>
#include <string>
#include <iomanip>
#include <sstream>
#include <type_traits>

#include <bitloop/platform/platform_macros.h>

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

class flt128 {
public:
    double hi; // leading component
    double lo; // trailing error

    /// ======== Constructors ========
    constexpr flt128() : hi(0.0), lo(0.0) {}
    constexpr flt128(double x) : hi(x), lo(0.0) {}
    constexpr flt128(double h, double l) : hi(h), lo(l) {}

    constexpr flt128(const flt128&) = default;
    constexpr flt128& operator=(const flt128&) = default;
    constexpr flt128(flt128&&) = default;
    constexpr flt128& operator=(flt128&&) = default;

    /// ======== Basic helpers ========

    static FAST_INLINE constexpr flt128 quick_two_sum(double a, double b)
    {
        double s = a + b;
        double err = b - (s - a);
        return { s, err };
    }

    friend FAST_INLINE constexpr flt128 recip(flt128 b)
    {
        flt128 y = flt128(1.0 / b.hi);
        flt128 one = flt128(1.0);
        flt128 e = one - b * y;

        y += y * e;
        e = one - b * y;
        y += y * e;

        return y;
    }

    /// ======== Normalisation (ensure |lo| <= 0.5 ulp(hi)) ========
    static FAST_INLINE constexpr flt128 renorm(double hi, double lo)
    {
        double s, e;
        two_sum_precise(hi, lo, s, e);
        return { s, e };
    }

    /// ======== Arithmetic operators ========
    friend FAST_INLINE constexpr flt128 operator*(flt128 a, double b)
    {
        return a * flt128(b);
    }
    friend FAST_INLINE constexpr flt128 operator*(double a, flt128 b)
    {
        return flt128(a) * b;
    }
    friend FAST_INLINE constexpr flt128 operator+(flt128 a, flt128 b)
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
    friend FAST_INLINE constexpr flt128 operator-(flt128 a, flt128 b)
    {
        b.hi = -b.hi;
        b.lo = -b.lo;
        return a + b;
    }
    friend FAST_INLINE constexpr flt128 operator*(flt128 a, flt128 b)
    {
        double p1, e1;
        two_prod_precise(a.hi, b.hi, p1, e1);
        e1 += a.hi * b.lo + a.lo * b.hi;
        return renorm(p1, e1);
    }

    friend FAST_INLINE constexpr flt128 operator/(flt128 a, flt128 b)
    {
        return a * recip(b);
    }

    /// ======== compound assignments ========
    FAST_INLINE constexpr flt128& operator+=(flt128 rhs) { *this = *this + rhs; return *this; }
    FAST_INLINE constexpr flt128& operator-=(flt128 rhs) { *this = *this - rhs; return *this; }
    FAST_INLINE constexpr flt128& operator*=(flt128 rhs) { *this = *this * rhs; return *this; }
    FAST_INLINE constexpr flt128& operator/=(flt128 rhs) { *this = *this / rhs; return *this; }

    /// ======== [sqrt, sin, cos] Dekker/Bailey refinement ========
    friend FAST_INLINE flt128 sqrt(flt128 a)
    {
        double approx = std::sqrt(a.hi);
        flt128 y(approx);
        flt128 y_sq = y * y;
        flt128 r = a - y_sq;
        flt128 half = 0.5;
        y += r * (half / y);
        return y;
    }
    friend FAST_INLINE flt128 sin(flt128 a)
    {
        double s = std::sin(a.hi);
        double c = std::cos(a.hi);
        return flt128(s) + flt128(c) * a.lo;
    }
    friend FAST_INLINE flt128 cos(flt128 a)
    {
        double c = std::cos(a.hi);
        double s = std::sin(a.hi);
        return flt128(c) - flt128(s) * a.lo;
    }

    /// ======== Conversions ========
    explicit constexpr operator double() const { return hi + lo; }
    explicit constexpr operator float() const { return static_cast<float>(hi + lo); }
    explicit constexpr operator int() const { return static_cast<int>(hi + lo); }

    constexpr flt128 operator+() const { return *this; }
    constexpr flt128 operator-() const { return (*this) * -1; }

    /// ======== Utility ========
    static constexpr flt128 eps()
    {
        return { std::numeric_limits<double>::epsilon(), 0.0 };
    }
};

namespace std
{
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

// comparisons
constexpr FAST_INLINE bool operator<(const flt128& a, const flt128& b)  { return (a.hi < b.hi) || (a.hi == b.hi && a.lo < b.lo); }
constexpr FAST_INLINE bool operator>(const flt128& a, const flt128& b)  { return b < a; }
constexpr FAST_INLINE bool operator<=(const flt128& a, const flt128& b) { return (a < b) || (a.hi == b.hi && a.lo == b.lo); }
constexpr FAST_INLINE bool operator>=(const flt128& a, const flt128& b) { return b <= a; }
constexpr FAST_INLINE bool operator==(const flt128& a, const flt128& b) { return a.hi == b.hi && a.lo == b.lo; }
constexpr FAST_INLINE bool operator!=(const flt128& a, const flt128& b) { return !(a == b); }

static constexpr double DD_PI = 3.141592653589793238462643383279502884;
static constexpr double DD_PI2 = 1.570796326794896619231321691639751442;

inline flt128 log(const flt128& a)
{
    double log_hi = std::log(a.hi); // first guess
    flt128 exp_log_hi(std::exp(log_hi)); // exp(guess)
    flt128 r = (a - exp_log_hi) / exp_log_hi; // (a - e^g) / e^g ≈ error
    return flt128(log_hi) + r; // refined log
}
inline flt128 log2(const flt128& a)
{
    constexpr flt128 LOG2_RECIPROCAL(1.442695040888963407359924681001892137); // 1/log(2)
    return log(a) * LOG2_RECIPROCAL;
}
inline flt128 log10(const flt128& x)
{
    // 1 / ln(10) to full-double accuracy
    static constexpr double INV_LN10 = 0.434294481903251827651128918916605082;
    return log(x) * flt128(INV_LN10);   // log(x) already returns float128
}
inline flt128 fabs(const flt128& a) 
{
    return (a.hi < 0.0) ? -a : a;
}
inline flt128 floor(const flt128& a)
{
    double f = std::floor(a.hi);
    /*     hi already integral ?  trailing part < 0  →  round one below   */
    if (f == a.hi && a.lo < 0.0) f -= 1.0;
    return { f, 0.0 };
}
inline flt128 ceil(const flt128& a)
{
    double c = std::ceil(a.hi);
    if (c == a.hi && a.lo > 0.0) c += 1.0;
    return { c, 0.0 };
}
inline flt128 trunc(const flt128& a) { return (a.hi < 0.0) ? ceil(a) : floor(a); }
inline flt128 round(const flt128& a)
{
    flt128 t = floor(a + flt128(0.5));
    /* halfway cases: round to even like std::round */
    if (((t.hi - a.hi) == 0.5) && (std::fmod(t.hi, 2.0) != 0.0)) t -= flt128(1.0);
    return t;
}

/*------------ transcendentals -------------------------------------------*/

inline flt128 tan(const flt128& a) { return sin(a) / cos(a); }
inline flt128 exp(const flt128& a)
{
    /*  exp(a.hi + a.lo) ≈ exp(a.hi) * (1 + a.lo)   (|a.lo| < 1 ulp)  */
    flt128 y(std::exp(a.hi));
    return y * (flt128(1.0) + a.lo);
}
inline flt128 pow(const flt128& x, const flt128& y) { return exp(y * log(x)); }

/*------------ atan/atan2 --------------------------------------------------*/

inline flt128 atan2(const flt128& y, const flt128& x)
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
inline flt128 atan(const flt128& x) { return atan2(x, flt128(1.0)); }
/*inline std::string to_string(const flt128& x)
{
    if (x.hi == 0.0) return "0";

    // --- 1. get sign and magnitude ----------------------------------------
    flt128 v = x;
    bool neg = false;
    if (v.hi < 0) { neg = true; v.hi = -v.hi; v.lo = -v.lo; }

    // --- 2. scale into [1,10) and find exponent ---------------------------
    int exp10 = static_cast<int>(std::floor(std::log10(v.hi)));
    flt128 scale = std::pow(10.0, -exp10);   // 10^(-exp10)
    v *= scale;                                    // now 1 ≤ v < 10

    // --- 3. generate 32 digits (safely > 31) ------------------------------
    std::ostringstream oss;
    oss << std::setprecision(32) << std::fixed;
    for (int i = 0; i < 32; ++i) {
        int d = static_cast<int>(v.hi);
        oss << d;
        v -= flt128(d);              // remove integer part
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
}*/

inline std::string to_string(const flt128& x)
{
    if (x.hi == 0.0 && x.lo == 0.0)
        return "0";

    flt128 v = x;
    bool neg = false;
    if (v.hi < 0) { neg = true; v.hi = -v.hi; v.lo = -v.lo; }

    // estimate decimal exponent
    int exp10 = static_cast<int>(std::floor(std::log10(v.hi)));
    flt128 scale = std::pow(10.0, -exp10);
    v *= scale; // normalized to [1,10)

    constexpr int max_digits = 32; // double-double: ~106 bits ≈ 31–32 decimal digits

    std::string digits;
    digits.reserve(max_digits);
    for (int i = 0; i < max_digits; ++i) {
        int d = static_cast<int>(v.hi);
        digits.push_back(static_cast<char>('0' + d));
        v -= flt128(d);
        v *= 10.0;
    }

    int pointPos = exp10 + 1;
    std::string s;
    if (neg) s.push_back('-');

    if (pointPos <= 0) {
        s += "0.";
        s.append(-pointPos, '0');
        s += digits;
    }
    else if (pointPos >= (int)digits.size()) {
        s += digits;
        s.append(pointPos - (int)digits.size(), '0');
    }
    else {
        s.append(digits.begin(), digits.begin() + pointPos);
        s.push_back('.');
        s.append(digits.begin() + pointPos, digits.end());
    }

    return s;
}


inline std::ostream& operator<<(std::ostream& os, const flt128& x)
{
    return os << to_string(x);
}


