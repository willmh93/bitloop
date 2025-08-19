#include "Mandelbrot.h"
#include "compression.h"
#include "constexpr_dispatch.h"
#include "platform.h"

SIM_DECLARE(Mandelbrot)

std::ostream& operator<<(std::ostream& os, const TweenableMandelState&)
{
    return os << "TweenableMandelState";
}

inline int mandelbrotIterLimit(double zoom)
{
    const double l = std::log10(zoom * 400.0);
    int iters = static_cast<int>(-19.35 * l * l + 741.0 * l - 1841.0);
    return (100 + (std::max(0, iters)))*3;
}

inline double qualityFromIterLimit(int iter_lim, double zoom_x)
{
    const int base = mandelbrotIterLimit(zoom_x);   // always >= 100
    if (base == 0) return 0.0;                      // defensive, though impossible here
    return static_cast<double>(iter_lim) / base;    // <= true quality by < 1/base
}

///-------------///
///   Project   ///
///-------------///

void Mandelbrot_Project::projectPrepare(Layout& layout)
{
    Mandelbrot_Scene::Config config1;
    create<Mandelbrot_Scene>(config1)->mountTo(layout);
}

///-----------///
///   Scene   ///
///-----------///

void Mandelbrot_Data::initData()
{
    loadColorTemplate(*this, GRADIENT_CLASSIC);
}

void Mandelbrot_Data::populate()
{
    bool is_tweening = tweening;
    if (is_tweening)
        ImGui::BeginDisabled();
    
    if (ImGui::SceneSection("Examples", 0.0f, 2.0f, true)) 
    {
        if (ImGui::Button("Home"))
        {
            TweenableMandelState dest;
            dest.cam_view.cam_x = -0.5;
            dest.cam_view.cam_y = 0.0;
            dest.cam_view.cam_zoom = 1.0;

            // compute
            dest.dynamic_iter_lim = true;
            dest.quality = 0.5;

            // color cycle
            dest.dynamic_color_cycle_limit = true;
            dest.normalize_depth_range = true;
            dest.cycle_iter_value = 0.5f;
            dest.log1p_weight = 0.0;

            loadColorTemplate(dest, GRADIENT_CLASSIC);

            // animation
            dest.show_color_animation_options = false;
            dest.gradient_shift_step = 0;
            dest.hue_shift_step = 0;

            startTween(dest);
        }

        if (ImGui::Button("Firework"))
        {
            TweenableMandelState dest;
            dest.cam_view.cam_x = -1.766500164390;
            dest.cam_view.cam_y = -0.041755606186;
            dest.cam_view.cam_zoom = 11636977827.66;

            // compute
            dest.dynamic_iter_lim = true;
            dest.quality = 0.25;

            // color cycle
            dest.dynamic_color_cycle_limit = true;
            dest.normalize_depth_range = true;
            dest.cycle_iter_value = 0.65 / 100.0;
            dest.log1p_weight = 1.0;

            loadColorTemplate(dest, GRADIENT_CLASSIC);

            // animation
            dest.show_color_animation_options = true;
            dest.gradient_shift_step = 0.0015;
            dest.hue_shift_step = 0.62;

            startTween(dest);
        }

       
        ImGui::SameLine();
        if (ImGui::Button("Tendrils"))
        {
            TweenableMandelState dest;
            dest.cam_view.cam_x = -0.105807817548;
            dest.cam_view.cam_y = -0.926364255583;
            dest.cam_view.cam_radians = Math::PI / 2.0;
            dest.cam_view.cam_zoom = 56455846258.14;

            // compute
            dest.dynamic_iter_lim = true;
            dest.quality = 0.6;

            // color cycle
            dest.dynamic_color_cycle_limit = true;
            dest.normalize_depth_range = true;
            dest.cycle_iter_value = 0.2 / 100.0;
            dest.log1p_weight = 1.0;

            loadColorTemplate(dest, GRADIENT_SINUSOIDAL_RAINBOW_CYCLE);
            dest.hue_shift = 0.0;
            
            // animation
            dest.show_color_animation_options = true;
            dest.gradient_shift_step = 0.0044;
            dest.hue_shift_step = 0.002;

            startTween(dest);
        }

        ImGui::SameLine();
        if (ImGui::Button("Julia Island"))
        {
            TweenableMandelState dest;
            dest.cam_view.cam_x = -1.76877883;
            dest.cam_view.cam_y = -0.00173892;
            dest.cam_view.cam_radians = 0;
            dest.cam_view.cam_zoom = 3000000.0;

            // compute
            dest.dynamic_iter_lim = true;
            dest.quality = 0.25;

            // color cycle
            dest.dynamic_color_cycle_limit = true;
            dest.normalize_depth_range = true;
            dest.cycle_iter_value = 0.1 / 100.0;
            dest.log1p_weight = 1.0;

            loadColorTemplate(dest, GRADIENT_SINUSOIDAL_RAINBOW_CYCLE);
            dest.hue_shift = 0.0;

            // animation
            dest.show_color_animation_options = true;
            dest.gradient_shift_step = 0.0025;
            dest.hue_shift_step = 0.5;

            startTween(dest);
        }

        if (ImGui::Button("Spiral Spears"))
        {
            TweenableMandelState dest;
            dest.cam_view.cam_x = -1.77169952;
            dest.cam_view.cam_y = 0.00508483;
            dest.cam_view.cam_radians = Math::toRadians(180.0);
            dest.cam_view.cam_zoom = 8650000;

            // compute
            dest.dynamic_iter_lim = true;
            dest.quality = 0.1;

            // color cycle
            dest.dynamic_color_cycle_limit = true;
            dest.normalize_depth_range = true;
            dest.cycle_iter_value = 0.0488 / 100.0;
            dest.log1p_weight = 1.0;

            loadColorTemplate(dest, GRADIENT_WAVES);

            // animation
            dest.show_color_animation_options = true;
            dest.gradient_shift_step = 0.003;
            dest.hue_shift_step = 0.5;

            startTween(dest);
        }

        ImGui::SameLine();
        if (ImGui::Button("Whirlpool"))
        {
            TweenableMandelState dest;
            dest.cam_view.cam_x = -0.75139;
            dest.cam_view.cam_y = 0.03001;
            dest.cam_view.cam_radians = 0.0;
            dest.cam_view.cam_zoom = 9000;

            // compute
            dest.dynamic_iter_lim = true;
            dest.quality = 0.75;

            // color cycle
            dest.dynamic_color_cycle_limit = true;
            dest.normalize_depth_range = true;
            dest.cycle_iter_value = 0.25 / 100.0;
            dest.log1p_weight = 0.94f;

            loadColorTemplate(dest, GRADIENT_SINUSOIDAL_RAINBOW_CYCLE);
            dest.hue_shift = 0.0;

            // animation
            dest.show_color_animation_options = true;
            dest.gradient_shift_step = 0.004;
            dest.hue_shift_step = 0.0;

            startTween(dest);
        }

        ImGui::SameLine();
        if (ImGui::Button("Waves"))
        {
            TweenableMandelState dest;
            dest.cam_view.cam_x = -0.14883;
            dest.cam_view.cam_y = 0.65170;
            dest.cam_view.cam_radians = 0.0;
            dest.cam_view.cam_zoom = 2924.93;

            // compute
            dest.dynamic_iter_lim = true;
            dest.quality = 0.5;

            // color cycle
            dest.dynamic_color_cycle_limit = true;
            dest.normalize_depth_range = true;
            dest.cycle_iter_value = 0.025 / 100.0;
            dest.log1p_weight = 0.99f;

            loadColorTemplate(dest, GRADIENT_WAVES);
            dest.hue_shift = 0.0;

            // animation
            dest.show_color_animation_options = true;
            dest.gradient_shift_step = 0.003;
            dest.hue_shift_step = 0.0;

            startTween(dest);
        }

        ImGui::SameLine();
        if (ImGui::Button("Baby Mandel"))
        {
            TweenableMandelState dest;
            dest.cam_view.cam_x = -0.413270;
            dest.cam_view.cam_y = 0.595879;
            dest.cam_view.cam_radians = Math::toRadians(150.0);
            dest.cam_view.cam_zoom = 12700;

            // compute
            dest.dynamic_iter_lim = true;
            dest.quality = 0.8;

            // color cycle
            dest.dynamic_color_cycle_limit = true;
            dest.normalize_depth_range = true;
            dest.cycle_iter_value = 0.1155 / 100.0;
            dest.log1p_weight = 1.0;

            loadColorTemplate(dest, GRADIENT_SINUSOIDAL_RAINBOW_CYCLE);
            dest.hue_shift = 0.0;

            // animation
            dest.show_color_animation_options = true;
            dest.gradient_shift_step = 0.004;
            dest.hue_shift_step = 0.0;

            startTween(dest);
        }
    }


    /// --------------------------------------------------------------
    if (ImGui::SceneSection("View", 0.0f, 2.0f, true)) {
    /// --------------------------------------------------------------

    ImGui::Checkbox("Show Axis", &show_axis);

    if (!Platform()->is_mobile())
    {
        if (!flatten)
        {
            ImGui::SameLine();
            ImGui::Dummy(ImVec2(10.0f, 0.0f));
            ImGui::SameLine();
            ImGui::Checkbox("Interactive Cardioid", &interactive_cardioid);
        }
    }

    /*int decimals = 1 + Math::countWholeDigits(cam_zoom);
    char format[16];
    snprintf(format, sizeof(format), "%%.%df", decimals);

    static double init_cam_x = cam_x;
    static double init_cam_y = cam_y;
    ImGui::RevertableDragDouble("X", &cam_x, &init_cam_x, 0.01 / cam_zoom, -5.0, 5.0, format);
    ImGui::RevertableDragDouble("Y", &cam_y, &init_cam_y, 0.01 / cam_zoom, -5.0, 5.0, format);

    static double init_degrees = cam_degrees;
    if (ImGui::RevertableSliderDouble("Rotation", &cam_degrees, &init_degrees, 0.0, 360.0, "%.0f\xC2\xB0"))
    {
        cam_rot = cam_degrees * Math::PI / 180.0;
    }

    // 1e16 = double limit before preicions loss
    static double zoom_speed = cam_zoom / 100.0;
    static double init_cam_zoom = 1.0;
    if (ImGui::RevertableDragDouble("Zoom", &cam_zoom, &init_cam_zoom, zoom_speed, 1.0, 1e16, "%.2f"))
        zoom_speed = cam_zoom / 100.0;

    static DVec2 init_cam_zoom_xy = cam_zoom_xy;
    ImGui::RevertableSliderDouble2("Zoom X/Y", cam_zoom_xy.asArray(), init_cam_zoom_xy.asArray(), 0.1, 10.0, "%.2fx");
    */

    cam_view.populateUI({-5.0, -5.0, 5.0, 5.0});

    } // End Header

    /// --------------------------------------------------------------
    if (ImGui::SceneSection("Quality", 5.0f, 2.0f, true)) {
    /// --------------------------------------------------------------

    if (ImGui::Checkbox("Dynamic Iteration Limit", &dynamic_iter_lim))
    {
        if (!dynamic_iter_lim)
        {
            quality = iter_lim;
        }
        else
        {
            quality = qualityFromIterLimit(iter_lim, cam_view.cam_zoom);
        }
    }

    if (dynamic_iter_lim)
    {
        double quality_pct = quality * 100.0;
        static double initial_quality = quality_pct;
        ImGui::PushID("QualitySlider");
        ImGui::RevertableSliderDouble("Quality", &quality_pct, &initial_quality, 1.0, 100.0, "%.0f%%");
        ImGui::PopID();
        quality = quality_pct / 100.0;

        ImGui::Text("max_iter = %d", calculateIterLimit());
    }
    else
        ImGui::DragDouble("Max Iterations", &quality, 1000.0, 1.0, 1000000.0, "%.0f", ImGuiSliderFlags_Logarithmic);

    ImGui::SeparatorText("Smoothing");
    ImGui::SliderDouble("Iter/Dist Mix", &smooth_iter_dist_ratio, 0.0, 1.0, "%.2f");

    } // End Header

    /// --------------------------------------------------------------
    if (ImGui::SceneSection("Colour Cycle", 5.0f, 2.0f, true)) {
    /// --------------------------------------------------------------

    ImGui::SeparatorText("Iteration"); 
    if (ImGui::Checkbox("Dynamic", &dynamic_color_cycle_limit))
    {
        if (dynamic_color_cycle_limit)
            cycle_iter_value /= iter_lim;
        else
            cycle_iter_value *= iter_lim;
    }

    ImGui::SameLine();
    ImGui::Dummy(ImVec2(10.0f, 0.0f));
    ImGui::SameLine();
    ImGui::Checkbox("Normalize depth", &normalize_depth_range);

    double raw_cycle_iters;

    if (dynamic_color_cycle_limit)
    {
        double cycle_pct = cycle_iter_value * 100.0;
        ImGui::SliderDouble("% Iterations", &cycle_pct, 0.001, 100.0, "%.4f%%",
            (/*color_cycle_use_log1p ?*/ ImGuiSliderFlags_Logarithmic /*: 0*/) | 
            ImGuiSliderFlags_AlwaysClamp);
        cycle_iter_value = cycle_pct / 100.0;

        raw_cycle_iters = calculateIterLimit() * cycle_iter_value;
        
    }
    else
    {
        ImGui::SliderDouble("Iterations", &cycle_iter_value, 1.0, (float)iter_lim, "%.3f",
            (/*color_cycle_use_log1p ?*/ ImGuiSliderFlags_Logarithmic /*: 0*/) |
            ImGuiSliderFlags_AlwaysClamp);

        raw_cycle_iters = cycle_iter_value;
    }

    double log1p_weight_v = 1.0 - log1p_weight;
    ImGui::SliderDouble("Log -- Linear", &log1p_weight_v, 0.0, 1.0, "%.5f", ImGuiSliderFlags_Logarithmic);
    log1p_weight = 1.0 - log1p_weight_v;

    ///// Print cycle iter values
    ///{
    ///    if (dynamic_color_cycle_limit)
    ///        ImGui::Text("raw_cycle_iters = %.1f", raw_cycle_iters);
    ///
    ///    double log_cycle_iters = Math::linear_log1p_lerp(raw_cycle_iters, log1p_weight);
    ///    ImGui::Text("log_cycle_iters = %.1f", log_cycle_iters);
    ///}

    ImGui::SeparatorText("Distance");
    ImGui::SliderDouble("Dist", &cycle_dist_value, 0.001, 1.0, "%.5f", ImGuiSliderFlags_Logarithmic);

    ImGui::SeparatorText("Gradient transforms");

    ImGui::Checkbox("Animate", &show_color_animation_options);

    if (ImGui::DragDouble("Gradient shift", &gradient_shift, 0.01, -100.0, 100.0, " %.3f"))
    {
        gradient_shift = Math::wrap(gradient_shift, 0.0, 1.0);
        colors_updated = true;
    }

    if (show_color_animation_options)
    {
        ImGui::Indent();
        ImGui::PushID("gradient_increment");
        ImGui::SliderDouble("Increment", &gradient_shift_step, -0.02, 0.02, "%.4f");
        ImGui::PopID();
        ImGui::Unindent();
    }

    if (ImGui::SliderDouble("Hue shift", &hue_shift, 0.0, 360, "%.3f"))
        colors_updated = true;

    if (show_color_animation_options)
    {
        ImGui::Indent();
        ImGui::PushID("hue_increment");
        ImGui::SliderDouble("Increment", &hue_shift_step, -5.0, 5.0, "%.3f");
        ImGui::PopID();
        ImGui::Unindent();
    }

    //} // End Header

    /// --------------------------------------------------------------
    ImGui::SeparatorText("Gradient Picker");
    /// --------------------------------------------------------------

    if (ImGui::Combo("###ColorTemplate", &active_color_template, ColorGradientNames, GRADIENT_TEMPLATE_COUNT))
    {
        loadColorTemplate(*this, (ColorGradientTemplate)active_color_template);
        colors_updated = true;
    }

    if (ImGui::GradientEditor(&gradient,
        Platform()->ui_scale_factor(), 
        Platform()->ui_scale_factor(2.0f)))
    {
        colors_updated = true;
    }

    } // End Header



    /*if (ImGui::SceneSection("Experimental")) {

        ImGui::Checkbox("Flatten", &flatten);

        if (flatten)
        {
            ImGui::Indent();
            if (ImGui::SliderDouble("Flatness", &flatten_amount, 0.0, 1.0, "%.2f"))
                cardioid_lerp_amount = 1.0 - flatten_amount;

            ImGui::Checkbox("Show period-2 bulb", &show_period2_bulb);
            ImGui::Unindent();
            ImGui::Dummy(ScaleSize(0, 10));
        }

        //static ImRect vr = { 0.0f, 0.8f, 0.8f, 0.0f };
        //ImGui::SeparatorText("Iteration Spline Mapping");
        //if (ImSpline::SplineEditor("iter_spline", &iter_gradient_spline, &vr))
        //{
        //    colors_updated = true;
        //}

        if (!flatten)
        {
            /// --------------------------------------------------------------
            ImGui::SeparatorText("XX, YY Spline Relationship");
            /// --------------------------------------------------------------

            static ImRect vr = { 0.0f, 0.8f, 0.8f, 0.0f };
            ImSpline::SplineEditorPair("X/Y Spline", &x_spline, &y_spline, &vr, 900.0f);
        }

    } // End Header
    */
    
    //if (Platform()->is_desktop_native())
    {
        /// --------------------------------------------------------------
        //ImGui::SeparatorText("Saving & Loading");
        if (ImGui::SceneSection("Saving & Loading")) {
        /// --------------------------------------------------------------

            
            if (ImGui::Button("Copy to Clipboard"))
            {
                ImGui::SetClipboardText(config_buf);
            }
            if (ImGui::InputTextMultiline("###Config", config_buf, 1024, ImVec2(0, 0), ImGuiInputTextFlags_AllowTabInput))
            {
                loadConfigBuffer();
            }
            else
            {
                updateConfigBuffer();
            }
        }
    }

    if (is_tweening)
        ImGui::EndDisabled();

    // ======== Developer ========

    /*static ImRect vr = { 0.0f, 1.0f, 1.0f, 0.0f };

    ImGui::SeparatorText("Position Tween");
    ImSpline::SplineEditor("tween_pos", &tween_pos_spline, &vr);
    ImSpline::SplineEditor("tween_zoom_lift", &tween_zoom_lift_spline, &vr);

    //ImGui::InputTextMultiline("###pos_buf", pos_tween_buf, 1024, ImVec2(0, 0), ImGuiInputTextFlags_AllowTabInput));
    if (ImGui::Button("Copy position spline")) ImGui::SetClipboardText(tween_pos_spline.serialize(SplineSerializationMode::CPP_ARRAY, 3).c_str());

    //ImGui::InputTextMultiline("###pos_buf", zoom_tween_buf, 1024, ImVec2(0, 0), ImGuiInputTextFlags_AllowTabInput));
    if (ImGui::Button("Copy zoom spline")) ImGui::SetClipboardText(tween_zoom_lift_spline.serialize(SplineSerializationMode::CPP_ARRAY, 3).c_str());
    */
}

int Mandelbrot_Data::calculateIterLimit() const
{
    int iters;
    if (dynamic_iter_lim)
        iters = static_cast<int>(mandelbrotIterLimit(cam_view.cam_zoom) * quality);
    else
    {
        iters = static_cast<int>(quality);

        if (tweening)
        {
            // Cap manual iter_lim to avoid accidental high depth calculations on large cardioids
            if (iters > mandelbrotIterLimit(cam_view.cam_zoom))
                iters = mandelbrotIterLimit(cam_view.cam_zoom);
        }
    }
    return iters;
}

#define COMPRESS_CONFIG

std::string TweenableMandelState::serialize()
{
    uint32_t flags = 0;
    uint32_t version = 0;

    if (dynamic_iter_lim)           flags |= MANDEL_DYNAMIC_ITERS;
    if (show_axis)                  flags |= MANDEL_SHOW_AXIS;
    if (flatten)                    flags |= MANDEL_FLATTEN;
    if (dynamic_color_cycle_limit)  flags |= MANDEL_DYNAMIC_COLOR_CYCLE;
    if (normalize_depth_range)      flags |= MANDEL_NORMALIZE_DEPTH;

    flags |= ((uint32_t)smoothing_type << MANDEL_SMOOTH_BITSHIFT);
    flags |= (version << MANDEL_VERSION_BITSHIFT);

    JSON::json info;

    int decimals = 1 + Math::countWholeDigits(cam_view.cam_zoom);

    // Version >= 0
    info["f"] = flags; // Compression::base64_encode((const unsigned char*)&flags, sizeof(flags));

    // View
    info["x"] = JSON::markCleanFloat(cam_view.cam_x, decimals);
    info["y"] = JSON::markCleanFloat(cam_view.cam_y, decimals);
    info["z"] = JSON::markCleanFloat(cam_view.cam_zoom, 2); /// todo: If you increase to 128 bit precision, be careful
    info["a"] = JSON::markCleanFloat(cam_view.cam_zoom_xy.x, 3);
    info["b"] = JSON::markCleanFloat(cam_view.cam_zoom_xy.y, 3);
    info["r"] = JSON::markCleanFloat(Math::toDegrees(cam_view.cam_radians), 0);

    // Quality
    info["q"] = JSON::markCleanFloat(quality, 3);

    // Color cycle
    info["i"] = JSON::markCleanFloat(cycle_iter_value);
    info["l"] = JSON::markCleanFloat(log1p_weight);

    // Shift
    info["g"] = JSON::markCleanFloat(gradient_shift);
    info["h"] = JSON::markCleanFloat(hue_shift);

    // Shift increment
    info["G"] = JSON::markCleanFloat(gradient_shift_step);
    info["H"] = JSON::markCleanFloat(hue_shift_step);

    // Gradient
    info["p"] = gradient.serialize();

    //info["A"] = x_spline.serialize(SplineSerializationMode::COMPRESS_SHORTEST);
    //info["B"] = y_spline.serialize(SplineSerializationMode::COMPRESS_SHORTEST);

    #ifdef COMPRESS_CONFIG
    {
        std::string json = JSON::unquoteCleanFloats(info.dump());
        std::string compressed_txt = Compression::base64_compress(json);

        std::string ret;
        ret +=   "========= Mandelbrot =========\n";
        ret += Helpers::wrapString(compressed_txt, 30);
        ret += "\n==============================\n";

        return ret;
    }
    #else
    {
        std::string json = JSON::unquoteCleanFloats(info.dump());
        return json;
    }
    #endif
}

bool TweenableMandelState::deserialize(std::string txt)
{
    size_t i0 = txt.find_first_of('\n') + 1;
    size_t i1 = txt.find_last_of('=');
    i1 = txt.find_last_of('\n', i1);
    txt = txt.substr(i0, i1 - i0);

    std::string uncompressed;
    #ifdef COMPRESS_CONFIG
    {
        uncompressed = Compression::base64_decompress(Helpers::unwrapString(txt));
    }
    #else
    {
        uncompressed = txt;
    }
    #endif

    nlohmann::json info = nlohmann::json::parse(uncompressed, nullptr, false);
    if (info.is_discarded()) return false;


    uint32_t flags = info.value("f", 0ul);
    uint32_t version = (flags & MANDEL_VERSION_MASK) >> MANDEL_VERSION_BITSHIFT;

    if (version >= 0)
    {
        smoothing_type = ((flags & MANDEL_SMOOTH_MASK) >> MANDEL_SMOOTH_BITSHIFT);

        dynamic_iter_lim          = flags & MANDEL_DYNAMIC_ITERS;
        show_axis                 = flags & MANDEL_SHOW_AXIS;
        flatten                   = flags & MANDEL_FLATTEN;
        dynamic_color_cycle_limit = flags & MANDEL_DYNAMIC_COLOR_CYCLE;
        normalize_depth_range     = flags & MANDEL_NORMALIZE_DEPTH;

        // View
        cam_view.cam_x = info.value("x", 0.0);
        cam_view.cam_y = info.value("y", 0.0);
        cam_view.cam_zoom = info.value("z", 1.0);
        cam_view.cam_zoom_xy.x = info.value("a", 1.0);
        cam_view.cam_zoom_xy.y = info.value("b", 1.0);
        double cam_degrees = info.value("r", 0.0);
        cam_view.cam_radians = cam_degrees * Math::PI / 180.0;

        // Quality
        quality = info.value("q", quality);

        // Color cycle
        cycle_iter_value = info.value("i", cycle_iter_value);
        log1p_weight = info.value("l", log1p_weight);

        // Shift
        gradient_shift = info.value("g", 0.0);
        hue_shift = info.value("h", 0.0);

        // Shift increment
        gradient_shift_step = info.value("G", gradient_shift_step);
        hue_shift_step = info.value("H", hue_shift_step);

        // Gradient
        if (info.contains("p"))
            gradient.deserialize(info.value("p", ""));

        //if (info.contains("A")) x_spline.deserialize(info["A"].get<std::string>());
        //if (info.contains("B")) y_spline.deserialize(info["B"].get<std::string>());

    }

    return true;
}

void Mandelbrot_Data::updateConfigBuffer()
{
    strcpy(config_buf, serialize().c_str());
}

void Mandelbrot_Data::loadConfigBuffer()
{
    deserialize(config_buf);
}

void Mandelbrot_Scene::sceneStart()
{
    // todo: Stop this getting called twice on startup

    cardioid_lerper.create(Math::TWO_PI / 5760.0, 0.005);
}

void Mandelbrot_Scene::sceneMounted(Viewport* ctx)
{
    camera->setOriginViewportAnchor(Anchor::CENTER);
    camera->setDirectCameraPanning(true);
    camera->focusWorldRect(-2, -1.25, 1, 1.25);
    //camera->restrictRelativeZoomRange(0.5, 1e+300);

    //cam_view.cam_x = camera->x();
    //cam_view.cam_y = camera->y();
    //cam_view.cam_zoom = (camera->getRelativeZoom().x / cam_view.cam_zoom_xy.x);
    cam_view.read(camera);

    reference_zoom = camera->getReferenceZoom();
    ctx_stage_size = ctx->size();

    //preview = *this;
}


void Mandelbrot_Data::lerpState(
    TweenableMandelState& dst, 
    TweenableMandelState& a, 
    TweenableMandelState& b, 
    double f, 
    bool complete)
{
    float pos_f = tween_pos_spline((float)f);

    // Calculate true 'b' iter_lim for tweening 
    // (using destination zoom, not current zoom)
    double dst_iter_lim = b.dynamic_iter_lim ?
        (mandelbrotIterLimit(b.cam_view.cam_zoom) * b.quality) :
        b.quality;

    
    double lift_weight = (double)tween_zoom_lift_spline((float)f);
    double lift_height = tween_lift * lift_weight;

    double a_height = toHeight(a.cam_view.cam_zoom);
    double b_height = toHeight(b.cam_view.cam_zoom);

    double dst_height = Math::lerp(a_height, b_height, f) + lift_height;

    dst.cam_view = CameraViewController::lerp(a.cam_view, b.cam_view, pos_f);
    dst.cam_view.cam_zoom = fromHeight(dst_height);
    //
    
    // Quality
    dst.quality        = Math::lerp(a.quality,        dst_iter_lim, pos_f);

    // Color cycle
    float color_cycle_f = tween_color_cycle((float)f);
    dst.cycle_iter_value = Math::lerp(a.cycle_iter_value, b.cycle_iter_value, color_cycle_f);
    dst.log1p_weight = Math::lerp(a.log1p_weight, b.log1p_weight, color_cycle_f);

    // Shift
    dst.gradient_shift = Math::lerp(a.gradient_shift, b.gradient_shift, pos_f);
    dst.hue_shift      = Math::lerp(a.hue_shift,      b.hue_shift, pos_f);
    
    // Shift animation
    dst.gradient_shift_step = Math::lerp(a.gradient_shift_step,  b.gradient_shift_step, pos_f);
    dst.hue_shift_step      = Math::lerp(a.hue_shift_step,       b.hue_shift_step, pos_f);
    
    // Color gradient
    ImGradient::lerp(gradient, a.gradient, b.gradient, (float)f);

    if (complete)
    {
        dst.dynamic_iter_lim = b.dynamic_iter_lim;
        dst.quality = b.quality;

        dst.dynamic_color_cycle_limit = b.dynamic_color_cycle_limit;
        dst.cycle_iter_value = b.cycle_iter_value;

        dst.show_color_animation_options = b.show_color_animation_options;
    }

    // Compute
    //dst->iter_lim = Math::lerp(state_a.iter_lim, state_b.iter_lim, f);
    //dst->cardioid_lerp_amount = Math::lerp(state_a.cardioid_lerp_amount, state_b.cardioid_lerp_amount, f);

    // Spline Data
    //memcpy(dst->x_spline_point, Math::lerp(state_a.x_spline_points, state_b.x_spline_points, f));
}

void Mandelbrot_Scene::viewportProcess(Viewport* ctx, double dt)
{
    /// Process Viewports running this Scene
    ctx_stage_size = ctx->size();

    // ======== Progressing animation ========
    if (show_color_animation_options)
    {
        double ani_speed = dt / 0.016;

        // Animation
        if (fabs(gradient_shift_step) > 1.0e-4)
            gradient_shift = Math::wrap(gradient_shift + gradient_shift_step*ani_speed, 0.0, 1.0);
        if (fabs(hue_shift_step) > 1.0e-4)
            hue_shift = Math::wrap(hue_shift + hue_shift_step*ani_speed, 0.0, 360.0);
    }


    // ======== Color cycle changed? ========
    {
        // Reshade active_bmp but don't recalculate depth field 
        if (Changed(
            cycle_iter_value, 
            dynamic_color_cycle_limit, 
            log1p_weight, 
            normalize_depth_range,
            cycle_dist_value))
        {
            colors_updated = true;
        }
    }

    // ======== Tweening ========
    if (tweening)
    {
        double speed = 0.4 / tween_duration;
        tween_progress += speed * dt;
        //tween_progress += 0.2 * dt;
        
        if (tween_progress < 1.0)
        {
            //lerpState(preview, state_a, state_b, tween_progress, false);
            lerpState(*this, state_a, state_b, tween_progress, false);
        }
        else
        {
            //lerpState(preview, state_a, state_b, 1.0, true);
            lerpState(*this, state_a, state_b, 1.0, true);

            tween_progress = 0.0;
            tweening = false;
        }
    }

    // ======== Updating Shifted Gradient ========
    if (first_frame || Changed(gradient, gradient_shift, hue_shift))
    {
        updateShiftedGradient();
        colors_updated = true;
    }

    // ======== Calculate Depth Limit ========
    iter_lim = calculateIterLimit();

    // ======== Flattening ========
    {
        if (Changed(flatten))
        {
            if (flatten)
                flatten_amount = 0.0;

            camera->focusWorldRect(-2, -1, 2, 1);
            cam_view.cam_zoom = camera->getRelativeZoom().x;
        }

        if (Changed(flatten_amount))
        {
            DRect a(-2.0, -1.5, 0.5, 1.5);
            DRect b(-2.0, -0.2, 1.5, 3.5);
            DRect c(-1.5, -0.2, 4.0, 3.5);
            DRect d(0.0, -1.5, 4.5, 0.5);
            DRect r;

            if (flatten_amount < 0.5)
                r = Math::lerp(a, b, Math::lerpFactor(flatten_amount, 0.0, 0.5));
            else if (flatten_amount < 0.7)
                r = Math::lerp(b, c, Math::lerpFactor(flatten_amount, 0.5, 0.7));
            else
                r = Math::lerp(c, d, Math::lerpFactor(flatten_amount, 0.7, 1.0));

            camera->focusWorldRect(r, false);
            cam_view.cam_zoom = camera->getRelativeZoom().x;
        }
    }

    // ======== Camera View ========
    cam_view.apply(camera);
    /*{
        cam_degrees = cam_rot * 180.0 / Math::PI;

        if (Changed(cam_rot)) camera->setRotation(cam_rot);
        if (Changed(cam_x)) camera->setX(cam_x);
        if (Changed(cam_y)) camera->setY(cam_y);
        if (Changed(cam_zoom_xy, cam_zoom))
        {
            double real_zoom = cam_zoom;
            camera->setRelativeZoomX(real_zoom * cam_zoom_xy.x);
            camera->setRelativeZoomY(real_zoom * cam_zoom_xy.y);
        }
        else
        {
            cam_zoom = (camera->getRelativeZoom().x / cam_zoom_xy.x);
        }
    }*/


    // Ensure size divisble by 9 for perfect result forwarding from: [9x9] to [3x3] to [1x1]
    int iw = (static_cast<int>(ceil(ctx->width() / 9))) * 9;
    int ih = (static_cast<int>(ceil(ctx->height() / 9))) * 9;

    world_quad = camera->toWorldQuad(0, 0, iw, ih);

    // ======== Determine smoothing mode ========
    //if (smooth_iter_dist_ratio <= std::numeric_limits<double>::epsilon())
    //    smoothing_type = (int)MandelSmoothing::ITER;
    //else if (smooth_iter_dist_ratio >= 1.0 - std::numeric_limits<double>::epsilon())
    //    smoothing_type = (int)MandelSmoothing::DIST;
    //else
    //    smoothing_type = (int)MandelSmoothing::MIX;

    if (Changed(smooth_iter_dist_ratio))
        colors_updated = true;

    // Does depth field need recalculating?
    bool mandel_changed = first_frame || Changed(
        world_quad,
        quality,
        ///smoothing_type,
        dynamic_iter_lim,
        flatten,
        show_period2_bulb,
        cardioid_lerp_amount,
        x_spline.hash(),
        y_spline.hash()
    );

    // Presented Mandelbrot *actually* changed? Restart on 9x9 bmp (phase 0)
    if (mandel_changed)
    {
        bmp_9x9.clear(0, 255, 0, 255);
        bmp_3x3.clear(0, 255, 0, 255);
        bmp_1x1.clear(0, 255, 0, 255);

        computing_phase = 0;
        current_row = 0;
        field_9x9.setAllDepth(-1.0);

        compute_t0 = std::chrono::steady_clock::now();
    }

    // Has the compute phase changed? Force update
    if (Changed(computing_phase))
    {
        mandel_changed = true;
    }

    // Set pending bitmap & pending field
    switch (computing_phase)
    {
        case 0:    pending_bmp = &bmp_9x9;    pending_field = &field_9x9;   break;
        case 1:    pending_bmp = &bmp_3x3;    pending_field = &field_3x3;   break;
        case 2:    pending_bmp = &bmp_1x1;    pending_field = &field_1x1;   break;
    }

    bool do_compute = false;

    // Run/continue compute?
    if (Changed(computing_phase) ||
        current_row != 0 || // Still haven't finished computing the previous frame phase
        mandel_changed)
    {
        do_compute = true;
    }

    // ======== Bitmap / Depth-field dimensions and view rect ========
    {
        //BL::print("COMPUTING FROM CAMERA: %.2f, %.2f", (double)camera->x, (double)camera->y);
        pending_bmp->setStageRect(0, 0, iw, ih);

        auto stagePos = pending_bmp->stagePos();
        auto stageSize = pending_bmp->stageSize();

        bmp_9x9.setBitmapSize(iw / 9, ih / 9);
        bmp_3x3.setBitmapSize(iw / 3, ih / 3);
        bmp_1x1.setBitmapSize(iw, ih);

        field_9x9.setDimensions(iw / 9, ih / 9);
        field_3x3.setDimensions(iw / 3, ih / 3);
        field_1x1.setDimensions(iw, ih);
    }

    finished_compute = false;

    // ======== Computing Mandelbrot Depth-Field ========
    if (do_compute)
    {
        //if (!flatten)
        {
            // Standard
            ///bool linear = x_spline.isSimpleLinear() && y_spline.isSimpleLinear();
            MandelSmoothing smoothing = static_cast<MandelSmoothing>(smoothing_type);

            double MAX_ZOOM_FLOAT;
            double MAX_DOUBLE_ZOOM;

            if (smoothing != MandelSmoothing::DIST)
            {
                MAX_ZOOM_FLOAT = 10000;
                MAX_DOUBLE_ZOOM = 2e12;
            }
            else
            {
                MAX_ZOOM_FLOAT = 40;// 100;
                MAX_DOUBLE_ZOOM = 2e10;
            }

            bool b32 = (cam_view.cam_zoom < MAX_ZOOM_FLOAT);// && (smoothing != MandelSmoothing::DIST);
            bool b64 = (cam_view.cam_zoom < MAX_DOUBLE_ZOOM);


            if (b32)      finished_compute = table_invoke<float>(build_table(mandelbrot, [&]), smoothing, flatten);
            else if (b64) finished_compute = table_invoke<double>(build_table(mandelbrot, [&]), smoothing, flatten);
            else          finished_compute = table_invoke<flt128>(build_table(mandelbrot, [&]), smoothing, flatten);


            // Continue progress calculating depth field for "pending"
            ///finished_compute = dispatchBooleans(
            ///    boolsTemplate(mandelbrot, [&]),
            ///    smoothing_type == MandelSmoothing::ITER,
            ///    smoothing_type == MandelSmoothing::DIST,
            ///    !linear
            ///);
        }
        //else
        //{
        //    // Flat lerp
        //    finished_compute = dispatchBooleans(
        //        boolsTemplate(radialMandelbrot, [&]),
        //        smoothing_type != MandelSmoothing::NONE,
        //        show_period2_bulb
        //    );
        //}

        // Has pending_field finished computing? Set as active field and use for future color updates
        if (finished_compute)
        {
            //BL::print("Finished computing phase: %d. Setting as active", pending_field->compute_phase);
            active_bmp = pending_bmp;
            active_field = pending_field;

            // Calculate boundaries for this phase
            /*{
                int max_boundary_depth = 1;
                boundary_paths.resize(max_boundary_depth);
                for (int i = 0; i < max_boundary_depth; i++)
                {
                    std::vector<DVec2>& path = boundary_paths[i];

                    path.clear();
                    generateBoundary(active_field, i, path);
                }
            }*/

           
        }

        // ======== Result forwarding ========
        if (finished_compute)
        {
            switch (computing_phase)
            {
                case 0:
                    field_3x3.setAllDepth(-1.0);
                    bmp_9x9.forEachPixel([this](int x, int y) { field_3x3(x*3+1, y*3+1) = field_9x9(x, y); });
                    break;

                case 1:
                    field_1x1.setAllDepth(-1.0);
                    bmp_3x3.forEachPixel([this](int x, int y) { field_1x1(x*3+1, y*3+1) = field_3x3(x, y); });
                    break;

                case 2:
                    // Finish final 1x1 compute
                    auto elapsed = std::chrono::steady_clock::now() - compute_t0;
                    double dt = std::chrono::duration<double, std::milli>(elapsed).count();
                    double dt_avg = timer_ma.push(dt);

                    BL::print() << "Compute timer: " << BL::to_fixed(4) << dt_avg;
                    break;
            }

            if (computing_phase < 2)
                computing_phase++;
        }
    }



    if (finished_compute || colors_updated)
    {
        if (Changed(log1p_weight, normalize_depth_range, smooth_iter_dist_ratio))
            refreshFieldDepthNormalized();

        // ======== Update Color Cycle iterations ========
        {
            if (dynamic_color_cycle_limit)
            {
                double assumed_iter_lim = mandelbrotIterLimit(cam_view.cam_zoom) * 0.5;

                /// "cycle_iter_value" represents ratio of iter_lim
                double color_cycle_iters = (cycle_iter_value * (assumed_iter_lim - (normalize_depth_range ? active_field->min_depth : 0)));
                log_color_cycle_iters = Math::linear_log1p_lerp(color_cycle_iters, log1p_weight);

                if (!isfinite(log_color_cycle_iters))
                {
                    log_color_cycle_iters = Math::linear_log1p_lerp(color_cycle_iters, log1p_weight);
                    blBreak();
                }
            }
            else
            {
                /// "cycle_iter_value" represents actual iter_lim
                log_color_cycle_iters = Math::linear_log1p_lerp(cycle_iter_value, log1p_weight);

                if (!isfinite(log_color_cycle_iters))
                {
                    log_color_cycle_iters = Math::linear_log1p_lerp(cycle_iter_value, log1p_weight);
                    blBreak();
                }
            }
        }

        shadeBitmap();
        colors_updated = false;
    }


    first_frame = false;
}

//------------------------------------------------------------
// 2. marching squares helper
//------------------------------------------------------------
static const int edgeTable[16] = {
    /*0 0000*/ 0,
    /*1 0001*/ 9 ,/*2 0010*/ 3 ,/*3 0011*/ 10,
    /*4 0100*/ 6 ,/*5 0101*/ 0 ,/*6 0110*/ 5 ,/*7 0111*/ 12,
    /*8 1000*/ 12,/*9 1001*/ 5 ,/*A 1010*/ 0 ,/*B 1011*/ 6 ,
    /*C 1100*/ 10,/*D 1101*/ 3 ,/*E 1110*/ 9 ,/*F 1111*/ 0
};

inline void addSeg(std::vector<DVec2>& out,
    int x0, int y0,
    int code,
    double dx, double dy,
    double xmin, double ymin)
{
    auto interp = [&](double a, double b)->double
    {
        // linear interp between integer grid points (half-pixel accurate)
        return a + 0.5 * (b - a);
    };

    auto emit = [&](int e)
    {
        switch (e)
        {
        case 1:  out.push_back({ xmin + (x0 + interp(0,1)) * dx,
                                 ymin + (y0)*dy }); break; // bottom
        case 2:  out.push_back({ xmin + (x0 + 1) * dx,
                                 ymin + (y0 + interp(0,1)) * dy }); break; // right
        case 4:  out.push_back({ xmin + (x0 + interp(0,1)) * dx,
                                 ymin + (y0 + 1) * dy }); break; // top
        case 8:  out.push_back({ xmin + (x0)*dx,
                                 ymin + (y0 + interp(0,1)) * dy }); break; // left
        }
    };

    // two bits in edgeTable encode at most two segment end-points
    if (code & 1) emit(1);
    if (code & 2) emit(2);
    if (code & 4) emit(4);
    if (code & 8) emit(8);
}

///void Mandelbrot_Scene::generateBoundary(EscapeField* field, double level, std::vector<DVec2>& path)
///{
///    UNUSED(field);
///    UNUSED(level);
///    UNUSED(path);
///
///    /*int w = field->w;
///    int h = field->h;
///    auto step = camera->toWorldOffset({ 1, 1 });
///    double dx = step.x;
///    double dy = step.y;
///    for (int y = 0; y < h - 1; ++y)
///    {
///        for (int x = 0; x < w - 1; ++x)
///        {
///            int idx = 0;
///
///            // build the 4-bit cell code
///            if (field->at(y * w + x) > level) idx |= 1;
///            if (field->at(y * w + x + 1) > level) idx |= 2;
///            if (field->at((y + 1) * w + x + 1) > level) idx |= 4;
///            if (field->at((y + 1) * w + x) > level) idx |= 8;
///
///            int edges = edgeTable[idx];
///            if (edges) addSeg(path, x, y, edges, dx, dy, xmin, ymin);
///        }
///    }*/
///}



void Mandelbrot_Scene::viewportDraw(Viewport* ctx) const
{
    //camera->stageTransform();

    // Draw active phase bitmap
    if (active_bmp)
    {
        //int iw = (static_cast<int>(ceil(ctx->width / 9))) * 9;
        //int ih = (static_cast<int>(ceil(ctx->height / 9))) * 9;
        //pending_bmp->setStageRect(0, 0, iw, ih);

        //BL::print("DRAWING WITH CAMERA: %.2f, %.2f", (double)camera->x, (double)camera->y);


        //ctx->print() << "\nBmp world quad: " << active_bmp->worldQuad();
        //ctx->print() << "\nBmp stage quad: " << ctx->QUAD(active_bmp->worldQuad());
        //ctx->drawImage(*active_bmp);
        ctx->drawImage(*active_bmp, active_bmp->worldQuad());
        //ctx->drawImage(*active_bmp, (DQuad)DRect({ 0,0,ctx->width, ctx->height }));
        
        //camera->scalingLines(false);
        //DQuad q = (DQuad)DRect(0, 0, 0.5, 0.5);
        //ctx->setStrokeStyle(255, 0, 0);
        //ctx->setLineWidth(3);
        //ctx->strokeQuad(q);
    }

    if (show_axis)
        ctx->drawWorldAxis(0.5, 0, 0.5);

    if (interactive_cardioid)
    {
        if (!flatten)
        {
            // Regular interactive Mandelbrot
            //if (!log_x && !log_y)
                Cardioid::plot(this, ctx, true);
        }
        else
        {
            // Lerped/Flattened Mandelbrot
            camera->scalingLines(false);
            ctx->setLineWidth(1);
            ctx->beginPath();
            ctx->drawPath(cardioid_lerper.lerped(cardioid_lerp_amount));
            ctx->stroke();
        }
    }

    if (active_field && active_bmp)
    {
        ctx->print() << "\nDepth Max: " << active_field->max_depth;
        ctx->print() << "\nDist Min: " << active_field->min_dist;
        ctx->print() << "\nDist Max: " << active_field->max_dist;

        int px = (int)mouse->stage_x;
        int py = (int)mouse->stage_y;
        if (px >= 0 && py >= 0 && px < active_bmp->width() && py < active_bmp->height())
        {
            IVec2 pos = active_bmp->pixelPosFromWorld(DVec2(mouse->world_x, mouse->world_y));
            EscapeFieldPixel* p = active_field->get(pos.x, pos.y);
        
            if (p)
            {
                double depth = p->depth;
                double dist = p->dist;

                double lower_depth_bound = normalize_depth_range ? pending_field->min_depth : 0;

                double normalized_depth = Math::lerpFactor(depth, lower_depth_bound, (double)iter_lim);
                double normalized_dist = Math::lerpFactor(dist, pending_field->min_dist, pending_field->max_dist);

                ctx->print() << std::setprecision(18);
                ctx->print() << "\n\nnormalized_depth: " << normalized_depth;
                ctx->print() << "\nnormalized_dist: " << normalized_dist;

                ctx->print() << "\n\nraw_iters: " << depth;
                ctx->print() << "\nraw_dist: " << dist;
            }
        }

    }

    //ctx->print().precision(4);
    //ctx->print() << "Frame delta time: " << this->frame_dt(1) << " ms";
    ///if (active_field)
    ///    ctx->print() << "floor_iterations: " << active_field->min_depth;
    //ctx->print() << "\nceil_iterations: " << max_depth;

    ///ctx->print() << "\nmax_iterations: " << iter_lim;

    /*double zoom_factor = camera->getRelativeZoom().average();
    ctx->print() << "\nzoom: " << cam_zoom;

    ctx->print().precision(1);
    ctx->print() << "\nfps: " << std::fixed << fps(60);
    ctx->print() << "\nthreads: " << Thread::idealThreadCount();


    ctx->print() << "\n\ncomputing_phase: " << computing_phase;
    ctx->print() << "\nvisible_phase: " << visible_phase;

    ctx->print() << "\n\nframe progress: " << (((float)current_row / (float)pending_bmp->height()) * 100.0f) << "%";
    ctx->print() << "\npending iter_lim: " << iter_lim;
    ctx->print() << "\nfinished_compute: " << finished_compute;

    ctx->print().precision(3);
    ctx->print() << "\n\nquad: " << world_quad;*/

    ///ctx->print() << "\n\n" << toString().c_str();

    ///ctx->print() << "\nZoom real: " << cam_zoom;
    ///ctx->print() << "\nHeight: " << toHeight(cam_zoom);
    //ctx->print() << "\nZoom orig: " << fromHeight(toHeight(cam_zoom));

    /*camera->scalingLines(false);
    ctx->setLineWidth(2);
    ctx->setStrokeStyle(255, 255, 255);

    ctx->strokeQuad((DQuad)r1);
    ctx->strokeQuad((DQuad)r2);
    ctx->drawArrow(r1.cen, DRay(r1.cen, r1.angle).project(0.2), Color(255, 255, 255));
    ctx->drawArrow(r2.cen, DRay(r2.cen, r2.angle).project(0.2), Color(255, 255, 255));
    
    ///double cx = (r1.cx + r2.cx) / 2.0;
    ///double cy = (r1.cy + r2.cy) / 2.0;
    ///double angle = Math::avgAngle(r1.angle, r2.angle);
    ///DVec2 s = enclosingSize2(r1, r2, angle, r1.aspectRatio());
    ///DAngledRect encompassing(cx, cy, s.x, s.y, angle);
    
    for (double t = 0.1; t <= 0.9; t += 0.1)
    {
        DAngledRect mid = Math::lerp(r1, r2, t);
        ctx->strokeQuad((DQuad)mid);
        ctx->drawArrow(mid.cen, DRay(mid.cen, mid.angle).project(0.2), Color(255, 255, 255, 100));
    }

    
    ctx->print() << "\nFocused World Size: " << camera->getViewportFocusedWorldSize();
    ctx->print() << "\nEncompassing height: " << encompassing_height;*/

    /*camera->scalingLines(false);
    ctx->setLineWidth(4);
    ctx->setStrokeStyle(255, 255, 255);

    DAngledRect preview_rect = getAngledRect(preview);
    DAngledRect preview_rect_vis1 = preview_rect;
    DAngledRect preview_rect_vis2 = preview_rect;
    DAngledRect preview_rect_vis3 = preview_rect;
    DAngledRect preview_rect_vis4 = preview_rect;
    DAngledRect preview_rect_vis5 = preview_rect;
    DAngledRect preview_rect_vis6 = preview_rect;
    if (preview_rect_vis1.w < 0.005)      preview_rect_vis1.w = 0.005;
    if (preview_rect_vis1.h < 0.005)      preview_rect_vis1.h = 0.005;
    if (preview_rect_vis2.w < 0.0005)     preview_rect_vis2.w = 0.0005;
    if (preview_rect_vis2.h < 0.0005)     preview_rect_vis2.h = 0.0005;
    if (preview_rect_vis3.w < 0.00005)    preview_rect_vis3.w = 0.00005;
    if (preview_rect_vis3.h < 0.00005)    preview_rect_vis3.h = 0.00005;
    if (preview_rect_vis4.w < 0.000005)   preview_rect_vis4.w = 0.000005;
    if (preview_rect_vis4.h < 0.000005)   preview_rect_vis4.h = 0.000005;
    if (preview_rect_vis5.w < 0.0000005)  preview_rect_vis5.w = 0.0000005;
    if (preview_rect_vis5.h < 0.0000005)  preview_rect_vis5.h = 0.0000005;
    if (preview_rect_vis6.w < 0.00000005) preview_rect_vis6.w = 0.00000005;
    if (preview_rect_vis6.h < 0.00000005) preview_rect_vis6.h = 0.00000005;
    ctx->strokeQuad((DQuad)preview_rect_vis1);
    ctx->strokeQuad((DQuad)preview_rect_vis2);
    ctx->strokeQuad((DQuad)preview_rect_vis3);
    ctx->strokeQuad((DQuad)preview_rect_vis4);
    ctx->strokeQuad((DQuad)preview_rect_vis5);
    ctx->strokeQuad((DQuad)preview_rect_vis6);

    ctx->setStrokeStyle(255, 0, 255);
    ctx->strokeQuad((DQuad)preview_rect);*/

    ///DAngledRect encompassing;
    ///encompassing.fitTo(r1, r2, r1.aspectRatio());
    ///ctx->strokeQuad((DQuad)encompassing);

    //DVec2 default_focus_size = camera->getViewportFocusedWorldSize();
    //DVec2 encompassing_size = encompassing.size;

    //double encompassing_zoom = (ctx_world_size() / encompassing.size).average();
    //double encompassing_height = toHeight(encompassing_zoom);



    //ctx->strokeRect(0, 0, 10, 10);



    // Gradient test
    /*camera->stageTransform();
    double x = 8;
    for (double p=0; p<1.0; p+=0.01)
    {
        float c[3];
        //gradient_shifted.getColorAt(p, c);
        gradient_shifted.getColorAt(p, c);
        //ctx->setFillStyle(int(c[0] * 255.0), int(c[1] * 255.0), int(c[2] * 255.0), 255);
        ctx->setFillStyle(c);
        ctx->fillRect(x, 8, 1, 24);
        x++;
    }*/

    ctx->setFillStyle(0, 0, 0);
    ctx->setFontSize(16);
    ctx->setTextAlign(TextAlign::ALIGN_LEFT);
    ctx->setTextBaseline(TextBaseline::BASELINE_BOTTOM);
    ctx->fillText("Created by: Will Hemsworth", ScaleSize(4), ctx->height() - ScaleSize(4));
}

void Mandelbrot_Scene::onEvent(Event e)
{
    if (e.ctx_owner())
    {
        // Handle camera navigation for viewport that captured the event
        e.ctx_owner()->camera.handleWorldNavigation(e, true);
        cam_view.read(&e.ctx_owner()->camera);
    }
}

SIM_END(Mandelbrot)
