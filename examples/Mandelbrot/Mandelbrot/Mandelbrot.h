#pragma once
#include <bitloop.h>
#include <cmath>

#include "Cardioid/Cardioid.h"

#include "types.h"
#include "state.h"
#include "types.h"
#include "kernel.h"
#include "shading.h"

SIM_BEG;

using namespace BL;

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

/*
    Multiple layers of inheritance explained:

    Here, we inherit from MandelState so that the Scene itself has a "live" state it can use,
    and since it's part of the VarBuffer system, a separate copy is also made for the UI which only
    gets synced with the live state at the end of each frame.

    Additionally, we set up a 'start' / 'end' tween state, and lerp between them, saving
    the result into the inherited "live" MandelState state.
*/

struct Mandelbrot_Scene_Data : public VarBuffer, public MandelState
{
    bool show_save_dialog = false;
    bool show_load_dialog = false;
    bool show_share_dialog = false;
    #ifdef __EMSCRIPTEN__
    bool opening_load_popup = false;
    #endif

    //char config_buf_name[32] = "Mandelbrot";
    std::string config_buf = "";
    //char pos_tween_buf[1024] = "";
    //char zoom_tween_buf[1024] = "";

    // == tweening ==
    bool tweening = false;
    double tween_progress = 0.0; // 0..1
    double tween_lift = 0.0;
    double tween_duration = 0.0;

    MandelState state_a;
    MandelState state_b;

    DAngledRect tween_r1;
    DAngledRect tween_r2;

    double cardioid_lerp_amount = 1.0; // 1 - flatten

    bool show_period2_bulb = true;
    bool interactive_cardioid = false;
    
    int iter_lim = 0; // Actual iter limit

    bool colors_updated = false;

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

    ImSpline::Spline tween_pos_spline = ImSpline::Spline(100,
        { {-0.147,0},{0.253,0},{0.553,0},{0.371,0.999},{0.675,1},{1.07,1} }
    );

    ImSpline::Spline tween_zoom_lift_spline = ImSpline::Spline(100,
        { {-0.1,0},{0,0},{0.1,0},{0.201,0.802},{0.251,1.002},{0.351,1.402},{0.572,1.419},{0.672,0.799},{0.8,0},{0.8,0},{1,0},{1.104,0} }
    );

    ImSpline::Spline tween_base_zoom_spline = ImSpline::Spline(100,
        { {-0.3f,0.0f},{0.0f,0.0f},{0.5f,0.0f},{0.5f,1.0f},{1.0f,1.0f},{1.5f,1.0f} }
    );

    ImSpline::Spline tween_color_cycle = ImSpline::Spline(100,
        { {0.072f, 0.0f}, {0.5f,0.0f}, {0.845f,0.0f}, {0.75f,1.0f}, {1.0f,1.0f}, {1.25f,1.0f} }
    );

    void registerSynced() override
    {
        sync(config_buf);
        
        sync(state_a);
        sync(state_b);
        sync(tweening);
        sync(tween_duration);
        sync(tween_progress);
        sync(tween_lift);

        #ifdef BL_DEBUG
        sync(tween_pos_spline);
        sync(tween_zoom_lift_spline);
        sync(tween_base_zoom_spline);
        #endif

        sync(reference_zoom);
        sync(ctx_stage_size);

        sync(tween_r1);
        sync(tween_r2);

        sync(show_axis);
        sync(cam_view);
        sync(iter_dist_mix);
        sync(flatten);
        sync(flatten_amount);
        sync(show_period2_bulb);
        sync(interactive_cardioid);
        sync(dynamic_iter_lim);
        sync(quality);
        sync(use_smoothing);
        sync(smoothing_type); // todo, not needed in state
        sync(iter_lim);
        sync(x_spline);
        sync(y_spline);

        sync(cycle_iter_dynamic_limit);
        sync(cycle_iter_normalize_depth);
        sync(cycle_iter_log1p_weight);
        sync(cycle_dist_invert);
        sync(cycle_iter_value);
        sync(cycle_dist_value);
        sync(cycle_dist_sharpness);

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
    void populateUI() override;

    int calculateIterLimit() const;

    
    DAngledRect getAngledRect(const MandelState& s) const
    {
        DVec2 world_size = (s.ctx_stage_size / reference_zoom) / s.cam_view.zoom;
        return DAngledRect(s.cam_view.x, s.cam_view.y, world_size.x, world_size.y, s.cam_view.angle);
    }


    void loadTemplate(std::string_view data);
    void updateConfigBuffer();
    void loadConfigBuffer();

    void savefileChanged();
    std::string getURL();
};

struct Mandelbrot_Scene : public Scene<Mandelbrot_Scene_Data>
{

    // --- Custom Launch Config ---
    struct Config {};
    Mandelbrot_Scene(Config&) {}

    static constexpr int MAX_THREADS = 0;

    std::shared_ptr<NanoFont> font;

    bool display_intro = true;

    int current_row = 0;
    EscapeField field_9x9 = EscapeField(0); // Processed in a single frame (fast)
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

    ///std::chrono::steady_clock::time_point compute_t0;
    ///Math::MovingAverage::MA<double> timer_ma = Math::MovingAverage::MA<double>(10);

    Math::MovingAverage::MA<DVec2> avg_vel_pos = Math::MovingAverage::MA<DVec2>(20);
    Math::MovingAverage::MA<double> avg_vel_zoom = Math::MovingAverage::MA<double>(20);

    DVec2 camera_vel_pos{0, 0};
    double camera_vel_zoom{1};
    
    /// std::vector< std::vector<DVec2> > boundary_paths;
    /// 
    /// // Per-level boundry tracing
    /// void generateBoundary(EscapeField* field, double level, std::vector<DVec2>& path);
    ///

    inline int numThreads() const
    {
        if constexpr (MAX_THREADS > 0) return MAX_THREADS;
        return Thread::idealThreadCount();
    }

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
        //blPrint("Shading compute phase: %d", active_field->compute_phase);
        active_bmp->forEachPixel([&, this](int x, int y)
        {
            EscapeFieldPixel& field_pixel = active_field->at(x, y);

            if (field_pixel.depth >= INSIDE_MANDELBROT_SET_SKIPPED) 
            {
                ///#ifdef BL_DEBUG
                ///if (field_pixel.depth == INSIDE_MANDELBROT_SET_SKIPPED)
                ///    active_bmp->setPixel(x, y, 0xFF7F007F);
                ///else
                ///#endif
                    active_bmp->setPixel(x, y, 0xFF000000);
                return;
            }
             

            uint32_t u32;
            //double final_value = field_pixel.final_depth;
            //double t = (final_value - log_color_cycle_iters * std::floor(final_value / log_color_cycle_iters)) / log_color_cycle_iters;
            
            double iter_r = field_pixel.final_depth / log_color_cycle_iters;
            double dist_r = field_pixel.final_dist  / cycle_dist_value;

            double iter_w = 1.0 - iter_dist_mix;
            double dist_w = iter_dist_mix;

            double combined_t = Math::wrap(iter_r*iter_w + dist_r*dist_w, 0.0, 1.0);
            //double combined_t = Math::wrap(iter_r*dist_r, 0.0, 1.0);

            //double iter_t = Math::wrap(iter_r, 0.0, 1.0);
            //double dist_t = Math::wrap(dist_r, 0.0, 1.0);
            //double avg_t = iter_t;// Math::avg(iter_t, dist_t);
            
            gradient_shifted.unguardedRGBA(combined_t, u32);

            active_bmp->setPixel(x, y, u32);
        }, numThreads());
    }

    bool frame_complete = false;

    //bool Use_Splines
    template<
        typename T,
        MandelSmoothing Smoothing,
        bool flatten,
        bool Axis_Visible
    >
    bool mandelbrot()
    {
        int timeout;

        switch (computing_phase)
        {
        case 0: timeout = 0; break;
        default: timeout = 16; break;
        }

        //DQuad world_quad = pending_bmp->worldQuad();
        //unsigned w = pending_bmp->width();
        //unsigned h = pending_bmp->height();

        //blPrint() << "Computing field: " << computing_phase;
        frame_complete = pending_bmp->forEachWorldPixel<T>(
            current_row, [&](int x, int y, T wx, T wy)
        {
            EscapeFieldPixel& field_pixel = pending_field->at(x, y);

            // Pixel already calculated/forwarded from previous phase? Return early
            double depth = field_pixel.depth;
            if (depth >= 0)
                return; 

            // Do we already have this result mirrored from across the X-axis?
            //if constexpr (Axis_Visible)
            //{
            //    // Possibly not thread-safe since another task could be writing to mirrored pixel.
            //    // However, in practice this seems to work fine
            //    IVec2 mirrored_pixel_pos = pending_bmp->pixelPosFromWorld(DVec2{ wx, -wy });
            //
            //    if ((unsigned)mirrored_pixel_pos.x < w && (unsigned)mirrored_pixel_pos.y < h)
            //    {
            //        EscapeFieldPixel& mirrored_pixel = pending_field->at(mirrored_pixel_pos.x, mirrored_pixel_pos.y);
            //        if (mirrored_pixel.depth >= 0)
            //        {
            //            // Yes
            //            field_pixel.depth = mirrored_pixel.depth;
            //            field_pixel.dist = mirrored_pixel.dist;
            //            return;
            //        }
            //    }
            //}

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
                mandel_kernel<T, Smoothing>(wx, wy, iter_lim, depth, dist);
            }

            field_pixel.depth = depth;
            field_pixel.dist = dist;
 
        }, numThreads(), timeout);

        if (frame_complete)
            refreshFieldDepthNormalized();

        return frame_complete;
    };

    void refreshFieldDepthNormalized()
    {
        pending_field->min_depth = std::numeric_limits<double>::max();
        pending_field->max_depth = std::numeric_limits<double>::lowest();
 
        // Redetermine minimum depth for entire visible field
        pending_bmp->forEachPixel([&, this](int x, int y)
        {
            EscapeFieldPixel& field_pixel = pending_field->at(x, y);
            double depth = field_pixel.depth;

            if (depth >= INSIDE_MANDELBROT_SET_SKIPPED) return;
            if (depth < pending_field->min_depth) pending_field->min_depth = depth;
            if (depth > pending_field->max_depth) pending_field->max_depth = depth;
        }, 0);

        if (pending_field->min_depth == std::numeric_limits<double>::max()) pending_field->min_depth = 0;
        

        // Calculate normalized depth/dist
        double dist_min_pixel_ratio = (100.0 - cycle_dist_sharpness) / 100.0 + 0.00001;
        double stable_min_raw_dist = camera->stageToWorldOffset(DVec2{ dist_min_pixel_ratio, 0 }).magnitude(); // quarter pixel
        //double stable_min_raw_dist = camera->stageToWorldOffset(DVec2{ 0.0000000000001, 0 }).magnitude(); // quarter pixel
        double stable_max_raw_dist = pending_bmp->worldSize().magnitude() / 2.0; // half diagonal world viewport size

        double stable_min_dist = (cycle_dist_invert ? -1 : 1) * log(stable_min_raw_dist);
        double stable_max_dist = (cycle_dist_invert ? -1 : 1) * log(stable_max_raw_dist);

        pending_bmp->forEachPixel([&](int x, int y)
        {
            EscapeFieldPixel& field_pixel = pending_field->at(x, y);

            double depth = field_pixel.depth;
            double raw_dist = field_pixel.dist;

            // "dist" is highest next to mandelbrot set
            double dist = (smoothing_type & (int)MandelSmoothing::DIST) ?
                ((cycle_dist_invert ? -1 : 1) * log(raw_dist))
                : 0;

            double dist_factor = cycle_dist_invert ?
                (0 - Math::lerpFactor(dist, stable_min_dist, stable_max_dist)) :
                     Math::lerpFactor(dist, stable_min_dist, stable_max_dist);

            double floor_depth = cycle_iter_normalize_depth ? pending_field->min_depth : 0;

            double final_dist = dist_factor;
            double final_depth = Math::linear_log1p_lerp(depth - floor_depth, cycle_iter_log1p_weight);

            #ifdef BL_DEBUG
            if (isnan(final_depth))
            {
                // There is a definite crash here, don't remove until you've caught and solved it
                blBreak();
            }
            #endif
            
            field_pixel.final_depth = final_depth;
            field_pixel.final_dist = final_dist;
        }, numThreads());
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
    };

    // --- Simulation processing ---
    void sceneStart() override;
    void sceneMounted(Viewport* viewport) override;

    // Viewport handling
    void viewportProcess(Viewport* ctx, double dt) override;
    void viewportDraw(Viewport* ctx) const override;
    void viewportOverlay(Viewport* ctx) const override;

    DRect minimizeMaximizeButtonRect(Viewport* ctx) const;
    DRect minimizeMaximizeButton(Viewport* ctx, bool minimize) const;

    // Input
    void onEvent(Event e) override;
};

struct Mandelbrot_Project : public BasicProject
{
    static ProjectInfo info()
    {
        return ProjectInfo({ "Fractal", "Mandelbrot", "Mandelbrot Viewer" });
    }

    void projectPrepare(Layout& layout) override;
};

SIM_END;
