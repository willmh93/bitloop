#include <bitloop.h>

#include "Mandelbrot.h"
#include "examples.h"
#include "tween.h"

#ifdef __EMSCRIPTEN__
#include <bitloop/platform/emscripten_browser_clipboard.h>
#endif
#include <imgui_stdlib.h>

SIM_BEG;



/// ─────────────────────── Project ───────────────────────

void Mandelbrot_Project::projectPrepare(Layout& layout)
{
    Mandelbrot_Scene::Config config;
    create<Mandelbrot_Scene>(config)->mountTo(layout);
}

/// ─────────────────────── Scene ───────────────────────

void Mandelbrot_Scene_Data::initData()
{
    loadGradientPreset(GradientPreset::CLASSIC);
}

void Mandelbrot_Scene_Data::populateUI()
{
    bool is_tweening = tweening;
    if (is_tweening)
        ImGui::BeginDisabled();

    if (ImGui::Section("Saving & Loading", true, 0))
    {
        if (ImGui::Button("Save"))
        {
            show_save_dialog = true;
            //strcpy(config_buf_name, "");
            updateConfigBuffer();
            ImGui::OpenPopup("Save Data"); // open on this frame
        }

        ImGui::SameLine();
        if (ImGui::Button("Load"))
        {
            //show_load_dialog = true;

            // Attempt to load immediately from the clipboard (if valid save data)
            //emscripten_browser_clipboard::paste_now(&on_paste, &config_buf);

            ///#ifdef __EMSCRIPTEN__
            ///emscripten_browser_clipboard::paste_now([&](std::string&& buf)
            ///{
            ///    config_buf = buf;
            ///    blPrint() << "config_buf: " << config_buf;
            ///
            ///    opening_load_popup = true;
            ///});
            ///#else
            config_buf = "";
            show_load_dialog = true;
            ImGui::OpenPopup("Load Data");
            ///#endif
        }

        #ifdef __EMSCRIPTEN__
        if (opening_load_popup)
        {
            opening_load_popup = false;
            show_load_dialog = true;
            ImGui::OpenPopup("Load Data");
        }
        #endif

        static std::string url;
        ImGui::SameLine();
        if (ImGui::Button("Share"))
        {
            show_share_dialog = true;
            url = getURL();
            ImGui::OpenPopup("Share URL");

        }

        // Save Dialog
        ImGui::SetNextWindowSize(ScaleSize(350, 300), ImGuiCond_FirstUseEver);
        if (ImGui::BeginPopupModal("Save Data", &show_save_dialog))
        {
            ImVec2 avail = ImGui::GetContentRegionAvail();
            avail.y -= ImGui::GetFrameHeightWithSpacing(); // leave room for buttons/input

            //if (!Platform()->is_mobile())
            //{
            //    avail.y -= ImGui::GetFrameHeightWithSpacing();
            //    ImGui::AlignTextToFramePadding();
            //    ImGui::Text("Name:");
            //    ImGui::SameLine();
            //    if (ImGui::InputText("###mandel_name", config_buf_name, 28))
            //        updateConfigBuffer();
            //}

            ImGui::PushFont(MainWindow::instance()->monoFont());
            ImGui::InputTextMultiline("###Config", &config_buf, avail, ImGuiInputTextFlags_ReadOnly);
            ImGui::PopFont();

            if (ImGui::Button("Copy to Clipboard"))
                ImGui::SetClipboardText(config_buf.c_str());

            ImGui::SameLine();
            if (ImGui::Button("Close")) ImGui::CloseCurrentPopup();
            ImGui::EndPopup();
        }

        // Load Dialog
        ImGui::SetNextWindowSize(ScaleSize(350, 300), ImGuiCond_FirstUseEver);
        if (ImGui::BeginPopupModal("Load Data", &show_load_dialog))
        {
            ImVec2 avail = ImGui::GetContentRegionAvail();
            avail.y -= ImGui::GetFrameHeightWithSpacing(); // leave room for buttons

            ImGui::PushFont(MainWindow::instance()->monoFont());
            //blPrint() << "InputTextMultiline @ config_buf: " << config_buf;
            ImGui::InputTextMultiline("###Config", &config_buf, avail, ImGuiInputTextFlags_AlwaysOverwrite);
            ImGui::PopFont();

            if (ImGui::Button("Paste"))
            {
                #ifdef __EMSCRIPTEN__
                emscripten_browser_clipboard::paste_now([&](std::string&& buf) {
                    config_buf = buf;
                });
                #else
                size_t s;
                config_buf = (char*)SDL_GetClipboardData("text/plain", &s);
                #endif
            }

            ImGui::SameLine();
            ImGui::Dummy(ImVec2(10.0f, 0.0f));
            ImGui::SameLine();

            if (ImGui::Button("Load"))
            {
                loadConfigBuffer();
                ImGui::CloseCurrentPopup();
            }

            ImGui::SameLine();
            if (ImGui::Button("Cancel")) ImGui::CloseCurrentPopup();
            ImGui::EndPopup();
        }

        // Share Dialog
        ImGui::SetNextWindowSize(ScaleSize(350, 120), ImGuiCond_FirstUseEver);
        if (ImGui::BeginPopupModal("Share URL", &show_share_dialog))
        {
            ImVec2 avail = ImGui::GetContentRegionAvail();
            avail.y -= ImGui::GetFrameHeightWithSpacing() * 1; // leave room for buttons/input
            ImGui::PushFont(MainWindow::instance()->monoFont());
            ImGui::InputTextMultiline("###url", &url, avail, ImGuiInputTextFlags_ReadOnly);
            ImGui::PopFont();
            if (ImGui::Button("Copy to Clipboard"))
                ImGui::SetClipboardText(url.c_str());
            ImGui::SameLine();
            if (ImGui::Button("Close")) ImGui::CloseCurrentPopup();
            ImGui::EndPopup();
        }
    }

    if (ImGui::Section("Examples", true, 0.0f, 2.0f))
    {
        ImGuiStyle& style = ImGui::GetStyle();
        float avail_full = ImGui::GetContentRegionAvail().x;
        float min_btn_w = ScaleSize(100.0f);
        int   cols = (int)((avail_full + style.ItemSpacing.x) / (min_btn_w + style.ItemSpacing.x));
        cols = cols < 1 ? 1 : cols;
    
        if (ImGui::BeginTable("preset_grid", cols, ImGuiTableFlags_SizingStretchProp))
        {
            static std::vector<MandelPreset> mandel_presets = generateMandelPresets();
            for (int i = 0; i < (int)mandel_presets.size(); ++i)
            {
                const auto& preset = mandel_presets[i];
                ImGui::TableNextColumn();
                ImGui::PushID(i);
                if (ImGui::Button(preset.name.c_str()))
                {
                    MandelState dest;
                    dest.deserialize(preset.data);
                    startTween(*this, dest);
                }
                ImGui::PopID();
            }
            ImGui::EndTable();
        }
    }

    /// --------------------------------------------------------------
    static bool show_view_by_default = !Platform()->is_mobile(); // Expect navigate by touch for mobile
    if (ImGui::Section("View", true, 5.0f, 2.0f))
    {
        ImGui::Checkbox("Show Axis", &show_axis);
        if (!flatten && !Platform()->is_mobile())
        {
            ImGui::SameLine();
            ImGui::Dummy(ImVec2(10.0f, 0.0f));
            ImGui::SameLine();
            ImGui::Checkbox("Interactive Cardioid", &interactive_cardioid);
        }
        cam_view.populateUI({-5.0, -5.0, 5.0, 5.0});
    }

    /// --------------------------------------------------------------
    if (ImGui::Section("Quality", false, 5.0f, 2.0f))
    {
        if (ImGui::Checkbox("Dynamic Iteration Limit", &dynamic_iter_lim))
        {
            if (!dynamic_iter_lim)
            {
                quality = iter_lim;
            }
            else
            {
                quality = qualityFromIterLimit(iter_lim, cam_view.zoom);
            }
        }

        if (dynamic_iter_lim)
        {
            double quality_pct = quality * 100.0;
            static double initial_quality = quality_pct;
            ImGui::PushID("QualitySlider");
            ImGui::RevertableDragDouble("Quality", &quality_pct, &initial_quality, 1, 1.0, 500.0, "%.0f%%");
            ImGui::PopID();
            quality = quality_pct / 100.0;

            ImGui::Text("max_iter = %d", calculateIterLimit());
        }
        else
            ImGui::DragDouble("Max Iterations", &quality, 1000.0, 1.0, 1000000.0, "%.0f", ImGuiSliderFlags_Logarithmic);


    } // End Header

    //if (ImGui::Section("Instant Styles", false, 5.0f, 2.0f))
    //{
    //    if (ImGui::Button("High Contrast"))
    //    {
    //        iter_dist_mix = 1;
    //        //cycle_iter_dynamic_limit = true;
    //        //cycle_iter_normalize_depth = true;
    //        //cycle_iter_value = 4;
    //        cycle_dist_invert = false;
    //        cycle_dist_value = 1;
    //        cycle_dist_sharpness = 100;
    //        show_color_animation_options = false;
    //        gradient_shift = 0.0;
    //        hue_shift = 0.0;
    //        loadGradientPreset(GradientPreset::CLASSIC);
    //    }
    //}

    /// --------------------------------------------------------------
    if (ImGui::Section("Colour Cycle", true, 5.0f, 2.0f))
    {
        ImGui::SetNextItemWidthForLabel("Iter/Dist Mix");
        ImGui::SliderDouble("Iter/Dist Mix", &iter_dist_mix, 0.0, 1.0, "%.2f");

        int iter_pct = (int)((1.0 - iter_dist_mix) * 100.0);
        int dist_pct = (int)(iter_dist_mix * 100.0);
        bool disable_iter_options = (iter_pct == 0);
        bool disable_dist_options = (dist_pct == 0);

        char iter_header[32];
        char dist_header[32];
        sprintf(iter_header, "Iteration  -  %d%% Weight", iter_pct);
        sprintf(dist_header, "Distance  -  %d%% Weight", dist_pct);

        //ImGui::Dummy(ScaleSize(0, 6));
        //ImGui::PushStyleVar(ImGuiStyleVar_SeparatorTextBorderSize, Platform()->line_height());
        //ImGui::SeparatorText(iter_header);
        //ImGui::PopStyleVar();

        if (disable_iter_options) ImGui::BeginDisabled();
        {
            ImGui::GroupBox box("iter_group", iter_header, ScaleSize(13.0f), ScaleSize(20.0f));

            if (ImGui::Checkbox("% of Max Iters", &cycle_iter_dynamic_limit))
            {
                if (cycle_iter_dynamic_limit)
                    cycle_iter_value /= iter_lim;
                else
                    cycle_iter_value *= iter_lim;
            }
            ImGui::SameLine();
            ImGui::Dummy(ImVec2(10.0f, 0.0f));
            ImGui::SameLine();
            ImGui::Checkbox("Smooth", &use_smoothing);

            //ImGui::SameLine();
            //ImGui::Dummy(ImVec2(10.0f, 0.0f));
            ImGui::Checkbox("Normalize to Visible Range", &cycle_iter_normalize_depth);

            double raw_cycle_iters;

            float required_space = 0.0f;
            box.IncreaseRequiredSpaceForLabel(required_space, cycle_iter_dynamic_limit ? "% Iterations" : "Iterations");
            box.IncreaseRequiredSpaceForLabel(required_space, "Logarithmic");

            if (cycle_iter_dynamic_limit)
            {
                double cycle_pct = cycle_iter_value * 100.0;



                box.SetNextItemWidthForSpace(required_space);
                ImGui::SliderDouble("% Iterations", &cycle_pct, 0.001, 100.0, "%.4f%%",
                    (/*color_cycle_use_log1p ?*/ ImGuiSliderFlags_Logarithmic /*: 0*/) |
                    ImGuiSliderFlags_AlwaysClamp);

                cycle_iter_value = cycle_pct / 100.0;

                raw_cycle_iters = calculateIterLimit() * cycle_iter_value;

            }
            else
            {
                box.SetNextItemWidthForSpace(required_space);
                ImGui::SliderDouble("Iterations", &cycle_iter_value, 1.0, (float)iter_lim, "%.3f",
                    (/*color_cycle_use_log1p ?*/ ImGuiSliderFlags_Logarithmic /*: 0*/) |
                    ImGuiSliderFlags_AlwaysClamp);

                raw_cycle_iters = cycle_iter_value;
            }

            double log_pct = cycle_iter_log1p_weight * 100.0;
            box.SetNextItemWidthForSpace(required_space);
            if (ImGui::SliderDouble_InvLog("Logarithmic", &log_pct, 0.0, 100.0, "%.2f%%", ImGuiSliderFlags_NoRoundToFormat | ImGuiSliderFlags_AlwaysClamp))
            {
                cycle_iter_log1p_weight = log_pct / 100.0;
            }


            ///// Print cycle iter values
            ///{
            ///    if (cycle_iter_dynamic_limit)
            ///        ImGui::Text("raw_cycle_iters = %.1f", raw_cycle_iters);
            ///
            ///    double log_cycle_iters = Math::linear_log1p_lerp(raw_cycle_iters, cycle_iter_log1p_weight);
            ///    ImGui::Text("log_cycle_iters = %.1f", log_cycle_iters);
            ///}
        }
        if (disable_iter_options) ImGui::EndDisabled();

        if (disable_dist_options) ImGui::BeginDisabled();
        {
            ImGui::GroupBox box("dist_group", dist_header);

            float required_width = 0.0f;
            box.IncreaseRequiredSpaceForLabel(required_width, "Dist");
            box.IncreaseRequiredSpaceForLabel(required_width, "Sharpness");

            ImGui::Checkbox("Invert", &cycle_dist_invert);

            box.SetNextItemWidthForSpace(required_width);
            ImGui::SliderDouble("Dist", &cycle_dist_value, 0.001, 1.0, "%.5f", ImGuiSliderFlags_Logarithmic | ImGuiSliderFlags_AlwaysClamp);
            ImGui::PushID("DistLog");
            ImGui::PopID();

            box.SetNextItemWidthForSpace(required_width);
            ImGui::SliderDouble_InvLog("Sharpness", &cycle_dist_sharpness, 0.0, 100.0, "%.4f%%", ImGuiSliderFlags_NoRoundToFormat | ImGuiSliderFlags_AlwaysClamp);
        }
        if (disable_dist_options) ImGui::EndDisabled();
    }

    if (ImGui::Section("Color Offset + Animation", true, 5.0f, 2.0f))
    {
        //ImGui::SeparatorText("Color Offset + Animation");

        ImGui::Checkbox("Animate", &show_color_animation_options);

        float required_space = 0.0f;
        ImGui::IncreaseRequiredSpaceForLabel(required_space, "Gradient shift");

        ImGui::SetNextItemWidthForSpace(required_space);
        if (ImGui::DragDouble("Gradient shift", &gradient_shift, 0.01, -100.0, 100.0, " %.3f", ImGuiSliderFlags_AlwaysClamp))
        {
            gradient_shift = Math::wrap(gradient_shift, 0.0, 1.0);
            colors_updated = true;
        }

        if (show_color_animation_options)
        {
            ImGui::Indent();
            ImGui::PushID("gradient_increment");
            ImGui::SetNextItemWidthForSpace(required_space);
            ImGui::SliderDouble("Increment", &gradient_shift_step, -0.02, 0.02, "%.4f", ImGuiSliderFlags_AlwaysClamp);
            ImGui::PopID();
            ImGui::Unindent();
        }

        ImGui::SetNextItemWidthForSpace(required_space);
        if (ImGui::SliderDouble("Hue shift", &hue_shift, 0.0, 360, "%.3f"))
            colors_updated = true;

        if (show_color_animation_options)
        {
            ImGui::Indent();
            ImGui::PushID("hue_increment");
            ImGui::SetNextItemWidthForSpace(required_space);
            ImGui::SliderDouble("Increment", &hue_shift_step, -5.0, 5.0, "%.3f", ImGuiSliderFlags_AlwaysClamp);
            ImGui::PopID();
            ImGui::Unindent();
        }
    }

    if (ImGui::Section("Gradient Picker", true, 5.0f, 2.0f))
    {
        ImGui::Text("Load Preset");
        static int selecting_template = -1;
        if (ImGui::Combo("###ColorTemplate", &selecting_template, ColorGradientNames, (int)GradientPreset::COUNT))
        {
            loadGradientPreset((GradientPreset)selecting_template);
            selecting_template = -1;
            colors_updated = true;
        }

        ImGui::Dummy(ScaleSize(0, 8));

        if (ImGui::GradientEditor(&gradient,
            Platform()->ui_scale_factor(), 
            Platform()->ui_scale_factor(2.0f)))
        {
            colors_updated = true;
        }

    } // End Header

    /*if (ImGui::Section("Experimental")) {

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
    
    if (is_tweening)
        ImGui::EndDisabled();

    // ======== Developer ========
    #ifdef BL_DEBUG
    static ImRect vr = { 0.0f, 1.0f, 1.0f, 0.0f };

    ImGui::SeparatorText("Position Tween");
    ImSpline::SplineEditor("tween_pos", &tween_pos_spline, &vr);
    ImSpline::SplineEditor("tween_zoom_lift", &tween_zoom_lift_spline, &vr);
    ImSpline::SplineEditor("tween_base_zoom", &tween_base_zoom_spline, &vr);

    //ImGui::InputTextMultiline("###pos_buf", pos_tween_buf, 1024, ImVec2(0, 0), ImGuiInputTextFlags_AllowTabInput));
    if (ImGui::Button("Copy position spline")) ImGui::SetClipboardText(tween_pos_spline.serialize(SplineSerializationMode::CPP_ARRAY, 3).c_str());

    //ImGui::InputTextMultiline("###pos_buf", zoom_tween_buf, 1024, ImVec2(0, 0), ImGuiInputTextFlags_AllowTabInput));
    if (ImGui::Button("Copy lift spline")) ImGui::SetClipboardText(tween_zoom_lift_spline.serialize(SplineSerializationMode::CPP_ARRAY, 3).c_str());

    if (ImGui::Button("Copy base zoom spline")) ImGui::SetClipboardText(tween_base_zoom_spline.serialize(SplineSerializationMode::CPP_ARRAY, 3).c_str());
    #endif
    
}

int Mandelbrot_Scene_Data::calculateIterLimit() const
{
    if (dynamic_iter_lim)
        return (int)(mandelbrotIterLimit(cam_view.zoom) * quality);
    else
    {
        int iters = (int)(quality);
        if (tweening) 
            iters = std::min(iters, (int)(mandelbrotIterLimit(cam_view.zoom)*0.25f));
        return iters;
    }
}




void Mandelbrot_Scene_Data::loadTemplate(std::string_view data)
{
    config_buf = data.data();
    loadConfigBuffer();
}

void Mandelbrot_Scene_Data::updateConfigBuffer()
{
    config_buf = serialize();
}

void Mandelbrot_Scene_Data::loadConfigBuffer()
{
    deserialize(config_buf);
}

std::string Mandelbrot_Scene_Data::getURL()
{
    #ifdef __EMSCRIPTEN__
    return Platform()->url_get_base() + "?data=" + serialize();
    #else
    return "https://bitloop.dev/Mandelbrot?data=" + serialize();
    #endif
}

void Mandelbrot_Scene_Data::savefileChanged()
{
    config_buf = serialize();
    #ifdef __EMSCRIPTEN__
    Platform()->url_set_string("data", config_buf.c_str());
    #endif
}

void Mandelbrot_Scene::sceneStart()
{
    // todo: Stop this getting called twice on startup

    cardioid_lerper.create(Math::TWO_PI / 5760.0, 0.005);

    font = NanoFont::create("/data/fonts/DroidSans.ttf");
}

void Mandelbrot_Scene::sceneMounted(Viewport* ctx)
{
    camera->setOriginViewportAnchor(Anchor::CENTER);
    camera->setDirectCameraPanning(true);
    camera->focusWorldRect(-2, -1.25, 1, 1.25);
    //camera->restrictRelativeZoomRange(0.5, 1e+300);
    cam_view.read(camera);

    reference_zoom = camera->getReferenceZoom();
    ctx_stage_size = ctx->size();

    #ifdef __EMSCRIPTEN__
    if (Platform()->url_has("data"))
    {
        config_buf = Platform()->url_get_string("data");
        loadConfigBuffer();
    }
    #endif
}


void Mandelbrot_Scene::viewportProcess(Viewport* ctx, double dt)
{
    

    /// Process Viewports running this Scene
    ctx_stage_size = ctx->size();

    bool savefile_changed = false;

    // ======== Progressing animation ========
    if (Changed(show_color_animation_options))
        savefile_changed = true;

    if (show_color_animation_options)
    {
        double ani_speed = dt / 0.016;

        // Animation
        if (fabs(gradient_shift_step) > 1.0e-4)
            gradient_shift = Math::wrap(gradient_shift + gradient_shift_step*ani_speed, 0.0, 1.0);

        if (fabs(hue_shift_step) > 1.0e-4)
            hue_shift = Math::wrap(hue_shift + hue_shift_step*ani_speed, 0.0, 360.0);
    }
    else
    {
        if (Changed(gradient_shift, hue_shift, gradient_shift_step, hue_shift_step))
            savefile_changed = true;
    }

    //if (input.touch.now().fingers.size())
    //    blPrint() << input.touch.now().fingers[0].stage_x;

    


    // ======== Color cycle changed? ========
    {
        // Reshade active_bmp, but don't recalculate depth field 
        if (Changed(
            iter_dist_mix,
            cycle_iter_value, 
            cycle_iter_dynamic_limit, 
            cycle_iter_log1p_weight, 
            cycle_iter_normalize_depth,
            cycle_dist_value,
            cycle_dist_invert,
            cycle_dist_sharpness
            ))
        {
            colors_updated = true;
            savefile_changed = true;
        }
    }

    // ======== Tweening ========
    if (tweening)
    {
        double speed = 0.4 / tween_duration;
        tween_progress += speed * dt;
        //tween_progress += 0.2 * dt;
        
        if (tween_progress < 1.0)
            lerpState(*this, state_a, state_b, tween_progress, false);
        else
        {
            lerpState(*this, state_a, state_b, 1.0, true);

            tween_progress = 0.0;
            tweening = false;
        }
    }

    // ======== Updating Shifted Gradient ========
    bool gradient_changed = Changed(gradient);
    if (gradient_changed) savefile_changed = true;
    if (first_frame || gradient_changed || Changed(gradient_shift, hue_shift))
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
            if (flatten) flatten_amount = 0.0;
            camera->focusWorldRect(-2, -1, 2, 1);
            cam_view.zoom = camera->getRelativeZoom().x;
        }

        if (Changed(flatten_amount))
        {
            using namespace Math;
            double t = flatten_amount;
            DRect r;

            if (t < 0.5)      r = lerp(DRect(-2.0,-1.5,0.5,1.5), DRect(-2.0,-0.2,1.5,3.5), lerpFactor(t,0.0,0.5));
            else if (t < 0.7) r = lerp(DRect(-2.0,-0.2,1.5,3.5), DRect(-1.5,-0.2,4.0,3.5), lerpFactor(t,0.5,0.7));
            else              r = lerp(DRect(-1.5,-0.2,4.0,3.5), DRect( 0.0,-1.5,4.5,0.5), lerpFactor(t,0.7,1.0));

            camera->focusWorldRect(r, false);
            cam_view.zoom = camera->getRelativeZoom().x;
        }
    }

    // ======== Camera View ========
    cam_view.apply(camera);

	// Process camera velocity
    if (mouse->pressed)
    {
        camera_vel_pos = { 0, 0 };

        // Stop zoom velocity on touch
        //if (Platform()->is_mobile())
            camera_vel_zoom = 1.0;
    }
    else
    {
        //double threshold = cam_view.getPositionPrecision() / 10.0;
        double threshold = 0.001 / cam_view.zoom;
        if (camera_vel_pos.magnitude() > threshold)    camera->setPos(camera->pos() + camera_vel_pos);
        if (std::abs(camera_vel_zoom-1.0) > 0.001) camera->setZoom(camera->zoom() * camera_vel_zoom);

        camera_vel_pos *= 0.8;
        camera_vel_zoom += (1 - camera_vel_zoom) * 0.2;
    }

    cam_view.read(camera);

    // Ensure size divisble by 9 for perfect result forwarding from: [9x9] to [3x3] to [1x1]
    int iw = (static_cast<int>(ceil(ctx->width() / 9))) * 9;
    int ih = (static_cast<int>(ceil(ctx->height() / 9))) * 9;

    world_quad = camera->toWorldQuad(0, 0, iw, ih);

    // Does depth field need recalculating?
    bool mandel_changed = first_frame || Changed(
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

    if (mandel_changed)
        savefile_changed = true;

    // ======== Determine "minimum" smoothing mode (ITER always needed, DIST might not be) ========
    int old_smoothing = smoothing_type;
    int new_smoothing;

    if (iter_dist_mix <= std::numeric_limits<double>::epsilon())
        new_smoothing = (int)MandelSmoothing::ITER;
    else if (iter_dist_mix >= 1.0 - std::numeric_limits<double>::epsilon())
        new_smoothing = (int)MandelSmoothing::DIST;
    else
        new_smoothing = (int)MandelSmoothing::MIX;

    bool upgrade_smoothing_type   = (old_smoothing == (int)MandelSmoothing::ITER) && (new_smoothing >= (int)MandelSmoothing::DIST);
    bool downgrade_smoothing_type = (old_smoothing == (int)MandelSmoothing::MIX)  && (new_smoothing == (int)MandelSmoothing::ITER);

    // If we need DIST calcuation but we're missing it, force recalculate (regardless of whether mandel view changed)
    // If mandel needs recalculating and we no longer need DIST calculation, downgrade. Otherwise no harm in keeping DIST info
    if (upgrade_smoothing_type || (mandel_changed && downgrade_smoothing_type))
    {
        mandel_changed = true;
        smoothing_type = new_smoothing;
    }

    // Presented Mandelbrot *actually* changed? Restart on 9x9 bmp (phase 0)
    if (mandel_changed)
    {
        bmp_9x9.clear(0, 255, 0, 255);
        bmp_3x3.clear(0, 255, 0, 255);
        bmp_1x1.clear(0, 255, 0, 255);

        if (!first_frame)
            display_intro = false;

        computing_phase = 0;
        current_row = 0;
        field_9x9.setAllDepth(-1.0);

        ///compute_t0 = std::chrono::steady_clock::now();
    }

    // Has the compute phase changed? Force update
    if (Changed(computing_phase))
    {
        mandel_changed = true;
    }

    // Set pending bitmap & pending field
    switch (computing_phase)
    {
        case 0:  pending_bmp = &bmp_9x9;  pending_field = &field_9x9;  break;
        case 1:  pending_bmp = &bmp_3x3;  pending_field = &field_3x3;  break;
        case 2:  pending_bmp = &bmp_1x1;  pending_field = &field_1x1;  break;
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

            ///if (!use_smoothing && smoothing == MandelSmoothing::ITER)
            ///    smoothing = MandelSmoothing::NONE;

            /*double MAX_ZOOM_FLOAT;
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
            }*/

            //const DQuad wq = camera->toWorldQuad({ 0, 0, ctx->width(), ctx->height() });

            DQuad quad = ctx->worldQuad();
            bool x_axis_visible = quad.intersects({ {quad.minX(), 0}, {quad.maxX(), 0} });

            //bool b32 = (cam_view.zoom < MAX_ZOOM_FLOAT);// && (smoothing != MandelSmoothing::DIST);
            //bool b64 = (cam_view.zoom < MAX_DOUBLE_ZOOM);

           

            /*if (b32)      finished_compute = table_invoke<float>(build_table(mandelbrot, [&]), smoothing, flatten);
            else if (b64)*/ finished_compute = table_invoke<double>(build_table(mandelbrot, [&]), smoothing, flatten, x_axis_visible);
            //else          finished_compute = table_invoke<flt128>(build_table(mandelbrot, [&]), smoothing, flatten);
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
            //blPrint("Finished computing phase: %d. Setting as active", pending_field->compute_phase);
            active_bmp = pending_bmp;
            active_field = pending_field;
        }

        // ======== Result forwarding ========
        if (finished_compute)
        {
            switch (computing_phase) {
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
                    ///auto elapsed = std::chrono::steady_clock::now() - compute_t0;
                    ///double dt = std::chrono::duration<double, std::milli>(elapsed).count();
                    ///double dt_avg = timer_ma.push(dt);
                    ///blPrint() << "Compute timer: " << BL::to_fixed(4) << dt_avg;
                    break;
            }

            if (computing_phase < 2)
                computing_phase++;
        }
    }

    if (finished_compute || colors_updated)
    {
        if (Changed(
            iter_dist_mix,
            cycle_iter_log1p_weight,
            cycle_iter_normalize_depth,
            cycle_dist_invert,
            cycle_dist_sharpness
            ))
        {
            savefile_changed = true;
            if (frame_complete)
                refreshFieldDepthNormalized();
        }

        // ======== Update Color Cycle iterations ========
        {
            if (cycle_iter_dynamic_limit)
            {
                double assumed_iter_lim = mandelbrotIterLimit(cam_view.zoom) * 0.5;

                /// "cycle_iter_value" represents ratio of iter_lim
                double color_cycle_iters = (cycle_iter_value * (assumed_iter_lim - (cycle_iter_normalize_depth ? active_field->min_depth : 0)));
                log_color_cycle_iters = Math::linear_log1p_lerp(color_cycle_iters, cycle_iter_log1p_weight);
            }
            else
            {
                /// "cycle_iter_value" represents actual iter_lim
                log_color_cycle_iters = Math::linear_log1p_lerp(cycle_iter_value, cycle_iter_log1p_weight);
            }

            if (isnan(log_color_cycle_iters))
            {
                DebugBreak();
                if (cycle_iter_dynamic_limit)
                {
                    double assumed_iter_lim = mandelbrotIterLimit(cam_view.zoom) * 0.5;

                    /// "cycle_iter_value" represents ratio of iter_lim
                    double color_cycle_iters = (cycle_iter_value * (assumed_iter_lim - (cycle_iter_normalize_depth ? active_field->min_depth : 0)));
                    log_color_cycle_iters = Math::linear_log1p_lerp(color_cycle_iters, cycle_iter_log1p_weight);
                }
                else
                {
                    /// "cycle_iter_value" represents actual iter_lim
                    log_color_cycle_iters = Math::linear_log1p_lerp(cycle_iter_value, cycle_iter_log1p_weight);
                }
            }
        }

        shadeBitmap();
        colors_updated = false;
    }


    first_frame = false;

    if (savefile_changed)
        savefileChanged();
}

void Mandelbrot_Scene::viewportDraw(Viewport* ctx) const
{
    // Draw active phase bitmap
    if (active_bmp)
        ctx->drawImage(*active_bmp, active_bmp->worldQuad());

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

    /*if (active_field && active_bmp)
    {
        //ctx->print() << "\nactive_field->max_depth: " << active_field->max_depth;
        ///ctx->print() << "\nactive_field->min_dist: " << active_field->min_dist;
        ///ctx->print() << "\nactive_field->max_dist: " << active_field->max_dist;
    
        int px = (int)mouse->stage_x;
        int py = (int)mouse->stage_y;
        if (px >= 0 && py >= 0 && px < active_bmp->width() && py < active_bmp->height())
        {
            IVec2 pos = active_bmp->pixelPosFromWorld(DVec2(mouse->world_x, mouse->world_y));
            EscapeFieldPixel* p = active_field->get(pos.x, pos.y);
        
            if (p)
            {
                double raw_depth = p->depth;
                double raw_dist = p->dist;
    
                double dist = log(raw_dist);
    
                ///double dist_factor = Math::lerpFactor(dist, active_field->min_dist, active_field->max_dist);
    
                ctx->print() << "\nraw_depth: " << raw_depth << "\n";
                ctx->print() << "\nraw_dist: " << raw_dist << "\n";
                //ctx->print() << "log_dist: " << dist << "\n";
                ///ctx->print() << "dist_factor: " << dist_factor << "\n\n";
    
                double stable_min_raw_dist = camera->stageToWorldOffset(DVec2{ 0.5, 0 }).magnitude();
                double stable_max_raw_dist = active_bmp->worldSize().magnitude() / 2.0;
    
                double stable_min_dist = log(stable_min_raw_dist);
                double stable_max_dist = log(stable_max_raw_dist);
    
                ctx->print() << "stable_min_raw_dist: " << stable_min_raw_dist << "\n";
                ctx->print() << "stable_max_raw_dist: " << stable_max_raw_dist << "\n\n";
    
                ctx->print() << "min_possible_dist: " << stable_min_dist << "\n";
                ctx->print() << "max_possible_dist: " << stable_max_dist << "\n\n";
    
                ctx->print() << "Stabilized factor: " << Math::lerpFactor(dist, stable_min_dist, stable_max_dist);
    
    
                double lower_depth_bound = cycle_iter_normalize_depth ? pending_field->min_depth : 0;
    
                double normalized_depth = Math::lerpFactor(raw_depth, lower_depth_bound, (double)iter_lim);
                double normalized_dist = Math::lerpFactor(raw_dist, pending_field->min_dist, pending_field->max_dist);
    
                ctx->print() << std::setprecision(18);
                ctx->print() << "\n\nnormalized_depth: " << normalized_depth;
                ctx->print() << "\nnormalized_dist: " << normalized_dist;
    
                ctx->print() << "\n\nraw_iters: " << raw_depth;
                ctx->print() << "\nraw_dist: " << raw_dist << "\n";
    
                double single_pixel_raw_dist = camera->stageToWorldOffset(DVec2{ 0.001, 0 }).magnitude();
                double min_possible_dist = log(single_pixel_raw_dist);
    
                double max_possible_raw_dist = ctx->worldSize().magnitude();
                double max_possible_dist = log(max_possible_raw_dist);
                //double max_possible_dist = de_cap_from_view(
                //    camera->x(),
                //    camera->y(),
                //    ctx->worldSize().x / 2,
                //    ctx->worldSize().y,
                //    sqrt(escape_radius<MandelSmoothing::DIST>())
                //);
    
                ctx->print() << "\nlog_dist: " << log(raw_dist) << "\n";
    
                //ctx->print() << "max_possible_raw_dist: " << max_possible_raw_dist << "\n";
                ctx->print() << "single_pixel_raw_dist: " << single_pixel_raw_dist << "\n";
                ctx->print() << "max_possible_raw_dist: " << max_possible_raw_dist << "\n";
    
                ctx->print() << "min_possible_dist: " << min_possible_dist << "\n\n";
                ctx->print() << "max_possible_dist: " << max_possible_dist << "\n\n";
    
                double stable_normalized_dist = Math::lerpFactor(log(raw_dist), pending_field->min_dist, max_possible_dist);
                ctx->print() << "stable_normalized_dist: " << stable_normalized_dist << "\n";
    
            }
        }
    }
    */

    //if (smoothing_type == (int)MandelSmoothing::MIX)
    //    ctx->print() << "Smoothing: MIX";
    //else if (smoothing_type == (int)MandelSmoothing::ITER)
    //    ctx->print() << "Smoothing: ITER";
    //else if (smoothing_type == (int)MandelSmoothing::DIST)
    //    ctx->print() << "Smoothing: DIST";


    //DQuad quad = ctx->worldQuad();
    //bool x_axis_visible = quad.intersects({ {quad.minX(), 0}, {quad.maxX(), 0}});
    //ctx->print() << "\nx_axis_visible: " << (x_axis_visible ? "true" : "false");

    //ctx->printTouchInfo();

    camera->stageTransform();

    if (display_intro)
    {
        ctx->setFont(font);
        ctx->setFontSize(20);

        //if (!Platform()->is_desktop_native())
        {
            ctx->fillText("Controls:", ScaleSize(10), ScaleSize(10));
            ctx->fillText("  - Touch & drag to move", ScaleSize(10), ScaleSize(35));
            ctx->fillText("  - Pinch to zoom / rotate", ScaleSize(10), ScaleSize(60));
        }

        ctx->setTextAlign(TextAlign::ALIGN_LEFT);
        ctx->setTextBaseline(TextBaseline::BASELINE_BOTTOM);
        //ctx->fillText("Developer:    Will Hemsworth", ScaleSize(10), ctx->height() - ScaleSize(32));
        ctx->fillText("Contact:  will.hemsworth@bitloop.dev", ScaleSize(10), ctx->height() - ScaleSize(10));
    }

    //~ // Wont work nicely in slow scene, needs to be imgui or own it's own thread/layer/nanovg-overlay.
      // uiOverlay(ctx) for both nanovg and imgui (on gui main thread)

    //ctx->print() << "camera_vel: " << camera_vel_pos << "\n";
}

void Mandelbrot_Scene::viewportOverlay(Viewport* ctx) const
{
    //ctx->setFillStyle(255, 0, 0, 255);
    //ctx->fillRoundedRect(10, 10, 150, 50, 8);

    UNUSED(ctx);
    //if (input.touch.finger(0).pressed)
    //{
    //  minimizeMaximizeButton(ctx, false);
    //}

    //if (Input::Touch().Fingers(0).Released(minimizeMaximizeButton(ctx, false)))
    //{
    //
    //}
    //
    //if (input.touch.now().fingers(0).clicked(minimizeMaximizeButton(ctx, false)))
    //{
    //}
}

DRect Mandelbrot_Scene::minimizeMaximizeButtonRect(Viewport* ctx) const
{
    const DVec2  s = ScaleSize(40, 40);
    const double pad = ScaleSize(6);
    const DRect vr = ctx->viewportRect();

    DVec2 tl = vr.tr() + DVec2{ -s.x - pad, pad };
    return DRect(tl, tl + s);
}

DRect Mandelbrot_Scene::minimizeMaximizeButton(Viewport* ctx, bool minimize) const
{
    DRect rect = minimizeMaximizeButtonRect(ctx);
    DRect rect_a = minimize ? rect.scaled(0.65)  : rect.scaled(0.175);
    DRect rect_b = minimize ? rect.scaled(0.175) : rect.scaled(0.65);
    const double rounding = rect.size().magnitude() / 10.0;

    Color col(255, 255, 255);
    ctx->setStrokeStyle(col);
    ctx->setLineWidth(ScaleSize(2));

    ctx->setFillStyle(40, 40, 40, 150);
    ctx->fillRoundedRect(rect, rounding);
    ctx->strokeRoundedRect(rect, rounding);

    ctx->drawArrow(rect_a.tr(), rect_b.tr(), col, 45, 0.9, false);
    ctx->drawArrow(rect_a.bl(), rect_b.bl(), col, 45, 0.9, false);

    return rect;
}

void Mandelbrot_Scene::onEvent(Event e)
{
    cam_view.read(camera);

    DVec2 old_pos = { camera->x(), camera->y() };
    DVec2 old_zoom = camera->zoom();
    
    if (handleWorldNavigation(e, true, true))
    {
        DVec2 zoom = camera->zoom();

        avg_vel_pos.push({ camera->x() - old_pos.x, camera->y() - old_pos.y });
        avg_vel_zoom.push( (camera->zoom() / old_zoom).average() );

        camera_vel_pos  = avg_vel_pos.average();
        camera_vel_zoom = avg_vel_zoom.average();

        if (!Platform()->is_mobile())
        {
            // Stopped wheeling mouse
            avg_vel_zoom.clear();
        }
    }

    if (e.type() == SDL_EVENT_MOUSE_BUTTON_UP)
    {
        avg_vel_zoom.clear();
        avg_vel_pos.clear();
    }

    cam_view.apply(camera);
}

SIM_END;
