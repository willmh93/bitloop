#pragma once
#include <bitloop.h>

SIM_BEG;

using namespace bl;

inline flt128 toNormalizedZoom(flt128 zoom)
{
    return log(zoom) + flt128{ 1.0 };
}
inline flt128 fromNormalizedZoom(flt128 normalized_zoom)
{
    return exp(normalized_zoom - flt128{ 1.0 });
}

inline flt128 toHeight(flt128 zoom)
{
    return flt128{ 1.0 } / toNormalizedZoom(zoom);
}

inline flt128 fromHeight(flt128 height)
{
    return fromNormalizedZoom(flt128{ 1.0 } / height);
}

inline int mandelbrotIterLimit(flt128 zoom)
{
    const flt128 l = log10(zoom * 400.0);
    int iters = static_cast<int>(-19.35 * l * l + 741.0 * l - flt128{ 1841.0 });
    return (100 + (std::max(0, iters))) * 3;
}

inline double qualityFromIterLimit(int iter_lim, flt128 zoom)
{
    const int base = mandelbrotIterLimit(zoom);   // always >= 100
    if (base == 0) return 0.0;                      // defensive, though impossible here
    return static_cast<double>(iter_lim) / base;    // <= true quality by < 1/base
}

inline int finalIterLimit(
    CameraViewController& view,
    double quality,
    bool dynamic_iter_lim,
    bool tweening)
{
    if (dynamic_iter_lim)
        return (int)(mandelbrotIterLimit(view.zoom) * quality);
    else
    {
        int iters = (int)(quality);
        if (tweening)
            iters = std::min(iters, (int)(mandelbrotIterLimit(view.zoom) * 0.25f));
        return iters;
    }
}

inline void shadingRatios(
    double  iter_weight, double  dist_weight, double  stripe_weight,
    double& iter_ratio,  double& dist_ratio,  double& stripe_ratio)
{
    if (iter_weight + dist_weight + stripe_weight == 0.0)
        iter_weight = 1.0;

    double sum = iter_weight + dist_weight + stripe_weight;
    iter_ratio   = iter_weight / sum;
    dist_ratio   = dist_weight / sum;
    stripe_ratio = stripe_weight / sum;
}

inline std::string dataToURL(std::string config_buf)
{
    #ifdef __EMSCRIPTEN__
    return platform()->url_get_base() + "?data=" + config_buf;
    #else
    return "https://bitloop.dev/Mandelbrot?data=" + config_buf;
    #endif
}

SIM_END;
