#pragma once
#include <bitloop.h>

SIM_BEG;

inline double toNormalizedZoom(double zoom)
{
    return log(zoom) + 1.0;
}
inline double fromNormalizedZoom(double normalized_zoom)
{
    return exp(normalized_zoom - 1.0);
}

inline double toHeight(double zoom)
{
    return 1.0 / toNormalizedZoom(zoom);
}

inline double fromHeight(double height)
{
    return fromNormalizedZoom(1.0 / height);
}

inline int mandelbrotIterLimit(double zoom)
{
    const double l = std::log10(zoom * 400.0);
    int iters = static_cast<int>(-19.35 * l * l + 741.0 * l - 1841.0);
    return (100 + (std::max(0, iters))) * 3;
}

inline double qualityFromIterLimit(int iter_lim, double zoom_x)
{
    const int base = mandelbrotIterLimit(zoom_x);   // always >= 100
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
