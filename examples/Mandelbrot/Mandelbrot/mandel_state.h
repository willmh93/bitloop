#pragma once
#include <bitloop.h>
#include "shading.h"

SIM_BEG;
using namespace BL;

constexpr bool COMPRESS_CONFIG = true;

struct MandelState
{
    // The final world quad should be predictable using
    // only the state info & stage size.
    DVec2 reference_zoom, ctx_stage_size;
    DVec2 ctx_world_size() const {
        return ctx_stage_size / reference_zoom;
    }

    /// ─────────────────────── Saveable Info ───────────────────────

    bool show_axis = true;
    CameraViewController cam_view;

    /// ─────────────────────── Tweenable Info ───────────────────────

    bool     dynamic_iter_lim             = true;
    double   quality                      = 0.5; // Used for UI (ignored during tween, represents iter_lim OR % of iter_lim)

    double   iter_dist_mix                = 0.0;

    bool     cycle_iter_dynamic_limit   = true;
    bool     cycle_iter_normalize_depth = true;
    double   cycle_iter_log1p_weight    = 0.0;
    double   cycle_iter_value           = 0.5f; // If dynamic, iter_lim ratio, else iter_lim

    double   cycle_dist_value           = 0.5f;
    bool     cycle_dist_invert          = false;
    double   cycle_dist_sharpness       = 0.9; // Used for UI (ignored during tween)

    double gradient_shift               = 0.0;
    double hue_shift                    = 0.0;

    double gradient_shift_step          = 0.0078;
    double hue_shift_step               = 0.136;

    int smoothing_type                  = (int)MandelSmoothing::ITER;

    ImGradient gradient;

    // animate
    bool show_color_animation_options   = false;

    // Flattening
    bool flatten = false;
    double flatten_amount = 0.0;

    bool operator ==(const MandelState& rhs) const
    {
        return (
            cam_view == rhs.cam_view &&
            quality == rhs.quality &&
            iter_dist_mix == rhs.iter_dist_mix &&
            dynamic_iter_lim == rhs.dynamic_iter_lim &&
            cycle_iter_normalize_depth == rhs.cycle_iter_normalize_depth &&
            cycle_iter_log1p_weight == rhs.cycle_iter_log1p_weight &&
            cycle_iter_value == rhs.cycle_iter_value &&
            cycle_dist_value == rhs.cycle_dist_value &&
            cycle_dist_invert == rhs.cycle_dist_invert &&
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

    void loadGradientPreset(GradientPreset preset)
    {
        generateGradientFromPreset(gradient, preset);
    }

    std::string serialize(std::string name)
    {
        // Increment version each time the format changes
        uint32_t version = 0;
        uint32_t flags = 0;

        if (dynamic_iter_lim)           flags |= MANDEL_DYNAMIC_ITERS;
        if (show_axis)                  flags |= MANDEL_SHOW_AXIS;
        if (flatten)                    flags |= MANDEL_FLATTEN;
        if (cycle_iter_dynamic_limit)   flags |= MANDEL_DYNAMIC_COLOR_CYCLE;
        if (cycle_iter_normalize_depth) flags |= MANDEL_NORMALIZE_DEPTH;

        flags |= ((uint32_t)smoothing_type << MANDEL_SMOOTH_BITSHIFT);
        flags |= (version << MANDEL_VERSION_BITSHIFT);

        JSON::json info;
        int decimals = 1 + Math::countWholeDigits(cam_view.zoom);

        if (version >= 0)
        {
            info["f"] = flags; // Compression::base64_encode((const unsigned char*)&flags, sizeof(flags));

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

            // Color cycle (ITER)
            info["i"] = JSON::markCleanFloat(cycle_iter_value);
            info["l"] = JSON::markCleanFloat(cycle_iter_log1p_weight);

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

        if constexpr (COMPRESS_CONFIG)
        {
            // Post-process: Remove CLEANFLOAT() markers, compress & wrap lines
            std::string json = JSON::unquoteCleanFloats(info.dump());
            std::string compressed_txt = Compression::brotli_ascii_compress(json);
            std::string ret = "====================================\n";

            // Insert name if provided
            int name_len = (int)name.size();
            if (name_len > 0)
            {
                int wrap_len = 36;
                int eq_pad_len = (wrap_len - (name_len + 2)) / 2;
                std::string eq_padding(eq_pad_len, '=');
                ret = eq_padding + ' ' + name + ' ' + eq_padding + ((name.size() % 2) ? "=" : "") + '\n';
            }

            ret += TextUtil::wrapString(compressed_txt, 36);
            ret += "\n====================================\n";

            return ret;
        }
        else
        {
            std::string json = JSON::unquoteCleanFloats(info.dump());
            return json;
        }
    }

    bool deserialize(std::string_view sv)
    {
        sv = TextUtil::trim_view(sv);
        std::string txt = TextUtil::dedent_max(sv);

        size_t i0 = txt.find_first_of('\n') + 1;
        size_t i1 = txt.find_last_of('=');
        i1 = txt.find_last_of('\n', i1);
        txt = txt.substr(i0, i1 - i0);

        std::string uncompressed;
        if constexpr (COMPRESS_CONFIG)
        {
            uncompressed = Compression::brotli_ascii_decompress(TextUtil::unwrapString(txt));
        }
        else
        {
            uncompressed = txt;
        }

        nlohmann::json info = nlohmann::json::parse(uncompressed, nullptr, false);
        if (info.is_discarded())
            return false; // Failed to parse

        uint32_t flags = info.value("f", 0ul);
        uint32_t version = (flags & MANDEL_VERSION_MASK) >> MANDEL_VERSION_BITSHIFT;

        if (version >= 0)
        {
            smoothing_type = ((flags & MANDEL_SMOOTH_MASK) >> MANDEL_SMOOTH_BITSHIFT);

            dynamic_iter_lim            = flags & MANDEL_DYNAMIC_ITERS;
            show_axis                   = flags & MANDEL_SHOW_AXIS;
            flatten                     = flags & MANDEL_FLATTEN;
            cycle_iter_dynamic_limit    = flags & MANDEL_DYNAMIC_COLOR_CYCLE;
            cycle_iter_normalize_depth  = flags & MANDEL_NORMALIZE_DEPTH;

            // View
            cam_view.x = info.value("x", 0.0);
            cam_view.y = info.value("y", 0.0);
            cam_view.zoom = info.value("z", 1.0);
            cam_view.zoom_xy.x = info.value("a", 1.0);
            cam_view.zoom_xy.y = info.value("b", 1.0);
            double angle_degrees = info.value("r", 0.0);
            cam_view.angle = angle_degrees * Math::PI / 180.0;

            // Quality
            quality = info.value("q", quality);

            // Color cycle
            cycle_iter_value = info.value("i", cycle_iter_value);
            cycle_iter_log1p_weight = info.value("l", cycle_iter_log1p_weight);

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
};

SIM_END;
