#pragma once
#include <bitloop/core/types.h>

BL_BEGIN_NS

struct Color
{
    union {
        struct { uint8_t r, g, b, a; };
        uint32_t u32 = 0;
    };

    Color() = default;
    constexpr Color(uint32_t rgba) : u32(rgba) {}
    constexpr Color(const float(&c)[3]) : r(int(c[0] * 255.f)), g(int(c[1] * 255.f)), b(int(c[2] * 255.f)), a(255) {}
    constexpr Color(const float(&c)[4]) : r(int(c[0] * 255.f)), g(int(c[1] * 255.f)), b(int(c[2] * 255.f)), a(int(c[3] * 255.f)) {}
    constexpr Color(uint8_t _r, uint8_t _g, uint8_t _b, uint8_t _a = 255) : r(_r), g(_g), b(_b), a(_a) {}

    constexpr Color& operator =(const Color& rhs) { u32 = rhs.u32; return *this; }
    [[nodiscard]] constexpr bool operator ==(const Color& rhs) const { return u32 == rhs.u32; }

    constexpr operator uint32_t() const { return u32; }
    constexpr Vec4<float> vec4() const { return { (float)r / 255.0f, (float)g / 255.0f, (float)b / 255.0f, (float)a / 255.0f }; }

    Color& adjustHue(float amount)
    {
        setHue(getHue() + amount);
        return *this;
    }

    // Return hue in degrees [0,360).
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
        r = uint8_t(std::round((r1 + m) * 255.f));
        g = uint8_t(std::round((g1 + m) * 255.f));
        b = uint8_t(std::round((b1 + m) * 255.f));
    }

    static void RGBtoHSV(uint8_t r, uint8_t g, uint8_t b, float& h, float& s, float& v)
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
