#ifdef _MSC_VER
#pragma float_control(precise, off)
#endif

#include "shading.h"

SIM_BEG;

constexpr double INSIDE_MANDELBROT_SET = std::numeric_limits<double>::max();
const double INSIDE_MANDELBROT_SET_SKIPPED = std::nextafter(INSIDE_MANDELBROT_SET, 0.0);

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
constexpr double escape_radius()
{
    return (((int)Smooth_Iter & (int)MandelSmoothing::DIST) ? 512.0 : 64.0);
}

template<MandelSmoothing Smooth_Iter>
constexpr double mandelbrot_smoothing_offset()
{
    constexpr double r2 = escape_radius<Smooth_Iter>();
    return log2(log2(r2)) - 1.0;
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



//dist = mandelbrot_dist((double)x0, (double)y0, iter_lim);

template<class T, MandelSmoothing S>
FAST_INLINE void mandel_kernel(
    const T& x0,
    const T& y0,
    int iter_lim,
    double& depth, double& dist)
{
    if (interiorCheck(x0, y0))
    {
        depth = INSIDE_MANDELBROT_SET_SKIPPED;
        if constexpr (((int)S & (int)MandelSmoothing::DIST) != 0)
            dist = INSIDE_MANDELBROT_SET;
        return;
    }

    using detail::cplx;
    constexpr bool NEED_DIST = (bool)((int)S & (int)MandelSmoothing::DIST);
    constexpr bool NEED_SMOOTH_ITER = (bool)((int)S & (int)MandelSmoothing::ITER);
    constexpr T escape_r2 = T(escape_radius<S>());
    constexpr T zero = T(0), one = T(1), two = T(2);

    cplx<T> z{ zero, zero };
    cplx<T> c{ x0, y0 };
    cplx<T> dz{ one, zero };
    int iter = 0;
    T r2;

    while (true)
    {
        r2 = detail::mag2(z);
        if (r2 > escape_r2 || iter >= iter_lim) break;

        if constexpr (NEED_DIST)
            detail::step_d(z, dz);

        detail::step(z, c);
        ++iter;
    }

    const bool escaped = (r2 > escape_r2) && (iter < iter_lim);
    if constexpr (NEED_DIST)
    {
        if (escaped)
        {
            const T r = sqrt(r2);
            const T dz_abs = sqrt(detail::mag2(dz));
            dist = (dz_abs == zero) ? 0.0 : (double)(r * log(r) / dz_abs);
        }
        else
        {
            dist = INSIDE_MANDELBROT_SET;
        }
    }

    if (!escaped) {
        depth = INSIDE_MANDELBROT_SET;
        return;
    }

    if constexpr (NEED_SMOOTH_ITER)
    {
        T t = log2(r2) / two;
        T s = log2(t);
        depth = (double)(iter + (one - s)) - mandelbrot_smoothing_offset<S>();
    }
    else
    {
        depth = (double)iter;
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

SIM_END;
