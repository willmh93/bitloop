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
    constexpr double INV_TWO_PI = 1.0 / TWO_PI;

    template<typename T> [[nodiscard]] inline bool isfinite(const T& v) { return std::isfinite(v); }
    [[nodiscard]] inline bool isfinite(const DoubleDouble& v) {
        return std::isfinite(v.hi) && std::isfinite(v.lo);
    }

    template<typename T> [[nodiscard]] inline bool isnan(const T& v) { return std::isnan(v); }
    [[nodiscard]] inline bool isnan(const DoubleDouble& v) {
        return std::isnan(v.hi) || std::isnan(v.lo);
    }

    template<typename T> [[nodiscard]] inline bool isinf(const T& v) { return std::isinf(v); }
    [[nodiscard]] inline bool isinf(const DoubleDouble& v) {
        return std::isinf(v.hi);
    }

    [[nodiscard]] inline int countDecimals(double num)
    {
        // todo: Optimize?
        std::ostringstream out;
        out << std::fixed << std::setprecision(10) << num;
        std::string str = out.str();

        size_t pos = str.find('.');
        if (pos == std::string::npos) return 0;
        return int(str.size() - pos - 1);
    }

    [[nodiscard]] inline int countDigits(int n)
    {
        if (n == 0) return 1;

        // Work in unsigned to avoid overflow on INT_MIN
        unsigned v = (n < 0)
            ? static_cast<unsigned>(-(static_cast<long long>(n)))
            : static_cast<unsigned>(n);

        return static_cast<int>(std::log10(static_cast<double>(v))) + 1;
    }
    
    template <typename Float>
    [[nodiscard]] int countWholeDigits(Float x)
    {
        static_assert(std::is_floating_point_v<Float>,
            "Float must be a floating-point type");

        if (!std::isfinite(x))
            return 0;

        x = std::fabs(x);

        if (x < 1)
            return 1;

        return static_cast<int>(std::floor(std::log10(x))) + 1;
    }

    template<typename T, typename... Rest>
    [[nodiscard]] constexpr T avg(T first, Rest... rest)
    {
        T sum = (first + ... + rest);
        constexpr std::size_t count = 1 + sizeof...(rest);
        return sum / static_cast<T>(count);
    }

    template<typename T>
    [[nodiscard]] T wrap(T value, T min, T max)
    {
        T range = max - min;
        value = std::fmod(value - min, range);
        if (value < 0) value += range;
        return value + min;
    }

    template<typename T>
    [[nodiscard]] constexpr T fast_log1p(T x)
    {
        T y = x / (2 + x);
        T y2 = y * y;
        return 2 * y * (1 + y2 * (T{ 1.0 } / 3 + y2 * (T{ 1.0 } / 5 + y2 * (T{ 1.0 } / 7))));
    }

    template<typename T>
    [[nodiscard]] inline T log1pLerp(T a, T lerp_factor)
    {
        //return a + (fast_log1p(a) - a) * lerp_factor;
        return a + (log(1 + a) - a) * lerp_factor;
        //return a + (log1p(a) - a) * lerp_factor;
    }

    // Ratios (a->b,  0->1)
    template<typename T> [[nodiscard]] constexpr T ratio(T a, T b) { return ((b-a)/a); }
    template<typename T> [[nodiscard]] constexpr T absRatio(T a, T b) { return abs((b-a)/a); }
    template<typename T> [[nodiscard]] constexpr T avgRatio(T a, T b) { return ((a-b)/((a+b)/T{2})); }
    template<typename T> [[nodiscard]] constexpr T absAvgRatio(T a, T b) { return (abs(a-b)/((a+b)/T{2})); }

    // Percentages (a->b,  0->100)
    template<typename T> [[nodiscard]] constexpr T pct(T a, T b) { return ((b-a)/a)*T{100}; }
    template<typename T> [[nodiscard]] constexpr T absPct(T a, T b) { return abs((b-a)/a)*T{100}; }
    template<typename T> [[nodiscard]] constexpr T avgPct(T a, T b) { return (((a-b)/((a+b)/T{2}))*T{100}); }
    template<typename T> [[nodiscard]] constexpr T absAvgPct(T a, T b) { return (abs(a-b)/((a+b)/T{2}))*T{100}; }

    // Coordinate offset rotation
    [[nodiscard]]inline DVec2 rotateOffset(double dx, double dy, double rotation)
    {
        double _cos = cos(rotation);
        double _sin = sin(rotation);
        return {
            (dx * _cos - dy * _sin),
            (dy * _cos + dx * _sin)
        };
    }
    [[nodiscard]]inline DVec2 rotateOffset(double dx, double dy, double _cos, double _sin)
    {
        return {
            (dx * _cos - dy * _sin),
            (dy * _cos + dx * _sin)
        };
    }
    [[nodiscard]]inline DVec2 rotateOffset(const DVec2& offset, double rotation)
    {
        double _cos = cos(rotation);
        double _sin = sin(rotation);
        return {
            (offset.x * _cos - offset.y * _sin),
            (offset.y * _cos + offset.x * _sin)
        };
    }
    [[nodiscard]]inline DVec2 rotateOffset(const DVec2& offset, double _cos, double _sin)
    {
        return {
            (offset.x * _cos - offset.y * _sin),
            (offset.y * _cos + offset.x * _sin)
        };
    }

    [[nodiscard]]inline DVec2 reverseRotateOffset(double dx, double dy, double rotation)
    {
        double _cos = cos(rotation);
        double _sin = sin(rotation);
        return {
            (dx * _cos + dy * _sin),
            (dy * _cos - dx * _sin)
        };
    }
    [[nodiscard]]inline DVec2 reverseRotateOffset(double dx, double dy, double _cos, double _sin)
    {
        return {
            (dx * _cos + dy * _sin),
            (dy * _cos - dx * _sin)
        };
    }
    [[nodiscard]]inline DVec2 reverseRotateOffset(const DVec2& offset, double rotation)
    {
        double _cos = cos(rotation);
        double _sin = sin(rotation);
        return {
            (offset.x * _cos + offset.y * _sin),
            (offset.y * _cos - offset.x * _sin)
        };
    }
    [[nodiscard]]inline DVec2 reverseRotateOffset(const DVec2& offset, double _cos, double _sin)
    {
        return {
            (offset.x * _cos + offset.y * _sin),
            (offset.y * _cos - offset.x * _sin)
        };
    }

    // Angles
    template<typename T> [[nodiscard]] constexpr T toRadians(T degrees) { return degrees * std::numbers::pi_v<T> / T{ 180 }; }
    template<typename T> [[nodiscard]] constexpr T toDegrees(T radians) { return radians * T{ 180 } / std::numbers::pi_v<T>; }

    template<typename T> [[nodiscard]] inline T closestAngleDifference(T angle, T target_angle)
    {
        T diff = std::fmod((target_angle - angle) + std::numbers::pi_v<T>, std::numbers::pi_v<T>*2);
        if (diff < 0) diff += std::numbers::pi_v<T>*2;
        return diff - std::numbers::pi_v<T>;
    }
    template<typename T> [[nodiscard]] inline T wrapRadians(T a) noexcept { return std::remainder(a, std::numbers::pi_v<T>*2); }
    template<typename T> [[nodiscard]] inline T wrapRadians2PI(T a) noexcept { return a - std::numbers::pi_v<T>*2 * std::floor(a * T{1}/(std::numbers::pi_v<T>*2)); }
    template<typename T> [[nodiscard]] inline T lerpAngle(T a, T b, T f) { return wrapRadians(a + f * closestAngleDifference(a, b)); }
    template<typename T> [[nodiscard]] inline T avgAngle(T a, T b) { return lerpAngle(a, b, T{ 0.5 }); }

    // Intersections
    inline bool lineEqIntersect(DVec2* targ, const DRay& ray1, const DRay& ray2, bool bidirectional)
    {
        // Unit direction vectors for each ray
        double d1x = cos(ray1.angle), d1y = sin(ray1.angle);
        double d2x = cos(ray2.angle), d2y = sin(ray2.angle);

        // Origins
        double x1 = ray1.x, y1 = ray1.y;
        double x2 = ray2.x, y2 = ray2.y;

        // Denom of the 2x2 system
        double denom = d1x * d2y - d1y * d2x;

        // Are rays parallel?
        if (fabs(denom) < 1e-9)
            return false;

        // Compute t and u such that:  ray1.origin + t*d1 = ray2.origin + u*d2
        double dx = x2 - x1;
        double dy = y2 - y1;
        double t = (dx * d2y - dy * d2x) / denom;
        double u = (dx * d1y - dy * d1x) / denom;

        // If restricting to the forward direction, both t and u must be nonnegative.
        if (!bidirectional)
        {
            if (t < 0 || u < 0)
                return false;
        }

        // Compute intersection point.
        *targ = Vec2(x1 + t * d1x, y1 + t * d1y);
        return true;
    }
    inline bool rayRectIntersection(DVec2* back_intersect, DVec2* foward_intersect, const DRect& r, const DRay& ray)
    {
        // Normalize rect boundaries
        double minX = std::min(r.x1, r.x2);
        double maxX = std::max(r.x1, r.x2);
        double minY = std::min(r.y1, r.y2);
        double maxY = std::max(r.y1, r.y2);

        // DRay origin and direction
        double rx = ray.x;
        double ry = ray.y;
        double dx = cos(ray.angle);
        double dy = sin(ray.angle);

        // Aaccumulate candidate intersections as (t, point) pairs
        struct IntersectionCandidate
        {
            double t;
            DVec2 pt;
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
        // The one with the smaller t is the "back" intersection (behind the ray’s origin)
        // and the one with the larger t is the "foward" intersection.
        *back_intersect = unique[0].pt;
        *foward_intersect = unique[1].pt;
        return true;
    }

    // Lerp
    template<typename ValueT, typename Scalar>
    [[nodiscard]] constexpr ValueT lerp(const ValueT& a, const ValueT& b, Scalar x) {

        return (ValueT)(a + (b - a) * x);
    }
    
    template<typename ValueT, typename Scalar>
    [[nodiscard]] inline Rect<ValueT> lerp(const Rect<ValueT>& src, const Rect<ValueT>& targ, Scalar factor)
    {
        Rect<ValueT> ret = src;
        ret.x1 += (targ.x1 - src.x1) * factor;
        ret.y1 += (targ.y1 - src.y1) * factor;
        ret.x2 += (targ.x2 - src.x2) * factor;
        ret.y2 += (targ.y2 - src.y2) * factor;
        return ret;
    }

    template<typename ValueT, typename Scalar>
    [[nodiscard]] inline AngledRect<ValueT> lerp(const AngledRect<ValueT>& src, const AngledRect<ValueT>& targ, Scalar factor)
    {
        AngledRect<ValueT> ret;
        ret.cen = lerp(src.cen, targ.cen, factor);
        ret.size = lerp(src.size, targ.size, factor);
        ret.angle = lerpAngle(src.angle, targ.angle, factor);
        return ret;
    }

    template<typename T>
    [[nodiscard]] inline T lerpFactor(T value, T min, T max) {
        return ((value - min) / (max - min));
    }

    namespace MovingAverage
    {
        class MA
        {
            int ma_count = 0;
            double sum = 0.0;
            std::vector<double> samples;

        public:
            MA(int ma_count) : ma_count(ma_count)
            {}

            double push(double v)
            {
                sum += v;
                samples.push_back(v);

                if (samples.size() > ma_count)
                {
                    sum -= samples[0];
                    samples.erase(samples.begin());
                }

                return average();
            }

            [[nodiscard]] double average()
            {
                return (sum / static_cast<double>(samples.size()));
            }
        };
    }
}

