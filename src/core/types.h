#pragma once
#include <functional>
#include <string>
//#include "imgui_custom.h"

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
    enum State
    {
        INACTIVE,
        ACTIVE,
        RECORDING
    };

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
    T x = 0;
    T y = 0;

    Vec2() = default;
    Vec2(T _x, T _y) : x(_x), y(_y) {}

    //operator ImVec2() const 
    //{
    //    return ImVec2(static_cast<float>(x), static_cast<float>(y));
    //}

    Vec2 operator-() const { return Vec2(-x, -y); }
    bool operator==(const Vec2& other) const { return x == other.x && y == other.y; }
    bool operator!=(const Vec2& other) const { return x != other.x || y != other.y; }
    Vec2 operator+(const Vec2& rhs) const { return { x + rhs.x, y + rhs.y }; }
    Vec2 operator-(const Vec2& rhs) const { return { x - rhs.x, y - rhs.y }; }
    Vec2 operator*(const Vec2& rhs) const { return { x * rhs.x, y * rhs.y }; }
    Vec2 operator/(const Vec2& rhs) const { return { x / rhs.x, y / rhs.y }; }
    Vec2 operator*(T v) const { return { x * v, y * v }; }
    Vec2 operator/(T v) const { return { x / v, y / v }; }

    double* asArray() { return &x; }
    double  angle() const { return atan2(y, x); }
    double  average() const { return (x + y) / 2; }
    double  magnitude() const { return sqrt(x * x + y * y); }
    double  angleTo(const Vec2& b) const { return atan2(b.y - y, b.x - x); }

    Vec2 floored(double offset = 0) { return { floor(x) + offset, floor(y) + offset }; }
    Vec2 rounded(double offset = 0) { return { round(x) + offset, round(y) + offset }; }
    Vec2 normalized() const {
        T mag = sqrt(x * x + y * y);
        return { x / mag, y / mag };
    }

    static Vec2 lerp(const Vec2& a, Vec2& b, T ratio)
    {
        return {
            (a.x + (b.x - a.x) * ratio),
            (a.y + (b.y - a.y) * ratio)
        };
    }
};


/*struct DVec2
{
    double x = 0.0;
    double y = 0.0;

    DVec2() = default;
    DVec2(double _x, double _y) : x(_x), y(_y) {}

    DVec2 operator-() const { return DVec2(-x, -y); }
    bool operator==(const DVec2& other) const { return x == other.x && y == other.y; }
    bool operator!=(const DVec2& other) const { return x != other.x || y != other.y; }
    DVec2 operator+(const DVec2& rhs) const { return { x + rhs.x, y + rhs.y }; }
    DVec2 operator-(const DVec2& rhs) const { return { x - rhs.x, y - rhs.y }; }
    DVec2 operator*(const DVec2& rhs) const { return { x * rhs.x, y * rhs.y }; }
    DVec2 operator/(const DVec2& rhs) const { return { x / rhs.x, y / rhs.y }; }
    DVec2 operator*(double v) const { return { x * v, y * v }; }
    DVec2 operator/(double v) const { return { x / v, y / v }; }

    double* asArray() { return &x; }
    double angle() const { return atan2(y, x); }
    double average() const { return (x + y) / 2.0; }
    double magnitude() const { return sqrt(x * x + y * y); }
    double angleTo(const DVec2& b) const { return atan2(b.y - y, b.x - x); }

    DVec2 floored(double offset = 0) { return { floor(x) + offset, floor(y) + offset }; }
    DVec2 rounded(double offset = 0) { return { round(x) + offset, round(y) + offset }; }
    DVec2 normalized() const {
        double mag = sqrt(x * x + y * y);
        return { x / mag, y / mag };
    }

    static DVec2 lerp(const DVec2& a, const DVec2& b, double ratio)
    {
        return {
            (a.x + (b.x - a.x) * ratio),
            (a.y + (b.y - a.y) * ratio)
        };
    }
};*/

template<typename T>
struct Ray : public Vec2<T>
{
    double angle;
    Ray() = default;
    Ray(double x, double y, T angle) : Vec2<T>(x, y), angle(angle) {}
    Ray(const Vec2<T>& p, T angle) : Vec2<T>(p.x, p.y), angle(angle) {}
    Ray(const Vec2<T>& a, const Vec2<T>& b) : Vec2<T>(a.x, a.y), angle(a.angleTo(b)) {}
};

// Triangle structure
template <typename VecT>
struct Triangle {
    VecT* a;
    VecT* b;
    VecT* c;

    Triangle(VecT* p1, VecT* p2, VecT* p3)
    {
        // Calculate orientation
        double area = (p2->x - p1->x) * (p3->y - p1->y) -
            (p2->y - p1->y) * (p3->x - p1->x);

        // Store points in CCW order
        if (area < 0) { // If CW, swap two points
            a = p1; b = p3; c = p2;
        }
        else { // CCW or colinear
            a = p1; b = p2; c = p3;
        }
    }

    bool containsVertex(VecT* p) const
    {
        return (a == p || b == p || c == p);
    }

    // Returns true if 'p' lies in the circumcircle of the triangle
    bool isPointInCircumcircle(const VecT& p) const
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

    bool operator==(const Triangle& other) const
    {
        // Seems to work without sorting
        //return (a == other.a && b == other.b && c == other.c);

        // Make local copies
        VecT* p1 = a;
        VecT* p2 = b;
        VecT* p3 = c;

        VecT* q1 = other.a;
        VecT* q2 = other.b;
        VecT* q3 = other.c;

        // --- Sort (p1, p2, p3) in ascending order ---
        // If p2 < p1, swap them
        if (p2 < p1) {
            VecT* tmp = p1;
            p1 = p2;
            p2 = tmp;
        }
        // If p3 < p1, swap them
        if (p3 < p1) {
            VecT* tmp = p1;
            p1 = p3;
            p3 = tmp;
        }
        // Now p1 is the smallest; fix the order of (p2,p3).
        if (p3 < p2) {
            VecT* tmp = p2;
            p2 = p3;
            p3 = tmp;
        }

        // --- Sort (q1, q2, q3) in ascending order ---
        if (q2 < q1) {
            VecT* tmp = q1;
            q1 = q2;
            q2 = tmp;
        }
        if (q3 < q1) {
            VecT* tmp = q1;
            q1 = q3;
            q3 = tmp;
        }
        if (q3 < q2) {
            VecT* tmp = q2;
            q2 = q3;
            q3 = tmp;
        }

        // Compare the sorted pointers
        return (p1 == q1 && p2 == q2 && p3 == q3);
    }

    /*bool operator==(const Triangle& other) const {
        std::array<VecT*, 3> this_pts = { a, b, c };
        std::array<VecT*, 3> other_pts = { other.a, other.b, other.c };
        std::sort(this_pts.begin(), this_pts.end());
        std::sort(other_pts.begin(), other_pts.end());
        return this_pts == other_pts;
    }*/

    //bool operator==(const Triangle& other) const {
    //    return 
    //        (a == other.a || a == other.b || a == other.c) &&
    //        (b == other.a || b == other.b || b == other.c) &&
    //        (c == other.a || c == other.b || c == other.c);
    //}
};

template <typename VecT>
struct TriangleHash
{
    std::size_t operator()(const Triangle<VecT>& tri) const noexcept
    {
        // Sort the three pointers using branchless min/max logic
        /*VecT* p1 = tri.a;
        VecT* p2 = tri.b;
        VecT* p3 = tri.c;

        VecT* minP = std::min({ p1, p2, p3 });
        VecT* maxP = std::max({ p1, p2, p3 });
        VecT* midP = (p1 != minP && p1 != maxP) ? p1 :
            (p2 != minP && p2 != maxP) ? p2 : p3;

        // Hash the sorted pointers directly without extra function calls
        size_t h1 = reinterpret_cast<size_t>(minP);
        size_t h2 = reinterpret_cast<size_t>(maxP);
        size_t h3 = reinterpret_cast<size_t>(midP);*/

        size_t h1 = reinterpret_cast<size_t>(tri.a);
        size_t h2 = reinterpret_cast<size_t>(tri.b);
        size_t h3 = reinterpret_cast<size_t>(tri.c);

        // Murmur-inspired hash mixing for stronger hash distribution
        h1 ^= (h2 + 0x9e3779b97f4a7c15ULL + (h1 << 6) + (h1 >> 2));
        h1 ^= (h3 + 0x9e3779b97f4a7c15ULL + (h1 << 6) + (h1 >> 2));

        return h1;
    }
};

/*template <typename VecT>
struct TriangleHash
{
    std::size_t operator()(const Triangle<VecT>& tri) const
    {
        // Copy the 3 pointers
        VecT* p1 = tri.a;
        VecT* p2 = tri.b;
        VecT* p3 = tri.c;

        // Sort them so that (p1 <= p2 <= p3), matching your operator== logic
        if (p2 < p1) { VecT* tmp = p1; p1 = p2; p2 = tmp; }
        if (p3 < p1) { VecT* tmp = p1; p1 = p3; p3 = tmp; }
        if (p3 < p2) { VecT* tmp = p2; p2 = p3; p3 = tmp; }

        // Now p1 <= p2 <= p3
        auto h1 = std::hash<VecT*>()(p1);
        auto h2 = std::hash<VecT*>()(p2);
        auto h3 = std::hash<VecT*>()(p3);

        // Combine them (basic approach similar to boost::hash_combine)
        std::size_t seed = 0;
        auto hash_combine = [&](std::size_t val) {
            seed ^= val + 0x9e3779b97f4a7c15ULL + (seed << 6) + (seed >> 2);
            };
        hash_combine(h1);
        hash_combine(h2);
        hash_combine(h3);
        return seed;
    }
};*/

// A custom equality that relies on Triangle's operator==.
template <typename VecT>
struct TriangleEqual
{
    bool operator()(const Triangle<VecT>& lhs, const Triangle<VecT>& rhs) const
    {
        return lhs == rhs; // calls your Triangle::operator==(...)
    }
};


/*struct Triangle
{
    DVec2 a, b, c;

    // Check if a point is inside the circumcircle of the triangle
    bool isPointInCircumcircle(const DVec2& p) const
    {
        double ax = a.x - p.x, ay = a.y - p.y;
        double bx = b.x - p.x, by = b.y - p.y;
        double cx = c.x - p.x, cy = c.y - p.y;

        double dA = ax * ax + ay * ay;
        double dB = bx * bx + by * by;
        double dC = cx * cx + cy * cy;

        double determinant =
            (ax * (by * dC - cy * dB)) -
            (ay * (bx * dC - cx * dB)) +
            (dA * (bx * cy - by * cx));

        return determinant > 0; // Inside circumcircle
    }

    bool containsVertex(const DVec2& p) const
    {
        return (a == p || b == p || c == p);
    }

    // Define == operator to compare two triangles (ignoring order of points)
    bool operator==(const Triangle& other) const {
        return (a == other.a || a == other.b || a == other.c) &&
            (b == other.a || b == other.b || b == other.c) &&
            (c == other.a || c == other.b || c == other.c);
    }
};*/

struct Color
{
    union {
        struct { uint8_t r, g, b, a; };
        uint32_t u32 = 0;
    };

    Color() = default;
    Color(uint32_t rgba) : u32(rgba) {}
    Color(uint8_t _r, uint8_t _g, uint8_t _b, uint8_t _a=255) 
        : r(_r), g(_g), b(_b), a(_a) {}

    Color& operator =(const Color& rhs) { u32 = rhs.u32; return *this; }
    bool operator ==(const Color& rhs) const { return u32 == rhs.u32; }

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

    double  width()  { return x2 - x1; }
    double  height() { return y2 - y1; }
    Vec2<T> size()   { return { x2 - x1, y2 - y1 }; }

    bool hitTest(T x, T y) { return (x >= x1 && y >= y1 && x <= x2 && y <= y2); }
    Rect scaled(T mult) {
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
    bool operator ==(const Quad& rhs) { return (a == rhs.a && b == rhs.b && c == rhs.c && d == rhs.d); }
    bool operator !=(const Quad& rhs) { return (a != rhs.a || b != rhs.b || c != rhs.c || d != rhs.d); }
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