#pragma once
#include <bitloop/core/types.h>
#include <bit>
#include <numbers>

BL_BEGIN_NS

constexpr uint32_t pack_rgba(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255) {
    #if defined(__cpp_lib_endian)
    if constexpr (std::endian::native == std::endian::little) {
        // bytes in memory: r g b a  -> integer: 0xAABBGGRR
        return uint32_t(r) | (uint32_t(g) << 8) | (uint32_t(b) << 16) | (uint32_t(a) << 24);
    }
    else {
        // big-endian: integer matches 0xRRGGBBAA
        return (uint32_t(r) << 24) | (uint32_t(g) << 16) | (uint32_t(b) << 8) | uint32_t(a);
    }
    #else
    // assume little-endian if <bit> not available
    return uint32_t(r) | (uint32_t(g) << 8) | (uint32_t(b) << 16) | (uint32_t(a) << 24);
    #endif
}

constexpr std::uint8_t hex_nibble(char c)
{
    return (c >= '0' && c <= '9') ? std::uint8_t(c - '0') :
        (c >= 'a' && c <= 'f') ? std::uint8_t(c - 'a' + 10) :
        (c >= 'A' && c <= 'F') ? std::uint8_t(c - 'A' + 10) :
        std::uint8_t(0);
}

constexpr std::uint8_t parse_byte(std::string_view s, std::size_t i)
{
    return std::uint8_t((hex_nibble(s[i]) << 4) | hex_nibble(s[i + 1]));
}

struct Color
{
    // Core
    static constexpr uint32_t transparent = pack_rgba(0, 0, 0, 0);
    static constexpr uint32_t black = pack_rgba(0, 0, 0);
    static constexpr uint32_t white = pack_rgba(255, 255, 255);
    static constexpr uint32_t red = pack_rgba(255, 0, 0);
    static constexpr uint32_t green = pack_rgba(0, 255, 0);
    static constexpr uint32_t blue = pack_rgba(0, 0, 255);
    static constexpr uint32_t yellow = pack_rgba(255, 255, 0);
    static constexpr uint32_t cyan = pack_rgba(0, 255, 255);
    static constexpr uint32_t magenta = pack_rgba(255, 0, 255);
    static constexpr uint32_t gray = pack_rgba(128, 128, 128);
    static constexpr uint32_t light_gray = pack_rgba(211, 211, 211);
    static constexpr uint32_t dark_gray = pack_rgba(169, 169, 169);
    static constexpr uint32_t silver = pack_rgba(192, 192, 192);
    static constexpr uint32_t maroon = pack_rgba(128, 0, 0);
    static constexpr uint32_t purple = pack_rgba(128, 0, 128);
    static constexpr uint32_t fuchsia = pack_rgba(255, 0, 255);
    static constexpr uint32_t lime = pack_rgba(0, 255, 0);
    static constexpr uint32_t olive = pack_rgba(128, 128, 0);
    static constexpr uint32_t navy = pack_rgba(0, 0, 128);
    static constexpr uint32_t teal = pack_rgba(0, 128, 128);
    static constexpr uint32_t aqua = pack_rgba(0, 255, 255);
    static constexpr uint32_t orange = pack_rgba(255, 165, 0);
    static constexpr uint32_t orange_red = pack_rgba(255, 69, 0);
    static constexpr uint32_t brown = pack_rgba(165, 42, 42);
    static constexpr uint32_t tan_col = pack_rgba(210, 180, 140);
    static constexpr uint32_t beige = pack_rgba(245, 245, 220);
    static constexpr uint32_t gold = pack_rgba(255, 215, 0);
    static constexpr uint32_t khaki = pack_rgba(240, 230, 140);
    static constexpr uint32_t chocolate = pack_rgba(210, 105, 30);
    static constexpr uint32_t sienna = pack_rgba(160, 82, 45);
    static constexpr uint32_t pink = pack_rgba(255, 192, 203);
    static constexpr uint32_t deep_pink = pack_rgba(255, 20, 147);
    static constexpr uint32_t salmon = pack_rgba(250, 128, 114);
    static constexpr uint32_t coral = pack_rgba(255, 127, 80);
    static constexpr uint32_t crimson = pack_rgba(220, 20, 60);
    static constexpr uint32_t indigo = pack_rgba(75, 0, 130);
    static constexpr uint32_t violet = pack_rgba(238, 130, 238);
    static constexpr uint32_t orchid = pack_rgba(218, 112, 214);
    static constexpr uint32_t chartreuse = pack_rgba(127, 255, 0);
    static constexpr uint32_t spring_green = pack_rgba(0, 255, 127);
    static constexpr uint32_t turquoise = pack_rgba(64, 224, 208);
    static constexpr uint32_t sky_blue = pack_rgba(135, 206, 235);
    static constexpr uint32_t deep_sky_blue = pack_rgba(0, 191, 255);
    static constexpr uint32_t dodger_blue = pack_rgba(30, 144, 255);
    static constexpr uint32_t royal_blue = pack_rgba(65, 105, 225);
    static constexpr uint32_t forest_green = pack_rgba(34, 139, 34);
    static constexpr uint32_t dark_green = pack_rgba(0, 100, 0);
    static constexpr uint32_t sea_green = pack_rgba(46, 139, 87);
    static constexpr uint32_t dark_red = pack_rgba(139, 0, 0);
    static constexpr uint32_t dark_blue = pack_rgba(0, 0, 139);
    static constexpr uint32_t midnight_blue = pack_rgba(25, 25, 112);
    static constexpr uint32_t slate_gray = pack_rgba(112, 128, 144);

    union {
        struct { uint8_t r, g, b, a; };
        uint32_t rgba = 0;
    };

    Color() = default;
    constexpr Color(uint32_t _rgba) : rgba(_rgba) {}
    constexpr Color(Color rgb, uint8_t _a) : r(rgb.r), g(rgb.g), b(rgb.b), a(_a) {}
    constexpr Color(const float(&c)[3]) : r(int(c[0] * 255.f)), g(int(c[1] * 255.f)), b(int(c[2] * 255.f)), a(255) {}
    constexpr Color(const float(&c)[4]) : r(int(c[0] * 255.f)), g(int(c[1] * 255.f)), b(int(c[2] * 255.f)), a(int(c[3] * 255.f)) {}
    constexpr Color(uint8_t _r, uint8_t _g, uint8_t _b, uint8_t _a = 255) : r(_r), g(_g), b(_b), a(_a) {}
    constexpr Color(std::string_view hex) : r(0), g(0), b(0), a(255)
    {
        if (!hex.empty() && hex.front() == '#') hex.remove_prefix(1);
        if (hex.size() != 6 && hex.size() != 8) return;

        r = parse_byte(hex, 0);
        g = parse_byte(hex, 2);
        b = parse_byte(hex, 4);
        a = (hex.size() == 8) ? parse_byte(hex, 6) : std::uint8_t(255);
    }

    constexpr Color& operator =(const Color& rhs) { rgba = rhs.rgba; return *this; }
    [[nodiscard]] constexpr bool operator ==(const Color& rhs) const { return rgba == rhs.rgba; }

    constexpr operator uint32_t() const { return rgba; }
    constexpr Vec4<float> vec4() const { return { (float)r / 255.0f, (float)g / 255.0f, (float)b / 255.0f, (float)a / 255.0f }; }

    Color& adjustHue(float amount)
    {
        setHue(getHue() + amount);
        return *this;
    }

    [[nodiscard]] constexpr float getBrightness() const
    {
        const float rf = r / 255.0f;
        const float gf = g / 255.0f;
        const float bf = b / 255.0f;
        return std::max({ rf, gf, bf });
    }

    [[nodiscard]] constexpr float getLuminance() const
    {
        const float rf = r / 255.0f;
        const float gf = g / 255.0f;
        const float bf = b / 255.0f;
        return 0.2126f * rf + 0.7152f * gf + 0.0722f * bf;
    }

    [[nodiscard]] constexpr float getChroma() const
    {
        const float rf = r / 255.0f;
        const float gf = g / 255.0f;
        const float bf = b / 255.0f;

        const float maxc = std::max({ rf, gf, bf });
        const float minc = std::min({ rf, gf, bf });
        const float delta = maxc - minc;

        if (maxc < 1e-6f) return 0.0f;
        return delta / maxc;
    }

    [[nodiscard]] constexpr float getHueWeight() const
    {
        const float rf = r / 255.0f;
        const float gf = g / 255.0f;
        const float bf = b / 255.0f;

        const float maxc = std::max({ rf, gf, bf });
        const float minc = std::min({ rf, gf, bf });
        return maxc - minc;
    }

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

    Color& setHue(float hue)
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

        return *this;
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

    static float avgHueEstimate(const std::vector<Color>& colors)
    {
        constexpr float kPi = 3.14159265358979323846f;
        constexpr float kRad = kPi / 180.0f;
        constexpr float kDeg = 180.0f / kPi;

        double sx = 0.0, sy = 0.0, sw = 0.0;

        for (const Color& c : colors)
        {
            const float rf = c.r / 255.0f;
            const float gf = c.g / 255.0f;
            const float bf = c.b / 255.0f;

            const float maxc = std::max({ rf, gf, bf });
            const float minc = std::min({ rf, gf, bf });
            const float delta = maxc - minc;

            const double w = (delta > 1e-6f) ? (double)delta : 0.0;
            if (w == 0.0) continue;

            const double h = (double)c.getHue() * (double)kRad;
            sx += w * std::cos(h);
            sy += w * std::sin(h);
            sw += w;
        }

        if (sw == 0.0) return 0.0f;

        // If hues are broadly distributed, fall back to a dominant-hue

        const double mag = std::sqrt(sx * sx + sy * sy);
        const double R = mag / sw;
        if (R < 0.15)
        {
            constexpr int BINS = 360;
            double hist[BINS] = {};

            for (const Color& c : colors)
            {
                const float rf = c.r / 255.0f;
                const float gf = c.g / 255.0f;
                const float bf = c.b / 255.0f;

                const float maxc = std::max({ rf, gf, bf });
                const float minc = std::min({ rf, gf, bf });
                const float delta = maxc - minc;

                const double w = (delta > 1e-6f) ? (double)delta : 0.0;
                if (w == 0.0) continue;

                float h = c.getHue();
                int bin = (int)h;
                if (bin < 0) bin = 0;
                if (bin >= BINS) bin = BINS - 1;
                hist[bin] += w;
            }

            int best = 0;
            for (int i = 1; i < BINS; ++i)
                if (hist[i] > hist[best]) best = i;

            return (float)best + 0.5f;
        }

        float h = (float)(std::atan2(sy, sx) * kDeg); // [-180, 180]
        if (h < 0.0f) h += 360.0f;
        return h; // [0, 360)
    }

};

BL_END_NS
