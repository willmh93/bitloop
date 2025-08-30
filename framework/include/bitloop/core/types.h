#pragma once
#include <functional>
#include <string>
#include <sstream>
#include <iomanip>
#include <cmath>
#include <limits>

#include <bitloop/utility/flt128.h>

#define GLM_ENABLE_EXPERIMENTAL
#define GLM_FORCE_UNRESTRICTED_GENTYPE
#define GLM_FORCE_CTOR_INIT
#include "glm/glm.hpp"
#include "glm/gtx/transform2.hpp"

#define BL_BEGIN_NS namespace BL {
#define BL_END_NS   }

namespace glm {
    typedef mat<3, 3, flt128, glm::defaultp>	ddmat3;
    typedef vec<2, flt128, defaultp>		    ddvec2;
    typedef vec<3, flt128, defaultp>		    ddvec3;
}

BL_BEGIN_NS

template<typename T>
concept is_floating_point_v =
    std::is_floating_point_v<T> ||
    std::same_as<T, flt128>;

template<class T>
concept is_arithmetic_v = 
    std::is_arithmetic_v<T> || 
	std::is_same_v<T, flt128>;

template<typename T>
concept is_integral_v = std::is_integral_v<T>;

class ProjectBase;
using ProjectCreatorFunc = std::function<ProjectBase*()>;

namespace Math {
    template<typename T>
    T avgAngle(T a, T b);
}

// Enums

enum struct Anchor
{
    TOP_LEFT,     TOP,     TOP_RIGHT,
    LEFT,         CENTER,  RIGHT,
    BOTTOM_LEFT,  BOTTOM,  BOTTOM_RIGHT
};

template<typename T> struct GlmVec2Type;
template<typename T> struct GlmVec3Type;
template<> struct GlmVec2Type<float>        { using type = glm::vec2; };
template<> struct GlmVec2Type<double>       { using type = glm::dvec2; };
template<> struct GlmVec2Type<flt128>       { using type = glm::ddvec2; };
template<> struct GlmVec2Type<int>          { using type = glm::ivec2; };
template<> struct GlmVec3Type<float>        { using type = glm::vec3; };
template<> struct GlmVec3Type<double>       { using type = glm::dvec3; };
template<> struct GlmVec3Type<flt128>       { using type = glm::ddvec3; };
template<> struct GlmVec3Type<int>          { using type = glm::ivec3; };
template<typename T> using GlmVec2 = typename GlmVec2Type<T>::type;
template<typename T> using GlmVec3 = typename GlmVec3Type<T>::type;

template<typename T>
struct Vec2
{
    union {
        struct { T x, y; };
        T _data[2]{ 0 };
    };

    // constructors
    Vec2() = default;
    constexpr Vec2(T _x, T _y) : x(_x), y(_y) {}

    constexpr Vec2(const GlmVec2<T>& rhs) : x(rhs.x), y(rhs.y) {}
    constexpr explicit Vec2(const GlmVec3<T>& rhs) : x(rhs.x), y(rhs.y) {} // discards z for 2D

    // conversions
    template<typename T2> constexpr operator Vec2<T2>()    const { return Vec2<T2>{(T2)x, (T2)y}; }
    template<typename T2> constexpr operator GlmVec2<T2>() const { return GlmVec2<T2>{(T2)x, (T2)y}; }

    // access
    [[nodiscard]] constexpr const T& operator[](int i) const { return _data[i]; }
    [[nodiscard]] constexpr T& operator[](int i) { return _data[i]; }
    [[nodiscard]] constexpr T (&asArray())[2] { return _data; }

    // arithmetic
    [[nodiscard]] constexpr Vec2 operator-() const { return Vec2(-x, -y); }
    [[nodiscard]] constexpr bool operator==(const Vec2& other) const { return x == other.x && y == other.y; }
    [[nodiscard]] constexpr bool operator!=(const Vec2& other) const { return x != other.x || y != other.y; }
    [[nodiscard]] constexpr Vec2 operator+(const Vec2& rhs) const { return { x + rhs.x, y + rhs.y }; }
    [[nodiscard]] constexpr Vec2 operator-(const Vec2& rhs) const { return { x - rhs.x, y - rhs.y }; }
    [[nodiscard]] constexpr Vec2 operator*(const Vec2& rhs) const { return { x * rhs.x, y * rhs.y }; }
    [[nodiscard]] constexpr Vec2 operator/(const Vec2& rhs) const { return { x / rhs.x, y / rhs.y }; }
    [[nodiscard]] constexpr Vec2 operator+(T v) const { return { x + v, y + v }; }
    [[nodiscard]] constexpr Vec2 operator-(T v) const { return { x - v, y - v }; }
    [[nodiscard]] constexpr Vec2 operator*(T v) const { return { x * v, y * v }; }
    [[nodiscard]] constexpr Vec2 operator/(T v) const { return { x / v, y / v }; }
    
    // properties
    [[nodiscard]] constexpr T angle() const { return atan2(y, x); }
    [[nodiscard]] constexpr T average() const { return (x + y) / T{ 2 }; }
    [[nodiscard]] constexpr T magnitude() const { return sqrt(x * x + y * y); }
    [[nodiscard]] constexpr T dot(const Vec2& other) const { return x * other.x + y * other.y; }
    [[nodiscard]] constexpr T angleTo(const Vec2& b) const { return atan2(b.y - y, b.x - x); }

    // operators
    [[nodiscard]] constexpr Vec2 floored() const { return { floor(x), floor(y) }; }
    [[nodiscard]] constexpr Vec2 rounded() const { return { round(x), round(y) }; }
    [[nodiscard]] constexpr Vec2 floored(double offset) const { return { floor(x) + offset, floor(y) + offset }; }
    [[nodiscard]] constexpr Vec2 rounded(double offset) const { return { round(x) + offset, round(y) + offset }; }
    [[nodiscard]] constexpr Vec2 normalized() const { return (*this) / magnitude(); }

    // static
    [[nodiscard]] static constexpr Vec2 lerp(const Vec2& a, const Vec2& b, T ratio) { return a + (b - a) * ratio; }
};

// global arithmetic
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
    constexpr  Vec4(T _x, T _y, T _z, T _w) : x(_x), y(_y), z(_z), w(_w) {}

    [[nodiscard]] constexpr Vec4 operator-() const { return Vec4(-x, -y, -z, -w); }
    [[nodiscard]] constexpr bool operator==(const Vec4& other) const { return x == other.x && y == other.y && z == other.z && w == other.w; }
    [[nodiscard]] constexpr bool operator!=(const Vec4& other) const { return x != other.x || y != other.y || z != other.z || w != other.w; }
    [[nodiscard]] constexpr Vec4 operator+(const Vec4& rhs) const { return { x + rhs.x, y + rhs.y, z + rhs.z, w + rhs.w }; }
    [[nodiscard]] constexpr Vec4 operator-(const Vec4& rhs) const { return { x - rhs.x, y - rhs.y, z - rhs.z, w - rhs.w }; }
    [[nodiscard]] constexpr Vec4 operator*(const Vec4& rhs) const { return { x * rhs.x, y * rhs.y, z * rhs.z, w * rhs.w }; }
    [[nodiscard]] constexpr Vec4 operator/(const Vec4& rhs) const { return { x / rhs.x, y / rhs.y, z / rhs.z, w / rhs.w }; }
    [[nodiscard]] constexpr Vec4 operator*(T v) const { return { x * v, y * v, z * v, w * v }; }
    [[nodiscard]] constexpr Vec4 operator/(T v) const { return { x / v, y / v, z / v, w / v }; }
    [[nodiscard]] constexpr T& operator[](int i) const { return (&x)[i]; }

    [[nodiscard]] constexpr T(&asArray())[4] { return *reinterpret_cast<T(*)[4]>(&x); }
    //[[nodiscard]] T  average() const { return (x + y) / T{ 2 }; }
    //[[nodiscard]] T  magnitude() const { return sqrt(x * x + y * y); }

    [[nodiscard]] constexpr Vec4 floored(double off = 0) { return { floor(x) + off, floor(y) + off, floor(z) + off, floor(w) + off };}
    [[nodiscard]] constexpr Vec4 rounded(double off = 0) { return { round(x) + off, round(y) + off, round(z) + off, round(w) + off }; }
    //[[nodiscard]] Vec2 normalized() const {
    //    T mag = sqrt(x * x + y * y);
    //    return { x / mag, y / mag };
    //}

    [[nodiscard]] static constexpr Vec4 lerp(const Vec4& a, const Vec4& b, T ratio)
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
    constexpr Ray(double x, double y, T angle) : Vec2<T>(x, y), angle(angle) {}
    constexpr Ray(const Vec2<T>& p, T angle) : Vec2<T>(p.x, p.y), angle(angle) {}
    constexpr Ray(const Vec2<T>& a, const Vec2<T>& b) : Vec2<T>(a.x, a.y), angle(a.angleTo(b)) {}

    [[nodiscard]] constexpr Vec2<T> project(T dist) const {
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
        if (area < 0) { a = p1; b = p3; c = p2; }
        else          { a = p1; b = p2; c = p3; }
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

template<typename T> struct Quad;

template<typename T>
struct Rect
{
    union { struct { T x1, y1; }; Vec2<T> a; };
    union { struct { T x2, y2; }; Vec2<T> b; };

    Rect() : a(0, 0), b(0, 0) {}
    constexpr Rect(T _x1, T _y1, T _x2, T _y2) { set(_x1, _y1, _x2, _y2); }
    constexpr Rect(const Vec2<T>& _a, const Vec2<T>& _b) { set(_a, _b); }

    constexpr operator Quad<T>() const { return Quad<T>(x1, y1, x2, y1, x2, y2, x1, y2); }

    constexpr void set(const Rect& r) { a = r.a; b = r.b; }
    constexpr void set(const Vec2<T>& A, const Vec2<T>& B) { a = A; b = B; }
    constexpr void set(T _x1, T _y1, T _x2, T _y2) { x1 = _x1; y1 = _y1; x2 = _x2; y2 = _y2; }

    [[nodiscard]] constexpr double  width() const { return x2 - x1; }
    [[nodiscard]] constexpr double  height() const { return y2 - y1; }
    [[nodiscard]] constexpr Vec2<T> size() const { return { x2 - x1, y2 - y1 }; }

    [[nodiscard]] constexpr double  cx() const { return (x1 + x2) / 2; }
    [[nodiscard]] constexpr double  cy() const { return (y1 + y2) / 2; }
    [[nodiscard]] constexpr Vec2<T> center() const { return { (x1 + x2) / 2, (y1 + y2) / 2 }; }

    [[nodiscard]] constexpr bool hitTest(T x, T y) const { return (x >= x1 && y >= y1 && x <= x2 && y <= y2); }
    [[nodiscard]] constexpr Rect scaled(T mult) const {
        T cx = (x1 + x2) / 2;
        T cy = (y1 + y2) / 2;
        T r_x1 = cx + (x1 - cx) * mult;
        T r_y1 = cy + (y1 - cy) * mult;
        T r_x2 = cx + (x2 - cx) * mult;
        T r_y2 = cy + (y2 - cy) * mult;
        return Rect(r_x1, r_y1, r_x2, r_y2);
    }

    void merge(const Rect& r)
    {
        if (r.x1 < x1) x1 = r.x1;
        if (r.y1 < y1) y1 = r.y1;
        if (r.x2 > x2) x2 = r.x2;
        if (r.y2 > y2) y2 = r.y2;
    }

    //static constexpr Rect<T> full() { return Rect<T>(-std::numeric_limits<T>(), -std::numeric_limits<T>(), std::numeric_limits<T>(), std::numeric_limits<T>()); }
    static constexpr Rect<T> infinite() { return Rect<T>(-bl_infinity<T>(), -bl_infinity<T>(), bl_infinity<T>(), bl_infinity<T>()); }
};

template<typename T> struct Quad;
template<typename T> struct AngledRect;

template<typename T>
Vec2<T> enclosingSize(const AngledRect<T>& A, const AngledRect<T>& B, T avgAngle, T fixedAspectRatio = 0)
{
    // rotate world points by –avgAngle so the enclosing frame is axis-aligned
    const T cosC = cos(-avgAngle);
    const T sinC = sin(-avgAngle);

    T xmin =  bl_infinity<T>(), ymin =  bl_infinity<T>();
    T xmax = -bl_infinity<T>(), ymax = -bl_infinity<T>();

    auto accumulate = [&](const AngledRect<T>& R)
    {
        const T d = R.angle - avgAngle; // local tilt inside the frame
        const T cd = cos(d);
        const T sd = sin(d);

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

    [[nodiscard]] Quad<T> toQuad() const { return Quad<T>(*this); }
    operator Quad<T>() const { return Quad<T>(*this); }

    T aspectRatio() const
    {
        return (w / h);
    }

    void fitTo(AngledRect a, AngledRect b, T fixed_aspect_ratio=0)
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
    union
    {
        struct { Pt a, b, c, d; };
        Pt _data[4];
    };

    Quad() : a(0, 0), b(0, 0), c(0, 0), d(0, 0) {}
    constexpr Quad(T ax, T ay, T bx, T by, T cx, T cy, T dx, T dy) : a(ax,ay), b(bx, by), c(cx, cy), d(dx, dy) {}
    constexpr Quad(const Pt& A, const Pt& B, const Pt& C, const Pt& D) : a(A), b(B), c(C), d(D) {}
    constexpr Quad(const Rect<T>& r)              { set(r.cx(), r.cy(), r.width(), r.height(), 0); }
    constexpr Quad(const AngledRect<T>& r)        { set(r.cx, r.cy, r.w, r.h, r.angle); }
    constexpr Quad(T cx, T cy, T w, T h, T angle) { set(cx, cy, w, h, angle); }

    template<typename T2>
    [[nodiscard]] constexpr operator Quad<T2>()
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

    [[nodiscard]] constexpr bool operator ==(const Quad& rhs) { return (a == rhs.a && b == rhs.b && c == rhs.c && d == rhs.d); }
    [[nodiscard]] constexpr bool operator !=(const Quad& rhs) { return (a != rhs.a || b != rhs.b || c != rhs.c || d != rhs.d); }

    [[nodiscard]] Vec2<T> center() const { return (a + b + c + d) / T(4); }
    [[nodiscard]] T area() const
    {
        // shoelace formula
        return std::abs((a.x * b.y + b.x * c.y + c.x * d.y + d.x * a.y) - (a.y * b.x + b.y * c.x + c.y * d.x + d.y * a.x)) / 2;
    }
    [[nodiscard]] Rect<T> boundingRect() const
    {
        T min_x = std::min({ a.x, b.x, c.x, d.x });
        T max_x = std::max({ a.x, b.x, c.x, d.x });
        T min_y = std::min({ a.y, b.y, c.y, d.y });
        T max_y = std::max({ a.y, b.y, c.y, d.y });
        return Rect<T>(min_x, min_y, max_x, max_y);
    }

    //[[nodiscard]] AngledRect<T> toAngledRect() const;
};

typedef Vec2<float>         FVec2;
typedef Vec2<double>        DVec2;
typedef Vec2<flt128>      DDVec2;
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
typedef AngledRect<flt128>  DDAngledRect;

struct MassForceParticle : public DVec2
{
    double r;
    double fx;
    double fy;
    double vx;
    double vy;
    double mass;
};

BL_END_NS

template<typename T>
std::ostream& operator<<(std::ostream& os, const BL::Vec2<T>& vec) {
    return os << "(" << vec.x << ", " << vec.y << ")";
}

template<typename T>
std::ostream& operator<<(std::ostream& os, const BL::Quad<T>& q) {
    return os << "{" << q.a << ", " << q.b << ", " << q.c << ", " << q.d << "}";
}

template<typename T>
std::ostream& operator<<(std::ostream& os, const BL::AngledRect<T>& r) {
    return os << "{cx: " << r.cx << ", cy: " << r.cy << ", w: " << r.w << ", h: " << r.h << "rot: " << r.angle << "}";
}

