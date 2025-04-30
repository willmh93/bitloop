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

        xx = x_spline(static_cast<float>(x * x));
        yy = y_spline(static_cast<float>(y * y));
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

struct Mandelbrot_Scene : public Scene
{
    // --- Custom Launch Config ---
    struct Config
    {
        //double speed = 10.0;
    };

    Mandelbrot_Scene(Config& info) //:
        //speed(info.speed)
    {}

    CanvasImage bmp;

    bool discrete_step = false;
    bool flatten_main_cardioid = false;

    double quality = 20.0;
    bool thresholding = false;
    bool smoothing = true;
    double cam_rotation = 0.0;
    double cam_zoom = 1;
    double cam_zoom_xy[2] = { 1, 1 };

    bool show_period2_bulb = true;
    bool interactive_cardioid = false;

    int iter_lim;

    Cardioid::CardioidLerper cardioid_lerper;
    double flatten_amount = 0.0;
    double cardioid_lerp_amount = 1.0; // 1 - flatten

    ImSpline::Spline x_spline;
    ImSpline::Spline y_spline;

    float x_spline_points[ImSpline::PointsArrSize(6)] = {
        0.0f, 0.0f,
        0.1f, 0.1f,
        0.2f, 0.2f,
        0.3f, 0.3f,
        0.4f, 0.4f,
        0.5f, 0.5f
    };
    float y_spline_points[12] = {
        0.0f, 0.0f,
        0.1f, 0.1f,
        0.2f, 0.2f,
        0.3f, 0.3f,
        0.4f, 0.4f,
        0.5f, 0.5f
    };

    char config_buf[1024];

    // Scene management
    void sceneAttributes() override;
    void sceneStart() override;
    void sceneDestroy() override;
    void sceneMounted(Viewport* viewport) override;

    /// --- Sim Logic Here ---
    void step_color(double step, uint8_t& r, uint8_t& g, uint8_t& b);
    void iter_ratio_color(double ratio, uint8_t& r, uint8_t& g, uint8_t& b);

    std::string serializeConfig();
    void deserializeConfig(std::string json);

    void updateConfigBuffer();
    void loadConfigBuffer();

    template<
        bool smoothed,
        bool linear,
        bool vis_left_of_main_cardioid
    >
    void regularMandelbrot(Viewport* ctx)
    {
        double f_max_iter = static_cast<double>(iter_lim);
        bmp.forEachWorldPixel(camera, [&](int x, int y, double wx, double wy)
        {
            if constexpr (!vis_left_of_main_cardioid)
            {
                if (wx < -0.75)
                {
                    bmp.setPixel(x, y, 127, 0, 0, 255);
                    return;
                }
            }

            double smooth_iter;
            if constexpr (linear)
                smooth_iter = mandelbrot<smoothed>(wx, wy, iter_lim);
            else
                smooth_iter = spline_mandelbrot<smoothed>(wx, wy, iter_lim, x_spline, y_spline);

            uint8_t r, g, b;
            if (discrete_step)
                step_color(smooth_iter, r, g, b);
            else
            {
                double ratio = smooth_iter / f_max_iter;
                iter_ratio_color(ratio, r, g, b);
            }

            bmp.setPixel(x, y, r, g, b, 255);
        });
    };

    template<
        bool smoothed,
        bool vis_left_of_main_cardioid
    >
    void radialMandelbrot(Viewport* ctx)
    {
        double f_max_iter = static_cast<double>(iter_lim);
        bmp.forEachWorldPixel(camera, [&](int x, int y, double angle, double point_dist)
        {
            Vec2 polard_coord = cardioid_lerper.originalPolarCoordinate(angle, point_dist, cardioid_lerp_amount);

            if (polard_coord.y < 0)
            {
                bmp.setPixel(x, y, 0, 0, 0, 255);
                return;
            }

            Vec2 mandel_pt = Cardioid::fromPolarCoordinate(polard_coord.x, polard_coord.y);
            double recalculated_orig_angle = cardioid_lerper.originalPolarCoordinate(mandel_pt.x, mandel_pt.y, 1.0).x;

            bool hide =
                (polard_coord.x < M_PI && recalculated_orig_angle > M_PI * 1.1) ||
                (polard_coord.x > M_PI && recalculated_orig_angle < M_PI * 0.9);

            if (hide)
            {
                bmp.setPixel(x, y, 0, 0, 0, 255);
                return;
            }

            if constexpr (!vis_left_of_main_cardioid)
            {
                if (mandel_pt.x < -0.75)
                {
                    bmp.setPixel(x, y, 0, 0, 0, 255);
                    return;
                }
            }

            double smooth_iter = mandelbrot<smoothed>(mandel_pt.x, mandel_pt.y, iter_lim);
            double ratio = smooth_iter / f_max_iter;

            uint8_t r, g, b;
            iter_ratio_color(ratio, r, g, b);

            bmp.setPixel(x, y, r, g, b, 255);
        });
    };

    void cpu_mandelbrot(
        Viewport* ctx,
        double fw, double fh,
        double wx, double wy,
        double ww, double wh,
        int pixel_count
    );

    // --- Simulation processing ---
    void sceneProcess() override;

    // Viewport handling
    void viewportProcess(Viewport* ctx) override;
    void viewportDraw(Viewport* ctx) override;

    // Input
    ///void mouseDown() override;
    ///void mouseUp() override;
    ///void mouseMove() override;
    ///void mouseWheel() override;
};

struct Mandelbrot_Project : public Project
{
    int viewport_count = 1;

    void projectAttributes() override;
    void projectPrepare() override;
};

SIM_END(Mandelbrot)