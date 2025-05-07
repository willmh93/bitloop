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

void Mandelbrot_Scene_Vars::populate(Mandelbrot_Scene_Vars& dst)
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
    
    ImGui::DragDouble("X", &cam_x, 0.01 / cam_zoom, -5.0, 5.0, format);
    ImGui::DragDouble("Y", &cam_y, 0.01/cam_zoom, -5.0, 5.0, format);

    if (!Platform()->is_mobile())
    {
        if (ImGui::SliderDouble("Rotation", &cam_degrees, 0.0, 360.0, "%.0f"))
        {
            cam_rotation = cam_degrees * Math::PI / 180.0;
        }

        // 1e16 = double limit before preicions loss
        static double zoom_speed = cam_zoom / 100.0;
        if (ImGui::DragDouble("Zoom Mult", &cam_zoom, zoom_speed, 1.0, 1e16, "%.2f"))
        {
            zoom_speed = cam_zoom / 100.0;
        }
    }

    ImGui::SliderDouble2("Zoom X/Y", cam_zoom_xy.asArray(), 0.1, 10.0, "%.0f");

    /// --------------------------------------------------------------
    ImGui::SeparatorText("Compute");
    /// --------------------------------------------------------------

    ImGui::Checkbox("Flatten", &flatten);

    if (flatten)
    {
        if (ImGui::SliderDouble("Flatness", &flatten_amount, 0.0, 1.0))
            cardioid_lerp_amount = 1.0 - flatten_amount;

        ImGui::Checkbox("Show period-2 bulb", &show_period2_bulb);
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
        ImGui::SeparatorText("XX, YY Spline Relationship");
        /// --------------------------------------------------------------
    
        static ImRect vr = { 0.0f, 0.8f, 0.8f, 0.0f };
        ImSpline::SplineEditorPair("X/Y Spline", &x_spline, &y_spline, &vr, 900.0f);
    }


    /// --------------------------------------------------------------
    ImGui::SeparatorText("Visual Options");
    /// --------------------------------------------------------------

    ImGui::Checkbox("Show Axis", &show_axis);
    ImGui::Checkbox("Smoothing", &smoothing);

    static bool show_gradient_editor = false;
    if (ImGui::GradientButton(&gradient, Platform()->ui_scale_factor()))
    {
        show_gradient_editor = !show_gradient_editor;
    }
    
    if (show_gradient_editor)
    {
        colors_updated = ImGui::GradientEditor(
            &gradient, draggingMark, selectedMark, Platform()->ui_scale_factor());
    }
    
    ///// CPU only
    if (ImGui::InputTextMultiline("Config", config_buf, 1024, ImVec2(0, 0), ImGuiInputTextFlags_AllowTabInput))
    {
        loadConfigBuffer();
    }

    if (ImGui::IsAnyItemActive())            // true while the mouse is still held,
    {                                        // a text box has focus, etc.
        active_id = ImGui::GetActiveID();   // 0 means “none”; otherwise that
        ImGui::Text("Changing ID: %d", active_id);
    }
    
}



std::string Mandelbrot_Scene_Vars::serializeConfig()
{
    uint32_t flags = 0;
    uint32_t version = 1;

    if (dynamic_iter_lim)   flags |= MANDEL_DYNAMIC;
    if (smoothing)          flags |= MANDEL_SMOOTH;
    if (show_axis)          flags |= MANDEL_SHOW_AXIS;
    if (flatten)            flags |= MANDEL_FLATTEN;

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
    uint32_t version = (flags & MANDEL_VERSION) >> MANDEL_VERSION_BITSHIFT;

    dynamic_iter_lim = flags & MANDEL_DYNAMIC;
    smoothing        = flags & MANDEL_SMOOTH;
    show_axis        = flags & MANDEL_SHOW_AXIS;
    flatten          = flags & MANDEL_FLATTEN;

    cam_x = info.value("x", 0.0);
    cam_y = info.value("y", 0.0);
    cam_zoom = info.value("z", 1.0);
    cam_zoom_xy.x = info.value("a", 1.0);
    cam_zoom_xy.y = info.value("b", 1.0);
    cam_degrees = info.value("r", 0.0);
    cam_rotation = cam_degrees * Math::PI / 180.0;
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
    gradient.getMarks().clear();
    gradient.addMark(0.0, ImVec4(0, 0, 0, 1));
    gradient.addMark(0.5, ImVec4(1, 1, 1, 1));
    gradient.addMark(1.0, ImVec4(0, 0, 0, 1));

    cardioid_lerper.create((2.0 * M_PI) / 5760.0, 0.005);
}

void Mandelbrot_Scene::sceneMounted(Viewport* viewport)
{
    camera->setOriginViewportAnchor(Anchor::CENTER);
    camera->setPanningUsesOffset(false);
    camera->focusWorldRect(-2, -1, 2, 1);
    //cam_zoom_ref = camera->getRelativeZoomFactor().average();
    //cam_zoom = camera->zoom_x;
}

void Mandelbrot_Scene::step_color(double step, uint8_t& r, uint8_t& g, uint8_t& b)
{
    double ratio;
    if (smoothing)
        ratio = fmod(step, 1.0);
    else
        ratio = fmod(step / 10.0, 1.0);

    r = (uint8_t)(9 * (1 - ratio) * ratio * ratio * ratio * 255);
    g = (uint8_t)(15 * (1 - ratio) * (1 - ratio) * ratio * ratio * 255);
    b = (uint8_t)(8.5 * (1 - ratio) * (1 - ratio) * (1 - ratio) * ratio * 255);
}

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
    return 100 + std::max(0, it);
}

void Mandelbrot_Scene::viewportProcess(Viewport* ctx)
{
    /// Process Viewports running this Scene

    // Update View
    if (variableChanged(flatten))
    {
        if (flatten)
            flatten_amount = 0.0;

        camera->focusWorldRect(-2, -1, 2, 1);
        cam_zoom = camera->getRelativeZoomFactor().x;
    }

    if (variableChanged(cam_rotation))
        camera->rotation = cam_rotation;
    else
        cam_rotation = camera->rotation;

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

    if (variableChanged(cam_x)) camera->x = cam_x; else cam_x = camera->x;
    if (variableChanged(cam_y)) camera->y = cam_y; else cam_y = camera->y;

    int iw = static_cast<int>(ctx->width);
    int ih = static_cast<int>(ctx->height);

    if (dynamic_iter_lim)
        iter_lim = static_cast<int>(mandelbrotIterLimit(camera->zoom_x) * quality);
    else
        iter_lim = static_cast<int>(quality);

    world_quad = camera->toWorldQuad(0, 0, ctx->width, ctx->height);

    // Has the view changed?
    bool mandel_changed = anyChanged(
        world_quad,
        quality,
        smoothing,
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

    // Has config changed in any way? Update 
    bool config_changed = mandel_changed || other_option_changed;
    if (config_changed)
        updateConfigBuffer();

    // Has the compute phase changed? Force update
    if (variableChanged(compute_phase))
        mandel_changed = true;

    // Set active_bmp
    switch (compute_phase)
    {
        case 0: active_bmp = &bmp_9x9; break;
        case 1: active_bmp = &bmp_3x3; break;
        case 2: active_bmp = &bmp_1x1; break;
    }

    // Reshade active_bmp
    if (mandel_changed || variableChanged(compute_phase))
        active_bmp->setNeedsReshading();

    active_bmp->setStageRect(0, 0, ctx->width, ctx->height);
    
    bmp_9x9.setBitmapSize(iw / 9, ih / 9);
    bmp_3x3.setBitmapSize(iw / 3, ih / 3);
    bmp_1x1.setBitmapSize(iw, ih);

    if (active_bmp->needsReshading(camera))
    {
        if (!flatten)
        {
            // Standard
            bool linear = x_spline.isSimpleLinear() && y_spline.isSimpleLinear();

            dispatchBooleans(
                boolsTemplate(regularMandelbrot, [&], ctx),
                smoothing,
                linear
            );
        }
        else
        {
            // Flat lerp
            dispatchBooleans(
                boolsTemplate(radialMandelbrot, [&], ctx),
                smoothing,
                show_period2_bulb
            );
        }

        // Forward current phase results to next phase
        switch (compute_phase)
        {
            case 0:
            {
                bmp_3x3.clear(0);
                bmp_9x9.forEachWorldPixel([this](int x, int y) {
                    bmp_3x3.setPixel(x * 3 + 1, y * 3 + 1, bmp_9x9.getPixel(x, y));
                });
            }
            break;

            case 1:
            {
                bmp_1x1.clear(0);
                bmp_3x3.forEachWorldPixel([this](int x, int y) {
                    bmp_1x1.setPixel(x * 3 + 1, y * 3 + 1, bmp_3x3.getPixel(x, y));
                });
            }
            break;
        }

        if (compute_phase < 2)
            compute_phase++;
    }
}

void Mandelbrot_Scene::viewportDraw(Viewport* ctx)
{
    camera->stageTransform();

    // Draw active phase bitmap
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
    ctx->print() << "Max Iterations: " << iter_lim;

    double zoom_factor = camera->getRelativeZoomFactor().average();
    ctx->print() << "\nzoom: " << cam_zoom;
}

void Mandelbrot_Scene::onEvent(Event& e)
{
    if (e.owner_ctx())
        e.owner_ctx()->camera.handleWorldNavigation(e, true);
}

SIM_END(Mandelbrot)
