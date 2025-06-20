#pragma once
#include "project.h"
#include "Cardioid.h"
#include <math.h>

SIM_BEG(Mandelbrot)

enum MandelFlag : uint32_t
{
    // bits
    MANDEL_DYNAMIC_ITERS        = 1u << 0,
    MANDEL_SHOW_AXIS            = 1u << 1,
    MANDEL_FLATTEN              = 1u << 2,
    MANDEL_DYNAMIC_COLOR_CYCLE  = 1u << 3,
    MANDEL_NORMALIZE_DEPTH      = 1u << 4,

    // bitmasks
    MANDEL_FLAGS_MASK   = 0x000FFFFFu, // max 24 bit-flags
    MANDEL_SMOOTH_MASK  = 0x00F00000u, // max 16 smooth types
    MANDEL_VERSION_MASK = 0xFF000000u, // max 255 versions

    MANDEL_SMOOTH_BITSHIFT = 20,
    MANDEL_VERSION_BITSHIFT = 24
};

enum class MandelSmoothing
{
    NONE = 0,
    ITER = 1,
    DIST = 2,
    MIX = 3, // ITER | DIST
    COUNT
};

//static const char* MandelSmoothingNames[2] = {
//    "None",
//    "Iteration",
//    "Distance"
//};

enum class MandelTransform
{
    NONE,
    FLATTEN,
    //COUNT
};

enum ColorGradientTemplate
{
    GRADIENT_CUSTOM,

    GRADIENT_CLASSIC,
    GRADIENT_SINUSOIDAL_RAINBOW_CYCLE,
    GRADIENT_WAVES,


    GRADIENT_TEMPLATE_COUNT
};

static const char* ColorGradientNames[GRADIENT_TEMPLATE_COUNT] = {
    "",
    "CLASSIC",
    "SINUSOIDAL_RAINBOW_CYCLE",
    "WAVES"
};

struct EscapeFieldPixel
{
    double depth;
    double dist;

    double final_value;
};

struct EscapeField : public std::vector<EscapeFieldPixel>
{
    int compute_phase;

    double min_depth = 0.0;
    double max_depth = 0.0;

    double min_dist = 0.0;
    double max_dist = 0.0;

    int w=0, h=0;

    EscapeField(int phase) : compute_phase(phase)
    {}

    void setAllDepth(double value)
    {
        for (int i = 0; i < size(); i++)
            at(i) = { value, value };
    }
    void setDimensions(int _w, int _h)
    {
        if (size() >= (_w * _h))
            return;
        w = _w;
        h = _h;
        resize(w * h, { -1.0, -1.0 });
    }
    
    EscapeFieldPixel& operator ()(int x, int y)
    {
        return at(y * w + x);
    }

    EscapeFieldPixel& get(int x, int y)
    {
        return at(y * w + x);
    }

    EscapeFieldPixel& get(IVec2 pos)
    {
        return at(pos.y * w + pos.x);
    }
    //void setPixelDepth(int x, int y, double depth)
    //{
    //    at(y * w + x) = depth;
    //}
    //double getPixelDepth(int x, int y)
    //{
    //    return at(y * w + x);
    //}
};

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

constexpr double INSIDE_MANDELBROT_SET = std::numeric_limits<double>::max();

template<typename T, MandelSmoothing Smooth_Iter>
inline void mandelbrot_ex(T x0, T y0, int iter_lim, double& depth, double &dist)
{
    constexpr T escape_radius_squared = T(
        ((int)Smooth_Iter & (int)MandelSmoothing::DIST) ? 256.0 : 64.0
    );
    constexpr T one = T(1);
    constexpr T two = T(2);
    constexpr T eps = std::numeric_limits<T>::epsilon();

    constexpr int safety_bits = 4;
    constexpr T deriv_limit = std::numeric_limits<T>::max() / (1 << safety_bits);


    T x = T(0), y = T(0), xx = T(0), yy = T(0);
    int iter = 0;

    // Distance mode only
    T dx = one, dy = T(0); 
    T xold, yold, dx_new, dy_new;

    while (xx + yy <= escape_radius_squared && iter < iter_lim)
    {
        if constexpr ((int)Smooth_Iter & (int)MandelSmoothing::DIST)
        {
            xold = x;
            yold = y;

            x = xold * xold - yold * yold + x0;
            y = two * xold * yold + y0;

            dx_new = two * (xold * dx - yold * dy) + one;
            dy_new = two * (xold * dy + yold * dx);

            //if (!Math::isfinite(dx_new) || !Math::isfinite(dy_new))
            //{
            //    DebugBreak();
            //}

            dx = dx_new;
            dy = dy_new;

            if (dx > deriv_limit || dy > deriv_limit || dx < -deriv_limit || dy < -deriv_limit)
                break;
        }
        else
        {
            y = two * x * y + y0;
            x = xx - yy + x0;
        }

        xx = x * x;
        yy = y * y;
        iter++;
    }

    

    if constexpr ((int)Smooth_Iter & (int)MandelSmoothing::DIST)
    {
        // Calculating Distance necessary
        //if (xx + yy <= escape_radius_squared)
        //{
        //    out = INSIDE_MANDELBROT_SET;
        //    return;
        //}

        T r2 = xx + yy;
        T r = sqrt(r2);
        T dz = sqrt(dx * dx + dy * dy);
        T d = ((dz == T(0)) ? T(0) : r * log(r) / dz);
        if (d < eps) d = eps;

        //T normalized_d = -log(d);
        //dist = static_cast<double>(normalized_d);
        dist = static_cast<double>(d);
    }

    if (iter == iter_lim)
        depth = INSIDE_MANDELBROT_SET;
    else
    {
        if constexpr ((int)Smooth_Iter & (int)MandelSmoothing::ITER)
        {
            // Calculating smoothed iteration necessary
            T t = log2(xx + yy) / two;
            T s = log2(t);
            T value = static_cast<T>(iter) + (one - s);
            depth = static_cast<double>(value);
        }
        else
        {
            depth = static_cast<double>(iter);
        }
    }
}

template<typename T, MandelSmoothing Smooth_Iter>
inline double mandelbrot_iter_ex(T x0, T y0, int iter_lim)
{
    constexpr T escape_limit = T(64.0);
    //constexpr T escape_limit = T(256.0);
    constexpr T one = T(1);
    constexpr T two = T(2);

    T x = T(0), y = T(0), xx = T(0), yy = T(0);
    int iter = 0;

    while (xx + yy <= escape_limit && iter < iter_lim)
    {
        y = (two * x * y + y0);
        x = (xx - yy + x0);
        xx = x * x;
        yy = y * y;
        iter++;
    }

    // Ensures black for deep-set points
    if (iter == iter_lim)
        return INSIDE_MANDELBROT_SET;

    if constexpr (Smooth_Iter == MandelSmoothing::ITER)
    {
        T t = log2(xx + yy) / two;
        T s = log2(t);
        T value = static_cast<T>( iter ) + (one - s);

        return static_cast<double>(value);
    }
    else
        return static_cast<double>(iter);
}

inline double mandelbrot_dist(double x0, double y0, int iter_lim)
{
    double x = 0.0, y = 0.0;        // z
    double dx = 1.0, dy = 0.0;        // dz/dc

    for (int i = 0; i < iter_lim; ++i)
    {
        double r2 = x * x + y * y;
        if (r2 > 4.0)
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
    return -1.0;                         // inside set
}

template<bool smooth>
inline double mandelbrot_iter(double x0, double y0, int iter_lim)
{
    double x = 0.0, y = 0.0, xx = 0.0, yy = 0.0;
    int iter = 0;

    while (xx + yy <= 64.0 && iter < iter_lim)
    {
        y = (2.0 * x * y + y0);
        x = (xx - yy + x0);
        xx = x * x;
        yy = y * y;
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


template<ColorGradientTemplate type>
inline void colorGradientTemplate(double t, uint8_t& r, uint8_t& g, uint8_t& b)
{
    if constexpr (type == GRADIENT_CLASSIC)
    {
        r = (uint8_t)(9 * (1 - t) * t * t * t * 255);
        g = (uint8_t)(15 * (1 - t) * (1 - t) * t * t * 255);
        b = (uint8_t)(8.5 * (1 - t) * (1 - t) * (1 - t) * t * 255);
    }
    else if constexpr (type == GRADIENT_SINUSOIDAL_RAINBOW_CYCLE)
    {
        float a = (float)t * 3.14159265f;
        r = (uint8_t)(sin(a) * sin(a) * 255);
        g = (uint8_t)(sin(a + 2.0944f) * sin(a + 2.0944f) * 255);
        b = (uint8_t)(sin(a + 4.1888f) * sin(a + 4.1888f) * 255);
    }
}

void colorGradientTemplate(ColorGradientTemplate type, double t, uint8_t& r, uint8_t& g, uint8_t& b)
{
    switch (type)
    {
    case GRADIENT_CLASSIC:                  colorGradientTemplate<GRADIENT_CLASSIC>(t, r, g, b); break;
    //case GRADIENT_FOREST:                   colorGradientTemplate<GRADIENT_FOREST>(t, r, g, b); break;
    case GRADIENT_SINUSOIDAL_RAINBOW_CYCLE: colorGradientTemplate<GRADIENT_SINUSOIDAL_RAINBOW_CYCLE>(t, r, g, b); break;
    default: break;
    }
}

void RGBtoHSV(uint8_t r, uint8_t g, uint8_t b,
    float& h, float& s, float& v)
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



struct TweenableMandelState
{
    // The final world quad should be predictable using
    // only the state info & stage size.
    DVec2 reference_zoom, ctx_stage_size;
    DVec2 ctx_world_size() const {
        return ctx_stage_size / reference_zoom;
    }

    /// ======== Saveable/Tweenable info ========
    bool show_axis = true;
    double cam_x = -0.5;
    double cam_y = 0.0;
    double cam_rot = 0.0;
    double cam_zoom = 1.0;
    DVec2  cam_zoom_xy = DVec2(1, 1);

    bool dynamic_iter_lim = true;
    double quality = 0.5; // Used for UI (ignore during tween until complete)
    double smooth_iter_dist_ratio = 0.0;

    bool dynamic_color_cycle_limit = true;
    bool normalize_depth_range = true;
    double log1p_weight = 0.0;
    
    double color_cycle_value = 0.5f; // If dynamic, iter_lim ratio, else iter_lim
    
    double gradient_shift = 0.0;
    double hue_shift = 0.0;

    double gradient_shift_step = 0.0078;
    double hue_shift_step = 0.136;

    int active_color_template = GRADIENT_CLASSIC;
    int smoothing_type = (int)MandelSmoothing::ITER;

    // Gradient
    //ImGradient current_gradient;
    ImGradient gradient;

    // animate
    bool show_color_animation_options = false;

    // Flattening
    bool flatten = false;
    double flatten_amount = 0.0;

    bool operator ==(const TweenableMandelState& rhs) const
    {
        return (
            cam_x == rhs.cam_x &&
            cam_y == rhs.cam_y &&
            cam_rot == rhs.cam_rot &&
            cam_zoom == rhs.cam_zoom &&
            cam_zoom_xy == rhs.cam_zoom_xy &&
            quality == rhs.quality &&
            smooth_iter_dist_ratio == rhs.smooth_iter_dist_ratio &&
            dynamic_iter_lim == rhs.dynamic_iter_lim &&
            normalize_depth_range == rhs.normalize_depth_range &&
            log1p_weight == rhs.log1p_weight &&
            color_cycle_value == rhs.color_cycle_value &&
            gradient_shift == rhs.gradient_shift &&
            hue_shift == rhs.hue_shift &&
            gradient_shift_step == rhs.gradient_shift_step &&
            hue_shift_step == rhs.hue_shift_step &&
            smoothing_type == rhs.smoothing_type &&
            gradient == rhs.gradient &&
            show_color_animation_options == rhs.show_color_animation_options &&
            flatten == rhs.flatten &&
            flatten_amount == rhs.flatten_amount
        );
    }

    std::string serialize();
    bool deserialize(std::string txt);
};

std::ostream& operator<<(std::ostream& os, const TweenableMandelState&);

enum class A { a0, a1, COUNT };
enum class B { b0, b1, b2, COUNT };
enum class C { c0, c1, c2, c3, COUNT };

struct Mandelbrot_Data : public VarBuffer, public TweenableMandelState
{
    template<typename Float, A a, bool  b, C c>
    Float foobar(Float d)
    {
        d = 5;
        // do something with Float, a, b, c
        DebugPrint("foo (a): %d", (int)a);
        DebugPrint("foo (b): %s", b ? "TRUE" : "FALSE");
        DebugPrint("foo (c): %d", (int)c);

        return d + 5;
    }

    template<typename Float, typename Int, A a, bool  b, C c>
    Float barfoo(Float d)
    {
        d = 5;
        // do something with Float, a, b, c
        DebugPrint("foo (a): %d", (int)a);
        DebugPrint("foo (b): %s", b ? "TRUE" : "FALSE");
        DebugPrint("foo (c): %d", (int)c);

        return d + 5;
    }

    char config_buf[1024] = "";
    char pos_tween_buf[1024] = "";
    char zoom_tween_buf[1024] = "";


    TweenableMandelState state_a;
    TweenableMandelState state_b;
    bool tweening = false;
    double tween_progress = 0.0; // 0..1
    double tween_lift = 0.0;

    DAngledRect r1;
    DAngledRect r2;

    double cardioid_lerp_amount = 1.0; // 1 - flatten

    double cam_degrees = 0.0;


    
    bool show_period2_bulb = true;
    bool interactive_cardioid = false;
    
    

    int iter_lim = 0; // Actual iter limit
 
    
    
    

    //double color_cycle_iters = 32.0;

    bool colors_updated = false;



    
    //ImGradient gradientB;
    //ImGradient gradientC;

    
 

    ImSpline::Spline x_spline = ImSpline::Spline(100, {
        {0.0f, 0.0f}, {0.1f, 0.1f}, {0.2f, 0.2f},
        {0.3f, 0.3f}, {0.4f, 0.4f}, {0.5f, 0.5f},
        {0.6f, 0.6f}, {0.7f, 0.7f}, {0.8f, 0.8f}
    });
    ImSpline::Spline y_spline = ImSpline::Spline(100, {
        {0.0f, 0.0f}, {0.1f, 0.1f}, {0.2f, 0.2f},
        {0.3f, 0.3f}, {0.4f, 0.4f}, {0.5f, 0.5f},
        {0.6f, 0.6f}, {0.7f, 0.7f}, {0.8f, 0.8f}
    });
    /*ImSpline::Spline iter_gradient_spline = ImSpline::Spline(100, {
        {0.0f, 0.0f}, {0.1f, 0.1f}, {0.2f, 0.2f},
        {0.3f, 0.3f}, {0.4f, 0.4f}, {0.5f, 0.5f}
    });*/
    
    //ImSpline::Spline tween_pos_spline = ImSpline::Spline(100, {
    //    {-0.2f, 0.0f}, {0.2f, 0.0f}, {0.5f, 0.0f},
    //    {0.5f, 1.0f}, {0.8f, 1.0f}, {1.2f, 1.0f}
    //});
    ImSpline::Spline tween_pos_spline = ImSpline::Spline(100,
        { {-0.147f,0.0f},{0.253f,0.0f},{0.553f,0.0f},{0.439f,1.0f},{0.74f,1.0f},{1.14f,1.0f} }
    );

    ImSpline::Spline tween_zoom_lift_spline = ImSpline::Spline(100,
        { {-0.1f,0.0f},{0.0f,0.0f},{0.1f,0.0f},{0.201f,0.802f},{0.251f,1.002f},{0.351f,1.402f},{0.643f,1.397f},{0.744f,0.997f},{0.794f,0.797f},{0.9f,0.0f},{1.0f,0.0f},{1.1f,0.0f} }
    );

    ImSpline::Spline tween_color_cycle = ImSpline::Spline(100,
        { { 0.072f, 0.0f }, { 0.5f,0.0f }, { 0.845f,0.0f }, { 0.75f,1.0f }, { 1.0f,1.0f }, { 1.25f,1.0f } }
    );
    //ImSpline::Spline tween_zoom_lift_spline = ImSpline::Spline(100, {
    //    {-0.1f, 0.0f}, {0.0f, 0.0f}, {0.1f, 0.0f},
    //    {0.2f, 0.3f}, {0.25f, 0.5f}, {0.35f, 0.9f},
    //    {0.65f, 0.9f}, {0.75f, 0.5f}, {0.8f, 0.3f},
    //    {0.9f, 0.0}, {1.0f, 0.0f}, {1.1f, 0.0f}
    //});

    void registerSynced() override
    {
        sync(config_buf);
        
        //sync(initial_viewport_world_size);
        //sync(current_viewport_world_size);
        sync(state_a);
        sync(state_b);
        sync(tweening);
        sync(tween_progress);
        sync(tween_lift);

        sync(tween_pos_spline);
        sync(tween_zoom_lift_spline);

        sync(reference_zoom);
        sync(ctx_stage_size);

        sync(r1);
        sync(r2);

        sync(show_axis);
        sync(cam_x);
        sync(cam_y);
        sync(cam_degrees);
        sync(cam_rot);
        sync(cam_zoom);
        sync(cam_zoom_xy);
        sync(smooth_iter_dist_ratio);
        sync(flatten);
        sync(flatten_amount);
        sync(show_period2_bulb);
        sync(interactive_cardioid);
        sync(dynamic_iter_lim);
        sync(quality);
        sync(smoothing_type);
        sync(iter_lim);
        sync(x_spline);
        sync(y_spline);
        sync(dynamic_color_cycle_limit);
        sync(normalize_depth_range);
        sync(log1p_weight);
        sync(color_cycle_value);
        sync(active_color_template);
        sync(gradient);
        sync(hue_shift);
        sync(show_color_animation_options);
        sync(gradient_shift);
        sync(gradient_shift_step);
        sync(hue_shift_step);
        sync(colors_updated);
        sync(config_buf);
    }

    void initData() override;
    void populate() override;

    double toNormalizedZoom(double zoom) const {
        return log(zoom) + 1.0;
    }

    double fromNormalizedZoom(double normalized_zoom) const {
        return exp(normalized_zoom - 1.0);
    }

    double toHeight(double zoom) const {
        return 1.0 / toNormalizedZoom(zoom);
    }

    double fromHeight(double height) const {
        return fromNormalizedZoom(1.0 / height);
    }

    void setNormalizedZoom(double normalized_zoom) {
        cam_zoom = fromNormalizedZoom(normalized_zoom);
    }

    double getNormalizedZoom() const {
        return toNormalizedZoom(cam_zoom);
    }

    DAngledRect getAngledRect(const TweenableMandelState& s/*, Camera *cam*/) const
    {
        DVec2 world_size = (s.ctx_stage_size / reference_zoom) / s.cam_zoom;

        return DAngledRect(
            s.cam_x, s.cam_y,
            world_size.x, world_size.y,
            s.cam_rot);
    }

    void loadColorTemplate(
        TweenableMandelState& state,
        ColorGradientTemplate type,
        float hue_threshold = 0.3f,
        float sat_threshold = 0.3f,
        float val_threshold = 0.3f)
    {
        ImGradient& grad = state.gradient;
        grad.getMarks().clear();

        state.active_color_template = type;

        switch (type)
        {
        case GRADIENT_CLASSIC:
        {
            grad.addMark(0.0f, ImColor(0, 0, 0));
            grad.addMark(0.2f, ImColor(39, 39, 214));
            grad.addMark(0.4f, ImColor(0, 143, 255));
            grad.addMark(0.6f, ImColor(255, 255, 68));
            grad.addMark(0.8f, ImColor(255, 30, 0));
        }
        break;

        case GRADIENT_WAVES:
        {
            grad.addMark(0.14f, ImColor(11, 14, 64));
            grad.addMark(0.3f, ImColor(255, 255, 255));
            grad.addMark(0.5f, ImColor(73, 179, 255));
            grad.addMark(0.8f, ImColor(50, 90, 113));
        }
        break;

        default:
        {
            uint8_t last_r, last_g, last_b;
            colorGradientTemplate(type, 0.0f, last_r, last_g, last_b);
            grad.addMark(0.0f, ImColor(last_r, last_g, last_b));

            float last_h, last_s, last_v;
            RGBtoHSV(last_r, last_g, last_b, last_h, last_s, last_v);

            for (float x = 0.0f; x < 1.0f; x += 0.01f)
            {
                uint8_t r, g, b;
                float h, s, v;

                colorGradientTemplate(type, x, r, g, b);
                RGBtoHSV(r, g, b, h, s, v);

                float h_ratio = Math::absAvgRatio(last_h, h);
                float s_ratio = Math::absAvgRatio(last_s, s);
                float v_ratio = Math::absAvgRatio(last_v, v);

                //float avg_ratio = Math::avg(h_ratio, s_ratio, v_ratio);
                if (h_ratio > hue_threshold ||
                    s_ratio > sat_threshold ||
                    v_ratio > val_threshold)
                {
                    grad.addMark(x, ImColor(r, g, b));
                    last_h = h;
                    last_s = s;
                    last_v = v;
                }
            }
        }
        break;
        }
    }

    // Tweening
    void lerpState(
        TweenableMandelState& dst,
        TweenableMandelState& a,
        TweenableMandelState& b,
        double f,
        bool complete);

    //TweenableMandelState preview;

    void startTween(TweenableMandelState& target)
    {
        // Switch to raw iter_lim for tween (and switch to quality mode on finish)
        dynamic_iter_lim = false;
        quality = iter_lim;

        //TweenableMandelState& state = preview;
        TweenableMandelState& state = *this;

        target.reference_zoom = reference_zoom;
        target.ctx_stage_size = ctx_stage_size;

        r1 = getAngledRect(state);
        r2 = getAngledRect(target);


        DAngledRect encompassing;
        encompassing.fitTo(r1, r2, r1.aspectRatio());

        double encompassing_zoom = (state.ctx_world_size() / encompassing.size).average();
        double encompassing_height = std::min(1.0, toHeight(encompassing_zoom)); // Cap at max height
        tween_lift = encompassing_height - std::max(toHeight(state.cam_zoom), toHeight(target.cam_zoom));

        double max_lift = 1.0 - toHeight(target.cam_zoom);
        if (max_lift < 0) max_lift = 0;
        if (tween_lift > max_lift)
            tween_lift = max_lift;

        // Now, set start/end tween states
        state_a = state;
        state_b = target;

        // Begin tween
        tween_progress = 0.0;
        tweening = true;

        // Immediately applied attributes (todo: tween as much as possible)
        state.active_color_template = target.active_color_template;
    }

    //std::string serializeConfig();
    //bool deserializeConfig(std::string txt);
    
    void updateConfigBuffer();
    void loadConfigBuffer();


};

struct Mandelbrot_Scene : public Scene<Mandelbrot_Data>
{
    // --- Custom Launch Config ---
    struct Config {};
    Mandelbrot_Scene(Config&) {}

   
    
    int current_row = 0;
    EscapeField field_9x9 = EscapeField(0); // Processed in a single frame
    EscapeField field_3x3 = EscapeField(1); // Processed over multiple frames
    EscapeField field_1x1 = EscapeField(2); // Processed over multiple frames

    CanvasImage bmp_9x9;
    CanvasImage bmp_3x3;
    CanvasImage bmp_1x1;

    CanvasImage* pending_bmp = nullptr;
    CanvasImage* active_bmp = nullptr;

    EscapeField* pending_field = nullptr;
    EscapeField* active_field = nullptr;

    DQuad world_quad;

    Cardioid::CardioidLerper cardioid_lerper;

    double log_color_cycle_iters = 0.0;

    // 0 = 9x smaller, 1 = 3x smaller, 2 = full resolution
    int computing_phase = 0;
    bool first_frame = true;
    bool finished_compute = false;

    std::chrono::steady_clock::time_point compute_t0;
    Math::MovingAverage::MA timer_ma = Math::MovingAverage::MA(10);


    std::vector< std::vector<DVec2> > boundary_paths;

    // Per-level boundry tracing
    void generateBoundary(EscapeField* field, double level, std::vector<DVec2>& path);

    ImGradient gradient_shifted;
    void updateShiftedGradient()
    {
        auto& marks = gradient.getMarks();
        auto& shifted_marks = gradient_shifted.getMarks();
        shifted_marks.resize(marks.size());
        for (size_t i = 0; i < marks.size(); i++)
        {
            auto adjusted = Color(marks[i].color).adjustHue((float)hue_shift).vec4();
            memcpy(shifted_marks[i].color, adjusted.asArray(), sizeof(FVec4));
            shifted_marks[i].position = Math::wrap(marks[i].position + (float)gradient_shift, 0.0f, 1.0f);
        }
        gradient_shifted.refreshCache();
    }

    inline void iter_gradient_color(double iters, uint32_t& c) const
    {
        // interior of the set -> black
        if (iters >= iter_lim) {
            c = 0xFF000000; 
            return;
        }

        // wrap smooth iteration count so the gradient repeats
        double t = (iters - log_color_cycle_iters * std::floor(iters / log_color_cycle_iters)) / log_color_cycle_iters;
        gradient_shifted.unguardedRGBA(t, c);
    }

    void shadeBitmap()
    {
        //DebugPrint("Shading compute phase: %d", active_field->compute_phase);
        active_bmp->forEachPixel([&, this](int x, int y)
        {
            EscapeFieldPixel& field_pixel = active_field->get(x, y);

            if (field_pixel.depth == INSIDE_MANDELBROT_SET) {
                active_bmp->setPixel(x, y, 0xFF000000);
                return;
            }

            uint32_t u32;
            double final_value = field_pixel.final_value;
            //double t = (final_value - log_color_cycle_iters * std::floor(final_value / log_color_cycle_iters)) / log_color_cycle_iters;
            double t = Math::wrap(final_value / color_cycle_value, 0.0, 1.0);
            gradient_shifted.unguardedRGBA(t, u32);

            active_bmp->setPixel(x, y, u32);
        });
    }


    template<
        MandelSmoothing Smooth_Iter,
        bool flatten
    >
    bool mandelbrot_simple()
    {
        return true;
    }

        //bool Smooth_Dist,
        //bool Use_Splines
    template<
        typename T,
        MandelSmoothing Smooth_Iter,
        bool flatten
    >
    bool mandelbrot()
    {
        //if (current_row == 0)
        //{
        //    pending_field->min_depth = std::numeric_limits<double>::max();
        //    pending_field->max_dist = std::numeric_limits<double>::min();
        //}

        bool frame_complete = pending_bmp->forEachWorldPixel<T>(
            current_row, [&](int x, int y, T wx, T wy)
        {
            // Result already calculated in previous phase? (forwarded to active_bmp)
            double depth = pending_field->get(x, y).depth;
            if (depth >= 0)
                return;

            double dist;

            /// ------------------------ Compute -------------------------
            ///-----------------------------------------------------------
            ///if constexpr (Use_Splines)
            ///{
            ///    v = mandelbrot_spline_iter<Smooth_Iter>(wx, wy, iter_lim, x_spline, y_spline);
            ///
            ///    // Extreme splines can return infinite
            ///    if (!std::isfinite(v)) v = iter_lim;
            ///}
            ///else // linear
            {
                //mandelbrot_ex<T, Smooth_Iter>(wx, wy, iter_lim, depth, dist);
                mandelbrot_ex<T, MandelSmoothing::MIX>(wx, wy, iter_lim, depth, dist);

                ///if (isnan(depth))
                ///{
                ///    //mandelbrot_ex<T, Smooth_Iter>(wx, wy, iter_lim, depth, dist);
                ///    depth = mandelbrot_iter_ex<T, Smooth_Iter>(wx, wy, iter_lim);
                ///    mandelbrot_ex<T, Smooth_Iter>(wx, wy, iter_lim, depth, dist);
                ///
                ///    dist = 0;
                ///}

                /*if constexpr (Smooth_Iter == MandelSmoothing::DIST)
                {
                    // Distance to edge smoothing
                    UNUSED(wx);
                    UNUSED(wy);
                    //depth = (double)mandelbrot_dist(wx, wy, iter_lim);
                }
                else
                //if constexpr (Smooth_Iter == MandelSmoothing::ITER)
                {
                    // If Smooth_Iter *or* all smoothing options are disabled, handle here
                    //v = mandelbrot_iter<Smooth_Iter>(wx, wy, iter_lim);

                    //depth = mandelbrot_iter_ex<T, Smooth_Iter>(wx, wy, iter_lim);
                    mandelbrot_ex<T, Smooth_Iter>(wx, wy, iter_lim, depth);
                }*/
            }

            

            EscapeFieldPixel& field_pixel = pending_field->get(x, y);
            field_pixel.depth = depth;
            field_pixel.dist = dist;
 
        }, (int)(1.5f*(float)Thread::idealThreadCount()), computing_phase > 0 ? 16 : 0);

        if (frame_complete)
        {
            refreshFieldDepthNormalized();
        }

        return frame_complete;
    };

    void refreshFieldDepthNormalized()
    {
        //bool calculate_floor_depth = normalize_depth_range && 
        //    smoothing_type & (int)MandelSmoothing::ITER;

        //if (calculate_floor_depth)
        {
            pending_field->min_depth = std::numeric_limits<double>::max();
            pending_field->max_depth = std::numeric_limits<double>::min();
            pending_field->min_dist = std::numeric_limits<double>::max();
            pending_field->max_dist = std::numeric_limits<double>::min();

            // Redetermine minimum depth for entire visible field
            pending_bmp->forEachPixel([&, this](int x, int y)
            {
                EscapeFieldPixel& field_pixel = pending_field->get(x, y);
                double depth = field_pixel.depth;
                double dist = field_pixel.dist;

                if (depth == INSIDE_MANDELBROT_SET)
                    return;

                //if (depth < 0)
                //{
                //    DebugBreak();
                //}

                if (depth < pending_field->min_depth) pending_field->min_depth = depth;
                if (depth > pending_field->max_depth) pending_field->max_depth = depth;
                if (dist < pending_field->min_dist) pending_field->min_dist = dist;
                if (dist > pending_field->max_dist) pending_field->max_dist = dist;
            }, 0);
        }

        // Calculate normalized depth
        pending_bmp->forEachPixel([&](int x, int y)
        {
            EscapeFieldPixel& field_pixel = pending_field->get(x, y);

            double depth = field_pixel.depth;
            //double dist = field_pixel.dist;// / pending_field->max_dist;

            //double lower_depth_bound = normalize_depth_range ? pending_field->min_depth : 0;
            //
            //double normalized_depth = Math::lerpFactor(depth, lower_depth_bound, (double)iter_lim);
            //double normalized_dist  = Math::lerpFactor(dist,  pending_field->min_dist,  pending_field->max_dist);
            //
            //double log_depth = -log(std::max(normalized_depth,0.0000000001));
            //double final_depth = Math::lerp(log_depth, normalized_depth, log1p_weight);
            //
            //if (isnan(final_depth))
            //{
            //    DebugBreak();
            //}
            //double normalized_dist = -log(dist);

            double final_depth;
            double normalized_dist = 0;

            if (normalize_depth_range)
                final_depth = Math::log1pLerp(depth - pending_field->min_depth, log1p_weight);
            else
                final_depth = Math::log1pLerp(depth, log1p_weight);


            //field_pixel.final_value = final_value;
            
            switch ((MandelSmoothing)smoothing_type)
            {
                case MandelSmoothing::ITER:
                {
                    field_pixel.final_value = final_depth;
                }
                break;
                case MandelSmoothing::DIST:
                {
                    field_pixel.final_value = normalized_dist;
                }
                break;
                case MandelSmoothing::MIX:
                {
                    field_pixel.final_value = Math::lerp(final_depth, normalized_dist, smooth_iter_dist_ratio);
                }
                break;
            }

            if (!isfinite(field_pixel.final_value))
            {
                DebugBreak();
            }
        });
    }

    template<
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
                (polard_coord.x < M_PI && recalculated_orig_angle > M_PI * 1.1) ||
                (polard_coord.x > M_PI && recalculated_orig_angle < M_PI * 0.9);

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
    };

    // --- Simulation processing ---
    void sceneStart() override;
    void sceneMounted(Viewport* viewport) override;

    // Viewport handling
    void viewportProcess(Viewport* ctx, double dt) override;
    void viewportDraw(Viewport* ctx) const override;

    // Input
    void onEvent(Event e) override;
    ///void onPointerDown() override;
    ///void onPointerUp() override;
    ///void onPointerMove() override;
    ///void onWheel() override;
};

struct Mandelbrot_Project : public BasicProject
{
    void projectPrepare(Layout& layout) override;
};

SIM_END(Mandelbrot)