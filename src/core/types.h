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
typedef Vec4<float>  FVec4;
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