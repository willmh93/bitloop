#include <bitloop.h>

#include "Mandelbrot.h"
#include "examples.h"

#include "tween.h"
#include "conversions.h"

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

// ────── Compute/Normalize dispatch helpers ──────

bool Mandelbrot_Scene::compute_mandelbrot()
{
    MandelSmoothing smoothing = static_cast<MandelSmoothing>(smoothing_type);
    MandelFloatQuality float_type = getRequiredFloatType(smoothing, cam_view.zoom);

    int timeout;
    switch (computing_phase)
    {
    case 0: timeout = 0; break;
    default: timeout = 16; break;
    }

    if (float_type == MandelFloatQuality::F32)
    {
        return table_invoke<float>(
            build_table(mandelbrot, [&], pending_bmp, pending_field, iter_lim, numThreads(), timeout, current_row, stripe_params),
            smoothing, flatten
        );
    }
    else if (float_type == MandelFloatQuality::F64)
    {
        return table_invoke<double>(
            build_table(mandelbrot, [&], pending_bmp, pending_field, iter_lim, numThreads(), timeout, current_row, stripe_params),
            smoothing, flatten
        );
    }
    else if (float_type == MandelFloatQuality::F128)
    {
        return table_invoke<flt128>(
            build_table(mandelbrot, [&], pending_bmp, pending_field, iter_lim, numThreads(), timeout, current_row, stripe_params),
            smoothing, flatten
        );
    }

    return false;
}

void Mandelbrot_Scene::normalize_field(EscapeField* field, CanvasImage128* bmp)
{
    MandelSmoothing    smoothing = static_cast<MandelSmoothing>(smoothing_type);
    MandelFloatQuality float_type = getRequiredFloatType(smoothing, cam_view.zoom);

    // bool linear = x_spline.isSimpleLinear() && y_spline.isSimpleLinear();
            

    if (float_type == MandelFloatQuality::F32)
    {
        refreshFieldDepthNormalized<float>(field, bmp, smoothing,
            cycle_iter_normalize_depth, cycle_iter_log1p_weight, cycle_dist_invert,// cycle_dist_sharpness,
            numThreads());
    }
    else if (float_type == MandelFloatQuality::F64)
    {
        refreshFieldDepthNormalized<double>(field, bmp, smoothing,
            cycle_iter_normalize_depth, cycle_iter_log1p_weight, cycle_dist_invert,// cycle_dist_sharpness,
            numThreads());
    }
    else if (float_type == MandelFloatQuality::F128)
    {
        refreshFieldDepthNormalized<flt128>(field, bmp, smoothing,
            cycle_iter_normalize_depth, cycle_iter_log1p_weight, cycle_dist_invert,// cycle_dist_sharpness,
            numThreads());
    }
}

void normalize_shading_limits(
    EscapeField* field,
    CanvasImage128* bmp,
    CameraViewController& cam_view,

    bool   cycle_iter_dynamic_limit,
    bool   cycle_iter_normalize_depth,
    double cycle_iter_log1p_weight,
    double cycle_iter_value,

    bool   cycle_dist_invert,
    double cycle_dist_sharpness,
    double cycle_dist_value
)
{
    field->min_depth = std::numeric_limits<double>::max();
    field->max_depth = std::numeric_limits<double>::lowest();

    // Redetermine minimum depth for entire visible field
    bmp->forEachPixel([&](int x, int y)
    {
        EscapeFieldPixel& field_pixel = field->at(x, y);
        double depth = field_pixel.depth;

        if (depth >= INSIDE_MANDELBROT_SET_SKIPPED || field_pixel.flag_for_skip) return;
        if (depth < field->min_depth) field->min_depth = depth;
        if (depth > field->max_depth) field->max_depth = depth;
    }, 0);

    if (field->min_depth == std::numeric_limits<double>::max()) field->min_depth = 0;


    field->assumed_iter_min = mandelbrotIterLimit(cam_view.zoom) * 0.25;
    field->assumed_iter_max = mandelbrotIterLimit(cam_view.zoom) * 0.5;


    // Calculate normalized depth/dist
    double dist_min_pixel_ratio = ((100.0 - cycle_dist_sharpness) / 100.0 + 0.00001);
    flt128 stable_min_raw_dist = Camera::active->stageToWorldOffset<flt128>(dist_min_pixel_ratio, 0.0).magnitude(); // fraction of a pixel
    flt128 stable_max_raw_dist = flt128{ (bmp->worldSize().magnitude()) } / flt128{ 2.0 };                         // half diagonal world viewport size

    // @@ todo: Use downgraded type from: stable_min_raw_dist?
    field->stable_min_dist = (cycle_dist_invert ? -1 : 1) * log(stable_min_raw_dist);
    field->stable_max_dist = (cycle_dist_invert ? -1 : 1) * log(stable_max_raw_dist);

    blPrint() << "stable_min_dist: " << field->stable_min_dist << "   stable_max_dist: " << field->stable_max_dist;

    if (cycle_iter_dynamic_limit)
    {
        //double assumed_iter_lim = mandelbrotIterLimit(cam_view.zoom) * 0.5;

        /// "cycle_iter_value" represents ratio of iter_lim
        //double color_cycle_iters = (cycle_iter_value * (assumed_iter_lim - (cycle_iter_normalize_depth ? field->min_depth : 0)));
        double color_cycle_iters = (cycle_iter_value * (field->assumed_iter_max - (cycle_iter_normalize_depth ? field->assumed_iter_min : 0)));
        field->log_color_cycle_iters = Math::linear_log1p_lerp(color_cycle_iters, cycle_iter_log1p_weight);
    }
    else
    {
        /// "cycle_iter_value" represents actual iter_lim
        field->log_color_cycle_iters = Math::linear_log1p_lerp(cycle_iter_value, cycle_iter_log1p_weight);
    }

    field->cycle_dist_value = cycle_dist_value;

    ///#ifdef BL_DEBUG // todo: Remove when certain bug fixed
    ///if (isnan(field->log_color_cycle_iters))
    ///{
    ///    DebugBreak();
    ///    if (cycle_iter_dynamic_limit)
    ///    {
    ///        double assumed_iter_lim = mandelbrotIterLimit(cam_view.zoom) * 0.5;
    ///
    ///        /// "cycle_iter_value" represents ratio of iter_lim
    ///        double color_cycle_iters = (cycle_iter_value * (assumed_iter_lim - (cycle_iter_normalize_depth ? field->min_depth : 0)));
    ///        field->log_color_cycle_iters = Math::linear_log1p_lerp(color_cycle_iters, cycle_iter_log1p_weight);
    ///    }
    ///    else
    ///    {
    ///        /// "cycle_iter_value" represents actual iter_lim
    ///        field->log_color_cycle_iters = Math::linear_log1p_lerp(cycle_iter_value, cycle_iter_log1p_weight);
    ///    }
    ///}
    ///#endif
}

// ────── UI ──────

void Mandelbrot_Scene::UI::populate()
{
    bl_pull(tweening);
    bl_pull(cam_view);
    bl_pull(dynamic_iter_lim);
    bl_pull(quality);
    bl_pull(iter_lim);

    bl_pull(gradient);
    bl_pull(colors_updated);

    // experimental
    bl_pull(flatten);

    bool is_tweening = tweening;
    if (is_tweening)
        ImGui::BeginDisabled();

    //bl_pull(test);
    //static flt128 inc = flt128{ 0.0000000000000000000000001 };
    //static double test2 = (double)test;
    //static double inc2 = (double)inc;
    //
    //ImGui::DragFloat128("Test", &test, inc, static_cast<flt128>(-100), static_cast<flt128>(100), "%.25f", ImGuiSliderFlags_NoRoundToFormat);
    //ImGui::DragDouble("Test2", &test2, inc2, -100, 100, "%.25f", ImGuiSliderFlags_NoRoundToFormat);
    //bl_push(test);


    if (ImGui::Section("Saving & Loading", true, 0))
    {
        bl_pull(config_buf);

        if (ImGui::Button("Save"))
        {
            show_save_dialog = true;
            //strcpy(config_buf_name, "");
            bl_schedule([](Mandelbrot_Scene& scene)
            {
                scene.updateConfigBuffer();
            });

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
            url = dataToURL(config_buf);
            ImGui::OpenPopup("Share URL");

        }

        // Save Dialog
        ImGui::SetNextWindowSize(scale_size(350, 300), ImGuiCond_FirstUseEver);
        if (ImGui::BeginPopupModal("Save Data", &show_save_dialog))
        {
            ImVec2 avail = ImGui::GetContentRegionAvail();
            avail.y -= ImGui::GetFrameHeightWithSpacing(); // leave room for buttons/input

            //if (!platform()->is_mobile())
            //{
            //    avail.y -= ImGui::GetFrameHeightWithSpacing();
            //    ImGui::AlignTextToFramePadding();
            //    ImGui::Text("Name:");
            //    ImGui::SameLine();
            //    if (ImGui::InputText("###mandel_name", config_buf_name, 28))
            //        updateConfigBuffer();
            //}

            ImGui::PushFont(main_window()->monoFont());
            ImGui::InputTextMultiline("###Config", &config_buf, avail, ImGuiInputTextFlags_ReadOnly);
            ImGui::PopFont();

            if (ImGui::Button("Copy to Clipboard"))
                ImGui::SetClipboardText(config_buf.c_str());

            ImGui::SameLine();
            if (ImGui::Button("Close")) ImGui::CloseCurrentPopup();
            ImGui::EndPopup();
        }

        // Load Dialog
        ImGui::SetNextWindowSize(scale_size(350, 300), ImGuiCond_FirstUseEver);
        if (ImGui::BeginPopupModal("Load Data", &show_load_dialog))
        {
            ImVec2 avail = ImGui::GetContentRegionAvail();
            avail.y -= ImGui::GetFrameHeightWithSpacing(); // leave room for buttons

            ImGui::PushFont(main_window()->monoFont());
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
                bl_schedule([](Mandelbrot_Scene& scene)
                {
                    scene.loadConfigBuffer();
                });
                ImGui::CloseCurrentPopup();
            }

            ImGui::SameLine();
            if (ImGui::Button("Cancel")) ImGui::CloseCurrentPopup();
            ImGui::EndPopup();
        }

        // Share Dialog
        ImGui::SetNextWindowSize(scale_size(350, 120), ImGuiCond_FirstUseEver);
        if (ImGui::BeginPopupModal("Share URL", &show_share_dialog))
        {
            ImVec2 avail = ImGui::GetContentRegionAvail();
            avail.y -= ImGui::GetFrameHeightWithSpacing() * 1; // leave room for buttons/input
            ImGui::PushFont(main_window()->monoFont());
            ImGui::InputTextMultiline("###url", &url, avail, ImGuiInputTextFlags_ReadOnly);
            ImGui::PopFont();
            if (ImGui::Button("Copy to Clipboard"))
                ImGui::SetClipboardText(url.c_str());
            ImGui::SameLine();
            if (ImGui::Button("Close")) ImGui::CloseCurrentPopup();
            ImGui::EndPopup();
        }

        bl_push(config_buf);
    }

    if (ImGui::Section("Examples", true, 0.0f, 2.0f))
    {
        ImGuiStyle& style = ImGui::GetStyle();
        float avail_full = ImGui::GetContentRegionAvail().x;
        float min_btn_w = scale_size(100.0f);
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
                    //MandelState dest;
                    //dest.deserialize(preset.data);

                    bl_schedule([preset](Mandelbrot_Scene& scene)
                    {
                        scene.state_b.deserialize(preset.data);
                        startTween(scene);
                    });
                }
                ImGui::PopID();
            }
            ImGui::EndTable();
        }
    }

    /// --------------------------------------------------------------
    static bool show_view_by_default = !platform()->is_mobile(); // Expect navigate by touch for mobile
    if (ImGui::Section("View", true, 5.0f, 2.0f))
    {
        bl_pull(show_axis);
        ImGui::Checkbox("Show Axis", &show_axis);
        bl_push(show_axis);

        #if MANDEL_FEATURE_INTERACTIVE_CARDIOID
        if (!flatten && !platform()->is_mobile())
        {
            ImGui::SameLine();
            ImGui::Dummy(ImVec2(10.0f, 0.0f));
            ImGui::SameLine();

            bl_pull(interactive_cardioid);
            ImGui::Checkbox("Interactive Cardioid", &interactive_cardioid);
            bl_push(interactive_cardioid);
        }
        #endif
        

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
            ImGui::RevertableDragDouble("###Quality", &quality_pct, &initial_quality, 1, 1.0, 500.0, "%.0f%%");
            ImGui::PopID();
            quality = quality_pct / 100.0;

            ImGui::SameLine();
            ImGui::Text("= %d Iters", finalIterLimit(cam_view, quality, dynamic_iter_lim, tweening));
        }
        else
            ImGui::DragDouble("Max Iterations", &quality, 1000.0, 1.0, 1000000.0, "%.0f", ImGuiSliderFlags_Logarithmic);



        bl_pull(maxdepth_optimize);
        bl_pull(interior_phases_contract_expand);

        ImGui::Spacing(); ImGui::Spacing();
        ImGui::Text("Max-depth result forwarding");
        if (ImGui::Combo("###MandelMaxDepthOptimization", &maxdepth_optimize, MandelMaxDepthOptimizationNames, (int)MandelMaxDepthOptimization::COUNT))
        {
            switch (maxdepth_optimize)
            {
            case MandelMaxDepthOptimization::SLOWEST: interior_phases_contract_expand = { 10, 0, 10, 0 }; break;
            case MandelMaxDepthOptimization::SLOW:    interior_phases_contract_expand = { 6, 2, 8, 2 };   break;
            case MandelMaxDepthOptimization::MEDIUM:  interior_phases_contract_expand = { 5, 3, 7, 3 };   break;
            case MandelMaxDepthOptimization::FAST:    interior_phases_contract_expand = { 1, 0, 1, 0 };   break;
            }

            bl_push(maxdepth_optimize);
        }

        bl_pull(maxdepth_show_optimized);
        ImGui::Checkbox("Highlight optimized regions", &maxdepth_show_optimized);
        bl_push(maxdepth_show_optimized);
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
        bl_pull(iter_weight);
        bl_pull(dist_weight);
        bl_pull(stripe_weight);

        bl_pull(cycle_iter_dynamic_limit);
        bl_pull(cycle_iter_normalize_depth);
        bl_pull(cycle_iter_log1p_weight);
        bl_pull(cycle_iter_value);

        bl_pull(cycle_dist_invert);
        bl_pull(cycle_dist_value);
        bl_pull(cycle_dist_sharpness);

        bl_pull(stripe_params);

        bl_pull(shade_formula);

        double iter_ratio, dist_ratio, stripe_ratio;
        shadingRatios(
            iter_weight, dist_weight, stripe_weight,
            iter_ratio, dist_ratio, stripe_ratio
        );

        char iter_header[64], dist_header[64], stripe_header[64];
        sprintf(iter_header, "Iteration  -  %d%% Weight", (int)(iter_ratio * 100.0));
        sprintf(dist_header, "Distance  -  %d%% Weight", (int)(dist_ratio * 100.0));
        sprintf(stripe_header, "Stripe  -  %d%% Weight", (int)(stripe_ratio * 100.0));

        // ────── Color Cycle: ITER ──────
        {
            ImGui::GroupBox box("iter_group", iter_header, scale_size(13.0f), scale_size(20.0f));
            ImGui::SliderDouble("Iter Ratio", &iter_weight, 0.0, 1.0, "%.2f");
            ImGui::Spacing(); ImGui::Spacing();

            if (ImGui::Checkbox("% of Max Iters", &cycle_iter_dynamic_limit))
            {
                if (cycle_iter_dynamic_limit)
                    cycle_iter_value /= iter_lim;
                else
                    cycle_iter_value *= iter_lim;
            }
            ImGui::SameLine(); ImGui::Dummy(ImVec2(10.0f, 0.0f)); ImGui::SameLine();

            bl_pull(use_smoothing);
            ImGui::Checkbox("Smooth", &use_smoothing);
            bl_push(use_smoothing);

            ImGui::Checkbox("Normalize to Visible Range", &cycle_iter_normalize_depth);


            float required_space = 0.0f;
            box.IncreaseRequiredSpaceForLabel(required_space, cycle_iter_dynamic_limit ? "% Iterations" : "Iterations");
            box.IncreaseRequiredSpaceForLabel(required_space, "Logarithmic");

            double raw_cycle_iters;
            if (cycle_iter_dynamic_limit)
            {
                double cycle_pct = cycle_iter_value * 100.0;

                box.SetNextItemWidthForSpace(required_space);
                ImGui::SliderDouble("% Iterations", &cycle_pct, 0.001, 100.0, "%.4f%%",
                    (/*color_cycle_use_log1p ?*/ ImGuiSliderFlags_Logarithmic /*: 0*/) |
                    ImGuiSliderFlags_AlwaysClamp);

                cycle_iter_value = cycle_pct / 100.0;

                raw_cycle_iters = finalIterLimit(cam_view, quality, dynamic_iter_lim, tweening) * cycle_iter_value;

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

        // ────── Color Cycle: DIST ──────
        {
            ImGui::GroupBox box("dist_group", dist_header);
            ImGui::SliderDouble("Dist Weight", &dist_weight, 0.0, 1.0, "%.2f");
            ImGui::Spacing(); ImGui::Spacing();

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

        // ────── Color Cycle: STRIPE ──────
        {
            ImGui::GroupBox box("stripe_group", stripe_header);
            ImGui::SliderDouble("Stripe Weight", &stripe_weight, 0.0, 1.0, "%.2f");
            ImGui::Spacing();
            ImGui::Spacing();

            ImGui::SliderFloat("Frequency", &stripe_params.freq, 1.0f, 100.0f, "%.0f", ImGuiSliderFlags_AlwaysClamp);
            ImGui::SliderFloat("Phase",     &stripe_params.phase, 0.0f, Math::PI*2.0f, "%.5f", ImGuiSliderFlags_AlwaysClamp);
            ImGui::SliderFloat("Contrast",  &stripe_params.contrast, 1.0f, 100.0f, "%.3f", ImGuiSliderFlags_AlwaysClamp);
        }

        ImGui::Combo("###MandelFormula", &shade_formula, MandelFormulaNames, (int)MandelShaderFormula::COUNT);

        bl_push(iter_weight);
        bl_push(dist_weight);
        bl_push(stripe_weight);

        bl_push(cycle_iter_dynamic_limit);
        bl_push(cycle_iter_normalize_depth);
        bl_push(cycle_iter_log1p_weight);
        bl_push(cycle_iter_value);

        bl_push(cycle_dist_invert);
        bl_push(cycle_dist_value);
        bl_push(cycle_dist_sharpness);

        bl_push(stripe_params);

        bl_push(shade_formula);
    }

    if (ImGui::Section("Color Offset + Animation", true, 5.0f, 2.0f))
    {
        // Shift
        bl_pull(gradient_shifted);
        bl_pull(hue_shift);
        bl_pull(gradient_shift);

        // Animation
        bl_pull(show_color_animation_options);
        bl_pull(gradient_shift_step);
        bl_pull(hue_shift_step);

        ImGui::Checkbox("Animate", &show_color_animation_options);

        float required_space = 0.0f;
        ImGui::IncreaseRequiredSpaceForLabel(required_space, "Gradient shift");

        ImGui::SetNextItemWidthForSpace(required_space);
        if (ImGui::DragDouble("Gradient shift", &gradient_shift, 0.01, -100.0, 100.0, " %.3f", ImGuiSliderFlags_AlwaysClamp))
        {
            gradient_shift = Math::wrap(gradient_shift, 0.0, 1.0);
            colors_updated = true;

            transformGradient(gradient_shifted, gradient, (float)gradient_shift, (float)hue_shift);
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
        {
            colors_updated = true;
            transformGradient(gradient_shifted, gradient, (float)gradient_shift, (float)hue_shift);
        }

        if (show_color_animation_options)
        {
            ImGui::Indent();
            ImGui::PushID("hue_increment");
            ImGui::SetNextItemWidthForSpace(required_space);
            ImGui::SliderDouble("Increment", &hue_shift_step, -5.0, 5.0, "%.3f", ImGuiSliderFlags_AlwaysClamp);
            ImGui::PopID();
            ImGui::Unindent();
        }


        ImGui::Spacing();
        ImGui::Text("Live preview");
        ImGui::GradientButton(&gradient_shifted, platform()->dpr());

        // Shift
        bl_push(hue_shift);
        bl_push(gradient_shift);

        // Animation
        bl_push(show_color_animation_options);
        bl_push(gradient_shift_step);
        bl_push(hue_shift_step);

    }

    if (ImGui::Section("Base Color Gradient", true, 5.0f, 2.0f))
    {
        ImGui::Text("Load Preset");
        static int selecting_template = -1;
        if (ImGui::Combo("###ColorTemplate", &selecting_template, ColorGradientNames, (int)GradientPreset::COUNT))
        {
            generateGradientFromPreset(gradient, (GradientPreset)selecting_template);

            selecting_template = -1;
            colors_updated = true;
        }

        if (ImGui::Button("Copy gradient C++ code"))
        {
            ImGui::SetClipboardText(gradient.to_cpp_marks().c_str());
        }

        ImGui::Dummy(scale_size(0, 8));

        if (ImGui::GradientEditor(&gradient,
            platform()->ui_scale_factor(), 
            platform()->ui_scale_factor(2.0f)))
        {
            colors_updated = true;

            // Shift
            bl_pull(gradient_shifted);
            bl_pull(hue_shift);
            bl_pull(gradient_shift);

            transformGradient(gradient_shifted, gradient, (float)gradient_shift, (float)hue_shift);
        }

    } // End Header

    /*if (ImGui::Section("Experimental")) {

        bl_pull(show_period2_bulb);
        bl_pull(flatten_amount);

        ImGui::Checkbox("Flatten", &flatten);

        if (flatten)
        {
            ImGui::Indent();
            if (ImGui::SliderDouble("Flatness", &flatten_amount, 0.0, 1.0, "%.2f"))
                cardioid_lerp_amount = 1.0 - flatten_amount;

            ImGui::Checkbox("Show period-2 bulb", &show_period2_bulb);
            ImGui::Unindent();
            ImGui::Dummy(scale_size(0, 10));
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

            bl_pull(x_spline);
            bl_pull(y_spline);

            static ImRect vr = { 0.0f, 0.8f, 0.8f, 0.0f };
            ImSpline::SplineEditorPair("X/Y Spline", &x_spline, &y_spline, &vr, 900.0f);

            bl_push(x_spline);
            bl_push(y_spline);
        }

        bl_push(show_period2_bulb);
        bl_push(flatten_amount);

    } // End Header
    */
    
    if (is_tweening)
        ImGui::EndDisabled();

    // ======== Developer ========
    #if MANDEL_DEV_EDIT_TWEEN_SPLINES
    bl_pull(tween_pos_spline);
    bl_pull(tween_zoom_lift_spline);
    bl_pull(tween_base_zoom_spline);

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

    bl_push(tween_pos_spline);
    bl_push(tween_zoom_lift_spline);
    bl_push(tween_base_zoom_spline);
    #endif


    bl_push(tweening);
    bl_push(cam_view);
    bl_push(dynamic_iter_lim);
    bl_push(quality);
    bl_push(iter_lim);

    if (colors_updated) bl_push(gradient);

    
    bl_push(colors_updated);

   

    // experimental
    bl_push(flatten);
}

void Mandelbrot_Scene::updateConfigBuffer()
{
    config_buf = serialize();
}

void Mandelbrot_Scene::loadConfigBuffer()
{
    deserialize(config_buf);
}

void Mandelbrot_Scene::onSavefileChanged()
{
    config_buf = serialize();
    #ifdef __EMSCRIPTEN__
    platform()->url_set_string("data", config_buf.c_str());
    #endif
}

// ────── Gradients ──────




// ────── Scene overrides ──────

void Mandelbrot_Scene::sceneStart()
{
    // todo: Stop this getting called twice on startup
    //loadGradientPreset(GradientPreset::CLASSIC);
    generateGradientFromPreset(gradient, GradientPreset::CLASSIC);

    cardioid_lerper.create(Math::TWO_PI / 5760.0, 0.005);

    font = NanoFont::create("/data/fonts/DroidSans.ttf");
}

void Mandelbrot_Scene::sceneMounted(Viewport* ctx)
{
    camera->setCameraStageSnappingSize(1);

    camera->setOriginViewportAnchor(Anchor::CENTER);
    camera->setDirectCameraPanning(true);
    camera->focusWorldRect(-2, -1.25, 1, 1.25);
    //camera->restrictRelativeZoomRange(0.5, 1e+300);

    cam_view.read(camera);
    cam_view.setCurrentAsDefault();

    reference_zoom = camera->getReferenceZoom<flt128>();
    ctx_stage_size = ctx->size();

    #ifdef __EMSCRIPTEN__
    if (platform()->url_has("data"))
    {
        config_buf = platform()->url_get_string("data");
        loadConfigBuffer();
    }
    #endif
}

void Mandelbrot_Scene::viewportProcess(Viewport* ctx, double dt)
{
    /// Process Viewports running this Scene
    ctx_stage_size = ctx->size();

    bool savefile_changed = false;

    if (Changed(show_color_animation_options, show_axis, gradient_shift_step, hue_shift_step))
        savefile_changed = true;

    // ────── Progressing animation ──────
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

    // ────── Tweening ──────
    if (tweening)
    {
        double speed = 0.4 / tween_duration;
        tween_progress += speed * dt;
        
        if (tween_progress < 1.0)
            lerpState(*this, state_a, state_b, tween_progress, false);
        else
        {
            lerpState(*this, state_a, state_b, 1.0, true);

            tween_progress = 0.0;
            tweening = false;
        }
    }

    // ────── Updating Shifted Gradient ──────
    if (first_frame || Changed(gradient, gradient_shift, hue_shift))
    {
        transformGradient(gradient_shifted, gradient, (float)gradient_shift, (float)hue_shift);
        savefile_changed = true;
        colors_updated = true;
    }

    // ────── Calculate Depth Limit ──────
    iter_lim = finalIterLimit(cam_view, quality, dynamic_iter_lim, tweening);

    // ────── Flattening ──────
    #if MANDEL_FEATURE_FLATTEN_MODE
    if (Changed(flatten))
    {
        if (flatten) flatten_amount = 0.0;
        camera->focusWorldRect(-2, -1, 2, 1);
        cam_view.zoom = camera->getRelativeZoom<flt128>().x;
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
        cam_view.zoom = camera->getRelativeZoom<flt128>().x;
    }
    #endif

    // ────── Camera View ──────
    {
        cam_view.apply(camera);

        #if MANDEL_FEATURE_CAMERA_EASING
        // Process camera velocity
        if (mouse->pressed)
        {
            camera_vel_pos = DDVec2{};

            // Stop zoom velocity on touch
            //if (platform()->is_mobile())
            camera_vel_zoom = 1.0;
        }
        else
        {
            flt128 threshold = cam_view.zoom / flt128{ 1000.0 };
            if (camera_vel_pos.magnitude() > threshold)
                camera->setPos(camera->pos128() + camera_vel_pos);
            else
                camera_vel_pos = DDVec2{};

            if (fabs(camera_vel_zoom - 1.0) > 0.01)
                camera->setZoom(camera->zoom128() * f128(camera_vel_zoom));
            else
                camera_vel_zoom = 1.0;

            camera_vel_pos *= flt128{ 0.8 };
            camera_vel_zoom += (1.0 - camera_vel_zoom) * 0.2;
        }
        #endif

        cam_view.read(camera);
    }

    // ────── Ensure size divisble by 9 for result forwarding from: [9x9] to [3x3] to [1x1] ────── 
    int iw = (int)(ceil(ctx->width() / 9)) * 9;
    int ih = (int)(ceil(ctx->height() / 9)) * 9;

    world_quad = camera->toWorldQuad<flt128>(0, 0, iw, ih);

    // ────── Any properties changed that require restarting compute? ──────
    bool view_changed         = Changed(world_quad);
    bool quality_opts_changed = Changed(quality, dynamic_iter_lim, smoothing_type, maxdepth_optimize, maxdepth_show_optimized);
    bool splines_changed      = Changed(x_spline.hash(), y_spline.hash());
    bool flatten_changed      = Changed(flatten, show_period2_bulb, cardioid_lerp_amount);
    bool compute_opts_changed = Changed(stripe_params);

    bool mandel_changed = (first_frame || view_changed || quality_opts_changed || compute_opts_changed || splines_changed || flatten_changed);

    if (mandel_changed)
        savefile_changed = true;

    // ────── Upgrade mode if new feature, downgrade if lost feature & mandel changed  ──────
    {
        constexpr double eps = std::numeric_limits<double>::epsilon();
        int old_smoothing = smoothing_type;
        int new_smoothing = 0;

        if (iter_weight > eps)   new_smoothing |= (int)MandelSmoothing::ITER;
        if (dist_weight > eps)   new_smoothing |= (int)MandelSmoothing::DIST;
        if (stripe_weight > eps) new_smoothing |= (int)MandelSmoothing::STRIPES;

        bool force_upgrade       = new_smoothing & ~old_smoothing;
        bool downgrade_on_change = ~new_smoothing & old_smoothing;

        if (force_upgrade || (mandel_changed && downgrade_on_change))
        {
            mandel_changed = true;
            smoothing_type = new_smoothing;
        }
    }

    // ────── Presented Mandelbrot *actually* changed? Restart on 9x9 bmp (phase 0) ──────
    if (mandel_changed)
    {
        bmp_9x9.clear(0, 255, 0, 255);
        bmp_3x3.clear(0, 255, 0, 255);
        bmp_1x1.clear(0, 255, 0, 255);

        // Hide intro
        if (!first_frame)
            display_intro = false;

        computing_phase = 0;
        current_row = 0;
        field_9x9.setAllDepth(-1.0);

        #if MANDEL_DEV_PERFORMANCE_TIMERS
        compute_t0 = std::chrono::steady_clock::now();
        #endif
    }

    // ────── Prepare bmp / depth-field dimensions and view rect ──────
    {
        // Set pending bitmap & pending field
        switch (computing_phase)
        {
            case 0:  pending_bmp = &bmp_9x9;  pending_field = &field_9x9;  break;
            case 1:  pending_bmp = &bmp_3x3;  pending_field = &field_3x3;  break;
            case 2:  pending_bmp = &bmp_1x1;  pending_field = &field_1x1;  break;
        }

        pending_bmp->setStageRect(0, 0, iw, ih);

        bmp_9x9.setBitmapSize(iw / 9, ih / 9);
        bmp_3x3.setBitmapSize(iw / 3, ih / 3);
        bmp_1x1.setBitmapSize(iw, ih);

        field_9x9.setDimensions(iw / 9, ih / 9);
        field_3x3.setDimensions(iw / 3, ih / 3);
        field_1x1.setDimensions(iw, ih);
    }


    bool do_compute = false;
    bool finished_compute = false;

    bool phase_changed = Changed(computing_phase);

    // ────── Start new (or resume an ongoing) compute? ──────
    if (mandel_changed   ||  // Has ANY option would would alter the final mandelbrot changed?
        phase_changed    ||  // Finished computing last phase, begin computing next phase
        current_row != 0)    // Not finished computing current phase, resume computing current phase
    {
        do_compute = true;
    }

    // ────── Compute Mandelbrot & Normalize ──────
    if (do_compute)
    {
        //if (!flatten)
        {
            //DQuad quad = ctx->worldQuad();
            //bool x_axis_visible = quad.intersects({ {quad.minX(), 0}, {quad.maxX(), 0} });

            // Run appropriate kernel for given settings
            finished_compute = frame_complete = compute_mandelbrot();

            // Normalize the resulting depth/dist/stripe
            ///normalize_shading_limits(
            ///    pending_field, pending_bmp, cam_view,
            ///
            ///    cycle_iter_dynamic_limit,
            ///    cycle_iter_normalize_depth,
            ///    cycle_iter_log1p_weight,
            ///    cycle_iter_value,
            ///
            ///    cycle_dist_invert,
            ///    cycle_dist_sharpness,
            ///    cycle_dist_value
            ///);
            ///
            ///normalize_field(pending_field, pending_bmp);
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

        if (finished_compute)
        {
            // Finished computing pending_field, set as active field and use for future color updates
            active_bmp = pending_bmp;
            active_field = pending_field;

            // ======== Result forwarding ========
        
            switch (computing_phase) {
                case 0:
                    field_3x3.setAllDepth(-1.0);
                    bmp_9x9.forEachPixel([this](int x, int y)
                    {
                        field_3x3(x*3+1, y*3+1) = field_9x9(x, y);

                        if (!field_9x9.has_data(x, y))
                            field_9x9(x, y).flag_for_skip = true;
                    });

                    field_9x9.contractSkipFlags(interior_phases_contract_expand.c1);
                    field_9x9.expandSkipFlags(interior_phases_contract_expand.e1);

                    bmp_9x9.forEachPixel([this](int x, int y)
                    {
                        if (field_9x9(x, y).flag_for_skip)
                        {
                            int x0 = x * 3, y0 = y * 3;
                            for (int py = y0; py <= y0 + 3; py++)
                                for (int px = x0; px <= x0 + 3; px++)
                                    field_3x3(px, py).flag_for_skip = true;
                        }
                    });

                    break;

                case 1:
                    field_1x1.setAllDepth(-1.0);
                    bmp_3x3.forEachPixel([this](int x, int y)
                    {
                        field_1x1(x*3+1, y*3+1) = field_3x3(x, y);

                        if (!field_3x3.has_data(x, y))
                            field_3x3(x, y).flag_for_skip = true;
                    });

                    field_3x3.contractSkipFlags(interior_phases_contract_expand.c2);
                    field_3x3.expandSkipFlags(interior_phases_contract_expand.e2);

                    bmp_3x3.forEachPixel([this](int x, int y)
                    {
                        if (field_3x3(x, y).flag_for_skip)
                        {
                            int x0 = x * 3, y0 = y * 3;
                            for (int py = y0; py <= y0 + 3; py++)
                            {
                                for (int px = x0; px <= x0 + 3; px++)
                                {
                                    EscapeFieldPixel& pixel = field_1x1(px, py);
                                    pixel.depth = INSIDE_MANDELBROT_SET_SKIPPED;
                                    pixel.flag_for_skip = true;
                                }
                            }
                        }
                    });

                    break;

                case 2:
                    // Finish final 1x1 compute
                    #if MANDEL_DEV_PERFORMANCE_TIMERS
                    auto elapsed = std::chrono::steady_clock::now() - compute_t0;
                    double dt = std::chrono::duration<double, std::milli>(elapsed).count();
                    dt_avg = timer_ma.push(dt);
                    //blPrint() << "Compute timer: " << bl::to_fixed(4) << dt_avg;
                    #endif
                    break;
            }

            if (computing_phase < 2)
                computing_phase++;
        }
    }

    // ────── Color cycle changed? ──────
    bool shade_formula_changed    = Changed(shade_formula);
    bool weights_changed          = Changed(iter_weight, dist_weight, stripe_weight);
    bool cycle_iter_opts_changed  = Changed(cycle_iter_value, cycle_iter_log1p_weight, cycle_iter_normalize_depth);
    bool cycle_dist_opts_changed  = Changed(cycle_dist_value, cycle_dist_invert, cycle_dist_sharpness);

    if (shade_formula_changed || weights_changed || cycle_iter_opts_changed || cycle_dist_opts_changed)
    {
        // Reshade active_bmp
        colors_updated = true;
        savefile_changed = true;
    }

    //if (finished_compute)
    //    colors_updated = true;

    bool renormalize = finished_compute || cycle_iter_opts_changed || cycle_dist_opts_changed || shade_formula_changed || weights_changed;
    bool reshade_bmp = renormalize || (colors_updated && frame_complete);

    // ────── Renormalize cached data if necessary  ──────
    if (renormalize)
    {
        normalize_shading_limits(
            active_field, active_bmp, cam_view,

            cycle_iter_dynamic_limit,
            cycle_iter_normalize_depth,
            cycle_iter_log1p_weight,
            cycle_iter_value,

            cycle_dist_invert,
            cycle_dist_sharpness,
            cycle_dist_value
        );

        normalize_field(active_field, active_bmp);
    }

    // ────── Refresh normalized values if styles change, or if we do full compute ──────
    if (reshade_bmp)
    {
        table_invoke(
            build_table(shadeBitmap, [&], active_field, active_bmp, &gradient_shifted, /*(float)log_color_cycle_iters, (float)cycle_dist_value,*/ (float)iter_weight, (float)dist_weight, (float)stripe_weight, numThreads()),
            (MandelShaderFormula)shade_formula, maxdepth_show_optimized
        );

        colors_updated = false;
        savefile_changed = true;
    }


    first_frame = false;

    if (savefile_changed)
        onSavefileChanged();
}

void Mandelbrot_Scene::viewportDraw(Viewport* ctx) const
{
    // Draw active phase bitmap
    if (active_bmp)
        ctx->drawImage(*active_bmp, active_bmp->worldQuad());

    //if (show_axis)
    //    ctx->drawWorldAxis(0.5, 0, 0.5);

    #if MANDEL_FEATURE_INTERACTIVE_CARDIOID
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
    #endif

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

    //DQuad quad = ctx->worldQuad();
    //bool x_axis_visible = quad.intersects({ {quad.minX(), 0}, {quad.maxX(), 0}});
    //ctx->print() << "\nx_axis_visible: " << (x_axis_visible ? "true" : "false");

    //ctx->printTouchInfo();

    #if MANDEL_DEV_PRINT_ACTIVE_FLOAT_TYPE
    {
        ctx->print() << "Quality:   " << MandelFloatQualityNames[getRequiredFloatType((MandelSmoothing)smoothing_type, cam_view.zoom)] << "\n";
    }
    #endif


    #if MANDEL_DEV_PRINT_ACTIVE_COMPUTE_FEATURES
    {
        ctx->print() << "Computing: ";
        for (int i = 0, j = 0; i <= 4; i++) {
            if (smoothing_type & (1<<i))  {
                if (j++ != 0) ctx->print() << " | ";
                ctx->print() << MandelSmoothingNames[i+1];
            }
        }
        ctx->print() << "\n";
    }
    #endif

    #if MANDEL_DEV_PERFORMANCE_TIMERS
    {
        ctx->print() << "Compute timer: " << dt_avg << "\n";
    }
    #endif

    camera->stageTransform();

    #ifndef BL_DEBUG
    if (display_intro)
    {
        ctx->setFont(font);  
        ctx->setFontSize(20);

        //if (!platform()->is_desktop_native())
        {
            ctx->fillText("Controls:", scale_size(10), scale_size(10));
            ctx->fillText("  - Touch & drag to move", scale_size(10), scale_size(35));
            ctx->fillText("  - Pinch to zoom / rotate", scale_size(10), scale_size(60));
        }

        ctx->setTextAlign(TextAlign::ALIGN_LEFT);
        ctx->setTextBaseline(TextBaseline::BASELINE_BOTTOM);
        //ctx->fillText("Developer:    Will Hemsworth", scale_size(10), ctx->height() - scale_size(32));
        ctx->fillText("Contact:  will.hemsworth@bitloop.dev", scale_size(10), ctx->height() - scale_size(10));
    }
    #endif

    //~ // Wont work nicely in slow scene, needs to be imgui or own it's own thread/layer/nanovg-overlay.
      // uiOverlay(ctx) for both nanovg and imgui (on gui main thread)

    //ctx->print() << "camera_vel: " << camera_vel_pos << "\n";
    ///for (auto pair : ui_stage)
    ///{
    ///    auto entry = pair.second;
    ///    ctx->print() << entry.name << " live value: " << entry.to_string_value() << "\n";
    ///    ctx->print() << entry.name << " live mrked: " << entry.to_string_marked_live() << "\n";
    ///    ctx->print() << entry.name << " shdw mrked: " << entry.to_string_marked_shadow() << "\n";
    ///}
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
    const DVec2  s = scale_size(40, 40);
    const double pad = scale_size(6);
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
    ctx->setLineWidth(scale_size(2));

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

        if (!platform()->is_mobile())
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
