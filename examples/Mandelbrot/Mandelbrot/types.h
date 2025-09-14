#pragma once
#include <bitloop.h>

#include <math.h>
#include <cmath>
#include <vector>

SIM_BEG;

enum MandelFlag : uint32_t
{
    // bits
    MANDEL_DYNAMIC_ITERS        = 1u << 0,
    MANDEL_SHOW_AXIS            = 1u << 1,
    MANDEL_FLATTEN              = 1u << 2,
    MANDEL_DYNAMIC_COLOR_CYCLE  = 1u << 3,
    MANDEL_NORMALIZE_DEPTH      = 1u << 4,
    MANDEL_INVERT_DIST          = 1u << 5,
    MANDEL_USE_SMOOTHING        = 1u << 6,

    // bitmasks
    MANDEL_FLAGS_MASK = 0x000FFFFFu, // max 24 bit-flags
    MANDEL_SMOOTH_MASK = 0x00F00000u, // max 16 smooth types
    MANDEL_VERSION_MASK = 0xFF000000u, // max 255 versions

    MANDEL_VERSION_BITSHIFT = 24
};

struct StripeParams
{
    double freq = 8.0;  // stripes per 2π
    double phase = 0.0;  // radians
    double contrast = 3.0;  // tanh shaping
    int    skip = 0;    // skip first (skip) iterates (z1..z_skip)

    StripeParams(double _freq = 8.0, double _phase = 0.0, double _contrast = 3.0)
        : freq(_freq), phase(_phase), contrast(_contrast)
    {}

    bool operator==(const StripeParams&) const = default;
};


struct EscapeFieldPixel
{
    double depth;
    double dist;
    double stripe;

    double final_depth;
    double final_dist;
};

struct EscapeField : public std::vector<EscapeFieldPixel>
{
    int compute_phase;

    double min_depth = 0.0;
    double max_depth = 0.0;
    //double min_dist = 0.0;
    //double max_dist = 0.0;

    int w = 0, h = 0;

    EscapeField(int phase) : compute_phase(phase) {}

    void setAllDepth(double value)
    {
        for (int i = 0; i < size(); i++)
            std::vector<EscapeFieldPixel>::at(i) = { value, value };
    }
    void setDimensions(int _w, int _h)
    {
        if (size() >= (_w * _h)) return;
        w = _w;
        h = _h;
        resize(w * h, { -1.0, -1.0 });
    }

    EscapeFieldPixel& operator ()(int x, int y)
    {
        return std::vector<EscapeFieldPixel>::at(y * w + x);
    }

    EscapeFieldPixel& at(int x, int y)
    {
        return std::vector<EscapeFieldPixel>::at(y * w + x);
    }

    EscapeFieldPixel* get(int x, int y)
    {
        int i = y * w + x;
        if (i < 0 || i >= size()) return nullptr;
        return data() + i;
    }
};

SIM_END;
