#pragma once
#include <functional>
#include <string>
#include <sstream>
#include <iomanip>
#include <cmath>
#include <limits>

class ProjectBase;
using ProjectCreatorFunc = std::function<ProjectBase*()>;

namespace Math {
    template<typename T>
    T avgAngle(T a, T b);
}

// Enums

enum Anchor
{
    TOP_LEFT,
    CENTER
};

enum CoordinateType
{
    STAGE,
    WORLD
};

// Types

struct ProjectInfo
{
    enum State { INACTIVE, ACTIVE, RECORDING };

    std::vector<std::string> path;
    ProjectCreatorFunc creator;
    int sim_uid;
    State state;

    ProjectInfo(
        std::vector<std::string> path,
        ProjectCreatorFunc creator = nullptr,
        int sim_uid = -100,
        State state = State::INACTIVE
    )
        : path(path), creator(creator), sim_uid(sim_uid), state(state)
    {}
};

class Viewport;
struct MouseInfo
{
    Viewport* viewport = nullptr;
    double client_x = 0;
    double client_y = 0;
    double stage_x = 0;
    double stage_y = 0;
    double world_x = 0;
    double world_y = 0;
    int scroll_delta = 0;
};

/*struct long_double
{
    double hi;
    double lo;

    long_double(double h, double l)
    {
        hi = h;
        lo = l;
    }

    void set(double h, double l)
    {
        hi = h;
        lo = l;
    }

    void set(long_double rhs)
    {
        hi = rhs.h;
        lo = rhs.l;
    }

    long_double operator +(long_double rhs)
    {
        double a, b, c, d, e, f;
        e = hi + rhs.hi;
        d = hi - e;
        a = lo + rhs.lo;
        f = lo - a;
        d = ((hi - (d + e)) + (d + rhs.hi)) + a;
        b = e + d;
        c = ((lo - (f + a)) + (f + rhs.lo)) + (d + (e - b));
        a = b + c;
        return long_double(a, c + (b - a));
    }

    long_double operator -(long_double rhs)
    {
        double a, b, c, d, e, f, g;
        g = lo - rhs.lo;
        f = lo - g;
        e = hi - rhs.hi;
        d = hi - e;
        d = ((hi - (d + e)) + (d - rhs.hi)) + g;
        b = e + d;
        c = (d + (e - b)) + ((lo - (f + g)) + (f - rhs.lo));
        a = b + c;
        return long_double(a, c + (b - a));
    }

    long_double operator /(long_double rhs)
    {
        double a, b, c, d, e, f, g;
        f = hi / rhs.hi;
        a = 0x08000001 * rhs.hi;
        a += rhs.hi - a;
        b = rhs.hi - a;
        c = 0x08000001 * f;
        c += f - c;
        d = f - c;
        e = rhs.hi * f;
        c = (((a * c - e) + (a * d + b * c)) + b * d) + rhs.lo * f;
        b = lo - c;
        d = lo - b;
        a = hi - e;
        e = (hi - ((hi - a) + a)) + b;
        g = a + e;
        e += (a - g) + ((lo - (d + b)) + (d - c));
        a = g + e;
        b = a / rhs.hi;
        f += (e + (g - a)) / rhs.hi;
        a = f + b;
        return long_double(a, b + (f - a));
    }

    inline long_double operator *(long_double rhs) 
    {
        double a, b, c, d, e;
        a = 0x08000001 * hi;
        a += hi - a;
        b =  hi - a;
        c = 0x08000001 * rhs.hi;
        c += rhs.hi - c;
        d =  rhs.hi - c;
        e = hi * rhs.hi;
        c = (((a * c - e) + (a * d + b * c)) + b * d) + (lo * rhs.hi + hi * rhs.lo);
        a = e + c;
        return long_double(a, c + (e - a));
    }


    inline long_double operator *(unsigned int rhs) { return (*this) * long_double(rhs, 0); }
    inline long_double operator *(double rhs) { return (*this) * long_double(rhs, 0); }

    inline long_double operator +(long_double rhs) { set((*this) + rhs); }
    inline long_double operator -(long_double rhs) { set((*this) - rhs); }
    inline long_double operator *(long_double rhs) { set((*this) * rhs); }
    inline long_double operator /(long_double rhs) { set((*this) / rhs); }
};

inline long_double sqrt(long_double num) {
    double a, b, c;
    a = 0x08000001 * num.hi;
    a += num.hi - a;
    b = num.hi - a;
    c = num.hi * num.hi;
    b = ((((a * a - c) + a * b * 2) + b * b) + num.hi * num.lo * 2) + num.lo * num.lo;
    a = b + c;
    return long_double(a, b + (c - a));
}
*/



class DoubleDouble {
public:
    double hi; // leading component
    double lo; // trailing error

    /*------------------------------------------------------------------------
      Constructors
    ------------------------------------------------------------------------*/
    constexpr DoubleDouble() : hi(0.0), lo(0.0) {}
    constexpr DoubleDouble(double x) : hi(x), lo(0.0) {}
    constexpr DoubleDouble(double h, double l) : hi(h), lo(l) {}

    /*------------------------------------------------------------------------
      Basic helpers (error-free transforms)
    ------------------------------------------------------------------------*/
    //static inline void two_sum(double a, double b, double& s, double& e)
    //{
    //    s = a + b;
    //    double bv = s - a;
    //    e = (a - (s - bv)) + (b - bv);
    //}

    static constexpr void two_sum(double a, double b, double& s, double& e)
    {
        s = a + b;
        double bv = s - a;
        e = (a - (s - bv)) + (b - bv);
    }

    /*static inline void two_prod(double a, double b, double& p, double& e)
    {
        #if defined(__FMA__) || defined(__FMA) || defined(__FMA4__)
        p = a * b;
        e = std::fma(a, b, -p);
        #else
        const double split = 134217729.0; // 2^27+1, good for IEEE binary64
        double a_hi = a * split;
        double a_lo = a - a_hi;
        a_hi += a_lo;  // renormalise
        a_lo = a - a_hi;

        double b_hi = b * split;
        double b_lo = b - b_hi;
        b_hi += b_lo;
        b_lo = b - b_hi;

        p = a * b;
        e = ((a_hi * b_hi - p) + a_hi * b_lo + a_lo * b_hi) + a_lo * b_lo;
        #endif
    }*/

    static constexpr void two_prod(double a, double b, double& p, double& e)
    {
        const double split = 134217729.0;

        double a_c = a * split;
        double a_hi = a_c - (a_c - a);
        double a_lo = a - a_hi;

        double b_c = b * split;
        double b_hi = b_c - (b_c - b);
        double b_lo = b - b_hi;

        p = a * b;
        e = ((a_hi * b_hi - p) + a_hi * b_lo + a_lo * b_hi) + a_lo * b_lo;
    }

    //static inline DoubleDouble quick_two_sum(double a, double b)
    //{
    //    double s = a + b;
    //    double err = b - (s - a);
    //    return { s, err };
    //}

    static constexpr DoubleDouble quick_two_sum(double a, double b)
    {
        double s = a + b;
        double err = b - (s - a);
        return { s, err };
    }

    /*------------------------------------------------------------------------
      Normalisation (ensure |lo| <= 0.5 ulp(hi))
    ------------------------------------------------------------------------*/
    //static inline DoubleDouble renorm(double hi, double lo)
    //{
    //    double s, e;
    //    two_sum(hi, lo, s, e);
    //    return { s, e };
    //}

    static constexpr DoubleDouble renorm(double hi, double lo)
    {
        double s, e;
        two_sum(hi, lo, s, e);
        return { s, e };
    }

    /*------------------------------------------------------------------------
      Arithmetic operators
    ------------------------------------------------------------------------*/
    friend constexpr DoubleDouble operator*(DoubleDouble a, double b)
    {
        return a * DoubleDouble(b);
    }

    friend constexpr DoubleDouble operator*(double a, DoubleDouble b)
    {
        return DoubleDouble(a) * b;
    }


    friend constexpr DoubleDouble operator+(DoubleDouble a, DoubleDouble b)
    {
        double s1, e1;
        two_sum(a.hi, b.hi, s1, e1);
        double s2, e2;
        two_sum(a.lo, b.lo, s2, e2);
        double lo = e1 + s2;
        double result_hi, result_lo;
        two_sum(s1, lo, result_hi, result_lo);
        result_lo += e2;
        return renorm(result_hi, result_lo);
    }

    friend constexpr DoubleDouble operator-(DoubleDouble a, DoubleDouble b)
    {
        b.hi = -b.hi;
        b.lo = -b.lo;
        return a + b;
    }

    friend constexpr DoubleDouble operator*(DoubleDouble a, DoubleDouble b)
    {
        double p1, e1;
        two_prod(a.hi, b.hi, p1, e1);
        e1 += a.hi * b.lo + a.lo * b.hi;
        return renorm(p1, e1);
    }

    friend constexpr DoubleDouble operator/(DoubleDouble a, DoubleDouble b)
    {
        // one Newton step after double quotient
        double q = a.hi / b.hi;                // coarse
        DoubleDouble qb = b * q;               // q * b
        DoubleDouble r = a - qb;              // remainder
        double q_corr = r.hi / b.hi;         // refine
        return renorm(q + q_corr, 0.0);
    }

    /* compound assignments */
    constexpr DoubleDouble& operator+=(DoubleDouble rhs) { *this = *this + rhs; return *this; }
    constexpr DoubleDouble& operator-=(DoubleDouble rhs) { *this = *this - rhs; return *this; }
    inline DoubleDouble& operator*=(DoubleDouble rhs) { *this = *this * rhs; return *this; }
    constexpr DoubleDouble& operator/=(DoubleDouble rhs) { *this = *this / rhs; return *this; }



    /*------------------------------------------------------------------------
      Transcendentals (sqrt, sin, cos via Dekker/Bailey style refinement)
    ------------------------------------------------------------------------*/
    friend inline DoubleDouble sqrt(DoubleDouble a)
    {
        double approx = std::sqrt(a.hi);
        DoubleDouble y(approx);                    // y ≈ sqrt(a)
        DoubleDouble y_sq = y * y;                 // y^2
        DoubleDouble r = a - y_sq;                 // residual
        DoubleDouble half = 0.5;
        y += r * (half / y);                       // one Newton iteration
        return y;
    }

    friend inline DoubleDouble sin(DoubleDouble a)
    {
        // Reduce to double first, then correct with one term of series
        double s = std::sin(a.hi);
        double c = std::cos(a.hi);
        return DoubleDouble(s) + DoubleDouble(c) * a.lo;
    }

    friend inline DoubleDouble cos(DoubleDouble a)
    {
        double c = std::cos(a.hi);
        double s = std::sin(a.hi);
        return DoubleDouble(c) - DoubleDouble(s) * a.lo;
    }

    /*------------------------------------------------------------------------
      Conversions
    ------------------------------------------------------------------------*/
    explicit constexpr operator double() const { return hi + lo; }
    explicit constexpr operator float() const { return static_cast<float>(hi + lo); }

    constexpr DoubleDouble operator -() const
    {
        return (*this) * -1;
    }

    /*------------------------------------------------------------------------
      Utility
    ------------------------------------------------------------------------*/
    static constexpr DoubleDouble eps()
    {
        return { std::numeric_limits<double>::epsilon(), 0.0 };
    }
};

namespace std {
    template <>
    struct numeric_limits<DoubleDouble> {
        static constexpr bool is_specialized = true;

        static constexpr DoubleDouble min() noexcept {
            return { std::numeric_limits<double>::min(), 0.0 };
        }

        static constexpr DoubleDouble max() noexcept {
            return { std::numeric_limits<double>::max(), -std::numeric_limits<double>::epsilon() };
        }

        static constexpr DoubleDouble lowest() noexcept {
            return { -std::numeric_limits<double>::max(), std::numeric_limits<double>::epsilon() };
        }

        static constexpr DoubleDouble epsilon() noexcept {
            // ~2^-106, conservatively a single ulp of double-double
            return { 1.232595164407831e-32, 0.0 };
        }

        static constexpr DoubleDouble round_error() noexcept {
            return { 0.5, 0.0 };
        }

        static constexpr DoubleDouble infinity() noexcept {
            return { std::numeric_limits<double>::infinity(), 0.0 };
        }

        static constexpr DoubleDouble quiet_NaN() noexcept {
            return { std::numeric_limits<double>::quiet_NaN(), 0.0 };
        }

        static constexpr DoubleDouble signaling_NaN() noexcept {
            return { std::numeric_limits<double>::signaling_NaN(), 0.0 };
        }

        static constexpr DoubleDouble denorm_min() noexcept {
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
inline bool operator<(const DoubleDouble& a, const DoubleDouble& b)
{
    return (a.hi < b.hi) || (a.hi == b.hi && a.lo < b.lo);
}

inline bool operator<=(const DoubleDouble& a, const DoubleDouble& b)
{
    return (a < b) || (a.hi == b.hi && a.lo == b.lo);
}

inline bool operator>(const DoubleDouble& a, const DoubleDouble& b) { return b < a; }
inline bool operator>=(const DoubleDouble& a, const DoubleDouble& b) { return b <= a; }
inline bool operator==(const DoubleDouble& a, const DoubleDouble& b) { return a.hi == b.hi && a.lo == b.lo; }
inline bool operator!=(const DoubleDouble& a, const DoubleDouble& b) { return !(a == b); }

inline DoubleDouble log(const DoubleDouble& a)
{
    double log_hi = std::log(a.hi); // first guess
    DoubleDouble exp_log_hi(std::exp(log_hi)); // exp(guess)
    DoubleDouble r = (a - exp_log_hi) / exp_log_hi; // (a - e^g) / e^g ≈ error
    return DoubleDouble(log_hi) + r; // refined log
}

inline DoubleDouble log2(const DoubleDouble& a)
{
    constexpr DoubleDouble LOG2_RECIPROCAL(1.442695040888963407359924681001892137); // 1/log(2)
    return log(a) * LOG2_RECIPROCAL;
}

std::string to_string(const DoubleDouble& x);

std::ostream& operator<<(std::ostream& os, const DoubleDouble& x);

template<typename T>
struct Vec2
{
    union {
        struct { T x, y; };
        T _data[2]{ 0 };
    };

    Vec2() = default;
    Vec2(T _x, T _y) : x(_x), y(_y) {}

    [[nodiscard]] Vec2 operator-() const { return Vec2(-x, -y); }
    [[nodiscard]] bool operator==(const Vec2& other) const { return x == other.x && y == other.y; }
    [[nodiscard]] bool operator!=(const Vec2& other) const { return x != other.x || y != other.y; }
    [[nodiscard]] Vec2 operator+(const Vec2& rhs) const { return { x + rhs.x, y + rhs.y }; }
    [[nodiscard]] Vec2 operator-(const Vec2& rhs) const { return { x - rhs.x, y - rhs.y }; }
    [[nodiscard]] Vec2 operator*(const Vec2& rhs) const { return { x * rhs.x, y * rhs.y }; }
    [[nodiscard]] Vec2 operator/(const Vec2& rhs) const { return { x / rhs.x, y / rhs.y }; }
    [[nodiscard]] Vec2 operator+(T v) const { return { x + v, y + v }; }
    [[nodiscard]] Vec2 operator-(T v) const { return { x - v, y - v }; }
    [[nodiscard]] Vec2 operator*(T v) const { return { x * v, y * v }; }
    [[nodiscard]] Vec2 operator/(T v) const { return { x / v, y / v }; }
    
    [[nodiscard]] const T& operator[](int i) const { return _data[i]; }
    [[nodiscard]] T& operator[](int i) { return _data[i]; }

    [[nodiscard]] T  (&asArray())[2] { return _data; }
    [[nodiscard]] T  angle() const { return atan2(y, x); }
    [[nodiscard]] T  average() const { return (x + y) / T{ 2 }; }
    [[nodiscard]] T  magnitude() const { return sqrt(x * x + y * y); }
    [[nodiscard]] T  angleTo(const Vec2& b) const { return atan2(b.y - y, b.x - x); }

    [[nodiscard]] Vec2 floored(double offset = 0) { return { floor(x) + offset, floor(y) + offset }; }
    [[nodiscard]] Vec2 rounded(double offset = 0) { return { round(x) + offset, round(y) + offset }; }
    [[nodiscard]] Vec2 normalized() const {
        T mag = sqrt(x * x + y * y);
        return { x / mag, y / mag };
    }

    template<typename T2>
    inline operator Vec2<T2>()
    {
        return Vec2<T2>(
            static_cast<T2>(x),
            static_cast<T2>(y)
        );
    }

    [[nodiscard]] static Vec2 lerp(const Vec2& a, Vec2& b, T ratio)
    {
        return {
            (a.x + (b.x - a.x) * ratio),
            (a.y + (b.y - a.y) * ratio)
        };
    }
};

template<typename T> inline Vec2<T> operator*(T s, const Vec2<T>& v) { return v * s; }
template<typename T> inline Vec2<T> operator*(const Vec2<T>& v, T s) { return v * s; }
template<typename T> inline Vec2<T> operator+(T s, const Vec2<T>& v) { return v + s; }
template<typename T> inline Vec2<T> operator+(const Vec2<T>& v, T s) { return v + s; }
template<typename T> inline Vec2<T> operator-(T s, const Vec2<T>& v) { return v - s; }
template<typename T> inline Vec2<T> operator-(const Vec2<T>& v, T s) { return v - s; }

template<typename T>
struct Vec4
{
    T x = 0, y = 0, z = 0, w = 0;

    Vec4() = default;
    Vec4(T _x, T _y, T _z, T _w) : x(_x), y(_y), z(_z), w(_w) {}

    [[nodiscard]] Vec4 operator-() const { return Vec4(-x, -y, -z, -w); }
    [[nodiscard]] bool operator==(const Vec4& other) const { return x == other.x && y == other.y && z == other.z && w == other.w; }
    [[nodiscard]] bool operator!=(const Vec4& other) const { return x != other.x || y != other.y || z != other.z || w != other.w; }
    [[nodiscard]] Vec4 operator+(const Vec4& rhs) const { return { x + rhs.x, y + rhs.y, z + rhs.z, w + rhs.w }; }
    [[nodiscard]] Vec4 operator-(const Vec4& rhs) const { return { x - rhs.x, y - rhs.y, z - rhs.z, w - rhs.w }; }
    [[nodiscard]] Vec4 operator*(const Vec4& rhs) const { return { x * rhs.x, y * rhs.y, z * rhs.z, w * rhs.w }; }
    [[nodiscard]] Vec4 operator/(const Vec4& rhs) const { return { x / rhs.x, y / rhs.y, z / rhs.z, w / rhs.w }; }
    [[nodiscard]] Vec4 operator*(T v) const { return { x * v, y * v, z * v, w * v }; }
    [[nodiscard]] Vec4 operator/(T v) const { return { x / v, y / v, z / v, w / v }; }
    [[nodiscard]] T& operator[](int i) const { return (&x)[i]; }

    [[nodiscard]] T(&asArray())[4] { return *reinterpret_cast<T(*)[4]>(&x); }
    //[[nodiscard]] T  average() const { return (x + y) / T{ 2 }; }
    //[[nodiscard]] T  magnitude() const { return sqrt(x * x + y * y); }

    [[nodiscard]] Vec4 floored(double off = 0) { return { floor(x) + off, floor(y) + off, floor(z) + off, floor(w) + off };}
    [[nodiscard]] Vec4 rounded(double off = 0) { return { round(x) + off, round(y) + off, round(z) + off, round(w) + off }; }
    //[[nodiscard]] Vec2 normalized() const {
    //    T mag = sqrt(x * x + y * y);
    //    return { x / mag, y / mag };
    //}

    [[nodiscard]] static Vec4 lerp(const Vec4& a, Vec4& b, T ratio)
    {
        return {
            (a.x + (b.x - a.x) * ratio),
            (a.y + (b.y - a.y) * ratio),
            (a.z + (b.z - a.z) * ratio),
            (a.w + (b.w - a.w) * ratio)
        };
    }
};

template<typename T>
struct Ray : public Vec2<T>
{
    double angle;
    Ray() = default;
    Ray(double x, double y, T angle) : Vec2<T>(x, y), angle(angle) {}
    Ray(const Vec2<T>& p, T angle) : Vec2<T>(p.x, p.y), angle(angle) {}
    Ray(const Vec2<T>& a, const Vec2<T>& b) : Vec2<T>(a.x, a.y), angle(a.angleTo(b)) {}

    Vec2<T> project(T dist) const {
        return {
            Vec2<T>::x + cos(angle) * dist,
            Vec2<T>::y + sin(angle) * dist,
        };
    }
};

template <typename VecT>
struct Triangle 
{
    VecT* a, b, c;

    Triangle(VecT* p1, VecT* p2, VecT* p3)
    {
        // Calculate orientation
        double area = (p2->x - p1->x) * (p3->y - p1->y) - (p2->y - p1->y) * (p3->x - p1->x);

        // Store points in CCW order
        if (area < 0)
        {
            a = p1; b = p3; c = p2;
        }
        else
        {
            a = p1; b = p2; c = p3;
        }
    }

    [[nodiscard]] bool containsVertex(VecT* p) const {
        return (a == p || b == p || c == p);
    }

    // Returns true if 'p' lies in the circumcircle of the triangle
    [[nodiscard]] bool isPointInCircumcircle(const VecT& p) const
    {
        const double epsilon = 1e-10;
        double ax = a->x - p.x, ay = a->y - p.y;
        double bx = b->x - p.x, by = b->y - p.y;
        double cx = c->x - p.x, cy = c->y - p.y;

        double dA = ax * ax + ay * ay;
        double dB = bx * bx + by * by;
        double dC = cx * cx + cy * cy;

        double determinant =
            (ax * (by * dC - cy * dB)) -
            (ay * (bx * dC - cx * dB)) +
            (dA * (bx * cy - by * cx));

        return determinant >= -epsilon; // Changed to use epsilon
    }

    [[nodiscard]] bool operator==(const Triangle& other) const
    {
        // Make local copies
        VecT* p1 = a, p2 = b, p3 = c;
        VecT* q1 = other.a, q2 = other.b, q3 = other.c;

        // Sort in ascending order
        if (p2 < p1) { VecT* tmp = p1; p1 = p2; p2 = tmp; }
        if (p3 < p1) { VecT* tmp = p1; p1 = p3; p3 = tmp; }
        if (p3 < p2) { VecT* tmp = p2; p2 = p3; p3 = tmp; }

        if (q2 < q1) { VecT* tmp = q1; q1 = q2; q2 = tmp; }
        if (q3 < q1) { VecT* tmp = q1; q1 = q3; q3 = tmp; }
        if (q3 < q2) { VecT* tmp = q2; q2 = q3; q3 = tmp; }

        // Compare sorted pointers
        return (p1 == q1 && p2 == q2 && p3 == q3);
    }
};

template <typename VecT>
struct TriangleHash
{
    std::size_t operator()(const Triangle<VecT>& tri) const noexcept
    {
        size_t h1 = reinterpret_cast<size_t>(tri.a);
        size_t h2 = reinterpret_cast<size_t>(tri.b);
        size_t h3 = reinterpret_cast<size_t>(tri.c);

        // Murmur-inspired hash mixing for stronger hash distribution
        h1 ^= (h2 + 0x9e3779b97f4a7c15ULL + (h1 << 6) + (h1 >> 2));
        h1 ^= (h3 + 0x9e3779b97f4a7c15ULL + (h1 << 6) + (h1 >> 2));
        return h1;
    }
};

template <typename VecT>
struct TriangleEqual
{
    bool operator()(const Triangle<VecT>& lhs, const Triangle<VecT>& rhs) const {
        return lhs == rhs;
    }
};

struct Color
{
    union {
        struct { uint8_t r, g, b, a; };
        uint32_t u32 = 0;
    };

    Color() = default;
    Color(uint32_t rgba) : u32(rgba) {}
    Color(const float(&c)[3]) : r(int(c[0]* 255.f)), g(int(c[1] * 255.f)), b(int(c[2] * 255.f)), a(255) {}
    Color(const float(&c)[4]) : r(int(c[0]* 255.f)), g(int(c[1] * 255.f)), b(int(c[2] * 255.f)), a(int(c[3] * 255.f)) {}
    Color(uint8_t _r, uint8_t _g, uint8_t _b, uint8_t _a=255) 
        : r(_r), g(_g), b(_b), a(_a) {}

    Color& operator =(const Color& rhs) { u32 = rhs.u32; return *this; }
    [[nodiscard]] bool operator ==(const Color& rhs) const { return u32 == rhs.u32; }

    operator uint32_t() const { return u32; }
    Vec4<float> vec4() { return { (float)r/255.0f, (float)g/255.0f, (float)b/255.0f, (float)a/255.0f }; }

    Color& adjustHue(float amount)
    {
        setHue(getHue() + amount);
        return *this;
    }

    // Return hue in degrees [0,360).
    [[nodiscard]] float getHue() const
    {
        float rf = r / 255.0f;
        float gf = g / 255.0f;
        float bf = b / 255.0f;

        float maxc = std::max({ rf, gf, bf });
        float minc = std::min({ rf, gf, bf });
        float delta = maxc - minc;

        if (delta < 1e-6f) return 0.0f;

        float hue;
        if (maxc == rf)
            hue = 60.f * std::fmod((gf - bf) / delta, 6.f);
        else if (maxc == gf)
            hue = 60.f * (((bf - rf) / delta) + 2.f);
        else
            hue = 60.f * (((rf - gf) / delta) + 4.f);

        if (hue < 0.f) hue += 360.f;
        return hue;
    }

    // Rotate to given hue (degrees). Keeps original S and V.
    void setHue(float hue)
    {
        // normalise hue to [0,360)
        hue = std::fmod(hue, 360.f);
        if (hue < 0.f) hue += 360.f;

        float rf = r / 255.f;
        float gf = g / 255.f;
        float bf = b / 255.f;

        float maxc = std::max({ rf, gf, bf });
        float minc = std::min({ rf, gf, bf });
        float delta = maxc - minc;

        float v = maxc;
        float s = (maxc == 0.f) ? 0.f : delta / maxc;

        // HSV -> RGB with new hue
        float c = v * s;
        float hprime = hue / 60.f;
        float x = c * (1.f - std::fabs(std::fmod(hprime, 2.f) - 1.f));

        float r1, g1, b1;
        if (hprime < 1.f) { r1 = c; g1 = x; b1 = 0.f; }
        else if (hprime < 2.f) { r1 = x; g1 = c; b1 = 0.f; }
        else if (hprime < 3.f) { r1 = 0.f; g1 = c; b1 = x; }
        else if (hprime < 4.f) { r1 = 0.f; g1 = x; b1 = c; }
        else if (hprime < 5.f) { r1 = x; g1 = 0.f; b1 = c; }
        else { r1 = c; g1 = 0.f; b1 = x; }

        float m = v - c;
        r = uint8_t(std::round((r1 + m) * 255.f));
        g = uint8_t(std::round((g1 + m) * 255.f));
        b = uint8_t(std::round((b1 + m) * 255.f));
    }
};

template<typename T>
struct Rect
{
    union { struct { T x1, y1; }; Vec2<T> a; };
    union { struct { T x2, y2; }; Vec2<T> b; };

    //Rect() = default;
    Rect() : a(0, 0), b(0, 0) {}
    Rect(T _x1, T _y1, T _x2, T _y2)           { set(_x1, _y1, _x2, _y2); }
    Rect(const Vec2<T>& _a, const Vec2<T>& _b) { set(_a, _b); }

    void set(const Rect& r)                      { memcpy(this, &r, sizeof(Rect)); }
    void set(T _x1, T _y1, T _x2, T _y2)         { x1 = _x1; y1 = _y1; x2 = _x2; y2 = _y2; }
    void set(const Vec2<T>& a, const Vec2<T>& b) { x1 = a.x; y1 = a.y; x2 = b.x; y2 = b.y; }

    [[nodiscard]] double  width()  { return x2 - x1; }
    [[nodiscard]] double  height() { return y2 - y1; }
    [[nodiscard]] Vec2<T> size()   { return { x2 - x1, y2 - y1 }; }

    [[nodiscard]] bool hitTest(T x, T y) { return (x >= x1 && y >= y1 && x <= x2 && y <= y2); }
    [[nodiscard]] Rect scaled(T mult) {
        T cx = (x1 + x2) / 2;
        T cy = (y1 + y2) / 2;
        x1 = cx + (x1 - cx) * mult;
        y1 = cy + (y1 - cy) * mult;
        x2 = cx + (x2 - cx) * mult;
        y2 = cy + (y2 - cy) * mult;
        return Rect(x1, y1, x2, y2);
    }

    void merge(const Rect& r)
    {
        if (r.x1 < x1) x1 = r.x1;
        if (r.y1 < y1) y1 = r.y1;
        if (r.x2 > x2) x2 = r.x2;
        if (r.y2 > y2) y2 = r.y2;
    }
};

template<typename T> struct Quad;
template<typename T> struct AngledRect;

template<typename T>
Vec2<T> enclosingSize(const AngledRect<T>& A, const AngledRect<T>& B, T avgAngle, T fixedAspectRatio = 0)
{
    // rotate world points by –avgAngle so the enclosing frame is axis-aligned
    const T cosC = std::cos(-avgAngle);
    const T sinC = std::sin(-avgAngle);

    T xmin =  std::numeric_limits<T>::infinity(), ymin =  std::numeric_limits<T>::infinity();
    T xmax = -std::numeric_limits<T>::infinity(), ymax = -std::numeric_limits<T>::infinity();

    auto accumulate = [&](const AngledRect<T>& R)
    {
        const T d = R.angle - avgAngle; // local tilt inside the frame
        const T cd = std::cos(d);
        const T sd = std::sin(d);

        // centre in the avgAngle frame
        const T cxp = cosC * R.cx - sinC * R.cy;
        const T cyp = sinC * R.cx + cosC * R.cy;

        const T hw = R.w * 0.5;
        const T hh = R.h * 0.5;

        for (int sx = -1; sx <= 1; sx += 2)
        {
            for (int sy = -1; sy <= 1; sy += 2)
            {
                const T dx = sx * hw;
                const T dy = sy * hh;

                // rotate offset by d, then add to centre
                const T x = cxp + dx * cd - dy * sd;
                const T y = cyp + dx * sd + dy * cd;

                xmin = std::min(xmin, x);
                xmax = std::max(xmax, x);
                ymin = std::min(ymin, y);
                ymax = std::max(ymax, y);
            }
        }
    };

    accumulate(A);
    accumulate(B);

    T W, H;
    if (fixedAspectRatio > 0)
    {
        const T Wraw = xmax - xmin;
        const T Hraw = ymax - ymin;

        if (Wraw / Hraw >= fixedAspectRatio)
        {
            W = Wraw;
            H = W / fixedAspectRatio;
        }
        else
        {
            H = Hraw;
            W = fixedAspectRatio * H;
        }
    }
    else
    {
        W = xmax - xmin;
        H = ymax - ymin;
    }

    return { W, H };
}

template<typename T>
struct AngledRect
{
    union { struct { T cx, cy; }; Vec2<T> cen; };
    union { struct { T w, h; }; Vec2<T> size; };
    T angle;

    AngledRect() {}
    //AngledRect(T _cx, T _cy, T _w, T _h, T _angle)
    //{
    //    cx = _cx;
    //    cy = _cy;
    //
    //}

    AngledRect(T _cx, T _cy, T _w, T _h, T _angle) :
        cx(_cx), cy(_cy), w(_w), h(_h), angle(_angle) {}

    [[nodiscard]] Quad<T> toQuad() { return Quad<T>(*this); }
    operator Quad<T>() const { return Quad<T>(*this); }

    T aspectRatio() const
    {
        return (w / h);
    }

    void fitTo(AngledRect a, AngledRect b, double fixed_aspect_ratio=0)
    {
        cx = (a.cx + b.cx) / 2;
        cy = (a.cy + b.cy) / 2;
        angle = Math::avgAngle(a.angle, b.angle);
        size = enclosingSize(a, b, angle, fixed_aspect_ratio);
    }
};

template<typename T>
struct Quad
{
    using Pt = Vec2<T>;
    Pt a, b, c, d;

    Quad() = default;
    Quad(const Pt& A, const Pt& B, const Pt& C, const Pt& D) : a(A), b(B), c(C), d(D) {}
    Quad(const AngledRect<T> r)         { set(r.cx, r.cy, r.w, r.h, r.angle); }
    Quad(T cx, T cy, T w, T h, T angle) { set(cx, cy, w, h, angle); }

    template<typename T2>
    [[nodiscard]] inline operator Quad<T2>()
    {
        return Quad<T2>(
            static_cast<Vec2<T2>>(a),
            static_cast<Vec2<T2>>(b),
            static_cast<Vec2<T2>>(c),
            static_cast<Vec2<T2>>(d)
        );
    }

    void set(T cx, T cy, T w, T h, T angle)
    {
        T w2 = w / 2, h2 = h / 2, _cos = cos(angle), _sin = sin(angle);
        a = {cx + ((+w2) * _cos - (+h2) * _sin), cy + ((+h2) * _cos + (+w2) * _sin)};
        b = {cx + ((-w2) * _cos - (+h2) * _sin), cy + ((+h2) * _cos + (-w2) * _sin)};
        c = {cx + ((-w2) * _cos - (-h2) * _sin), cy + ((-h2) * _cos + (-w2) * _sin)};
        d = {cx + ((+w2) * _cos - (-h2) * _sin), cy + ((-h2) * _cos + (+w2) * _sin)};
    }

    [[nodiscard]] bool operator ==(const Quad& rhs) { return (a == rhs.a && b == rhs.b && c == rhs.c && d == rhs.d); }
    [[nodiscard]] bool operator !=(const Quad& rhs) { return (a != rhs.a || b != rhs.b || c != rhs.c || d != rhs.d); }

    //[[nodiscard]] AngledRect<T> toAngledRect() const;
};

typedef Vec2<float>         FVec2;
typedef Vec2<double>        DVec2;
typedef Vec2<int>           IVec2;
typedef Vec4<float>         FVec4;
typedef Rect<float>         FRect;
typedef Rect<double>        DRect;
typedef Rect<int>           IRect;
typedef Quad<float>         FQuad;
typedef Quad<double>        DQuad;
typedef Quad<int>           IQuad;
typedef Ray<float>          FRay;
typedef Ray<double>         DRay;

typedef AngledRect<double>  DAngledRect;

struct MassForceParticle : public DVec2
{
    double r;
    double fx;
    double fy;
    double vx;
    double vy;
    double mass;
};

template<typename T>
std::ostream& operator<<(std::ostream& os, const Vec2<T>& vec) {
    return os << "(" << vec.x << ", " << vec.y << ")";
}

template<typename T>
std::ostream& operator<<(std::ostream& os, const Quad<T>& q) {
    return os << "{" << q.a << ", " << q.b << ", " << q.c << ", " << q.d << "}";
}

template<typename T>
std::ostream& operator<<(std::ostream& os, const AngledRect<T>& r) {
    return os << "{cx: " << r.cx << ", cy: " << r.cy << ", w: " << r.w << ", h: " << r.h << "rot: " << r.angle << "}";
}