#include "Mandelbrot.h"
#include "encoding.h"


//SIM_SORT_KEY(0)
SIM_DECLARE(Mandelbrot, "Fractal", "Mandelbrot", "Mandelbrot Viewer")


///-------------///
///   Project   ///
///-------------///

void Mandelbrot_Project::projectPrepare(Layout& layout)
{
    Mandelbrot_Scene::Config config1;
    //auto config2 = make_shared<Test_Scene::Config>(Test_Scene::Config());

    create<Mandelbrot_Scene>(config1)->mountTo(layout);
}

///-----------///
///   Scene   ///
///-----------///


void Mandelbrot_Data::populate()
{
    /*static GLuint reset_icon = 0;
    static bool reset_loaded = false;
    static int reset_w, reset_h;
    static std::vector<unsigned char> reset_pixels;

    if (!reset_loaded)
    {
        if (LoadPixelsRGBA("/data/icon/reset_icon.png", &reset_w, &reset_h, reset_pixels))
        {
            reset_icon = CreateGLTextureRGBA8(reset_pixels.data(), reset_w, reset_h);
            reset_loaded = true;
        }
    }*/

    


    /// --------------------------------------------------------------
    //ImGui::SeparatorText("View");
    if (ImGui::SceneSection("View")) {
    /// --------------------------------------------------------------

    static ImGuiID active_id = 0;

    int decimals = 1 + Math::countWholeDigits(cam_zoom);
    char format[16];
    snprintf(format, sizeof(format), "%%.%df", decimals);

    //if (ImGui::DragDouble("X", &cam_x, 0.01 / cam_zoom, -5.0, 5.0, format))
    //    post([&]() { dst.cam_zoom = cam_zoom; });
    
    ImGui::Checkbox("Show Axis", &show_axis);

    static double init_cam_x = cam_x;
    static double init_cam_y = cam_y;
    ImGui::RevertableDragDouble("X", &cam_x, &init_cam_x, 0.01 / cam_zoom, -5.0, 5.0, format);
    ImGui::RevertableDragDouble("Y", &cam_y, &init_cam_y, 0.01 / cam_zoom, -5.0, 5.0, format);

    if (!Platform()->is_mobile())
    {
        static double init_degrees = cam_degrees;
        if (ImGui::RevertableSliderDouble("Rotation", &cam_degrees, &init_degrees, 0.0, 360.0, "%.0f"))
        {
            cam_rot = cam_degrees * Math::PI / 180.0;
        }

        // 1e16 = double limit before preicions loss
        static double zoom_speed = cam_zoom / 100.0;
        static double init_cam_zoom = 1.0;
        if (ImGui::RevertableDragDouble("Zoom Mult", &cam_zoom, &init_cam_zoom, zoom_speed, 1.0, 1e16, "%.2f"))
        {
            zoom_speed = cam_zoom / 100.0;
        }
    }

    //double *zoom_vars[2] = { ref(&cam_zoom_xy[0]), Var(&cam_zoom_xy[1]) };
    static DVec2 init_cam_zoom_xy = cam_zoom_xy;
    ImGui::RevertableSliderDouble2("Zoom X/Y", cam_zoom_xy.asArray(), init_cam_zoom_xy.asArray(), 0.1, 10.0, "%.2f");

    } // End Header

    /// --------------------------------------------------------------
    //ImGui::SeparatorText("Compute");
    if (ImGui::SceneSection("Compute")) {
    /// --------------------------------------------------------------

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
    
    if (!flatten)
    {
        ImGui::Checkbox("Interactive Cardioid", &interactive_cardioid);
    }

    if (ImGui::Checkbox("Dynamic Iteration Limit", &dynamic_iter_lim))
    {
        if (!dynamic_iter_lim)
        {
            quality = iter_lim;
        }
        else
        {
            quality = 0.5;
        }
    }

    if (dynamic_iter_lim)
    {
        ImGui::SliderDouble("Quality", &quality, 0.1, 1.0, "%.2f");
    }
    else
        ImGui::DragDouble("Max Iterations", &quality, 1000.0, 1.0, 1000000.0, "%.0f", ImGuiSliderFlags_Logarithmic);

    
    
    // /// --------------------------------------------------------------
    // ImGui::Dummy(ScaleSize(0, 10));
    // ImGui::SeparatorText("Visual Options");
    // /// --------------------------------------------------------------

    
    //ImGui::Checkbox("Smoothing", &smoothing);

    } // End Header

    /// --------------------------------------------------------------
    //ImGui::SeparatorText("Colour Cycling");
    if (ImGui::SceneSection("Colour Cycling")) {
    /// --------------------------------------------------------------

    ImGui::Checkbox("Dynamic Color Cycle", &dynamic_color_cycle_limit);


    if (dynamic_color_cycle_limit)
        ImGui::SliderDouble("Cycle Limit Ratio", &color_cycle_value, 0.0001, 1.0, "%.6f", ImGuiSliderFlags_Logarithmic);
    else
        ImGui::SliderDouble("Cycle Iterations", &color_cycle_iters, 0.001, iter_lim, "%.4f", ImGuiSliderFlags_Logarithmic);

    if (ImGui::DragDouble("Gradient Shift", &gradient_shift, 0.01, -100.0, 100.0, " %.3f"))
    {
        gradient_shift = Math::wrap(gradient_shift, 0.0, 1.0);
        colors_updated = true;
    }

    if (ImGui::SliderDouble("Hue Shift", &hue_shift, 0.0, 360, "%.3f"))
    {
        colors_updated = true;
    }

    ImGui::Checkbox("Animate", &show_color_animation_options);
    if (show_color_animation_options)
    {
        ImGui::SliderDouble("Gradient Shift Speed", &gradient_shift_step, -0.02, 0.02, "%.4f");
        ImGui::SliderDouble("Hue Shift Speed", &hue_shift_step, -5.0, 5.0, "%.3f");
    }

    } // End Header

    /// --------------------------------------------------------------
    //ImGui::SeparatorText("Colour Gradient");
    if (ImGui::SceneSection("Colour Gradient")) {
    /// --------------------------------------------------------------

    ///static float hue_threshold = 0.3f;
    ///static float sat_threshold = 0.3f;
    ///static float val_threshold = 0.3f;
    ///bool hue_changed = ImGui::SliderFloat("Hue Threshold", &hue_threshold, 0.1f, 1.0f, "%.2f");
    ///bool sat_changed = ImGui::SliderFloat("Sat Threshold", &sat_threshold, 0.1f, 1.0f, "%.2f");
    ///bool val_changed = ImGui::SliderFloat("Val Threshold", &val_threshold, 0.1f, 1.0f, "%.2f");
    ///if (hue_changed || sat_changed || val_changed)
    ///{
    ///    loadColorTemplate((ColorGradientTemplate)active_color_template,
    ///        hue_threshold,
    ///        sat_threshold,
    ///        val_threshold
    ///    );
    ///    colors_updated = true;
    ///}
    /// 
    if (ImGui::Combo("###ColorTemplate", &active_color_template, ColorGradientNames, GRADIENT_TEMPLATE_COUNT))
    {
        loadColorTemplate((ColorGradientTemplate)active_color_template);
        colors_updated = true;
    }

    //static bool show_gradient_editor = false;
    //if (ImGui::GradientButton(&gradient, Platform()->ui_scale_factor()))
    //{
    //    show_gradient_editor = !show_gradient_editor;
    //}
   

    //if (show_gradient_editor)
    {
        if (ImGui::GradientEditor(
            &gradient,
            Platform()->ui_scale_factor(), 
            Platform()->ui_scale_factor(2.0f)))
        {
            //active_color_template = GRADIENT_CUSTOM;
            colors_updated = true;
            gradient_shifted = gradient;
        }
    }

    } // End Header



    if (ImGui::SceneSection("Experimental")) {

        static ImRect vr = { 0.0f, 0.8f, 0.8f, 0.0f };

        /*ImGui::SeparatorText("Iteration Spline Mapping");
        if (ImSpline::SplineEditor("iter_spline", &iter_gradient_spline, &vr))
        {
            colors_updated = true;
        }*/

        if (!flatten)
        {
            /// --------------------------------------------------------------
            ImGui::SeparatorText("XX, YY Spline Relationship");
            /// --------------------------------------------------------------

            static ImRect vr = { 0.0f, 0.8f, 0.8f, 0.0f };
            ImSpline::SplineEditorPair("X/Y Spline", &x_spline, &y_spline, &vr, 900.0f);
        }

    } // End Header

    
    if (Platform()->is_desktop_native())
    {
        /// --------------------------------------------------------------
        //ImGui::SeparatorText("Saving & Loading");
        if (ImGui::SceneSection("Saving & Loading")) {
        /// --------------------------------------------------------------

        /*if (ImGui::InputTextMultiline("###Config", &sync(config_buf)[0], 1024, ImVec2(0, 0), ImGuiInputTextFlags_AllowTabInput))
        {
            loadConfigBuffer();
        }
        if (ImGui::Button("Copy to Clipboard"))
        {
            ImGui::SetClipboardText(config_buf);
        }*/

        }
    }

    if (colors_updated)
    {
        updateShiftedGradient();
    }

    //if (ImGui::IsAnyItemActive())            // true while the mouse is still held,
    //{                                        // a text box has focus, etc.
    //}

    /*if (ImGui::GetActiveID() != 0)
        active_id = ImGui::GetActiveID();   // 0 means “none”; otherwise that

    static char id_buf[100] = "";
    sprintf(id_buf, "%u (%s)", active_id, ImGui::IsAnyItemActive()?"active":"last active");
    ImGui::InputText("ID", id_buf, 100);*/
    
}



std::string Mandelbrot_Scene::serializeConfig()
{
    uint32_t flags = 0;
    uint32_t version = 0;

    if (dynamic_iter_lim)           flags |= MANDEL_DYNAMIC_ITERS;
    if (show_axis)                  flags |= MANDEL_SHOW_AXIS;
    if (flatten)                    flags |= MANDEL_FLATTEN;
    if (dynamic_color_cycle_limit)  flags |= MANDEL_DYNAMIC_COLOR_CYCLE;

    flags |= ((uint32_t)smoothing_type << MANDEL_SMOOTH_BITSHIFT);
    flags |= (version << MANDEL_VERSION_BITSHIFT);

    JSON::json info;

    // Version >= 0
    info["f"] = Encoding::base64_encode((const unsigned char*)&flags, sizeof(flags));
    info["x"] = cam_x;
    info["y"] = cam_y;
    info["z"] = cam_zoom;
    info["a"] = cam_zoom_xy.x;
    info["b"] = cam_zoom_xy.y;
    info["r"] = JSON::markCleanFloat((float)cam_degrees);
    info["q"] = quality;
    info["A"] = x_spline.serialize(false);
    info["B"] = y_spline.serialize(false);
    info["h"] = hue_shift;

    if (true)
    {
        std::string json = JSON::unquoteCleanFloats(info.dump());
        std::string compressed_txt = Encoding::base64_compress(json);

        std::string ret;
        ret += "========= Mandelbrot =========\n";
        ret += Helpers::wrapString(compressed_txt, 30);
        ret += "\n==============================\n";

        return ret;
    }
    else
    {
        std::string json = JSON::unquoteCleanFloats(info.dump());
        return json;
    }
}

bool Mandelbrot_Scene::deserializeConfig(std::string txt)
{
    size_t i0 = txt.find_first_of('\n') + 1;
    size_t i1 = txt.find_last_of('=');
    i1 = txt.find_last_of('\n', i1);
    txt = txt.substr(i0, i1 - i0);

    std::string uncompressed;
    if (true)
    {
        uncompressed = Encoding::base64_decompress(Helpers::unwrapString(txt));
    }
    else
    {
        uncompressed = txt;
    }

    nlohmann::json info = nlohmann::json::parse(uncompressed, nullptr, false);
    if (info.is_discarded()) return false;

    auto flag_bytes = Encoding::base64_decode(info.value("f", "AAAA"));
    uint32_t flags = *reinterpret_cast<uint32_t*>(flag_bytes.data());
    uint32_t version = (flags & MANDEL_VERSION_MASK) >> MANDEL_VERSION_BITSHIFT;

    if (version >= 0)
    {
        smoothing_type = (MandelSmoothing)((flags & MANDEL_SMOOTH_MASK) >> MANDEL_SMOOTH_BITSHIFT);

        dynamic_iter_lim = flags & MANDEL_DYNAMIC_ITERS;
        show_axis = flags & MANDEL_SHOW_AXIS;
        flatten = flags & MANDEL_FLATTEN;
        dynamic_color_cycle_limit = flags & MANDEL_DYNAMIC_COLOR_CYCLE;

        cam_x = info.value("x", 0.0);
        cam_y = info.value("y", 0.0);
        cam_zoom = info.value("z", 1.0);
        cam_zoom_xy.x = info.value("a", 1.0);
        cam_zoom_xy.y = info.value("b", 1.0);
        cam_degrees = info.value("r", 0.0);
        cam_rot = cam_degrees * Math::PI / 180.0;
        quality = info.value("q", quality);
        hue_shift = info.value("h", 0.0);

        if (info.contains("A")) x_spline.deserialize(info["A"].get<std::string>());
        if (info.contains("B")) y_spline.deserialize(info["B"].get<std::string>());

    }

    return true;
}

void Mandelbrot_Scene::updateConfigBuffer()
{
    strcpy(config_buf, serializeConfig().c_str());
}

void Mandelbrot_Scene::loadConfigBuffer()
{
    deserializeConfig(config_buf);
}

void Mandelbrot_Scene::sceneStart()
{
    // todo: Stop this getting called twice on startup

    

    loadColorTemplate(GRADIENT_CLASSIC);

    cardioid_lerper.create((2.0 * M_PI) / 5760.0, 0.005);
}

void Mandelbrot_Scene::sceneMounted(Viewport*)
{
    camera->setOriginViewportAnchor(Anchor::CENTER);
    camera->setPanningUsesOffset(false);
    camera->focusWorldRect(-2, -1.25, 1, 1.25);
    //cam_zoom_ref = camera->getRelativeZoomFactor().average();
    //cam_zoom = camera->zoom_x;
}

//void Mandelbrot_Scene::step_color(double step, uint8_t& r, uint8_t& g, uint8_t& b)
//{
//    double ratio;
//    if (smoothing)
//        ratio = fmod(step, 1.0);
//    else
//        ratio = fmod(step / 10.0, 1.0);
//
//    r = (uint8_t)(9 * (1 - ratio) * ratio * ratio * ratio * 255);
//    g = (uint8_t)(15 * (1 - ratio) * (1 - ratio) * ratio * ratio * 255);
//    b = (uint8_t)(8.5 * (1 - ratio) * (1 - ratio) * (1 - ratio) * ratio * 255);
//}

void Mandelbrot_Scene::iter_ratio_color(double ratio, uint8_t& r, uint8_t& g, uint8_t& b)
{
    //if (thresholding)
    //{
    //    r = g = b = (ratio <= 0.9999) ? 0 : 127;
    //    return;
    //}
    r = (uint8_t)(9 * (1 - ratio) * ratio * ratio * ratio * 255);
    g = (uint8_t)(15 * (1 - ratio) * (1 - ratio) * ratio * ratio * 255);
    b = (uint8_t)(8.5 * (1 - ratio) * (1 - ratio) * (1 - ratio) * ratio * 255);
}

inline int mandelbrotIterLimit(double zoom)
{
    const double l = std::log10(zoom);
    int it = static_cast<int>(-19.35 * l * l + 741.0 * l - 1841.0);
    return 100 + (std::max(0, it) * 2);
}

///void Mandelbrot_Scene::lerpState(TweenableMandelState* dst, double f)
///{
///    // Basic View
///    dst->cam_x       = Math::lerp(state_a.cam_x, state_b.cam_x, f);
///    dst->cam_y       = Math::lerp(state_a.cam_y, state_b.cam_y, f);
///    dst->cam_rot     = Math::lerp(state_a.cam_rot, state_b.cam_rot, f);
///    dst->cam_zoom    = Math::lerp(state_a.cam_zoom, state_b.cam_zoom, f);
///    dst->cam_zoom_xy = Math::lerp(state_a.cam_zoom_xy, state_b.cam_zoom_xy, f);
///
///    // Compute
///    dst->iter_lim = Math::lerp(state_a.iter_lim, state_b.iter_lim, f);
///    dst->cardioid_lerp_amount = Math::lerp(state_a.cardioid_lerp_amount, state_b.cardioid_lerp_amount, f);
///
///    // Spline Data
///    //memcpy(dst->x_spline_point, Math::lerp(state_a.x_spline_points, state_b.x_spline_points, f));
///}

void Mandelbrot_Scene::viewportProcess(Viewport* ctx)
{
    /// Process Viewports running this Scene

    // Apply active tween (if present) to current state
    ///if (tweening)
    ///{
    ///    lerpState(this, tween);
    ///}

    // Update View
    if (anyChanged(flatten))
    {
        if (flatten)
            flatten_amount = 0.0;

        camera->focusWorldRect(-2, -1, 2, 1);

        cam_zoom = camera->getRelativeZoomFactor().x;
    }

    if (anyChanged(flatten_amount))
    {
        
        /// Flatness 0:    x[-2.0 to 0.5], y[-1.5 to 1.5]
        /// Flatness 0.5:  x[-2.0 to 1.5], y[-0.5 to 3.0]
        /// Flatness 0.7:  x[-1.5 to 4.0], y[-0.5 to 3.5]
        /// Flatness 1:    x[0.0 to 4.5],  y[-0.5 to 1.5]

        DRect a(-2.0, -1.5, 0.5, 1.5);
        DRect b(-2.0, -0.2, 1.5, 3.5);
        DRect c(-1.5, -0.2, 4.0, 3.5);
        DRect d( 0.0, -1.5, 4.5, 0.5);

        DRect r;

        if (flatten_amount < 0.5)
            r = Math::lerp(a, b, Math::lerpFactor(flatten_amount, 0.0, 0.5));
        else if (flatten_amount < 0.7)
            r = Math::lerp(b, c, Math::lerpFactor(flatten_amount, 0.5, 0.7));
        else
            r = Math::lerp(c, d, Math::lerpFactor(flatten_amount, 0.7, 1.0));

        camera->focusWorldRect(r, false);
        //camera->focusWorldRect(-2, -1, 2, 1);

        cam_zoom = camera->getRelativeZoomFactor().x;
    }

    //cam_rot += 0.001;
    cam_degrees = cam_rot * 180.0 / Math::PI;

    if (variableChanged(cam_rot))
        camera->rotation = cam_rot;
    //else
    //    cam_rot = camera->rotation;

    if (anyChanged(cam_zoom_xy, cam_zoom))
    {
        double real_zoom = cam_zoom;
        camera->setRelativeZoomFactorX(real_zoom * cam_zoom_xy.x);
        camera->setRelativeZoomFactorY(real_zoom * cam_zoom_xy.y);
    }
    else
    {
        cam_zoom = (camera->getRelativeZoomFactor().x / cam_zoom_xy.x);

        /*post_data([](auto& dst, const auto& src) {
            dst.cam_zoom = src.cam_zoom;
        });*/
    }

    if (variableChanged(cam_x))
    {
        //DebugPrint("Detected change: cam_x: %.3f", cam_x);

        //DebugPrint("viewportProcess: cam_x WAS %.12f", cam_x);
        camera->x = cam_x; // imgui edited OR panned
        //DebugPrint("viewportProcess: cam_x SET %.12f", cam_x);
        
    }
    //else 
    //    cam_x = camera->x; // use camera view
    
    if (variableChanged(cam_y))
        camera->y = cam_y;
    //else 
    //    cam_y = camera->y;

    // Ensure the bitmap size divisble by 9 for perfect
    // result forwarding from:  [9x9] to [3x3] to [1x1]
    int iw = (static_cast<int>(ceil(ctx->width / 9))) * 9;
    int ih = (static_cast<int>(ceil(ctx->height / 9))) * 9;

    if (dynamic_iter_lim)
        iter_lim = static_cast<int>(mandelbrotIterLimit(camera->zoom_x) * quality);
    else
        iter_lim = static_cast<int>(quality);

    if (dynamic_color_cycle_limit)
        color_cycle_iters = color_cycle_value * iter_lim;
    //else
    //    color_cycle_iters = color_cycle_value;


    world_quad = camera->toWorldQuad(0, 0, iw, ih);

    // Has the view changed?
    bool mandel_changed = anyChanged(
        world_quad,
        quality,
        smoothing_type,
        dynamic_iter_lim,
        flatten,
        show_period2_bulb,
        cardioid_lerp_amount,
        x_spline.hash(),
        y_spline.hash()
    );

    bool other_option_changed = anyChanged(
        show_axis,
        interactive_cardioid
    );

    // Color scheme changed? Don't recalculate depth field, but reshade active_bmp colors
    if (variableChanged(color_cycle_iters))
    {
        colors_updated = true;
        other_option_changed = true;
    }

    // Presented Mandelbrot *actually* changed? Restart on 9x9 bmp (phase 0)
    if (mandel_changed)
    {
        computing_phase = 0;
        current_row = 0;
        field_9x9.setAllDepth(-1.0);

        compute_t0 = std::chrono::steady_clock::now();
    }

    // Has config changed in any way? Update 
    bool config_changed = mandel_changed || other_option_changed;
    if (config_changed)
        updateConfigBuffer();

    // Has the compute phase changed? Force update
    if (variableChanged(computing_phase))
    {
        mandel_changed = true;
    }


    // Set active_bmp
    switch (computing_phase)
    {
        case 0: pending_bmp = &bmp_9x9; pending_field = &field_9x9; break;
        case 1: pending_bmp = &bmp_3x3; pending_field = &field_3x3; break;
        case 2: pending_bmp = &bmp_1x1; pending_field = &field_1x1; break;
    }

    bool do_compute = false;

    // Reshade active_bmp
    if (first_frame ||
        current_row != 0 || // Still haven't finished computing the previous frame phase
        mandel_changed ||
        variableChanged(computing_phase))
    {
        do_compute = true;
        first_frame = false;
    }

    pending_bmp->setStageRect(0, 0, iw, ih);
    
    bmp_9x9.setBitmapSize(iw / 9, ih / 9);
    bmp_3x3.setBitmapSize(iw / 3, ih / 3);
    bmp_1x1.setBitmapSize(iw, ih);

    field_9x9.setDimensions(iw / 9, ih / 9);
    field_3x3.setDimensions(iw / 3, ih / 3);
    field_1x1.setDimensions(iw, ih);

    finished_compute = false;

    //if (pending_bmp->needsReshading(camera))
    if (do_compute)
    {
        if (!flatten)
        {
            // Standard
            bool linear = x_spline.isSimpleLinear() && y_spline.isSimpleLinear();

            // Continue progress calculating depth field for "pending"
            finished_compute = dispatchBooleans(
                boolsTemplate(regularMandelbrot, [&]),
                smoothing_type == MandelSmoothing::SMOOTH_CONTINUOUS,
                smoothing_type == MandelSmoothing::SMOOTH_DISTANCE,
                !linear
            );
        }
        else
        {
            // Flat lerp
            finished_compute = dispatchBooleans(
                boolsTemplate(radialMandelbrot, [&]),
                smoothing_type != MandelSmoothing::SMOOTH_NONE,
                show_period2_bulb
            );
        }

        // Has pending_field finished computing? Set as active field and use for future color updates
        if (finished_compute)
        {
            active_bmp = pending_bmp;
            active_field = pending_field;

            //visible_phase = computing_phase;


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

        // Forward current phase results to next phase
        if (finished_compute)
        {
            switch (computing_phase)
            {
                case 0:
                {
                    field_3x3.setAllDepth(-1.0);
                    bmp_9x9.forEachPixel([this](int x, int y) {
                        field_3x3.setPixelDepth(x*3 + 1, y*3 + 1, field_9x9.getPixelDepth(x, y));
                    });
                }
                break;

                case 1:
                {
                    field_1x1.setAllDepth(-1.0);
                    bmp_3x3.forEachPixel([this](int x, int y) {
                        field_1x1.setPixelDepth(x*3 + 1, y*3 + 1, field_3x3.getPixelDepth(x, y));
                    });
                }
                break;

                case 2:
                {
                    // Finish final 1x1 compute
                    auto elapsed = std::chrono::steady_clock::now() - compute_t0;
                    double dt = std::chrono::duration<double, std::milli>(elapsed).count();
                    double dt_avg = timer_ma.push(dt);

                    DebugPrint("Compute timer: %.4f", dt_avg);
                }
                break;
            }

            if (computing_phase < 2)
            {
                computing_phase++;
            }
        }
    }

    if (show_color_animation_options)
    {
        // Animation
        bool updated_gradient = false;
        if (fabs(gradient_shift_step) > 1.0e-4) {
            gradient_shift = Math::wrap(gradient_shift + gradient_shift_step, 0.0, 1.0);
            updated_gradient = true;
        }
        if (fabs(hue_shift_step) > 1.0e-4) {
            hue_shift = Math::wrap(hue_shift + hue_shift_step, 0.0, 360.0);
            updated_gradient = true;
        }

        if (updated_gradient)
        {
            updateShiftedGradient();
            colors_updated = true;
        }
    }

    if (finished_compute || colors_updated)
    {
        //if (gradient.getMarks().front().color[0] == 0)
        //    DebugPrint("Mandelbrot shaded BLACK");
        //else
        //    DebugPrint("Mandelbrot shaded OTHER");
        //T0(shadeBitmap);
        shadeBitmap(); // bug info: UI sets shadow gradient to black DURING this
        //T1(shadeBitmap);

        colors_updated = false; // then LIVE marks that as "shaded" here, but used other color

        

        /// because gradient used expired data (e.g. mark color), the colors_updated should remain true
        // 
        // because GUI was populated during worker process, the shadow data should remain as-is
        // until the NEXT worker frame
        // 
        // i.e. Treated as still "changed" and therefore synced back to true on the next frame
        // because the GUI value was set AFTER updating live buffer, the marked shadow shouldn't 
        // be reset to the live value because it occured after the 
        // you updated the shadow. In fact, the shadow itself shouldn't have updated either 
        // so that the comparison remains a mismatch
        //DebugPrint("colors_updated = FALSE");
    }

    /*bool* shadow_updated = getShadow(&colors_updated);
    if (shadow_updated && *shadow_updated)
    {
        int a = 5;
    }*/

    /*if (variableChanged(cam_x))
    {
        cam_x.breakOnEveryAssignment();
    }*/
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

void Mandelbrot_Scene::generateBoundary(EscapeField* field, double level, std::vector<DVec2>& path)
{
    UNUSED(field);
    UNUSED(level);
    UNUSED(path);

    /*int w = field->w;
    int h = field->h;
    auto step = camera->toWorldOffset({ 1, 1 });
    double dx = step.x;
    double dy = step.y;
    for (int y = 0; y < h - 1; ++y)
    {
        for (int x = 0; x < w - 1; ++x)
        {
            int idx = 0;

            // build the 4-bit cell code
            if (field->at(y * w + x) > level) idx |= 1;
            if (field->at(y * w + x + 1) > level) idx |= 2;
            if (field->at((y + 1) * w + x + 1) > level) idx |= 4;
            if (field->at((y + 1) * w + x) > level) idx |= 8;

            int edges = edgeTable[idx];
            if (edges) addSeg(path, x, y, edges, dx, dy, xmin, ymin);
        }
    }*/
}

void Mandelbrot_Scene::viewportDraw(Viewport* ctx) const
{
    camera->stageTransform();

    // Draw active phase bitmap
    if (active_bmp)
        ctx->drawImage(*active_bmp);

    if (show_axis)
        ctx->drawWorldAxis(0.5, 0, 0.5);

    if (interactive_cardioid)
    {
        if (!flatten)
        {
            // Regular interactive Mandelbrot
            //if (!log_x && !log_y)
            //    Cardioid::plot(this, ctx, true);
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

    //ctx->print() << "Frame delta time: " << this->project_dt(10) << " ms";
    /*ctx->print() << "max_iterations: " << iter_lim;

    double zoom_factor = camera->getRelativeZoomFactor().average();
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

    // Gradient test
    camera->stageTransform();
    double x = 8;
    for (double p=0; p<1.0; p+=0.01)
    {
        float c[3];
        gradient_shifted.getColorAt(p, c);
        //ctx->setFillStyle(int(c[0] * 255.0), int(c[1] * 255.0), int(c[2] * 255.0), 255);
        ctx->setFillStyle(c);
        ctx->fillRect(x, 8, 1, 32);
        x++;
    }
}

void Mandelbrot_Scene::onEvent(Event e)
{
    if (e.ctx_owner())
        e.ctx_owner()->camera.handleWorldNavigation(e, true);
   
    cam_x = camera->x;
    cam_y = camera->y;
    cam_rot = camera->rotation;
    cam_zoom = (camera->getRelativeZoomFactor().x / cam_zoom_xy.x);
}

SIM_END(Mandelbrot)
