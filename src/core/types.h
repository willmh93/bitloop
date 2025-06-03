#pragma once
#include <functional>
#include <string>
#include <sstream>

class ProjectBase;
using ProjectCreatorFunc = std::function<ProjectBase*()>;

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

template<typename T>
struct Vec2
{
    T x = 0, y = 0;

    Vec2() = default;
    Vec2(T _x, T _y) : x(_x), y(_y) {}

    [[nodiscard]] Vec2 operator-() const { return Vec2(-x, -y); }
    [[nodiscard]] bool operator==(const Vec2& other) const { return x == other.x && y == other.y; }
    [[nodiscard]] bool operator!=(const Vec2& other) const { return x != other.x || y != other.y; }
    [[nodiscard]] Vec2 operator+(const Vec2& rhs) const { return { x + rhs.x, y + rhs.y }; }
    [[nodiscard]] Vec2 operator-(const Vec2& rhs) const { return { x - rhs.x, y - rhs.y }; }
    [[nodiscard]] Vec2 operator*(const Vec2& rhs) const { return { x * rhs.x, y * rhs.y }; }
    [[nodiscard]] Vec2 operator/(const Vec2& rhs) const { return { x / rhs.x, y / rhs.y }; }
    [[nodiscard]] Vec2 operator*(T v) const { return { x * v, y * v }; }
    [[nodiscard]] Vec2 operator/(T v) const { return { x / v, y / v }; }
    [[nodiscard]] T&   operator[](int i) const { return (&x)[i]; }

    [[nodiscard]] T  (&asArray())[2] { return *reinterpret_cast<T(*)[2]>(&x); }
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

    [[nodiscard]] static Vec2 lerp(const Vec2& a, Vec2& b, T ratio)
    {
        return {
            (a.x + (b.x - a.x) * ratio),
            (a.y + (b.y - a.y) * ratio)
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

    operator uint32_t() const {
        return u32;
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

template<typename T>
struct Quad
{
    Vec2<T> a, b, c, d;
    [[nodiscard]] bool operator ==(const Quad& rhs) { return (a == rhs.a && b == rhs.b && c == rhs.c && d == rhs.d); }
    [[nodiscard]] bool operator !=(const Quad& rhs) { return (a != rhs.a || b != rhs.b || c != rhs.c || d != rhs.d); }
};

typedef Vec2<float>  FVec2;
typedef Vec2<double> DVec2;
typedef Vec2<int>    IVec2;
typedef Rect<float>  FRect;
typedef Rect<double> DRect;
typedef Rect<int>    IRect;
typedef Quad<float>  FQuad;
typedef Quad<double> DQuad;
typedef Quad<int>    IQuad;
typedef Ray<float>   FRay;
typedef Ray<double>  DRay;


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