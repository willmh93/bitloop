#pragma once
#include <bitloop.h>
#include <cmath>

#include "Cardioid/Cardioid.h"


// Mandelbrot includes
#include "build_config.h"

#include "types.h"
#include "state.h"
#include "compute.h"
#include "shading.h"
#include "tween_splines.h"
#include "examples.h"

SIM_BEG;

using namespace bl;


//  ──────  MandelState inheritance  ────── 
//  Lerp between 'state_a' --> 'state_b' => Save result in inherited "live" MandelState

struct Mandelbrot_Scene : public MandelState, public Scene<Mandelbrot_Scene>
{
    // ──────  Config ──────
    struct Config { std::string load_example_name; };
    Mandelbrot_Scene(Config& config) : load_example_name(config.load_example_name) {}

    static inline std::map<std::string, std::string> mandel_presets = generateMandelPresets();

    std::string load_example_name;

    // ────── Threads ──────
    static constexpr int MAX_THREADS = 0; // 0 = Use max threads
    inline int numThreads() const {
        if constexpr (MAX_THREADS > 0) return MAX_THREADS;
        return Thread::idealThreadCount();
    }

    // ────── Resources ──────
    NanoFont font;

    // ────── Tweening ──────
    bool   tweening = false;
    double tween_progress = 0.0; // 0..1
    flt128 tween_lift = f128(0.0);
    double tween_duration = 0.0;

    MandelState state_a;
    MandelState state_b;
    DDAngledRect tween_r1;
    DDAngledRect tween_r2;

    DDVec2 reference_zoom;
    DDVec2 ctx_stage_size;

    DDVec2 stageWorldSize() const { return ctx_stage_size / reference_zoom; }
    DDAngledRect getAngledRect(const MandelState& s) const {
        return DDAngledRect(s.cam_pos(), stageWorldSize() / s.cam_zoom(), (flt128)s.cam_angle());
    }

    // ────── Cardioid ──────
    bool show_period2_bulb = true;

    #if MANDEL_FEATURE_INTERACTIVE_CARDIOID
    bool interactive_cardioid = false;
    #endif

    double cardioid_lerp_amount = 1.0; // 1 - flatten
    Cardioid::CardioidLerper cardioid_lerper;

    // ────── Gradients ──────
    ImGradient gradient_shifted;


    // ────── Splines ──────
    ImSpline::Spline x_spline               = MandelSplines::x_spline;
    ImSpline::Spline y_spline               = MandelSplines::y_spline;
    ImSpline::Spline tween_pos_spline       = MandelSplines::tween_pos_spline;
    ImSpline::Spline tween_zoom_lift_spline = MandelSplines::tween_zoom_lift_spline;
    ImSpline::Spline tween_base_zoom_spline = MandelSplines::tween_base_zoom_spline;
    ImSpline::Spline tween_color_cycle      = MandelSplines::tween_color_cycle;

    // ────── Saving & loading / URL ──────
    std::string config_buf = ""; // data for text box (save data / url)
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
    bool    display_intro = true;
    double  log_color_cycle_iters = 0.0;
    int     iter_lim = 0; // Actual iter limit
    bool    colors_updated = false; // still needed?
    DDQuad  world_quad{};
    int     current_row = 0;

    // Phase 0 = 9x smaller
    // Phase 1 = 3x smaller
    // Phase 2 = full resolution
    int  computing_phase = 0;
    bool frame_complete = false;  // Similar to finished_compute, but not cleared until next compute starts
    bool first_frame = true;

    // ────── Expensive Interior "forwarding" optimization ──────
    struct {
        int c1 = 1, e1 = 0, c2 = 1, e2 = 0;
    }  interior_phases_contract_expand;

    bool maxdepth_show_optimized = false;

    // ────── Computing / Shading ──────
    bool compute_mandelbrot();
    //void normalize_shading_limits();
    void normalize_field(EscapeField* field, CanvasImage128* bmp);

    // ────── Timers ──────
    #ifdef MANDEL_DEV_PERFORMANCE_TIMERS
    std::chrono::steady_clock::time_point compute_t0;
    Math::MovingAverage::MA<double> timer_ma = Math::MovingAverage::MA<double>(1);
    double dt_avg = 0;
    #endif

    // ────── Camera navigation easing ──────
    Math::MovingAverage::MA<DVec2> avg_vel_pos = Math::MovingAverage::MA<DVec2>(8);
    Math::MovingAverage::MA<double> avg_vel_zoom = Math::MovingAverage::MA<double>(8);

    DDVec2 camera_vel_pos{};
    double camera_vel_zoom{1};

    // ────── Deep Zoom Animation (temporary until timeline feature added) ──────
    bool steady_zoom = false;
    flt128 steady_zoom_mult_speed{ 0.01 };
    int tween_frames_elapsed = 0;
    int tween_expected_frames = 0;
    Math::MovingAverage::MA<double> expected_time_left_ma = Math::MovingAverage::MA<double>(5);


    // ────── User Interface ──────
    struct UI : Interface
    {
        using Interface::Interface;
        void sidebar();

        bool show_save_dialog = false;
        bool show_load_dialog = false;
        bool show_share_dialog = false;

        std::string url;

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

#undef DEV_MODE

SIM_END;
