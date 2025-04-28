#pragma once
#include "types.h"
#include <numbers>

namespace Math
{
    inline constexpr double PI = std::numbers::pi;
    inline constexpr double TWO_PI = std::numbers::pi * 2.0;
    inline constexpr double HALF_PI = std::numbers::pi / 2.0;

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
}

