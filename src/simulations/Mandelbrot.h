#pragma once
#include "project.h"
#include "Cardioid.h"

SIM_BEG(Mandelbrot)

enum MandelFlag : uint32_t
{
    // bits
    MANDEL_DYNAMIC_ITERS        = 1u << 0,
    MANDEL_SHOW_AXIS            = 1u << 1,
    MANDEL_FLATTEN              = 1u << 2,
    MANDEL_DYNAMIC_COLOR_CYCLE  = 1u << 3,

    // bitmasks
    MANDEL_FLAGS_MASK   = 0x000FFFFFu, // max 24 bit-flags
    MANDEL_SMOOTH_MASK  = 0x00F00000u, // max 16 smooth types
    MANDEL_VERSION_MASK = 0xFF000000u, // max 255 versions

    MANDEL_SMOOTH_BITSHIFT = 20,
    MANDEL_VERSION_BITSHIFT = 24
};

enum MandelSmoothing
{
    SMOOTH_NONE,
    SMOOTH_CONTINUOUS,
    SMOOTH_DISTANCE
};

enum ColorGradientTemplate
{
    GRADIENT_CUSTOM,

    GRADIENT_CLASSIC,
    GRADIENT_SINUSOIDAL_RAINBOW_CYCLE,
    GRADIENT_FOREST,


    GRADIENT_TEMPLATE_COUNT
};

static const char* ColorGradientNames[GRADIENT_TEMPLATE_COUNT] = {
    "",
    "CLASSIC",
    "SINUSOIDAL_RAINBOW_CYCLE",
    "FOREST"
};

template<bool smooth>
inline double mandelbrot_iter(double x0, double y0, int iter_lim)
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

    // Ensures black for deep-set points
    if (iter == iter_lim)
        return iter_lim;

    if constexpr (smooth)
        return iter + (1.0 - log2(log2(xx + yy) / 2.0));
    else
        return iter;
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

/*struct TweenableMandelState
{
    double cam_x = 0.0;
    double cam_y = 0.0;
    double cam_rot = 0.0;
    double cam_zoom = 1.0;
    DVec2  cam_zoom_xy = DVec2(1, 1);
    int iter_lim = 0; // Actual iter limit
    double cardioid_lerp_amount = 1.0; // 1 - flatten

    float x_spline_points[ImSpline::PointsArrSize(6)] = {
        0.0f, 0.0f,   0.1f, 0.1f,   0.2f, 0.2f, 
        0.3f, 0.3f,   0.4f, 0.4f,   0.5f, 0.5f
    };
    float y_spline_points[ImSpline::PointsArrSize(6)] = {
         0.0f, 0.0f,   0.1f, 0.1f,   0.2f, 0.2f,
         0.3f, 0.3f,   0.4f, 0.4f,   0.5f, 0.5f
    };
};*/


struct Mandelbrot_Scene_Vars : public VarBuffer
{
    //struct Primatives : public TweenableMandelState
    //{

    sync_struct
    {
        FiniteDouble cam_x = 0.0;
        double cam_y = 0.0;
        double cam_rot = 0.0;
        double cam_zoom = 1.0;
        DVec2  cam_zoom_xy = DVec2(1, 1);
        int iter_lim = 0; // Actual iter limit
        double cardioid_lerp_amount = 1.0; // 1 - flatten

        float x_spline_points[ImSpline::PointsArrSize(6)] = {
            0.0f, 0.0f,   0.1f, 0.1f,   0.2f, 0.2f,
            0.3f, 0.3f,   0.4f, 0.4f,   0.5f, 0.5f
        };
        float y_spline_points[ImSpline::PointsArrSize(6)] = {
             0.0f, 0.0f,   0.1f, 0.1f,   0.2f, 0.2f,
             0.3f, 0.3f,   0.4f, 0.4f,   0.5f, 0.5f
        };

        bool colors_updated = false;

        char config_buf[1024] = "";

        bool flatten = false;
        bool dynamic_iter_lim = true;
        double quality = 0.8; // Used for UI (ignore during tween until complete)
        MandelSmoothing smoothing_type = SMOOTH_CONTINUOUS;
        bool dynamic_color_cycle_limit = true;
        double color_cycle_value = 1.0; // If dynamic, iter_lim ratio, else iter_lim
        double color_cycle_iters = 32.0;
        bool show_axis = true;
        double cam_degrees = 0.0;
        double flatten_amount = 0.0;

        bool show_period2_bulb = true;
        bool interactive_cardioid = false;
    }
    sync_end

    // non-trivially copyable data
    ImSpline::Spline x_spline;
    ImSpline::Spline y_spline;
    
    ImGradient gradient;
    ImGradientMark* draggingMark = nullptr;
    ImGradientMark* selectedMark = nullptr;

    int active_color_template = GRADIENT_CLASSIC;

    Mandelbrot_Scene_Vars()
    {
        x_spline.set(x_spline_points, (int)ImSpline::PointsArrSize(x_spline_points), 1000, 100);
        y_spline.set(y_spline_points, (int)ImSpline::PointsArrSize(y_spline_points), 1000, 100);

    }

    ////////////////////////////////////

    void updateConfigBuffer();
    void loadConfigBuffer();

    std::string serializeConfig();
    bool deserializeConfig(std::string txt);

    /*void loadColorTemplate(ColorGradientTemplate type)
    {
        gradient.getMarks().clear();
        int marks = 8;// 7;
        float inc = 1.0f / (float)(1 + marks);
        for (float x = 0.0f; x < 1.0f; x += inc)
        {
            uint8_t r, g, b;
            colorGradientTemplate(type, x, r, g, b);

            gradient.addMark(x, ImColor(r, g, b));
        }
    }*/

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

    void loadColorTemplate(ColorGradientTemplate type,
        float hue_threshold=0.3f,
        float sat_threshold=0.3f,
        float val_threshold=0.3f)
    {
        gradient.getMarks().clear();

        switch (type)
        {
        case GRADIENT_CLASSIC:
        {
            gradient.addMark(0.0f, ImColor(0, 0, 0));
            gradient.addMark(0.2f, ImColor(39, 39, 214));
            gradient.addMark(0.4f, ImColor(0, 143, 255));
            gradient.addMark(0.6f, ImColor(255, 255, 68));
            gradient.addMark(0.8f, ImColor(255, 30, 0));
            return;
        }
        break;
        case GRADIENT_FOREST:
        {
            gradient.addMark(0.0f, ImColor(0, 0, 0));
            gradient.addMark(0.2f, ImColor(0, 91, 255));
            gradient.addMark(0.4f, ImColor(94, 224, 119));
            gradient.addMark(0.6f, ImColor(0, 120, 80));
            gradient.addMark(0.8f, ImColor(0, 69, 119));
            return;
        }
        break;
        default: break;
        }

        uint8_t last_r, last_g, last_b;
        colorGradientTemplate(type, 0.0f, last_r, last_g, last_b);
        gradient.addMark(0.0f, ImColor(last_r, last_g, last_b));

        float last_h, last_s, last_v;
        RGBtoHSV(last_r, last_g, last_b, last_h, last_s, last_v);

        for (float x = 0.0f; x < 1.0f; x += 0.01f)
        {
            uint8_t r, g, b;
            float h, s, v;

            colorGradientTemplate(type, x, r, g, b);
            RGBtoHSV(r, g, b, h, s, v);

            float h_ratio = Math::abs_avg_ratio(last_h, h);
            float s_ratio = Math::abs_avg_ratio(last_s, s);
            float v_ratio = Math::abs_avg_ratio(last_v, v);

            float avg_ratio = Math::avg(h_ratio, s_ratio, v_ratio);
            if (h_ratio > hue_threshold ||
                s_ratio > sat_threshold || 
                v_ratio > val_threshold)
            {
                gradient.addMark(x, ImColor(r, g, b));
                last_h = h;
                last_s = s;
                last_v = v;
            }
        }
    }

    void populate();
    void copyFrom(const Mandelbrot_Scene_Vars& rhs)
    {
        //varcpy<Primatives>(*this, rhs);
        varcpy(x_spline, rhs.x_spline);
        varcpy(y_spline, rhs.y_spline);
        varcpy(gradient, rhs.gradient);

        // Fix dangling mark pointers on data overwrite
        draggingMark = gradient.markFromUID(rhs.draggingMark ? rhs.draggingMark->uid : -1);
        selectedMark = gradient.markFromUID(rhs.selectedMark ? rhs.selectedMark->uid : -1);
    }
};

struct Mandelbrot_Scene : public Scene<Mandelbrot_Scene_Vars>
{
    // --- Custom Launch Config ---
    struct Config {};
    Mandelbrot_Scene(Config& info)
    {}

    int current_row = 0;
    CanvasImage bmp_9x9;
    CanvasImage bmp_3x3;
    CanvasImage bmp_1x1;

    CanvasImage* pending_bmp = nullptr;
    CanvasImage* active_bmp = nullptr;

    DQuad world_quad;

    Cardioid::CardioidLerper cardioid_lerper;

    // 0 = 9x smaller, 1 = 3x smaller, 2 = full resolution
    int compute_phase = 0; 

    // Tweening
    ///TweenableMandelState state_a;
    ///TweenableMandelState state_b;
    bool tweening = false;
    double tween = 0.0; // 0..1
    ///void lerpState(TweenableMandelState* dst, double f);

    // Scene management

    //void step_color(double step, uint8_t& r, uint8_t& g, uint8_t& b);
    void iter_ratio_color(double ratio, uint8_t& r, uint8_t& g, uint8_t& b);

    void iter_gradient_color(double mu, int iter_lim,
        uint8_t& r, uint8_t& g, uint8_t& b) const
    {
        // interior of the set -> black
        if (mu >= iter_lim) {
            r = g = b = 0;
            return;
        }

        // wrap smooth iteration count so the gradient repeats
        double t = std::fmod(mu, color_cycle_iters) / color_cycle_iters;  // 0..1
        if (t < 0.0) t += 1.0; // protect against negative fmod on some platforms

        //colorGradientTemplate<GRADIENT_GAMMA_POWER_RAMPS>(t, r, g, b);

        float col[3];
        gradient.getColorAt(t, col);

        r = static_cast<uint8_t>(col[0] * 255.0f + 0.5f);
        g = static_cast<uint8_t>(col[1] * 255.0f + 0.5f);
        b = static_cast<uint8_t>(col[2] * 255.0f + 0.5f);
    }

    // TODO: Switch to using Color struct everywhere
    void gradient_ratio_color(double ratio, uint8_t& r, uint8_t& g, uint8_t& b) const
    {
        //ratio = fmod(ratio, 1.0);

        float col[3];
        gradient.getColorAt(ratio, col);

        r = static_cast<uint8_t>(col[0] * 255.0f + 0.5f);
        g = static_cast<uint8_t>(col[1] * 255.0f + 0.5f);
        b = static_cast<uint8_t>(col[2] * 255.0f + 0.5f);
    }

    template<
        bool Smooth_Iter,
        bool Smooth_Dist,
        bool Use_Splines
    >
    bool regularMandelbrot(Viewport* ctx)
    {
        double f_max_iter = static_cast<double>(iter_lim);

        //DebugPrint("begin forEachWorldPixel2()");
        return pending_bmp->forEachWorldPixel(
            camera, current_row, [&](int x, int y, double wx, double wy)
        {
            // Result already calculated in previous phase? (forwarded to active_bmp)
            //if (pending_bmp->getPixel(x, y).a)
            //    return;

            double v;
            uint8_t r, g, b;
           
            /// ------------------------ Compute -------------------------
            ///-----------------------------------------------------------
            if constexpr (Use_Splines)
            {
                v = mandelbrot_spline_iter<Smooth_Iter>(wx, wy, iter_lim, x_spline, y_spline);

                // Extreme splines can return infinite
                if (!std::isfinite(v)) v = iter_lim;
            }
            else // linear
            {
                if constexpr (Smooth_Dist)
                {
                    // Distance to edge smoothing
                    v = mandelbrot_dist(wx, wy, iter_lim);
                }
                else
                {
                    // If Smooth_Iter *or* all smoothing options are disabled, handle here
                    v = mandelbrot_iter<Smooth_Iter>(wx, wy, iter_lim);
                }
            }

            /// ------------------------ Coloring -------------------------
            ///------------------------------------------------------------
            if constexpr (Smooth_Dist)
            {
                if (v < 0)
                {
                    r = g = b = 0;
                }
                else
                {
                    double scaled_dist = v * cam_zoom * 300.0;        // larger zoom => bigger scaled_dist

                    double ratio = std::log1p(scaled_dist) / 6.0;  // 6.0 tunes overall brightness
                    ratio = std::clamp(ratio, 0.0, 1.0);

                    gradient_ratio_color(ratio, r, g, b);
                }
            }
            else
            {
                // If Smooth_Iter *or* all smoothing options are disabled, handle here
                iter_gradient_color(v, iter_lim, r, g, b);
            }

            //double ratio = Smooth_Iter / f_max_iter;
            //iter_ratio_color(ratio, r, g, b);
            //if (discrete_step)
            //    step_color(Smooth_Iter, r, g, b);
            //else
            //{
            //    double ratio = Smooth_Iter / f_max_iter;
            //    iter_ratio_color(ratio, r, g, b);
            //}

            pending_bmp->setPixel(x, y, r, g, b, 255);
        });
    };

    template<
        bool Smooth,
        bool Show_Period2_Bulb
    >
    bool radialMandelbrot(Viewport* ctx)
    {
        double f_max_iter = static_cast<double>(iter_lim);
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

            uint8_t r, g, b;
            iter_gradient_color(smooth_iter, iter_lim, r, g, b);

            //double ratio = Smooth_Iter / f_max_iter;
            //iter_ratio_color(ratio, r, g, b);

            pending_bmp->setPixel(x, y, r, g, b, 255);
        });
    };

    // --- Simulation processing ---
    void sceneStart() override;
    void sceneMounted(Viewport* viewport) override;

    // Viewport handling
    void viewportProcess(Viewport* ctx) override;
    void viewportDraw(Viewport* ctx) const override;

    // Input
    void onEvent(Event& e) override;
    ///void mouseDown() override;
    ///void mouseUp() override;
    ///void mouseMove() override;
    ///void mouseWheel() override;
};

struct Mandelbrot_Project : public BasicProject
{
    void projectPrepare() override;
};

SIM_END(Mandelbrot)