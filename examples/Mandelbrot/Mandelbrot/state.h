#pragma once
#include <bitloop.h>
#include "shading.h"

SIM_BEG;
using namespace bl;

struct MandelState
{
    /// ─────────────────────── Saveable Info ───────────────────────

    bool show_axis = true;
    CameraViewController cam_view;

    /// ─────────────────────── Tweenable Info ───────────────────────

    bool        dynamic_iter_lim               = true;
    double      quality                        = 0.5; // Used for UI (ignored during tween, represents iter_lim OR % of iter_lim)
                                               
    bool        use_smoothing                  = true;

    double iter_weight = 1.0;
    double dist_weight = 0.0;
    double stripe_weight = 0.0;

    //double iter_ratio = 1.0;
    //double dist_ratio = 0.0;
    //double stripe_ratio = 0.0;
                                               
    double      cycle_iter_weight              = 0.0;
    bool        cycle_iter_dynamic_limit       = true;
    bool        cycle_iter_normalize_depth     = true;
    double      cycle_iter_log1p_weight        = 0.0;
    double      cycle_iter_value               = 0.5f; // If dynamic, iter_lim ratio, else iter_lim
                                               
    double      cycle_dist_weight              = 0.0;
    bool        cycle_dist_invert              = false;
    double      cycle_dist_value               = 0.5f;
    double      cycle_dist_sharpness           = 0.9; // Used for UI (ignored during tween)

    StripeParams stripe_params;
                                               
    double      cycle_stripe_weight            = 0.0;
                
    double      gradient_shift                 = 0.0;
    double      hue_shift                      = 0.0;
                                               
    double      gradient_shift_step            = 0.0078;
    double      hue_shift_step                 = 0.136;
                
    int         smoothing_type = (int)MandelSmoothing::ITER; // this is being dynamically set, no need to save
    int         shade_formula = (int)MandelShaderFormula::ITER_DIST_STRIPE;

    ImGradient  gradient;

    // animate
    bool        show_color_animation_options   = false;

    // Flatten
    bool        flatten = false;
    double      flatten_amount = 0.0;

    ///////////////////////////////////////////////////////////////////////////

    bool operator==(const MandelState&) const = default;

    inline DVec2  cam_pos() const { return cam_view.pos; }
    inline double cam_zoom() const { return cam_view.zoom; }
    inline double cam_angle() const { return cam_view.angle; }

    std::string serialize() const
    {
        constexpr bool COMPRESS_CONFIG = true;

        // Increment version each time the format changes
        uint32_t version = 0;
        uint32_t flags = 0;

        if (dynamic_iter_lim)           flags |= MANDEL_DYNAMIC_ITERS;
        if (show_axis)                  flags |= MANDEL_SHOW_AXIS;
        if (flatten)                    flags |= MANDEL_FLATTEN;

        if (cycle_iter_dynamic_limit)   flags |= MANDEL_DYNAMIC_COLOR_CYCLE;
        if (cycle_iter_normalize_depth) flags |= MANDEL_NORMALIZE_DEPTH;
        if (cycle_dist_invert)          flags |= MANDEL_INVERT_DIST;
        if (use_smoothing)              flags |= MANDEL_USE_SMOOTHING;

        //flags |= ((uint32_t)smoothing_type << MANDEL_SMOOTH_BITSHIFT);
        flags |= (version << MANDEL_VERSION_BITSHIFT);

        JSON::json info;
        int decimals = cam_view.getPositionDecimalPlaces();

        if (version >= 0)
        {
            info["f"] = flags;

            /// Mark JSON floats with "CLEANFLOAT()" to remove trailing zeros/rounding errors when serializing

            // View
            info["x"] = JSON::markCleanFloat(cam_view.x, decimals);
            info["y"] = JSON::markCleanFloat(cam_view.y, decimals);
            info["z"] = JSON::markCleanFloat(cam_view.zoom, 2); /// todo: If you increase to 128 bit precision, be careful
            info["a"] = JSON::markCleanFloat(cam_view.zoom_xy.x, 3);
            info["b"] = JSON::markCleanFloat(cam_view.zoom_xy.y, 3);
            info["r"] = JSON::markCleanFloat(Math::toDegrees(cam_view.angle), 0);

            // Quality
            info["q"] = JSON::markCleanFloat(quality, 3);

            // Color cycle
            {
                // Mix ratio
                info["m"] = JSON::markCleanFloat(iter_weight, 2);
                info["n"] = JSON::markCleanFloat(dist_weight, 2);
                info["o"] = JSON::markCleanFloat(stripe_weight, 2);

                // ITER
                info["i"] = JSON::markCleanFloat(cycle_iter_value);
                info["l"] = JSON::markCleanFloat(cycle_iter_log1p_weight);

                // DIST
                info["d"] = JSON::markCleanFloat(cycle_dist_value, 5);
                info["s"] = JSON::markCleanFloat(cycle_dist_sharpness, 5);

                // STRIPE
                info["v"] = (int)stripe_params.freq;
                info["j"] = JSON::markCleanFloat(stripe_params.phase, 4);
                info["c"] = JSON::markCleanFloat(stripe_params.contrast, 3);
            }

            // Shift
            info["g"] = JSON::markCleanFloat(gradient_shift);
            info["h"] = JSON::markCleanFloat(hue_shift);

            // Shift increment
            info["A"] = show_color_animation_options ? 1 : 0;
            info["G"] = JSON::markCleanFloat(gradient_shift_step);
            info["H"] = JSON::markCleanFloat(hue_shift_step);

            // Gradient
            info["p"] = gradient.serialize();
        }
        else if (version >= 1)
        {
            //info["A"] = x_spline.serialize(SplineSerializationMode::COMPRESS_SHORTEST);
            //info["B"] = y_spline.serialize(SplineSerializationMode::COMPRESS_SHORTEST);
        }

        std::string raw_json = JSON::unmarkCleanFloats(info.dump());

        if constexpr (COMPRESS_CONFIG)
        {
            // Post-process: Remove CLEANFLOAT() markers, compress & wrap lines
            std::string json_unquoted = JSON::json_remove_key_quotes(raw_json);
            std::string compressed_unquoted = Compression::brotli_ascii_compress(json_unquoted);
            return compressed_unquoted;
        }
        else
        {
            return raw_json;
        }
    }

private:
    bool _deserialize(std::string_view sv, bool COMPRESS_CONFIG)
    {
        sv = TextUtil::trim_view(sv);
        std::string txt = sv.data();

        std::string uncompressed;
        if (COMPRESS_CONFIG)
        {
            if (!Compression::valid_b62(txt)) return false;
            uncompressed = Compression::brotli_ascii_decompress(TextUtil::unwrapString(txt));
        }
        else
        {
            uncompressed = txt;
        }

        uncompressed = JSON::json_add_key_quotes(uncompressed);
        uncompressed = JSON::json_add_leading_zeros(uncompressed);

        blPrint() << "decoded: " <<  uncompressed;

        nlohmann::json info = nlohmann::json::parse(uncompressed, nullptr, false);
        if (info.is_discarded())
            return false; // Failed to parse

        uint32_t flags = info.value("f", 0ul);
        uint32_t version = (flags & MANDEL_VERSION_MASK) >> MANDEL_VERSION_BITSHIFT;

        if (version >= 0)
        {
            //smoothing_type = ((flags & MANDEL_SMOOTH_MASK) >> MANDEL_SMOOTH_BITSHIFT);

            dynamic_iter_lim            = flags & MANDEL_DYNAMIC_ITERS;
            show_axis                   = flags & MANDEL_SHOW_AXIS;
            flatten                     = flags & MANDEL_FLATTEN;
            cycle_iter_dynamic_limit    = flags & MANDEL_DYNAMIC_COLOR_CYCLE;
            cycle_iter_normalize_depth  = flags & MANDEL_NORMALIZE_DEPTH;
            cycle_dist_invert           = flags & MANDEL_INVERT_DIST;
            use_smoothing               = flags & MANDEL_USE_SMOOTHING;

            // View
            cam_view.x = info.value("x", 0.0);
            cam_view.y = info.value("y", 0.0);
            cam_view.zoom = info.value("z", 1.0);
            cam_view.zoom_xy.x = info.value("a", 1.0);
            cam_view.zoom_xy.y = info.value("b", 1.0);
            double angle_degrees = info.value("r", 0.0);
            cam_view.angle = Math::toRadians(angle_degrees);

            // Quality
            quality = info.value("q", quality);

            // Color cycle
            {
                // Mix ratio
                //iter_dist_mix = info.value("m", iter_dist_mix);
                iter_weight = info.value("m", iter_weight);
                dist_weight = info.value("n", 1.0 - iter_weight);
                stripe_weight = info.value("o", 0.0);

                // ITER
                cycle_iter_value = info.value("i", cycle_iter_value);
                cycle_iter_log1p_weight = info.value("l", cycle_iter_log1p_weight);

                // DIST
                cycle_dist_value = info.value("d", cycle_dist_value);
                cycle_dist_sharpness = info.value("s", cycle_dist_sharpness);

                // STRIPE
                stripe_params.freq = info.value("v", 0.0f);
                stripe_params.phase = info.value("j", 0.0f);
                stripe_params.contrast = info.value("c", 3.0f);
            }

            // Shift
            show_color_animation_options = info.value("A", show_color_animation_options ? 1 : 0) != 0;
            gradient_shift = info.value("g", 0.0);
            hue_shift = info.value("h", 0.0);

            // Shift increment
            gradient_shift_step = info.value("G", gradient_shift_step);
            hue_shift_step = info.value("H", hue_shift_step);

            // Gradient
            if (info.contains("p"))
                gradient.deserialize(info.value("p", ""));
        }
        else if (version >= 1)
        {
            //if (info.contains("A")) x_spline.deserialize(info["A"].get<std::string>());
            //if (info.contains("B")) y_spline.deserialize(info["B"].get<std::string>());
        }

        return true;
    }

public:
    bool deserialize(std::string_view sv)
    {
        // First, try decoding compressed
        if (_deserialize(sv, true)) return true;
        if (_deserialize(sv, false)) return true;
        return true;
    }
};

SIM_END;
