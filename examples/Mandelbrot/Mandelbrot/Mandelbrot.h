#pragma once
#include "project.h"
#include "Cardioid/Cardioid.h"
#include <math.h>
#include <cmath>


SIM_BEG(Mandelbrot)

using namespace BL;

#include "types.h"
#include "kernel.h"
#include "shading.h"


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
    CameraViewController cam_view;

    bool dynamic_iter_lim = true;
    double quality = 0.5; // Used for UI (ignore during tween until complete)
    double smooth_iter_dist_ratio = 0.0;

    bool dynamic_color_cycle_limit = true;
    bool normalize_depth_range = true;
    double log1p_weight = 0.0;
    
    double cycle_iter_value = 0.5f; // If dynamic, iter_lim ratio, else iter_lim
    double cycle_dist_value = 0.5f;
    
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
            cam_view == rhs.cam_view &&
            quality == rhs.quality &&
            smooth_iter_dist_ratio == rhs.smooth_iter_dist_ratio &&
            dynamic_iter_lim == rhs.dynamic_iter_lim &&
            normalize_depth_range == rhs.normalize_depth_range &&
            log1p_weight == rhs.log1p_weight &&
            cycle_iter_value == rhs.cycle_iter_value &&
            cycle_dist_value == rhs.cycle_dist_value &&
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

struct Mandelbrot_Data : public VarBuffer, public TweenableMandelState
{
    char config_buf[1024] = "";
    char pos_tween_buf[1024] = "";
    char zoom_tween_buf[1024] = "";

    TweenableMandelState state_a;
    TweenableMandelState state_b;
    bool tweening = false;
    double tween_progress = 0.0; // 0..1
    double tween_lift = 0.0;
    double tween_duration = 0.0;

    DAngledRect r1;
    DAngledRect r2;

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
        { {-0.147f,0.0f},{0.253f,0.0f},{0.553f,0.0f},{0.439f,1.0f},{0.74f,1.0f},{1.14f,1.0f} }
    );

    ImSpline::Spline tween_zoom_lift_spline = ImSpline::Spline(100,
        { {-0.1f,0.0f},{0.0f,0.0f},{0.1f,0.0f},{0.201f,0.802f},{0.251f,1.002f},{0.351f,1.402f},{0.643f,1.397f},{0.744f,0.997f},{0.794f,0.797f},{0.9f,0.0f},{1.0f,0.0f},{1.1f,0.0f} }
    );

    ImSpline::Spline tween_color_cycle = ImSpline::Spline(100,
        { { 0.072f, 0.0f }, { 0.5f,0.0f }, { 0.845f,0.0f }, { 0.75f,1.0f }, { 1.0f,1.0f }, { 1.25f,1.0f } }
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

        sync(tween_pos_spline);
        sync(tween_zoom_lift_spline);

        sync(reference_zoom);
        sync(ctx_stage_size);

        sync(r1);
        sync(r2);

        sync(show_axis);
        sync(cam_view);
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
        sync(cycle_iter_value);
        sync(cycle_dist_value);
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

    int calculateIterLimit() const;

    double toNormalizedZoom(double zoom) const { return log(zoom) + 1.0; }
    double fromNormalizedZoom(double normalized_zoom) const { return exp(normalized_zoom - 1.0); }

    double toHeight(double zoom) const { return 1.0 / toNormalizedZoom(zoom); }
    double fromHeight(double height) const { return fromNormalizedZoom(1.0 / height); }

    void setNormalizedZoom(double normalized_zoom) { cam_view.cam_zoom = fromNormalizedZoom(normalized_zoom); }
    double getNormalizedZoom() const { return toNormalizedZoom(cam_view.cam_zoom); }

    DAngledRect getAngledRect(const TweenableMandelState& s) const
    {
        DVec2 world_size = (s.ctx_stage_size / reference_zoom) / s.cam_view.cam_zoom;
        return DAngledRect(s.cam_view.cam_x, s.cam_view.cam_y, world_size.x, world_size.y, s.cam_view.cam_radians);
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
            grad.addMark(0.0f, ImColor(0, 0, 0));
            grad.addMark(0.3f, ImColor(73, 54, 254));
            grad.addMark(0.47f,  ImColor(242, 22, 116));
            grad.addMark(0.53f, ImColor(255, 56, 41));
            grad.addMark(0.62f ,ImColor(208, 171, 1));
            grad.addMark(0.62001f, ImColor(0, 0, 0));
            //grad.addMark(0.655f, ImColor(0, 0, 0));
        }
        break;

        default:
        {
            uint8_t last_r, last_g, last_b;
            colorGradientTemplate(type, 0.0f, last_r, last_g, last_b);
            grad.addMark(0.0f, ImColor(last_r, last_g, last_b));

            float last_h, last_s, last_v;
            Color::RGBtoHSV(last_r, last_g, last_b, last_h, last_s, last_v);

            for (float x = 0.0f; x < 1.0f; x += 0.01f)
            {
                uint8_t r, g, b;
                float h, s, v;

                colorGradientTemplate(type, x, r, g, b);
                Color::RGBtoHSV(r, g, b, h, s, v);

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

    double tweenDistance(
        TweenableMandelState& a,
        TweenableMandelState& b)
    {
        double dh = toHeight(b.cam_view.cam_zoom) - toHeight(a.cam_view.cam_zoom);
        double dx = b.cam_view.cam_x - a.cam_view.cam_x;
        double dy = b.cam_view.cam_y - a.cam_view.cam_y;
        double d = sqrt(dx*dx + dy*dy + dh*dy);
        return d;
    }

    void startTween(TweenableMandelState& target)
    {
        // Switch to raw iter_lim for tween (and switch to quality mode on finish)
        dynamic_iter_lim = false;
        quality = iter_lim;

        TweenableMandelState& state = *this;

        target.reference_zoom = reference_zoom;
        target.ctx_stage_size = ctx_stage_size;

        r1 = getAngledRect(state);
        r2 = getAngledRect(target);

        DAngledRect encompassing;
        encompassing.fitTo(r1, r2, r1.aspectRatio());

        double encompassing_zoom = (state.ctx_world_size() / encompassing.size).average();
        double encompassing_height = std::min(1.0, toHeight(encompassing_zoom)); // Cap at max height
        tween_lift = encompassing_height - std::max(toHeight(state.cam_view.cam_zoom), toHeight(target.cam_view.cam_zoom));

        double max_lift = 1.0 - toHeight(target.cam_view.cam_zoom);
        if (max_lift < 0) max_lift = 0;
        if (tween_lift > max_lift)
            tween_lift = max_lift;

        // Now, set start/end tween states
        state_a = state;
        state_b = target;

        // Begin tween
        tween_progress = 0.0;
        tweening = true;
        tween_duration = pow(tweenDistance(state_a, state_b), 0.5);

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
    
    /// std::vector< std::vector<DVec2> > boundary_paths;
    /// 
    /// // Per-level boundry tracing
    /// void generateBoundary(EscapeField* field, double level, std::vector<DVec2>& path);

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
        //BL::print("Shading compute phase: %d", active_field->compute_phase);
        active_bmp->forEachPixel([&, this](int x, int y)
        {
            EscapeFieldPixel& field_pixel = active_field->at(x, y);

            if (field_pixel.depth >= INSIDE_MANDELBROT_SET_SKIPPED) 
            {
                active_bmp->setPixel(x, y, 0xFF000000);
                return;
            }
             

            uint32_t u32;
            //double final_value = field_pixel.final_depth;
            //double t = (final_value - log_color_cycle_iters * std::floor(final_value / log_color_cycle_iters)) / log_color_cycle_iters;
            
            double iter_r = field_pixel.final_depth / log_color_cycle_iters;
            double dist_r = field_pixel.final_dist  / cycle_dist_value;

            double iter_w = 1.0 - smooth_iter_dist_ratio;
            double dist_w = smooth_iter_dist_ratio;

            double combined_t = Math::wrap(iter_r*iter_w + dist_r*dist_w, 0.0, 1.0);
            //double combined_t = Math::wrap(iter_r*dist_r, 0.0, 1.0);

            //double iter_t = Math::wrap(iter_r, 0.0, 1.0);
            //double dist_t = Math::wrap(dist_r, 0.0, 1.0);
            //double avg_t = iter_t;// Math::avg(iter_t, dist_t);
            
            gradient_shifted.unguardedRGBA(combined_t, u32);

            active_bmp->setPixel(x, y, u32);
        });
    }


    //bool Use_Splines
    template<
        typename T,
        MandelSmoothing Smooth_Iter,
        bool flatten
    >
    bool mandelbrot()
    {
        int timeout;

        switch (computing_phase)
        {
        case 0: timeout = 0; break;
        default: timeout = 16; break;
        }

        bool frame_complete = pending_bmp->forEachWorldPixel<T>(
            current_row, [&](int x, int y, T wx, T wy)
        {
            // Result already calculated in previous phase? (forwarded to active_bmp)
            EscapeFieldPixel& field_pixel = pending_field->at(x, y);
            double depth = field_pixel.depth;
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
                // 
                //mandelbrot_ex<T, MandelSmoothing::ITER>(wx, wy, iter_lim, depth, dist);
                mandel_kernel<T, MandelSmoothing::ITER>(wx, wy, iter_lim, depth, dist);

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

            

            field_pixel.depth = depth;
            field_pixel.dist = dist;
 
        }, (int)(1.5f*(float)Thread::idealThreadCount()), timeout);

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
                EscapeFieldPixel& field_pixel = pending_field->at(x, y);
                double depth = field_pixel.depth;

                if (depth >= INSIDE_MANDELBROT_SET_SKIPPED) return;
                if (depth < pending_field->min_depth) pending_field->min_depth = depth;
                if (depth > pending_field->max_depth) pending_field->max_depth = depth;

                if (smoothing_type & (int)MandelSmoothing::DIST) // todo: make constexpr
                {
                    double dist = -log(field_pixel.dist);
                    if (dist < pending_field->min_dist) pending_field->min_dist = dist;
                    if (dist > pending_field->max_dist) pending_field->max_dist = dist;
                }
            }, 0);

            if (pending_field->min_depth == std::numeric_limits<double>::max()) pending_field->min_depth = 0;
        }

        // Calculate normalized depth/dist
        pending_bmp->forEachPixel([&](int x, int y)
        {
            EscapeFieldPixel& field_pixel = pending_field->at(x, y);

            double depth = field_pixel.depth;
            double raw_dist = field_pixel.dist;// / pending_field->max_dist;

            //double lower_depth_bound = normalize_depth_range ? pending_field->min_depth : 0;
            //
            //double normalized_depth = Math::lerpFactor(depth, lower_depth_bound, (double)iter_lim);

            ///double dist_factor = Math::lerpFactor(raw_dist, pending_field->min_dist, pending_field->max_dist);

            //double floor_dist = log(pending_field->min_dist);
            //double ceil_dist  = log(pending_field->max_dist);
            double floor_dist = pending_field->min_dist;
            double ceil_dist  = pending_field->max_dist;
            double dist = (smoothing_type & (int)MandelSmoothing::DIST) ? -log(raw_dist) : 0;// std::numeric_limits<double>::epsilon();

            //floor_dist = Math::linear_log1p_lerp(floor_dist, log1p_weight);
            //ceil_dist = Math::linear_log1p_lerp(ceil_dist, log1p_weight);
            //dist = Math::linear_log1p_lerp(dist, log1p_weight);

            //double max_log_dist = Math::linear_log1p_lerp(ceil_dist - floor_dist, log1p_weight);
            //double final_dist   = Math::linear_log1p_lerp(dist      - floor_dist, log1p_weight) / max_log_dist;

            double dist_factor  = 1.0 - Math::lerpFactor(dist, floor_dist, ceil_dist);
            //dist_factor = Math::linear_log1p_lerp(dist_factor, log1p_weight);
            
            double final_dist = dist_factor;// dist;
            ///UNUSED(floor_dist);
            ///UNUSED(ceil_dist);
            ///UNUSED(dist_factor);

            //
            //double log_depth = -log(std::max(normalized_depth,0.0000000001));
            //double final_depth = Math::lerp(log_depth, normalized_depth, log1p_weight);
            //
            //if (isnan(final_depth))
            //{
            //    DebugBreak();
            //}
            //double normalized_dist = -log(dist);

            double floor_depth = normalize_depth_range ? pending_field->min_depth : 0;

            ///double max_log_depth = Math::linear_log1p_lerp(pending_field->max_depth - floor_depth, log1p_weight);
            double final_depth = Math::linear_log1p_lerp(depth - floor_depth, log1p_weight);// / max_log_depth;


            //field_pixel.final_value = final_value;
            
            field_pixel.final_depth = final_depth;
            field_pixel.final_dist = final_dist;

            /*switch ((MandelSmoothing)smoothing_type)
            {
                case MandelSmoothing::ITER:
                {
                    field_pixel.final_value = final_depth;
                }
                break;
                case MandelSmoothing::DIST:
                {
                    field_pixel.final_value = final_dist;
                }
                break;
                case MandelSmoothing::MIX:
                {
                    field_pixel.final_value = Math::lerp(final_depth, final_dist, smooth_iter_dist_ratio);
                }
                break;
            }

            if (!isfinite(field_pixel.final_value))
            {
                DebugBreak();
            }*/
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

    // Input
    void onEvent(Event e) override;
};

struct Mandelbrot_Project : public BasicProject
{
    //static std::vector<std::string> categorize() {
    //    return { "Fractal", "Mandelbrot", "Mandelbrot Viewer" };
    //}

    static ProjectInfo info()
    {
        return ProjectInfo({ "Fractal", "Mandelbrot", "Mandelbrot Viewer" });
    }

    void projectPrepare(Layout& layout) override;
};

SIM_END(Mandelbrot)
