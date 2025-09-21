#include "Mandelbrot.h"
#include "conversions.h"

SIM_BEG;

flt128 tweenDistance(
    MandelState& a,
    MandelState& b)
{
    flt128 dh = toHeight(b.cam_view.zoom) - toHeight(a.cam_view.zoom);
    flt128 dx = b.cam_view.x - a.cam_view.x;
    flt128 dy = b.cam_view.y - a.cam_view.y;
    flt128 d = sqrt(dx * dx + dy * dy + dh * dh);
    return d;
}

void startTween(Mandelbrot_Scene &scene)
{

    // Now, set start/end tween states
    scene.state_a = scene;
    //scene.state_b = target;

    // Switch to raw iter_lim for tween (and switch to quality mode on finish)
    scene.dynamic_iter_lim = false;
    scene.quality = scene.iter_lim;


    scene.tween_r1 = scene.getAngledRect(scene.state_a);
    scene.tween_r2 = scene.getAngledRect(scene.state_b);

    DDAngledRect encompassing;
    encompassing.fitTo(scene.tween_r1, scene.tween_r2, scene.tween_r1.aspectRatio());

    flt128 encompassing_zoom = (scene.stageWorldSize() / encompassing.size).average();
    flt128 encompassing_height = std::min((flt128)1.0, toHeight(encompassing_zoom)); // Cap at max height
    scene.tween_lift = encompassing_height - std::max(toHeight(scene.state_a.cam_view.zoom), toHeight(scene.state_b.cam_view.zoom));

    flt128 max_lift = flt128{ 1.0 } - toHeight(scene.state_b.cam_view.zoom);
    if (max_lift < flt128{ 0 }) max_lift = flt128{ 0 };
    if (scene.tween_lift > max_lift)
        scene.tween_lift = max_lift;


    // Begin tween
    scene.tween_progress = 0.0;
    scene.tweening = true;
    scene.tween_duration = 2;// tweenDistance(scene_data.state_a, scene_data.state_b);
    scene.cycle_dist_invert = scene.state_b.cycle_dist_invert;
}

void lerpState(
    Mandelbrot_Scene& scene,
    MandelState& a,
    MandelState& b,
    double f,
    bool complete)
{
    MandelState& dst = scene;

    float pos_f = scene.tween_pos_spline((float)f);

    // === Calculate true 'b' iter_lim for tweening  (using destination zoom, not current zoom) ===
    double dst_iter_lim = b.dynamic_iter_lim ?
        (mandelbrotIterLimit(b.cam_view.zoom) * b.quality) :
        b.quality;

    // === Lerp Camera View and normalized zoom from "height" ===
    double lift_weight = (double)scene.tween_zoom_lift_spline((float)f);
    flt128 lift_height = scene.tween_lift * lift_weight;
    flt128 a_height = toHeight(a.cam_view.zoom);
    flt128 b_height = toHeight(b.cam_view.zoom);

    double base_zoom_f = scene.tween_base_zoom_spline((float)f);
    flt128 dst_height = Math::lerp(a_height, b_height, base_zoom_f) + lift_height;
    dst.cam_view = CameraViewController::lerp(a.cam_view, b.cam_view, pos_f);
    dst.cam_view.zoom = fromHeight(dst_height); // override zoom from computed "height"

    // === Quality ===
    dst.quality = Math::lerp(a.quality, dst_iter_lim, pos_f);

    // === Color Cycle ===
    float color_cycle_f = scene.tween_color_cycle((float)f);

    // === Shader weights ===
    dst.iter_weight = Math::lerp(a.iter_weight, b.iter_weight, color_cycle_f);
    dst.dist_weight = Math::lerp(a.dist_weight, b.dist_weight, color_cycle_f);
    dst.stripe_weight = Math::lerp(a.stripe_weight, b.stripe_weight, color_cycle_f);

    // === Iter shader === 
    dst.cycle_iter_value = Math::lerp(a.cycle_iter_value, b.cycle_iter_value, color_cycle_f);
    dst.cycle_iter_log1p_weight = Math::lerp(a.cycle_iter_log1p_weight, b.cycle_iter_log1p_weight, color_cycle_f);

    // === Dist Shader
    dst.cycle_dist_value = Math::lerp(a.cycle_dist_value, b.cycle_dist_value, color_cycle_f);
    dst.cycle_dist_sharpness = Math::lerp(a.cycle_dist_sharpness, b.cycle_dist_sharpness, color_cycle_f);

    // === Stripe Shader
    dst.stripe_params.freq = Math::lerp(a.stripe_params.freq, b.stripe_params.freq, color_cycle_f);
    dst.stripe_params.phase = Math::lerp(a.stripe_params.phase, b.stripe_params.phase, color_cycle_f);
    dst.stripe_params.contrast = Math::lerp(a.stripe_params.contrast, b.stripe_params.contrast, color_cycle_f);

    // === Lerp Gradient/Hue Shift===
    dst.gradient_shift = Math::lerp(a.gradient_shift, b.gradient_shift, pos_f);
    dst.hue_shift = Math::lerp(a.hue_shift, b.hue_shift, pos_f);

    // === Lerp animation speed for Gradient/Hue Shift ===
    dst.gradient_shift_step = Math::lerp(a.gradient_shift_step, b.gradient_shift_step, pos_f);
    dst.hue_shift_step = Math::lerp(a.hue_shift_step, b.hue_shift_step, pos_f);

    // === Lerp Color Gradient ===
    ImGradient::lerp(scene.gradient, a.gradient, b.gradient, (float)f);

    if (complete)
    {
        dst.dynamic_iter_lim = b.dynamic_iter_lim;
        dst.quality = b.quality;

        dst.cycle_iter_dynamic_limit = b.cycle_iter_dynamic_limit;

        dst.show_axis = b.show_axis;
        dst.show_color_animation_options = b.show_color_animation_options;
    }

    // === Lerp quality ===
    //dst->iter_lim = Math::lerp(state_a.iter_lim, state_b.iter_lim, f);

    // === Lerp Cardioid Flattening Factor ===
    //dst->cardioid_lerp_amount = Math::lerp(state_a.cardioid_lerp_amount, state_b.cardioid_lerp_amount, f);

    // Spline Data
    //memcpy(dst->x_spline_point, Math::lerp(state_a.x_spline_points, state_b.x_spline_points, f));
}

SIM_END;
