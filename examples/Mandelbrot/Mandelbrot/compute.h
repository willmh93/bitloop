#ifdef _MSC_VER
#pragma float_control(precise, off)
#endif

#include "build_config.h"

#include "shading.h"
#include "conversions.h"

#include <cmath>
#include <complex>
#include <algorithm>

SIM_BEG;

inline MandelFloatQuality getRequiredFloatType(MandelSmoothing smoothing, flt128 zoom)
{
    flt128 MAX_ZOOM_FLOAT;
    flt128 MAX_DOUBLE_ZOOM;

    if (((int)smoothing & (int)MandelSmoothing::DIST))
    {
        MAX_ZOOM_FLOAT = f128(10);// 100;
        MAX_DOUBLE_ZOOM = f128(2e10);
    }
    else
    {
        MAX_ZOOM_FLOAT = f128(10000);
        MAX_DOUBLE_ZOOM = f128(2e12);
    }

    if (zoom < MAX_ZOOM_FLOAT) return MandelFloatQuality::F32;
    if (zoom < MAX_DOUBLE_ZOOM) return MandelFloatQuality::F64;

    return MandelFloatQuality::F128;
}



inline int mandelbrot_depth(double x0, double y0, int iter_lim)
{
    double x = 0.0, y = 0.0, xx = 0.0, yy = 0.0;
    int iter = 0;

    while (xx + yy <= 4.0 && iter < iter_lim)
    {
        y = (2.0 * x * y + y0);
        x = (xx - yy + x0);
        xx = x * x;
        yy = y * y;
        iter++;
    }

    return iter;
}

inline double mandelbrot_dist(double x0, double y0, int iter_lim)
{
    double x = 0.0, y = 0.0;        // z
    double dx = 1.0, dy = 0.0;        // dz/dc

    for (int i = 0; i < iter_lim; ++i)
    {
        double r2 = x * x + y * y;
        if (r2 > 512.0)
        {
            double r = std::sqrt(r2);                     // |z|
            double dz = std::sqrt(dx * dx + dy * dy);      // |dz|
            if (dz == 0.0) return 0.0;
            return r * std::log(r) / dz;                   // distance
        }

        // save current z for the derivative update
        double xold = x;
        double yold = y;

        // z_{n+1} = z_n^2 + c
        x = xold * xold - yold * yold + x0;
        y = 2.0 * xold * yold + y0;

        // dz_{n+1} = 2 z_n dz_n + 1   (uses xold,yold)
        double dx_new = 2.0 * (xold * dx - yold * dy) + 1.0;
        double dy_new = 2.0 * (xold * dy + yold * dx);

        dx = dx_new;
        dy = dy_new;
    }
    return INSIDE_MANDELBROT_SET;                         // inside set
}

namespace detail
{
    // Complex helpers

    template<class T> struct cplx { T x, y; };

    template<class T>
    FAST_INLINE constexpr void step(cplx<T>& z, const cplx<T>& c)
    {
        T xx = z.x * z.x;
        T yy = z.y * z.y;
        T xy = z.x * z.y;

        z.x = xx - yy + c.x;
        z.y = (xy + xy) + c.y;           // 2*x*y + cy
    }

    template<class T>
    FAST_INLINE constexpr void step_d(const cplx<T>& z, cplx<T>& dz) // dz in/out
    {
        const T zx_dzx = z.x * dz.x;
        const T zy_dzy = z.y * dz.y;
        const T zx_dzy = z.x * dz.y;
        const T zy_dzx = z.y * dz.x;

        dz.x = ((zx_dzx - zy_dzy) + (zx_dzx - zy_dzy)) + T(1);
        dz.y = (zx_dzy + zy_dzx) + (zx_dzy + zy_dzx);
    }

    template<class T>
    FAST_INLINE constexpr T mag2(const cplx<T>& z)
    {
        return z.x * z.x + z.y * z.y;
    }


} // namespace detail


template<MandelSmoothing Smooth_Iter>
constexpr float escape_radius()
{
    return (((int)Smooth_Iter & (int)MandelSmoothing::DIST) ? 512.0 : 64.0);
}

template<MandelSmoothing Smooth_Iter>
constexpr float mandelbrot_smoothing_offset()
{
    constexpr float r2 = escape_radius<Smooth_Iter>();
    return log2(log2(r2)) - 1.0f;
}

template<typename T>
FAST_INLINE bool interiorCheck(T x0, T y0)
{
    constexpr T a = T(0.25);
    constexpr T b = T(0.0625);
    constexpr T one = T(1);

    const T x_minus_a = x0 - a;
    const T q = x_minus_a * x_minus_a + y0 * y0;

    // Cardioid check (main bulb)
    if (q * (q + x_minus_a) < a * y0 * y0)
        return true;

    // Period-2 bulb check (left-side circle)
    const T x_plus_1 = x0 + one;
    if ((x_plus_1 * x_plus_1 + y0 * y0) < b)
        return true;

    return false;
}


template<class T> FAST_INLINE T clamp01(T v)
{
    return v < T(0) ? T(0) : (v > T(1) ? T(1) : v);
}

template<class T, MandelSmoothing S>
FAST_INLINE void mandel_kernel(
    const T& x0,
    const T& y0,
    int iter_lim,
    double& depth,
    T& dist,
    float& stripes ,
    StripeParams sp = {})
{
    //if (interiorCheck(x0, y0)) {
    //    depth = INSIDE_MANDELBROT_SET_SKIPPED;
    //    if constexpr (((int)S & (int)MandelSmoothing::DIST) != 0) dist = T{ INSIDE_MANDELBROT_SET };
    //    if constexpr (((int)S & (int)MandelSmoothing::STRIPES) != 0) stripes = 0.0;
    //    return;
    //}

    using detail::cplx;
    constexpr bool NEED_DIST = (bool)((int)S & (int)MandelSmoothing::DIST);
    constexpr bool NEED_SMOOTH_ITER = (bool)((int)S & (int)MandelSmoothing::ITER);
    constexpr bool NEED_STRIPES = (bool)((int)S & (int)MandelSmoothing::STRIPES);

    constexpr T escape_r2 = T(escape_radius<S>());
    constexpr T zero = T(0), one = T(1);

    int iter = 0;
    T r2 = T{ 0 };

    cplx<T> z{ zero, zero };
    cplx<T> c{ x0, y0 };
    cplx<T> dz{ one, zero };

    // stripe accumulators
    float sum = 0.0, last_added = 0.0;
    int sum_samples = 0;

    while (true)
    {
        if constexpr (NEED_DIST)
            detail::step_d(z, dz);

        cplx<T> z0 = z;

        // step
        detail::step(z, c);
        ++iter;


        // update radius^2
        r2 = detail::mag2(z);

        if constexpr (NEED_STRIPES)
        {
            //float a = (float)atan2(z.y, z.x);
            float a = (float)Math::atan2f_fast((float)z.y, (float)z.x);
            last_added = 0.5f + 0.5f * std::sin(sp.freq * a + sp.phase);
            sum += last_added;
            ++sum_samples;
        }

        if (r2 > escape_r2 || iter >= iter_lim) break;
    }

    const bool escaped = (r2 > escape_r2) && (iter < iter_lim);

    if constexpr (NEED_DIST)
    {
        if (escaped) {
            const T r = sqrt(r2);
            const T dz_abs = sqrt(detail::mag2(dz));
            dist = (dz_abs == zero) ? T{ 0 } : (r * log(r) / dz_abs); // log_as_double?
        }
        else {
            dist = T{ -1 };// T{ INSIDE_MANDELBROT_SET };
        }
    }

    if (!escaped)
    {
        depth = INSIDE_MANDELBROT_SET;
        if constexpr (NEED_STRIPES) stripes = 0.0;
        return;
    }

    // smooth iteration depth
    if constexpr (NEED_SMOOTH_ITER)
    {
        const double log_abs_z = 0.5 * (double)log(r2);
        const double nu = (double)iter + 1.0 - std::log2(std::max(log_abs_z, 1e-30));
        depth = nu - mandelbrot_smoothing_offset<S>();
    }
    else
    {
        depth = (double)iter;
    }

    if constexpr (NEED_STRIPES)
    {
        float avg = (sum_samples > 0) ? (sum / (float)sum_samples) : 0.0f;
        float prev = (sum_samples > 1) ? ((sum - last_added) / (float)(sum_samples - 1)) : avg;
        
        // stripeAC interpolation weight (fraction inside the last band)
        // frac = 1 + log2( log(ER^2) / log(|z|^2) ), clamped to [0,1]
        float frac = 1.0f + (float)std::log2(log_as_double(escape_r2) / std::max(log_as_double(r2), 1e-300));
        frac = clamp01(frac);
        
        float mix = frac * avg + (1.0f - frac) * prev; // linear interpolation
        mix = 0.5f + 0.5f * std::tanh(sp.contrast * (mix - 0.5f)); // optional shaping
        stripes = clamp01(mix);
    }
}


template<bool smooth>
inline double mandelbrot_spline_iter(double x0, double y0, int iter_lim, ImSpline::Spline& x_spline, ImSpline::Spline& y_spline)
{
    double x = 0.0, y = 0.0, xx = 0.0, yy = 0.0;
    int iter = 0;
    while (xx + yy <= 4.0 && iter < iter_lim)
    {
        y = (2.0 * x * y + y0);
        x = (xx - yy + x0);

        float spline_xx = x_spline(static_cast<float>(x * x));
        float spline_yy = y_spline(static_cast<float>(y * y));

        xx = spline_xx;
        yy = spline_yy;

        iter++;
    }

    // Ensures black for deep-set points
    if (iter == iter_lim)
        return iter_lim;

    if constexpr (smooth)
        return iter + (1.0 - log2(log2(xx + yy) / 2.0));
    else
        return iter;
}

/*template<
    bool Smooth,
    bool Show_Period2_Bulb
>
bool radialMandelbrot()
{
    //double f_max_iter = static_cast<double>(iter_lim);
    return pending_bmp->forEachWorldPixel(camera, current_row, [&](int x, int y, double angle, double point_dist)
    {
        DVec2 polard_coord = cardioid_lerper.originalPolarCoordinate(angle, point_dist, cardioid_lerp_amount);

        // Below x-axis
        if (polard_coord.y < 0)
        {
            pending_bmp->setPixel(x, y, 0, 0, 0, 255);
            return;
        }

        DVec2 mandel_pt = Cardioid::fromPolarCoordinate(polard_coord.x, polard_coord.y);
        double recalculated_orig_angle = cardioid_lerper.originalPolarCoordinate(mandel_pt.x, mandel_pt.y, 1.0).x;

        // Avoid picking pixels from opposite side of the cardioid which would be sampled twice
        bool hide =
            (polard_coord.x < Math::PI && recalculated_orig_angle > Math::PI * 1.1) ||
            (polard_coord.x > Math::PI && recalculated_orig_angle < Math::PI * 0.9);

        if (hide)
        {
            pending_bmp->setPixel(x, y, 0, 0, 0, 255);
            return;
        }

        // Optionally hide everything left of main cardioid
        if constexpr (!Show_Period2_Bulb)
        {
            if (mandel_pt.x < -0.75)
            {
                pending_bmp->setPixel(x, y, 0, 0, 0, 255);
                return;
            }
        }

        double smooth_iter = mandelbrot_iter<Smooth>(mandel_pt.x, mandel_pt.y, iter_lim);

        uint32_t u32;
        iter_gradient_color(smooth_iter, u32);

        //double ratio = Smooth_Iter / f_max_iter;
        //iter_ratio_color(ratio, r, g, b);

        pending_bmp->setPixel(x, y, u32);
    });
};*/


/// --------------------------------------------------------------
/// Uses the bmp's CanvasObject::worldQuad to to loop over pixels,
/// and calculates mandelbrot set for all pixels which don't already
/// have a depth > 0 (i.e. Filling in the blanks)
/// --------------------------------------------------------------

template<typename T, MandelSmoothing Smoothing, bool flatten>
bool mandelbrot(CanvasImage128* bmp, EscapeField* field, int iter_lim, int threads, int timeout, int& current_row, StripeParams stripe_params={})
{
    bool frame_complete = bmp->forEachWorldPixel<T>(current_row, [&](int x, int y, T wx, T wy)
    {
        EscapeFieldPixel& field_pixel = field->at(x, y);
        if (field_pixel.flag_for_skip)
        {
            field_pixel.depth = INSIDE_MANDELBROT_SET_SKIPPED;
            return;
        }

        double depth = field_pixel.depth;
        if (depth >= 0) return;

        T dist{};
        float stripe = 0.0f;
        mandel_kernel<T, Smoothing>(wx, wy, iter_lim, depth, dist, stripe, stripe_params);

        field_pixel.depth = depth;    // not likely not need more than float precision
        field_pixel.stripe = stripe;  // not likely not need more than float precision
        field_pixel.setDist(dist);    // store dist with appropriate precision

    }, threads, timeout);

    return frame_complete;
};

template<typename T>
void refreshFieldDepthNormalized(
    EscapeField* pending_field,
    CanvasImage128* pending_bmp,
    MandelSmoothing smoothing_type,
    bool   cycle_dist_invert,
    double cycle_dist_sharpness,
    bool   cycle_iter_normalize_depth,
    double  cycle_iter_log1p_weight,
    int threads
)
{
    pending_field->min_depth = std::numeric_limits<double>::max();
    pending_field->max_depth = std::numeric_limits<double>::lowest();

    // Redetermine minimum depth for entire visible field
    pending_bmp->forEachPixel([&](int x, int y)
    {
        EscapeFieldPixel& field_pixel = pending_field->at(x, y);
        double depth = field_pixel.depth;

        if (depth >= INSIDE_MANDELBROT_SET_SKIPPED || field_pixel.flag_for_skip) return;
        if (depth < pending_field->min_depth) pending_field->min_depth = depth;
        if (depth > pending_field->max_depth) pending_field->max_depth = depth;
    }, 0);

    if (pending_field->min_depth == std::numeric_limits<double>::max()) pending_field->min_depth = 0;


    // Calculate normalized depth/dist
    double dist_min_pixel_ratio = ((100.0 - cycle_dist_sharpness) / 100.0 + 0.00001);
    T stable_min_raw_dist = Camera::active->stageToWorldOffset<T>(dist_min_pixel_ratio, 0.0).magnitude(); // fraction of a pixel
    T stable_max_raw_dist = T{ (pending_bmp->worldSize().magnitude()) } / T{ 2.0 };                         // half diagonal world viewport size

    // @@ todo: Use downgraded type from: stable_min_raw_dist?
    T stable_min_dist = (cycle_dist_invert ? -1 : 1) * log(stable_min_raw_dist);
    T stable_max_dist = (cycle_dist_invert ? -1 : 1) * log(stable_max_raw_dist);

    blPrint() << "stable_min_dist: " << stable_min_dist << "   stable_max_raw_dist: " << stable_max_raw_dist;

    pending_bmp->forEachPixel([&](int x, int y)
    {
        EscapeFieldPixel& field_pixel = pending_field->at(x, y);
        double depth = field_pixel.depth;
        if (depth >= INSIDE_MANDELBROT_SET_SKIPPED || field_pixel.flag_for_skip) return;

        T raw_dist = field_pixel.getDist<T>();
        if (raw_dist < std::numeric_limits<T>::epsilon())
            raw_dist = std::numeric_limits<T>::epsilon();

        // raw_dist is highest next to mandelbrot set
        T dist = ((int)smoothing_type & (int)MandelSmoothing::DIST) ?
            ((cycle_dist_invert ? T{-1.0} : T{1.0}) * log(raw_dist))
            : T{0};

        // 
        float dist_factor = (float)(cycle_dist_invert ?
             -Math::lerpFactor(dist, stable_min_dist, stable_max_dist) :
              Math::lerpFactor(dist, stable_min_dist, stable_max_dist)
        );

        double floor_depth = cycle_iter_normalize_depth ? pending_field->min_depth : 0;

        float final_dist = dist_factor;
        float final_depth = (float)Math::linear_log1p_lerp(depth - floor_depth, cycle_iter_log1p_weight);

        #ifdef BL_DEBUG
        ///if (!isfinite(final_depth) || !isfinite(final_dist))
        ///{
        ///    // There is a definite crash here, don't remove until you've caught and solved it
        ///    blBreak();
        ///}
        #endif

        field_pixel.final_depth = final_depth;
        field_pixel.final_dist = final_dist;
    }, threads);
}

template<MandelShaderFormula F, bool maxdepth_show_optimized>
void shadeBitmap(
    EscapeField* active_field,
    CanvasImage128* bmp,
    ImGradient* gradient,
    float max_final_depth,
    float max_final_dist,
    float iter_weight,
    float dist_weight,
    float stripe_weight,
    int threads
)
{
    double iter_ratio, dist_ratio, stripe_ratio;
    shadingRatios(
        iter_weight, dist_weight, stripe_weight,
        iter_ratio, dist_ratio, stripe_ratio
    );

    blPrint() << "max_final_depth: " << max_final_depth;
    blPrint() << "max_final_dist: " << max_final_dist;

    bmp->forEachPixel([&, active_field](int x, int y)
    {
        EscapeFieldPixel& field_pixel = active_field->at(x, y);

        if (field_pixel.depth >= INSIDE_MANDELBROT_SET_SKIPPED)
        {
            if constexpr (maxdepth_show_optimized)
            {
                if (field_pixel.depth == INSIDE_MANDELBROT_SET_SKIPPED)
                    bmp->setPixel(x, y, 0xFF7F007F);
                else
                    bmp->setPixel(x, y, 0xFF000000);
            }
            else
            {
                bmp->setPixel(x, y, 0xFF000000);
            }
            return;
        }

        uint32_t u32;

        float iter_v = field_pixel.final_depth / max_final_depth;// log_color_cycle_iters;
        float dist_v = field_pixel.final_dist / max_final_dist; // cycle_dist_value;
        float stripe_v = field_pixel.stripe;

        float combined_t;

        if constexpr (F == MandelShaderFormula::ITER_DIST_STRIPE)
        {
            combined_t = Math::wrap(
                ((iter_v * (float)iter_ratio) +
                (dist_v * (float)dist_ratio)) +
                (stripe_v * (float)stripe_ratio),
                0.0f, 1.0f
            );
        }
        else if constexpr (F == MandelShaderFormula::ITER_DIST__MUL__STRIPE)
        {
            combined_t = Math::wrap(
                ((iter_v * (float)iter_ratio) + (dist_v * (float)dist_ratio)) *
                (stripe_v * (float)stripe_ratio),
                0.0f, 1.0f
            );
        }
        else if constexpr (F == MandelShaderFormula::ITER__MUL__DIST_STRIPE)
        {
            combined_t = Math::wrap(
                (iter_v * (float)iter_ratio) *
                ((dist_v * (float)dist_ratio) * (stripe_v * (float)stripe_ratio)),
                0.0f, 1.0f
            );
        }
        else if constexpr (F == MandelShaderFormula::ITER_STRIPE__MULT__DIST)
        {
            combined_t = Math::wrap(
                (stripe_v * (float)stripe_ratio) *
                ((iter_v * (float)iter_ratio) * (stripe_v * (float)stripe_ratio)),
                0.0f, 1.0f
            );
        }

        gradient->unguardedRGBA(combined_t, u32);

        bmp->setPixel(x, y, u32);
    }, threads);
}

SIM_END;
