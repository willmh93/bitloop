#pragma once
#include "project.h"
#include "Cardioid.h"

SIM_BEG(Mandelbrot)

template <bool smoothing>
inline double mandelbrot(double x0, double y0, int iter_lim)
{
    double x = 0.0, y = 0.0, xx = 0.0, yy = 0.0;
    int iter = 0;
    while (xx + yy <= 4.0 && iter < iter_lim)
    {
        y = (2.0 * x * y + y0);
        x = (xx - yy + x0);

        xx = x * x;
        yy = y * y;

        //if (xx + yy > 256.0)
        //{
        //    if (iter > 100)
        //    {
        //
        //    }
        //}

        iter++;
    }

    // Ensures black for deep-set points
    if (iter == iter_lim)
        return iter_lim;

    if constexpr (smoothing)
        return iter + (1.0 - log2(log2(xx + yy) / 2.0));
    else
        return iter;
}

template <bool smoothing>
inline double spline_mandelbrot(double x0, double y0, int iter_lim, ImSpline::Spline& x_spline, ImSpline::Spline& y_spline)
{
    double x = 0.0, y = 0.0, xx = 0.0, yy = 0.0;
    int iter = 0;
    while (xx + yy <= 4.0 && iter < iter_lim)
    {
        y = (2.0 * x * y + y0);
        x = (xx - yy + x0);

        /*if (std::isnan(y_spline(static_cast<float>(x * x))))
        {
            int a = 5;
        }

        if (std::isnan(y_spline(static_cast<float>(y * y))))
        {
            int a = 5;
        }*/

        //if (x < -3 || x > 3 || y < -3 || y > 3)
        //    break;

        //if (((*reinterpret_cast<uint64_t*>(&x)) & 0x7FFFFFFFFFFFFFFFULL) < 0x4340000000000000ULL)

        float spline_xx = x_spline(static_cast<float>(x * x));
        float spline_yy = y_spline(static_cast<float>(y * y));

        /*if (xx < -FLT_MIN || xx > FLT_MAX ||
            yy < -FLT_MIN || yy > FLT_MAX)
        {
            break;
        }*/

        xx = spline_xx;
        yy = spline_yy;

        iter++;
    }

    // Ensures black for deep-set points
    if (iter == iter_lim)
        return iter_lim;

    if constexpr (smoothing)
    {
        //if (std::isnan(iter + (1.0 - log2(log2(xx + yy) / 2.0))))
        //{
        //    int a = 5;
        //}
        return iter + (1.0 - log2(log2(xx + yy) / 2.0));
    }
    else
        return iter;
}

enum MANDEL_FLAG : uint32_t
{
    // bits
    MANDEL_DYNAMIC   = 1u << 0,
    MANDEL_SMOOTH    = 1u << 1,
    MANDEL_SHOW_AXIS = 1u << 2,
    MANDEL_FLATTEN   = 1u << 3,

    MANDEL_VERSION = 0xFF000000u,
    MANDEL_VERSION_BITSHIFT = 24
};

struct Primatives
{
    double cam_x = 0.0;
    double cam_y = 0.0;
    double cam_rotation = 0.0;
    double cam_zoom = 1.0;
    Vec2   cam_zoom_xy = Vec2(1, 1);

    float x_spline_points[ImSpline::PointsArrSize(6)] = {
        0.0f, 0.0f, 0.1f, 0.1f, 0.2f, 0.2f, 0.3f, 0.3f, 0.4f, 0.4f, 0.5f, 0.5f
    };
    float y_spline_points[ImSpline::PointsArrSize(6)] = {
         0.0f, 0.0f, 0.1f, 0.1f, 0.2f, 0.2f, 0.3f, 0.3f, 0.4f, 0.4f, 0.5f, 0.5f
    };
 
    bool colors_updated = false;

    char config_buf[1024] = "";

    bool flatten = false;
    bool dynamic_iter_lim = true;
    double quality = 0.5; // Used for UI
    int iter_lim = 0; // Actual iter limit
    bool smoothing = true;
    bool show_axis = true;
    double cam_degrees = 0.0;

    bool show_period2_bulb = true;
    bool interactive_cardioid = false;

    double flatten_amount = 0.0;
    double cardioid_lerp_amount = 1.0; // 1 - flatten
};

struct Mandelbrot_Scene_Vars : public VarBuffer<Mandelbrot_Scene_Vars>, public Primatives
{
    // non-trivially copyable data
    ImSpline::Spline x_spline;
    ImSpline::Spline y_spline;
    
    ImGradient gradient;
    ImGradientMark* draggingMark = nullptr;
    ImGradientMark* selectedMark = nullptr;

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

    void populate(Mandelbrot_Scene_Vars& dst) override;
    void copyFrom(const Mandelbrot_Scene_Vars& rhs) override
    {
        varcpy<Primatives>(*this, rhs);
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
    struct Config
    {
        //double speed = 10.0;
    };

    Mandelbrot_Scene(Config& info) //:
        //speed(info.speed)
    {}

    //void populate() override;

    CanvasImage bmp_9x9;
    CanvasImage bmp_3x3;
    CanvasImage bmp_1x1;
    CanvasImage* active_bmp = nullptr;

    //FRect stage_rect_on_world;
    FQuad world_quad;

    Cardioid::CardioidLerper cardioid_lerper;
    //ImGradient gradient;

    

    int compute_phase = 0; // 0 = 9x smaller, 1 = 3x smaller, 2 = full resolution

    ///std::string serializeConfig();
    ///bool deserializeConfig(std::string json);
    ///void updateConfigBuffer();
    ///void loadConfigBuffer();

    // Scene management

    void step_color(double step, uint8_t& r, uint8_t& g, uint8_t& b);
    void iter_ratio_color(double ratio, uint8_t& r, uint8_t& g, uint8_t& b);

    // pick something that looks good; 32 is a common default

    void iter_gradient_color(double mu, int iter_lim,
        uint8_t& r, uint8_t& g, uint8_t& b) const
    {
        // interior of the set -> black
        if (mu >= iter_lim) {
            r = g = b = 0;
            return;
        }

        // wrap smooth iteration count so the gradient repeats
        static const double cycleIterations = 32.0;
        double t = std::fmod(mu, cycleIterations) / cycleIterations;  // 0..1
        if (t < 0.0) t += 1.0;    // protect against negative fmod on some platforms

        //if (std::isnan(t))
        //{
        //    int b = 5;
        //}

        float col[3];
        gradient.getColorAt(static_cast<float>(t), col);  // your cached gradient

        r = static_cast<uint8_t>(col[0] * 255.0f + 0.5f);
        g = static_cast<uint8_t>(col[1] * 255.0f + 0.5f);
        b = static_cast<uint8_t>(col[2] * 255.0f + 0.5f);
    }

    

    template<
        //bool phase_0,
        //bool phase_1,
        bool smoothed,
        bool linear
    >
    void regularMandelbrot(Viewport* ctx)
    {
        double f_max_iter = static_cast<double>(iter_lim);

        //CanvasImage* prev_bmp = nullptr;
        //switch (compute_phase)
        //{
        //case 1: prev_bmp = &bmp_9x9; break;
        //case 2: prev_bmp = &bmp_3x3; break;
        //}


        active_bmp->forEachWorldPixel(camera, [&](int x, int y, double wx, double wy)
        {
            // Already calculated in previous phase
            //if (active_bmp->getPixel(x, y).a)
            //    return;

            double smooth_iter;
            if constexpr (linear)
                smooth_iter = mandelbrot<smoothed>(wx, wy, iter_lim);
            else
                smooth_iter = spline_mandelbrot<smoothed>(wx, wy, iter_lim, x_spline, y_spline);

            if (!std::isfinite(smooth_iter))
                smooth_iter = iter_lim;

            uint8_t r, g, b;

            iter_gradient_color(smooth_iter, iter_lim, r, g, b);

            //double ratio = smooth_iter / f_max_iter;
            //iter_ratio_color(ratio, r, g, b);

            
            //if (discrete_step)
            //    step_color(smooth_iter, r, g, b);
            //else
            //{
            //    double ratio = smooth_iter / f_max_iter;
            //    iter_ratio_color(ratio, r, g, b);
            //}

            active_bmp->setPixel(x, y, r, g, b, 255);
        });
    };

    template<
        bool smoothed,
        bool vis_left_of_main_cardioid
    >
    void radialMandelbrot(Viewport* ctx)
    {
        double f_max_iter = static_cast<double>(iter_lim);
        active_bmp->forEachWorldPixel(camera, [&](int x, int y, double angle, double point_dist)
        {
            Vec2 polard_coord = cardioid_lerper.originalPolarCoordinate(angle, point_dist, cardioid_lerp_amount);

            if (polard_coord.y < 0)
            {
                active_bmp->setPixel(x, y, 0, 0, 0, 255);
                return;
            }

            Vec2 mandel_pt = Cardioid::fromPolarCoordinate(polard_coord.x, polard_coord.y);
            double recalculated_orig_angle = cardioid_lerper.originalPolarCoordinate(mandel_pt.x, mandel_pt.y, 1.0).x;

            bool hide =
                (polard_coord.x < M_PI && recalculated_orig_angle > M_PI * 1.1) ||
                (polard_coord.x > M_PI && recalculated_orig_angle < M_PI * 0.9);

            if (hide)
            {
                active_bmp->setPixel(x, y, 0, 0, 0, 255);
                return;
            }

            if constexpr (!vis_left_of_main_cardioid)
            {
                if (mandel_pt.x < -0.75)
                {
                    active_bmp->setPixel(x, y, 0, 0, 0, 255);
                    return;
                }
            }

            double smooth_iter = mandelbrot<smoothed>(mandel_pt.x, mandel_pt.y, iter_lim);

            uint8_t r, g, b;
            iter_gradient_color(smooth_iter, iter_lim, r, g, b);

            //double ratio = smooth_iter / f_max_iter;
            //iter_ratio_color(ratio, r, g, b);

            active_bmp->setPixel(x, y, r, g, b, 255);
        });
    };

    // --- Simulation processing ---
    void sceneStart() override;
    void sceneMounted(Viewport* viewport) override;

    // Viewport handling
    void viewportProcess(Viewport* ctx) override;
    void viewportDraw(Viewport* ctx) override;

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