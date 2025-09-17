#pragma once
#include <bitloop.h>
#include <cmath>

#include "Cardioid/Cardioid.h"

#include "types.h"
#include "state.h"
#include "kernel.h"
#include "shading.h"
#include "splines.h"

SIM_BEG;

using namespace bl;

///  Multiple layers of inheritance explained:
///
///  Additionally, we set up a 'start' / 'end' tween state, and lerp between them, saving
///  the result into the inherited "live" MandelState state.

struct Mandelbrot_Scene : public MandelState, public Scene<Mandelbrot_Scene>
{
    // ──────  Config ──────
    struct Config {};
    Mandelbrot_Scene(Config&) {}

    // ────── Threads ──────
    static constexpr int MAX_THREADS = 0;
    inline int numThreads() const {
        if constexpr (MAX_THREADS > 0) return MAX_THREADS;
        return Thread::idealThreadCount();
    }

    // ────── Resources ──────
    NanoFont font;

    // ────── Tweening ──────
    bool   tweening = false;
    double tween_progress = 0.0; // 0..1
    double tween_lift = 0.0;
    double tween_duration = 0.0;

    MandelState state_a;
    MandelState state_b;
    DAngledRect tween_r1;
    DAngledRect tween_r2;

    DVec2 reference_zoom, ctx_stage_size;
    DVec2 stageWorldSize() const { return ctx_stage_size / reference_zoom; }
    DAngledRect getAngledRect(const MandelState& s) const {
        return DAngledRect(s.cam_pos(), stageWorldSize() / s.cam_zoom(), s.cam_angle());
    }

    // ────── Cardioid ──────
    bool show_period2_bulb = true;
    bool interactive_cardioid = false;
    double cardioid_lerp_amount = 1.0; // 1 - flatten
    Cardioid::CardioidLerper cardioid_lerper;

    // ────── Shading ──────
    ImGradient gradient_shifted;
    void updateShiftedGradient();

    //void shadeBitmap();
    void refreshFieldDepthNormalized();

    // ────── Splines ──────
    ImSpline::Spline x_spline               = MandelSplines::x_spline;
    ImSpline::Spline y_spline               = MandelSplines::y_spline;
    ImSpline::Spline tween_pos_spline       = MandelSplines::tween_pos_spline;
    ImSpline::Spline tween_zoom_lift_spline = MandelSplines::tween_zoom_lift_spline;
    ImSpline::Spline tween_base_zoom_spline = MandelSplines::tween_base_zoom_spline;
    ImSpline::Spline tween_color_cycle      = MandelSplines::tween_color_cycle;

    // ────── Quality calculations ──────
    //inline int    mandelbrotIterLimit(double zoom) const;
    //inline double qualityFromIterLimit(int iter_lim, double zoom_x) const;
    //inline int    finalIterLimit() const;

    // ────── Saving / loading / URL ──────
    std::string config_buf = "";
    void updateConfigBuffer();
    void loadConfigBuffer();

    void onSavefileChanged();
    std::string getURL() const;

    // ────── Fields & Bitmaps ──────
    EscapeField field_9x9 = EscapeField(0); // Processed in a single frame (fast)
    EscapeField field_3x3 = EscapeField(1); // Processed over multiple frames
    EscapeField field_1x1 = EscapeField(2); // Processed over multiple frames

    CanvasImage128 bmp_9x9;
    CanvasImage128 bmp_3x3;
    CanvasImage128 bmp_1x1;

    CanvasImage128* pending_bmp = nullptr;
    CanvasImage128* active_bmp = nullptr;

    EscapeField* pending_field = nullptr;
    EscapeField* active_field = nullptr;


    // ────── Dynamicly set at runtime ──────
    bool   display_intro = true;
    double log_color_cycle_iters = 0.0;
    int    iter_lim = 0; // Actual iter limit
    bool   colors_updated = false; // still needed?
    DDQuad  world_quad;
    int    current_row = 0;

    // 0 = 9x smaller
    // 1 = 3x smaller
    // 2 = full resolution
    int  computing_phase = 0;
    bool finished_compute = false; // Whether a frame finished computing THIS frame
    bool frame_complete = false;   // Similar to finished_compute, but not cleared each frame
    bool first_frame = true;

    // ────── Timers ──────
    ///std::chrono::steady_clock::time_point compute_t0;
    ///Math::MovingAverage::MA<double> timer_ma = Math::MovingAverage::MA<double>(10);

    // ────── Camera navigation easing ──────
    Math::MovingAverage::MA<DVec2> avg_vel_pos = Math::MovingAverage::MA<DVec2>(8);
    Math::MovingAverage::MA<double> avg_vel_zoom = Math::MovingAverage::MA<double>(8);

    DVec2 camera_vel_pos{0, 0};
    double camera_vel_zoom{1};

    // ────── User Interface ──────
    struct UI : Interface
    {
        using Interface::Interface;
        void populate();

        bool show_save_dialog = false;
        bool show_load_dialog = false;
        bool show_share_dialog = false;

        #ifdef __EMSCRIPTEN__
        bool opening_load_popup = false;
        #endif
    };

    // ────── Simulation processing ──────
    void sceneStart() override;
    void sceneMounted(Viewport* viewport) override;

    // ────── Viewport handling ──────
    void viewportProcess(Viewport* ctx, double dt) override;
    void viewportDraw(Viewport* ctx) const override;
    void viewportOverlay(Viewport* ctx) const override;

    DRect minimizeMaximizeButtonRect(Viewport* ctx) const;
    DRect minimizeMaximizeButton(Viewport* ctx, bool minimize) const;

    // ────── Input ──────
    void onEvent(Event e) override;
};

struct Mandelbrot_Project : public BasicProject
{
    static ProjectInfo info() {
        return ProjectInfo({ "Fractal", "Mandelbrot", "Mandelbrot Viewer" });
    }

    void projectPrepare(Layout& layout) override;
};

SIM_END;
