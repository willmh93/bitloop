#pragma once
#include "types.h"
#include <numbers>
#include <sstream>
#include <iomanip>
#include <algorithm>

namespace Math
{
    // Numbers
    constexpr double PI = std::numbers::pi;
    constexpr double TWO_PI = std::numbers::pi * 2.0;
    constexpr double HALF_PI = std::numbers::pi / 2.0;

    inline int countDecimals(double num)
    {
        // todo: Optimize?
        std::ostringstream out;
        out << std::fixed << std::setprecision(10) << num;
        std::string str = out.str();

        size_t pos = str.find('.');
        if (pos == std::string::npos) return 0;
        return int(str.size() - pos - 1);
    }

    // Coordinate offset rotation
    inline Vec2 rotateOffset(double dx, double dy, double rotation)
    {
        double _cos = cos(rotation);
        double _sin = sin(rotation);
        return {
            (dx * _cos - dy * _sin),
            (dy * _cos + dx * _sin)
        };
    }
    inline Vec2 rotateOffset(double dx, double dy, double _cos, double _sin)
    {
        return {
            (dx * _cos - dy * _sin),
            (dy * _cos + dx * _sin)
        };
    }
    inline Vec2 rotateOffset(const Vec2& offset, double rotation)
    {
        double _cos = cos(rotation);
        double _sin = sin(rotation);
        return {
            (offset.x * _cos - offset.y * _sin),
            (offset.y * _cos + offset.x * _sin)
        };
    }
    inline Vec2 rotateOffset(const Vec2& offset, double _cos, double _sin)
    {
        return {
            (offset.x * _cos - offset.y * _sin),
            (offset.y * _cos + offset.x * _sin)
        };
    }

    inline Vec2 reverseRotateOffset(double dx, double dy, double rotation)
    {
        double _cos = cos(rotation);
        double _sin = sin(rotation);
        return {
            (dx * _cos + dy * _sin),
            (dy * _cos - dx * _sin)
        };
    }
    inline Vec2 reverseRotateOffset(double dx, double dy, double _cos, double _sin)
    {
        return {
            (dx * _cos + dy * _sin),
            (dy * _cos - dx * _sin)
        };
    }
    inline Vec2 reverseRotateOffset(const Vec2& offset, double rotation)
    {
        double _cos = cos(rotation);
        double _sin = sin(rotation);
        return {
            (offset.x * _cos + offset.y * _sin),
            (offset.y * _cos - offset.x * _sin)
        };
    }
    inline Vec2 reverseRotateOffset(const Vec2& offset, double _cos, double _sin)
    {
        return {
            (offset.x * _cos + offset.y * _sin),
            (offset.y * _cos - offset.x * _sin)
        };
    }

    // Angles
    inline double closestAngleDifference(double angle, double target_angle)
    {
        double diff = std::fmod((target_angle - angle) + PI, TWO_PI);
        if (diff < 0)
            diff += TWO_PI;
        return diff - PI;
    }
    inline double wrapRadians(double angle)
    {
        angle = std::fmod(angle, 2.0 * TWO_PI);
        if (angle > PI) angle -= 2.0 * PI;
        else if (angle <= -PI) angle += 2.0 * PI;
        return angle;
    }
    inline double wrapRadians2PI(double angle)
    {
        angle = std::fmod(angle, 2.0 * PI);
        if (angle < 0.0)
            angle += 2.0 * PI;
        return angle;
    }

    // Intersections
    inline bool lineEqIntersect(Vec2* targ, const Ray& ray1, const Ray& ray2, bool bidirectional)
    {
        // Compute unit direction vectors for each ray.
        double d1x = cos(ray1.angle), d1y = sin(ray1.angle);
        double d2x = cos(ray2.angle), d2y = sin(ray2.angle);

        // Origins:
        double x1 = ray1.x, y1 = ray1.y;
        double x2 = ray2.x, y2 = ray2.y;

        // Denom of the 2x2 system:
        double denom = d1x * d2y - d1y * d2x;
        if (fabs(denom) < 1e-9)
        {
            // Rays are parallel (or nearly so)
            return false;
        }

        // Compute the parameters t and u such that:
        //   ray1.origin + t*d1 = ray2.origin + u*d2
        double dx = x2 - x1;
        double dy = y2 - y1;
        double t = (dx * d2y - dy * d2x) / denom;
        double u = (dx * d1y - dy * d1x) / denom;

        // If we're restricting to the forward direction, both t and u must be nonnegative.
        if (!bidirectional)
        {
            if (t < 0 || u < 0)
                return false;
        }

        // Compute intersection point.
        *targ = Vec2(x1 + t * d1x, y1 + t * d1y);
        return true;
    }
    inline bool rayRectIntersection(Vec2* back_intersect, Vec2* foward_intersect, const FRect& r, const Ray& ray)
    {
        // Normalize rect boundaries
        double minX = std::min(r.x1, r.x2);
        double maxX = std::max(r.x1, r.x2);
        double minY = std::min(r.y1, r.y2);
        double maxY = std::max(r.y1, r.y2);

        // Ray origin and direction
        double rx = ray.x;
        double ry = ray.y;
        double dx = cos(ray.angle);
        double dy = sin(ray.angle);

        // Aaccumulate candidate intersections as (t, point) pairs
        struct IntersectionCandidate
        {
            double t;
            Vec2 pt;
        };

        std::vector<IntersectionCandidate> candidates;
        const double eps = 1e-9;

        // --- Check intersection with vertical edges ---

        // Left edge: x = minX
        if (std::fabs(dx) > eps) {
            double t = (minX - rx) / dx;
            double y_int = ry + t * dy;
            if (y_int >= minY - eps && y_int <= maxY + eps)
                candidates.push_back({ t, Vec2(minX, y_int) });
        }

        // Right edge: x = maxX
        if (std::fabs(dx) > eps) {
            double t = (maxX - rx) / dx;
            double y_int = ry + t * dy;
            if (y_int >= minY - eps && y_int <= maxY + eps)
                candidates.push_back({ t, Vec2(maxX, y_int) });
        }

        // --- Check intersection with horizontal edges ---

        // Bottom edge: y = minY
        if (std::fabs(dy) > eps) {
            double t = (minY - ry) / dy;
            double x_int = rx + t * dx;
            if (x_int >= minX - eps && x_int <= maxX + eps) {
                candidates.push_back({ t, Vec2(x_int, minY) });
            }
        }

        // Top edge: y = maxY
        if (std::fabs(dy) > eps)
        {
            double t = (maxY - ry) / dy;
            double x_int = rx + t * dx;
            if (x_int >= minX - eps && x_int <= maxX + eps)
                candidates.push_back({ t, Vec2(x_int, maxY) });
        }

        // We expect exactly 2 intersection points
        if (candidates.size() < 2)
            return false;

        // Sort candidates by their t-value.
        std::sort(candidates.begin(), candidates.end(), [](
            const IntersectionCandidate& a,
            const IntersectionCandidate& b)
        {
            return a.t < b.t;
        });

        // Remove duplicates: if two candidate intersections have nearly the same t, keep only one.
        std::vector<IntersectionCandidate> unique;
        unique.push_back(candidates.front());
        for (size_t i = 1; i < candidates.size(); ++i)
        {
            if (std::fabs(candidates[i].t - unique.back().t) > eps)
                unique.push_back(candidates[i]);
        }

        if (unique.size() != 2)
        {
            // In degenerate cases (e.g. ray exactly tangent to the rectangle)
            return false;
        }

        // Order the two intersections:
        // The one with the smaller t is the "back" intersection (may be behind the ray’s origin)
        // and the one with the larger t is the "foward" intersection.
        *back_intersect = unique[0].pt;
        *foward_intersect = unique[1].pt;
        return true;
    }

    // Lerp
    inline FRect lerpRect(const FRect& src, const FRect& targ, double factor)
    {
        FRect ret = src;
        ret.x1 += (targ.x1 - src.x1) * factor;
        ret.y1 += (targ.y1 - src.y1) * factor;
        ret.x2 += (targ.x2 - src.x2) * factor;
        ret.y2 += (targ.y2 - src.y2) * factor;
        return ret;
    }
}

