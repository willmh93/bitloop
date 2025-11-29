#pragma once
#include <bitloop/core/types.h>
#include <bit>

BL_BEGIN_NS

constexpr u32 pack_rgba(u8 r, u8 g, u8 b, u8 a = 255) {
    #if defined(__cpp_lib_endian)
    if constexpr (std::endian::native == std::endian::little) {
        // bytes in memory: r g b a  -> integer: 0xAABBGGRR
        return u32(r) | (u32(g) << 8) | (u32(b) << 16) | (u32(a) << 24);
    }
    else {
        // big-endian: integer matches 0xRRGGBBAA
        return (u32(r) << 24) | (u32(g) << 16) | (u32(b) << 8) | u32(a);
    }
    #else
    // assume little-endian if <bit> not available
    return u32(r) | (u32(g) << 8) | (u32(b) << 16) | (u32(a) << 24);
    #endif
}

struct Color
{
    // Core
    static constexpr u32 transparent = pack_rgba(0, 0, 0, 0);
    static constexpr u32 black = pack_rgba(0, 0, 0);
    static constexpr u32 white = pack_rgba(255, 255, 255);
    static constexpr u32 red = pack_rgba(255, 0, 0);
    static constexpr u32 green = pack_rgba(0, 255, 0);
    static constexpr u32 blue = pack_rgba(0, 0, 255);
    static constexpr u32 yellow = pack_rgba(255, 255, 0);
    static constexpr u32 cyan = pack_rgba(0, 255, 255);
    static constexpr u32 magenta = pack_rgba(255, 0, 255);
    static constexpr u32 gray = pack_rgba(128, 128, 128);
    static constexpr u32 light_gray = pack_rgba(211, 211, 211);
    static constexpr u32 dark_gray = pack_rgba(169, 169, 169);
    static constexpr u32 silver = pack_rgba(192, 192, 192);
    static constexpr u32 maroon = pack_rgba(128, 0, 0);
    static constexpr u32 purple = pack_rgba(128, 0, 128);
    static constexpr u32 fuchsia = pack_rgba(255, 0, 255);
    static constexpr u32 lime = pack_rgba(0, 255, 0);
    static constexpr u32 olive = pack_rgba(128, 128, 0);
    static constexpr u32 navy = pack_rgba(0, 0, 128);
    static constexpr u32 teal = pack_rgba(0, 128, 128);
    static constexpr u32 aqua = pack_rgba(0, 255, 255);
    static constexpr u32 orange = pack_rgba(255, 165, 0);
    static constexpr u32 orange_red = pack_rgba(255, 69, 0);
    static constexpr u32 brown = pack_rgba(165, 42, 42);
    static constexpr u32 tan_col = pack_rgba(210, 180, 140);
    static constexpr u32 beige = pack_rgba(245, 245, 220);
    static constexpr u32 gold = pack_rgba(255, 215, 0);
    static constexpr u32 khaki = pack_rgba(240, 230, 140);
    static constexpr u32 chocolate = pack_rgba(210, 105, 30);
    static constexpr u32 sienna = pack_rgba(160, 82, 45);
    static constexpr u32 pink = pack_rgba(255, 192, 203);
    static constexpr u32 deep_pink = pack_rgba(255, 20, 147);
    static constexpr u32 salmon = pack_rgba(250, 128, 114);
    static constexpr u32 coral = pack_rgba(255, 127, 80);
    static constexpr u32 crimson = pack_rgba(220, 20, 60);
    static constexpr u32 indigo = pack_rgba(75, 0, 130);
    static constexpr u32 violet = pack_rgba(238, 130, 238);
    static constexpr u32 orchid = pack_rgba(218, 112, 214);
    static constexpr u32 chartreuse = pack_rgba(127, 255, 0);
    static constexpr u32 spring_green = pack_rgba(0, 255, 127);
    static constexpr u32 turquoise = pack_rgba(64, 224, 208);
    static constexpr u32 sky_blue = pack_rgba(135, 206, 235);
    static constexpr u32 deep_sky_blue = pack_rgba(0, 191, 255);
    static constexpr u32 dodger_blue = pack_rgba(30, 144, 255);
    static constexpr u32 royal_blue = pack_rgba(65, 105, 225);
    static constexpr u32 forest_green = pack_rgba(34, 139, 34);
    static constexpr u32 dark_green = pack_rgba(0, 100, 0);
    static constexpr u32 sea_green = pack_rgba(46, 139, 87);
    static constexpr u32 dark_red = pack_rgba(139, 0, 0);
    static constexpr u32 dark_blue = pack_rgba(0, 0, 139);
    static constexpr u32 midnight_blue = pack_rgba(25, 25, 112);
    static constexpr u32 slate_gray = pack_rgba(112, 128, 144);

    union {
        struct { u8 r, g, b, a; };
        u32 rgba = 0;
    };

    Color() = default;
    constexpr Color(u32 _rgba) : rgba(_rgba) {}
    constexpr Color(Color rgb, u8 _a) : r(rgb.r), g(rgb.g), b(rgb.b), a(_a) {}
    constexpr Color(const float(&c)[3]) : r(int(c[0] * 255.f)), g(int(c[1] * 255.f)), b(int(c[2] * 255.f)), a(255) {}
    constexpr Color(const float(&c)[4]) : r(int(c[0] * 255.f)), g(int(c[1] * 255.f)), b(int(c[2] * 255.f)), a(int(c[3] * 255.f)) {}
    constexpr Color(u8 _r, u8 _g, u8 _b, u8 _a = 255) : r(_r), g(_g), b(_b), a(_a) {}

    constexpr Color& operator =(const Color& rhs) { rgba = rhs.rgba; return *this; }
    [[nodiscard]] constexpr bool operator ==(const Color& rhs) const { return rgba == rhs.rgba; }

    constexpr operator u32() const { return rgba; }
    constexpr Vec4<float> vec4() const { return { (float)r / 255.0f, (float)g / 255.0f, (float)b / 255.0f, (float)a / 255.0f }; }

    Color& adjustHue(float amount)
    {
        setHue(getHue() + amount);
        return *this;
    }

    // Return hue in degrees [0,360].
    [[nodiscard]] constexpr float getHue() const
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
        r = u8(std::round((r1 + m) * 255.f));
        g = u8(std::round((g1 + m) * 255.f));
        b = u8(std::round((b1 + m) * 255.f));
    }

    static void RGBtoHSV(u8 r, u8 g, u8 b, float& h, float& s, float& v)
    {
        // normalize to [0,1]
        float rf = r / 255.0f;
        float gf = g / 255.0f;
        float bf = b / 255.0f;

        float maxc = std::fmax(rf, std::fmax(gf, bf));
        float minc = std::fmin(rf, std::fmin(gf, bf));
        float delta = maxc - minc;

        // brightness/value
        v = maxc;

        // saturation
        if (maxc != 0.0f)
            s = delta / maxc;
        else
            s = 0.0f;

        // hue
        if (delta == 0.0f) {
            h = 0.0f;  // undefined, maybe grayscale
        }
        else if (maxc == rf) {
            h = 60.0f * ((gf - bf) / delta);
        }
        else if (maxc == gf) {
            h = 60.0f * (2.0f + (bf - rf) / delta);
        }
        else {  // maxc == bf
            h = 60.0f * (4.0f + (rf - gf) / delta);
        }

        if (h < 0.0f)
            h += 360.0f;
    }
};

BL_END_NS
