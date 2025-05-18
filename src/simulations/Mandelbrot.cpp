#include "Mandelbrot.h"
#include "encoding.h"


//SIM_SORT_KEY(0)
SIM_DECLARE(Mandelbrot, "Fractal", "Mandelbrot", "Mandelbrot Viewer")


///-------------///
///   Project   ///
///-------------///

void Mandelbrot_Project::projectPrepare()
{
    auto& layout = newLayout();

    Mandelbrot_Scene::Config config1;
    //auto config2 = make_shared<Test_Scene::Config>(Test_Scene::Config());

    create<Mandelbrot_Scene>(config1)->mountTo(layout);
}

///-----------///
///   Scene   ///
///-----------///

void Mandelbrot_Scene_Vars::populate()
{
    /*for (PostFn& f : live_attributes->post_fn_queue)
    {
        f(*this, *live_attributes);
    }
    live_attributes->post_fn_queue.clear();*/

    /// --------------------------------------------------------------
    ImGui::SeparatorText("View");
    /// --------------------------------------------------------------

    static ImGuiID active_id = 0;

    int decimals = 1 + Math::countWholeDigits(cam_zoom);
    char format[16];
    snprintf(format, sizeof(format), "%%.%df", decimals);

    //if (ImGui::DragDouble("X", &cam_x, 0.01 / cam_zoom, -5.0, 5.0, format))
    //    post([&]() { dst.cam_zoom = cam_zoom; });
    
    ImGui::Checkbox("Show Axis", &show_axis);
    ImGui::DragDouble("X", &cam_x, 0.01/cam_zoom, -5.0, 5.0, format);
    ImGui::DragDouble("Y", &cam_y, 0.01/cam_zoom, -5.0, 5.0, format);

    if (!Platform()->is_mobile())
    {
        if (ImGui::SliderDouble("Rotation", &cam_degrees, 0.0, 360.0, "%.0f"))
        {
            cam_rot = cam_degrees * Math::PI / 180.0;
        }

        // 1e16 = double limit before preicions loss
        static double zoom_speed = cam_zoom / 100.0;
        if (ImGui::DragDouble("Zoom Mult", &cam_zoom, zoom_speed, 1.0, 1e16, "%.2f"))
        {
            zoom_speed = cam_zoom / 100.0;
        }
    }

    ImGui::SliderDouble2("Zoom X/Y", cam_zoom_xy.asArray(), 0.1, 10.0, "%.2f");

    /// --------------------------------------------------------------
    ImGui::Dummy(ScaleSize(0, 10));
    ImGui::SeparatorText("Compute");
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

    
    if (!flatten)
    {
        /// --------------------------------------------------------------
        ImGui::Dummy(ScaleSize(0, 10));
        ImGui::SeparatorText("XX, YY Spline Relationship");
        /// --------------------------------------------------------------
    
        static ImRect vr = { 0.0f, 0.8f, 0.8f, 0.0f };
        ImSpline::SplineEditorPair("X/Y Spline", &x_spline, &y_spline, &vr, 900.0f);
    }


    // /// --------------------------------------------------------------
    // ImGui::Dummy(ScaleSize(0, 10));
    // ImGui::SeparatorText("Visual Options");
    // /// --------------------------------------------------------------

    
    //ImGui::Checkbox("Smoothing", &smoothing);

    /// --------------------------------------------------------------
    ImGui::Dummy(ScaleSize(0, 10));
    ImGui::SeparatorText("Colour Cycling");
    /// --------------------------------------------------------------

    ImGui::Checkbox("Dynamic Color Cycle", &dynamic_color_cycle_limit);

    if (dynamic_color_cycle_limit)
        ImGui::SliderDouble("Cycle Limit Ratio", &color_cycle_value, 0.01, 1.0, "%.2f");
    else
        ImGui::SliderDouble("Cycle Iterations", &color_cycle_iters, 1.0, iter_lim, "%.0f");

    /// --------------------------------------------------------------
    ImGui::Dummy(ScaleSize(0, 10));
    ImGui::SeparatorText("Colour Gradient");
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
    //
    //if (show_gradient_editor)
    {
        if (ImGui::GradientEditor(
            &gradient, draggingMark, selectedMark,
            Platform()->ui_scale_factor(), 
            Platform()->ui_scale_factor(2.0f)))
        {
            colors_updated = true;
            active_color_template = GRADIENT_CUSTOM;
        }
    }
    
    if (Platform()->is_desktop_native())
    {
        /// --------------------------------------------------------------
        ImGui::Dummy(ScaleSize(0, 10));
        ImGui::SeparatorText("Saving & Loading");
        /// --------------------------------------------------------------

        if (ImGui::InputTextMultiline("###Config", config_buf, 1024, ImVec2(0, 0), ImGuiInputTextFlags_AllowTabInput))
        {
            loadConfigBuffer();
        }
        if (ImGui::Button("Copy to Clipboard"))
        {
            ImGui::SetClipboardText(config_buf);
        }
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



std::string Mandelbrot_Scene_Vars::serializeConfig()
{
    uint32_t flags = 0;
    uint32_t version = 1;

    if (dynamic_iter_lim)           flags |= MANDEL_DYNAMIC_ITERS;
    if (show_axis)                  flags |= MANDEL_SHOW_AXIS;
    if (flatten)                    flags |= MANDEL_FLATTEN;
    if (dynamic_color_cycle_limit)  flags |= MANDEL_DYNAMIC_COLOR_CYCLE;

    flags |= ((uint32_t)smoothing_type << MANDEL_SMOOTH_BITSHIFT);
    flags |= (version << MANDEL_VERSION_BITSHIFT);

    JSON::json info;
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

bool Mandelbrot_Scene_Vars::deserializeConfig(std::string txt)
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
    smoothing_type = (MandelSmoothing)((flags & MANDEL_SMOOTH_MASK) >> MANDEL_SMOOTH_BITSHIFT);

    dynamic_iter_lim          = flags & MANDEL_DYNAMIC_ITERS;
    show_axis                 = flags & MANDEL_SHOW_AXIS;
    flatten                   = flags & MANDEL_FLATTEN;
    dynamic_color_cycle_limit = flags & MANDEL_DYNAMIC_COLOR_CYCLE;

    cam_x = info.value("x", 0.0);
    cam_y = info.value("y", 0.0);
    cam_zoom = info.value("z", 1.0);
    cam_zoom_xy.x = info.value("a", 1.0);
    cam_zoom_xy.y = info.value("b", 1.0);
    cam_degrees = info.value("r", 0.0);
    cam_rot = cam_degrees * Math::PI / 180.0;
    quality = info.value("q", quality);

    if (info.contains("A")) x_spline.deserialize(info["A"].get<std::string>());
    if (info.contains("B")) y_spline.deserialize(info["B"].get<std::string>());
    return true;
}

void Mandelbrot_Scene_Vars::updateConfigBuffer()
{
    strcpy(config_buf, serializeConfig().c_str());
}

void Mandelbrot_Scene_Vars::loadConfigBuffer()
{
    deserializeConfig(config_buf);
}

void Mandelbrot_Scene::sceneStart()
{
    //gradient.getMarks().clear();
    //gradient.addMark(0.0, ImVec4(0, 0, 0, 1));
    //gradient.addMark(0.5, ImVec4(1, 1, 1, 1));
    //gradient.addMark(1.0, ImVec4(0, 0, 0, 1));

    loadColorTemplate(GRADIENT_CLASSIC);

    cardioid_lerper.create((2.0 * M_PI) / 5760.0, 0.005);
}

void Mandelbrot_Scene::sceneMounted(Viewport* viewport)
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


    if (variableChanged(cam_rot))
        camera->rotation = cam_rot;
    ///else
    ///    cam_rot = camera->rotation;

    if (anyChanged(cam_zoom_xy, cam_zoom))
    {
        double real_zoom = cam_zoom;
        camera->setRelativeZoomFactorX(real_zoom * cam_zoom_xy.x);
        camera->setRelativeZoomFactorY(real_zoom * cam_zoom_xy.y);
    }
    else
    {
        ///cam_zoom = (camera->getRelativeZoomFactor().x / cam_zoom_xy.x);

        /*post_data([](auto& dst, const auto& src) {
            dst.cam_zoom = src.cam_zoom;
        });*/
    }

    if (variableChanged(cam_x)) 
        camera->x = cam_x; // imgui edited
    ///else 
    ///    cam_x = camera->x; // use camera view
    
    if (variableChanged(cam_y))
        camera->y = cam_y;
    ///else 
    ///    cam_y = camera->y;

    int iw = static_cast<int>(ctx->width);
    int ih = static_cast<int>(ctx->height);

    if (dynamic_iter_lim)
        iter_lim = static_cast<int>(mandelbrotIterLimit(camera->zoom_x) * quality);
    else
        iter_lim = static_cast<int>(quality);

    if (dynamic_color_cycle_limit)
        color_cycle_iters = color_cycle_value * iter_lim;
    //else
    //    color_cycle_iters = color_cycle_value;


    world_quad = camera->toWorldQuad(0, 0, ctx->width, ctx->height);

    // Has the view changed?
    bool mandel_changed = anyChanged(
        world_quad,
        quality,
        color_cycle_iters,
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

    if (colors_updated)
    {
        mandel_changed = true;
        colors_updated = false;
    }

    // Presented Mandelbrot *actually* changed? Restart on 9x9 bmp (phase 0)
    if (mandel_changed)
        compute_phase = 0;
    //if (camera->panning)
    //    compute_phase = 0;

    // Has config changed in any way? Update 
    bool config_changed = mandel_changed || other_option_changed;
    if (config_changed)
        updateConfigBuffer();

    // Has the compute phase changed? Force update
    if (variableChanged(compute_phase))
    {
        //if (cancel_frame_callback)
        //    cancel_frame_callback();

        mandel_changed = true;
    }

    // Set active_bmp
    switch (compute_phase)
    {
        case 0: pending_bmp = &bmp_9x9; break;
        case 1: pending_bmp = &bmp_3x3; break;
        case 2: pending_bmp = &bmp_1x1; break;
    }

    // Reshade active_bmp
    if (current_row != 0 || // Still haven't finished computing the previous frame phase
        mandel_changed ||
        variableChanged(compute_phase))
    {
        pending_bmp->setNeedsReshading();
    }

    pending_bmp->setStageRect(0, 0, ctx->width, ctx->height);
    
    bmp_9x9.setBitmapSize(iw / 9, ih / 9);
    bmp_3x3.setBitmapSize(iw / 3, ih / 3);
    bmp_1x1.setBitmapSize(iw, ih);

    bool finished_compute = false;

    if (pending_bmp->needsReshading(camera))
    {
        if (!flatten)
        {
            // Standard
            bool linear = x_spline.isSimpleLinear() && y_spline.isSimpleLinear();

            finished_compute = dispatchBooleans(
                boolsTemplate(regularMandelbrot, [&], ctx),
                smoothing_type == MandelSmoothing::SMOOTH_CONTINUOUS,
                smoothing_type == MandelSmoothing::SMOOTH_DISTANCE,
                !linear
            );
        }
        else
        {
            // Flat lerp
            finished_compute = dispatchBooleans(
                boolsTemplate(radialMandelbrot, [&], ctx),
                smoothing_type != MandelSmoothing::SMOOTH_NONE,
                show_period2_bulb
            );
        }

        // Forward current phase results to next phase
        if (finished_compute)
        {
            switch (compute_phase)
            {
                case 0:
                {
                    bmp_3x3.clear(0);
                    bmp_9x9.forEachPixel([this](int x, int y) {
                        bmp_3x3.setPixel(x * 3 + 1, y * 3 + 1, bmp_9x9.getPixel(x, y));
                    });
                }
                break;

                case 1:
                {
                    bmp_1x1.clear(0);
                    bmp_3x3.forEachPixel([this](int x, int y) {
                        bmp_1x1.setPixel(x * 3 + 1, y * 3 + 1, bmp_3x3.getPixel(x, y));
                    });
                }
                break;
            }
        }
    }
    if (finished_compute)
    {
        active_bmp = pending_bmp;

        if (compute_phase < 2)
            compute_phase++;
    }

    /*if (variableChanged(cam_x))
    {
        cam_x.breakOnEveryAssignment();
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
    ctx->print() << "max_iterations: " << iter_lim;

    double zoom_factor = camera->getRelativeZoomFactor().average();
    ctx->print() << "\nzoom: " << cam_zoom;

    ctx->print().precision(1);
    ctx->print() << "\nfps: " << std::fixed << fps(60);
    ctx->print() << "\nthreads: " << Thread::idealThreadCount();
    ctx->print() << "\nframe progress: " << (((float)current_row / (float)pending_bmp->height()) * 100.0f) << "%";
}

void Mandelbrot_Scene::onEvent(Event& e)
{
    if (e.owner_ctx())
    {
        //DQuad old_quad = e.owner_ctx()->worldQuad();
        e.owner_ctx()->camera.handleWorldNavigation(e, true);

        // If view quad has changed, cancel current frame 
        // (unless compute phase is still 0, in which case we have no frame to fallback to)
        /*if (compute_phase > 0 && 
            e.owner_ctx()->worldQuad() != old_quad)
        {
            if (cancel_frame_callback)
                cancel_frame_callback();
        }*/
    }

    //DebugPrint("onEvent() called!");
   

    cam_x = camera->x;
    cam_y = camera->y;
    cam_rot = camera->rotation;
    cam_zoom = (camera->getRelativeZoomFactor().x / cam_zoom_xy.x);
}

SIM_END(Mandelbrot)
