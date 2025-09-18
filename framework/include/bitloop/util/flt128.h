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
    constexpr flt128(flt128&&) = default;

    constexpr flt128& operator=(const flt128&) = default;
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

static inline bool is_nan128(const flt128& x) { return std::isnan(x.hi); }
static inline bool is_inf128(const flt128& x) { return std::isinf(x.hi); }
static inline bool is_zero128(const flt128& x) { return x.hi == 0.0 && x.lo == 0.0; }

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
inline flt128 abs(const flt128& a)
{
    return (a.hi < 0.0) ? -a : a;
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
inline flt128 trunc(const flt128& a)
{
    return (a.hi < 0.0) ? ceil(a) : floor(a);
}
inline flt128 round(const flt128& a)
{
    flt128 t = floor(a + flt128(0.5));
    /* halfway cases: round to even like std::round */
    if (((t.hi - a.hi) == 0.5) && (std::fmod(t.hi, 2.0) != 0.0)) t -= flt128(1.0);
    return t;
}
inline flt128 fmod(const flt128& x, const flt128& y)
{
    if (y.hi == 0.0 && y.lo == 0.0)
        return std::numeric_limits<flt128>::quiet_NaN();
    return x - trunc(x / y) * y;
}
inline flt128 remainder(const flt128& x, const flt128& y)
{
    // Domain checks (match std::remainder)
    if (is_nan128(x) || is_nan128(y)) return std::numeric_limits<flt128>::quiet_NaN();
    if (is_zero128(y))                return std::numeric_limits<flt128>::quiet_NaN();
    if (is_inf128(x))                 return std::numeric_limits<flt128>::quiet_NaN();
    if (is_inf128(y))                 return x; // remainder(x, ±inf) = x

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
    if (is_zero128(r))
        return flt128(std::signbit(x.hi) ? -0.0 : 0.0);

    return r;
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

/*------------ printing --------------------------------------------------*/


/*inline std::string to_string(const flt128& x)
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
}*/

/*
inline std::ostream& operator<<(std::ostream& os, const flt128& x)
{
    // Special cases
    //if (x.hi == 0.0)
    //    return os << "0";

    flt128 v = x;
    bool neg = false;
    if (v.hi < 0.0) { neg = true; v.hi = -v.hi; v.lo = -v.lo; }

    // get precision from the stream
    std::streamsize prec = os.precision();
    if (prec <= 0) prec = 6; // C++ default

    // scale into [1,10) and get exponent
    int exp10 = static_cast<int>(std::floor(std::log10(v.hi)));
    flt128 scale = std::pow(10.0, -exp10);
    v *= scale;

    std::string digits;
    digits.reserve(prec + 1);

    for (int i = 0; i < prec; ++i) {
        int d = static_cast<int>(v.hi);
        digits.push_back(char('0' + d));
        v -= flt128(d);
        v *= 10.0;
    }

    // Decide format: fixed vs scientific
    std::ios_base::fmtflags f = os.flags();
    bool fixed = (f & std::ios_base::fixed) != 0;
    bool scientific = (f & std::ios_base::scientific) != 0;

    if (fixed && !scientific)
    {
        // fixed: put decimal point after first digit + all others
        os << (neg ? "-" : "");
        os << digits[0];
        if (prec > 1) {
            os << ".";
            os.write(&digits[1], prec - 1);
        }
        // scale back exponent if != 0
        if (exp10 != 0)
            os << "e" << exp10;
    }
    else
    {
        // default or scientific: one digit then '.' then rest + 'e'
        os << (neg ? "-" : "");
        os << digits[0];
        if (prec > 1) {
            os << ".";
            os.write(&digits[1], prec - 1);
        }
        os << "e" << exp10;
    }

    return os;
}

*/

// ---------- helpers (safe, no long double) ----------
static inline flt128 abs128(flt128 x) { return x < flt128(0) ? -x : x; }

static inline flt128 pow10_128(int k)
{
    flt128 r = flt128(1), ten = flt128(10);
    int n = (k >= 0) ? k : -k;
    for (int i = 0; i < n; ++i) r = r * ten;
    return (k >= 0) ? r : (flt128(1) / r);
}

// x = m * 10^exp10 with m in [1,10)   (seed from binary exponent, then correct)
static inline void normalize10(const flt128& x, flt128& m, int& exp10)
{
    if (x.hi == 0.0) { m = flt128(0); exp10 = 0; return; }

    flt128 ax = abs128(x);

    int e2 = 0;
    (void)std::frexp(ax.hi, &e2);             // ax.hi = f * 2^(e2-1)
    int e10 = (int)std::floor((e2 - 1) * 0.30102999566398114); // ≈ log10(2)

    m = ax * pow10_128(-e10);
    while (m >= flt128(10)) { m = m / flt128(10); ++e10; }
    while (m < flt128(1)) { m = m * flt128(10); --e10; }
    exp10 = e10;
}

// ---------- digit emitters using flt128::floor to avoid negative-digit artifacts ----------
static inline void emit_scientific(std::ostream& os, const flt128& x, std::streamsize prec)
{
    if (x.hi == 0.0) { os << "0"; return; }

    bool neg = x.hi < 0.0;
    flt128 v = neg ? flt128(-x.hi, -x.lo) : x;

    flt128 m; int e;
    normalize10(v, m, e);                 // m in [1,10)

    const flt128 ten = flt128(10);

    // first digit
    int d0 = (int)floor(m).hi;            // safe 0..9
    flt128 frac = m - flt128(d0);

    if (neg) os.put('-');
    os.put(char('0' + d0));

    if (prec > 1) {
        os.put('.');
        for (int i = 0; i < prec - 1; ++i) {
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

static inline void emit_fixed(std::ostream& os, const flt128& x, std::streamsize prec)
{
    // true fixed: integer '.' fractional(prec)
    bool neg = (x.hi < 0.0);
    flt128 v = neg ? flt128(-x.hi, -x.lo) : x;
    const flt128 ten = flt128(10);

    // integer part via repeated div-by-10 using floor (robust with dd)
    std::string int_rev;
    flt128 t = v;
    if (t < flt128(1)) {
        int_rev.push_back('0');
    }
    else {
        while (t >= flt128(1)) {
            flt128 q = floor(t / ten);           // q = floor(t/10)
            flt128 r = t - q * ten;              // r in [0,10)
            int d = (int)floor(r).hi;            // digit 0..9
            if (d < 0) d = 0; if (d > 9) d = 9;
            int_rev.push_back(char('0' + d));
            t = q;
        }
    }

    if (neg) os.put('-');
    for (auto it = int_rev.rbegin(); it != int_rev.rend(); ++it) os.put(*it);

    if (prec > 0) {
        os.put('.');
        // frac = v - integer_value (recompute exactly from digits)
        flt128 int_val = flt128(0);
        for (auto it = int_rev.rbegin(); it != int_rev.rend(); ++it)
            int_val = int_val * ten + flt128(*it - '0');
        flt128 frac = v - int_val;

        for (int i = 0; i < prec; ++i) {
            frac = frac * ten;
            int di = (int)floor(frac).hi;        // digit 0..9 (handles tiny negatives)
            if (di < 0) di = 0; if (di > 9) di = 9;
            os.put(char('0' + di));
            frac = frac - flt128(di);
        }
    }
}

// ---------- the ostream operator (uses stream flags/precision) ----------
inline std::ostream& operator<<(std::ostream& os, const flt128& x)
{
    std::streamsize prec = os.precision();
    if (prec <= 0) prec = 6;

    const auto f = os.flags();
    const bool fixed = (f & std::ios_base::fixed) != 0;
    const bool scientific = (f & std::ios_base::scientific) != 0;

    if (fixed && !scientific)
        emit_fixed(os, x, prec);
    else
        emit_scientific(os, x, prec);

    return os;
}


static inline std::string to_string(const flt128& x, int precision)
{
    std::stringstream ss;
    emit_scientific(ss, x, precision);
    return ss.str();
}

static inline bool parse_flt128(const char* s, flt128& out, const char** endptr = nullptr)
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
