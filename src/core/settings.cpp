#include <bitloop/core/settings.h>
#include <bitloop/core/main_window.h>

#include <SDL3/SDL_dialog.h>

BL_BEGIN_NS;

#ifndef __EMSCRIPTEN__
static void on_folder_chosen([[maybe_unused]] void* userdata, const char* const* filelist, [[maybe_unused]] int filter)
{
    if (!filelist) { SDL_Log("Dialog error"); return; }
    if (!*filelist) { SDL_Log("User canceled"); return; }
    blPrint() << "Chosen folder: " << filelist[0]; // first (and only) entry for folder dialog
}

static inline double lerp_log(double a, double b, double t) {
    return std::exp(std::log(a) + t * (std::log(b) - std::log(a)));
}

static inline bool is_x265(CaptureFormat format)
{
    return format == CaptureFormat::x265;
    //if (!s) return false;
    //std::string t(s); for (auto& c : t) c = (char)tolower(c);
    //return (t == "x265" || t == "hevc" || t == "h265");
}

// Range depends ONLY on resolution, fps, codec
static inline BitrateRange recommended_bitrate_range_mbps(IVec2 res, int fps, CaptureFormat format)
{
    int w = std::max(16, res.x);
    int h = std::max(16, res.y);
    fps = std::clamp(fps, 1, 120);

    // Broad coverage band for simple->extreme screen content
    // (bppf = bits per pixel per frame)
    const double bppf_min = 0.006; // very simple scenes
    const double bppf_max = 0.70;  // near-lossless-ish, extreme detail/motion

    // Codec efficiency: x265 typically ~30% less for same quality
    const double eff = is_x265(format) ? 0.70 : 1.00;

    const double ppf = (double)w * (double)h * (double)fps;
    double min_bps = ppf * bppf_min * eff;
    double max_bps = ppf * bppf_max * eff;

    BitrateRange r;
    r.min_mbps = std::clamp(min_bps / 1e6, 0.05, 1000.0);
    r.max_mbps = std::clamp(max_bps / 1e6, r.min_mbps, 1000.0);
    return r;
}

static inline double choose_bitrate_mbps_from_range(const BitrateRange& r, int quality_0_to_100)
{
    int q = std::clamp(quality_0_to_100, 0, 100);
    double t = q / 100.0;                  // 0..1
    return lerp_log(r.min_mbps, r.max_mbps, t); // same curve/endpoints as before
}
#endif

void SettingsPanel::init()
{
    #if BITLOOP_FFMPEG_ENABLED
    config.record_bitrate_mbps_range = recommended_bitrate_range_mbps(
        config.getRecordResolution(),
        config.record_fps,
        config.getRecordFormat());

    config.record_bitrate = (int64_t)(1000000.0 * choose_bitrate_mbps_from_range(config.record_bitrate_mbps_range, config.record_quality));
    #endif

    SnapshotPresetManager* manager = main_window->getSnapshotPresetManager();
    SnapshotPresetList& presets = manager->allPresets();
    CapturePreset* selected_preset = presets.at_safe(selected_capture_preset);
    setInputPreset(selected_preset);
}

void SettingsPanel::populateCapturePresetsEditor()
{
    bool is_capturing = main_window->getCaptureManager()->isCapturing();
    if (is_capturing)
        ImGui::BeginDisabled();

    SnapshotPresetManager* manager = main_window->getSnapshotPresetManager();
    SnapshotPresetList& presets = manager->allPresets();

    CapturePreset* selected_preset = presets.at_safe(selected_capture_preset);

    if (ImGui::Button("New target"))
    {
        CapturePreset new_preset;
        new_preset.setName("unnamed");
        new_preset.setResolution(1920, 1080);

        presets.add(new_preset);

        selected_capture_preset = presets.size() - 1;
        selected_preset = presets.at_safe(selected_capture_preset);
        setInputPreset(selected_preset);
    }

    if (selected_preset)
    {
        ImGui::SameLine();
        if (ImGui::Button("Duplicate target"))
        {
            CapturePreset new_preset(*selected_preset);
            new_preset.setAlias(std::string(new_preset.getAlias()) + "_clone");
            presets.add(new_preset);

            selected_capture_preset = presets.size() - 1;
            selected_preset = presets.at_safe(selected_capture_preset);
            setInputPreset(selected_preset);
        }
    }

    int changed_selected_index = populateCapturePresetsList([&](int i) -> CapturePreset& {
        return presets[i];
    }, (int)presets.size(), nullptr, selected_capture_preset);

    if (changed_selected_index >= 0)
    {
        selected_preset = presets.at_safe(selected_capture_preset);
        setInputPreset(selected_preset);
    }

    if (selected_preset)
    {
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        if (ImGui::BeginTable("##preset_table", 2, ImGuiTableFlags_SizingStretchProp))
        {
            ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed);
            ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);

            // Alias (only permit unique, otherwise revert)
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted("Alias");
            ImGui::TableSetColumnIndex(1);
            ImGui::SetNextItemWidth(-FLT_MIN);

            auto FilterLowerNumUnderscore = [](ImGuiInputTextCallbackData* data)
            {
                if (data->EventFlag != ImGuiInputTextFlags_CallbackCharFilter) return 0;
                ImWchar c = data->EventChar;
                if (c >= 'A' && c <= 'Z') { data->EventChar = (ImWchar)(c - 'A' + 'a'); return 0; } // convert to lowercase
                if ((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_') return 0; // allow: a-z, 0-9, underscore
                return 1; // Reject everything else
            };

            if (ImGui::InputText("##preset_alias", input_alias, selected_preset->maxAliasSize(),
                ImGuiInputTextFlags_CallbackCharFilter, FilterLowerNumUnderscore))
            {
                if (presets.isUniqueAliasChange(selected_capture_preset, input_alias))
                {
                    selected_preset->setAlias(input_alias);
                    presets.updateLookup();
                }
                else // revert
                {
                    strcpy(input_alias, revert_alias);
                }
            }

            ImGui::SetItemTooltip("Unique alias used for ID and filenames");

            // Name
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted("Description");
            ImGui::TableSetColumnIndex(1);
            ImGui::SetNextItemWidth(-FLT_MIN);
            if (ImGui::InputText("##preset_name", input_name, selected_preset->maxNameSize()))
            {
                selected_preset->setName(input_alias);
            }

            // Width
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted("Width");
            ImGui::TableSetColumnIndex(1);
            ImGui::SetNextItemWidth(-FLT_MIN);
            if (ImGui::InputInt("##res_x", &input_resolution.x, 10))
            {
                input_resolution.x = std::clamp(input_resolution.x, 16, 16384);
                input_resolution.x = (input_resolution.x & ~1); // force even
                selected_preset->setResolution(input_resolution);
            }

            // Height
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted("Height");
            ImGui::TableSetColumnIndex(1);
            ImGui::SetNextItemWidth(-FLT_MIN);
            if (ImGui::InputInt("##res_y", &input_resolution.y, 10))
            {
                input_resolution.y = std::clamp(input_resolution.y, 16, 16384);
                input_resolution.y = (input_resolution.y & ~1); // force even
                selected_preset->setResolution(input_resolution);
            }

            // Supersampling
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::AlignTextToFramePadding();

            bool had_specific_ssaa = input_ssaa > 0;
            bool use_specific_ssaa = had_specific_ssaa;
            ImGui::Checkbox("Supersample", &use_specific_ssaa);

            if (!had_specific_ssaa && use_specific_ssaa)
            {
                input_ssaa = 1; // enable
                selected_preset->setSSAA(input_ssaa);
            }
            else if (had_specific_ssaa && !use_specific_ssaa)
            {
                input_ssaa = 0; // disable
                selected_preset->setSSAA(input_ssaa);
            }

            ImGui::TableSetColumnIndex(1);
            ImGui::SetNextItemWidth(-FLT_MIN);
            if (!use_specific_ssaa) ImGui::BeginDisabled();
            if (ImGui::SliderInt("##ssaa",
                use_specific_ssaa ? &input_ssaa : &config.default_ssaa,
                1, 8,
                use_specific_ssaa ? "%dx" : "Using global (%dx)"))
            {
                selected_preset->setSSAA(input_ssaa);
            }

            if (!use_specific_ssaa) ImGui::EndDisabled();

            // Sharpening
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::AlignTextToFramePadding();
            bool had_specific_sharpen = input_sharpen >= 0.0;
            bool use_specific_sharpen = had_specific_sharpen;
            ImGui::Checkbox("Sharpening", &use_specific_sharpen);

            if (!had_specific_sharpen && use_specific_sharpen)
            {
                input_sharpen = 0.0f; // enable (non-negative)
                selected_preset->setSharpening(input_sharpen);
            }
            else if (had_specific_sharpen && !use_specific_sharpen)
            {
                input_sharpen = -1.0f; // disable (negative)
                selected_preset->setSharpening(input_sharpen);
            }

            ImGui::TableSetColumnIndex(1);
            ImGui::SetNextItemWidth(-FLT_MIN);
            if (!use_specific_sharpen) ImGui::BeginDisabled();

            if (ImGui::SliderFloat("##sharpen",
                use_specific_sharpen ? &input_sharpen : &config.default_sharpen,
                0.0f, 1.0f,
                use_specific_sharpen ? "%.2f" : "Using global (%.2f)"))
            {
                selected_preset->setSharpening(input_sharpen);
            }

            if (!use_specific_sharpen) ImGui::EndDisabled();
            ImGui::EndTable();
        }
    }

    if (is_capturing)
        ImGui::EndDisabled();
}

void SettingsPanel::populateSettings()
{
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, scale_size(6, 6));
    //ImGui::BeginChild("RecordingFrame", ImVec2(0, 0), 0, ImGuiWindowFlags_AlwaysUseWindowPadding);

    // Paths
    #ifndef __EMSCRIPTEN__
    ImGui::BeginLabelledBox("Paths");
    ImGui::Text("Media Output Directory:");
    ImGui::InputText("##folder", &config.capture_dir, ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_ElideLeft);
    ImGui::SameLine();
    if (ImGui::Button("..."))
    {
        SDL_ShowOpenFolderDialog(on_folder_chosen, NULL, platform()->sdl_window(), config.capture_dir.c_str(), false);
    }
    ImGui::EndLabelledBox();
    #endif

    // Global Options
    {
        ImGui::BeginLabelledBox("Global Capture Options");

        ImGui::Checkbox("Preview Selected Preset", &config.preview_mode);

        if (!platform()->is_mobile())
        {
            ImGui::Checkbox("Fixed time-delta", &config.fixed_time_delta);
        }

        ImGui::Spacing();
        ImGui::Spacing();
        ImGui::Text("FPS:");
        if (ImGui::InputInt("##fps", &config.record_fps, 1))
        {
            config.record_fps = std::clamp(config.record_fps, 1, 100);
        }

        ImGui::Spacing();
        ImGui::Spacing();
        ImGui::Text("Supersample Factor (SSAA)");
        //ImGui::SliderInt("##ssaa", &snapshot_supersample_factor, 1, 8, "%dx");
        ImGui::SliderInt("##ssaa", &config.default_ssaa, 1, 8, "%dx");

        ImGui::Spacing(); ImGui::Spacing();
        ImGui::Text("Sharpen");
        //ImGui::SliderFloat("##sharpen", &snapshot_sharpen, 0.0f, 1.0f, "%.2f");
        ImGui::SliderFloat("##sharpen", &config.default_sharpen, 0.0f, 1.0f, "%.2f");

        ImGui::EndLabelledBox();
    }

    ///// Capture Presets
    ///{
    ///    ImGui::BeginLabelledBox("Capture Presets");
    ///
    ///    //static int sel_aspect = -1;
    ///    //static int sel_tier = -1;
    ///    //constexpr IVec2 pixels_8k{ 7680, 4320 };
    ///    //static u64 max_pixels = (u64)pixels_8k.x * (u64)pixels_8k.y;
    ///    //static bool first = true;
    ///    ///populateResolutionOptions(snapshot_resolution, first, sel_aspect, sel_tier, max_pixels);
    ///
    ///    populateCapturePresetsEditor();
    ///
    ///    ImGui::EndLabelledBox();
    ///}

    if (ImGui::BeginTabBar("capture_tabs"))
    {
        if (ImGui::TabBox("Image"))
        {
            selected_preset_is_video = false;

            SnapshotPresetManager* manager = main_window->getSnapshotPresetManager();
            SnapshotPresetList& presets = manager->allPresets();

            populateCapturePresetsList<CapturePresetsSelectMode::MULTI>(
                [&](int i) -> CapturePreset& {
                return presets[i];
            }, (int)presets.size(), &config.target_image_presets, selected_image_preset);

            ImGui::EndTabBox();
        }

        if (ImGui::TabBox("Video"))
        {
            selected_preset_is_video = true;

            SnapshotPresetManager* manager = main_window->getSnapshotPresetManager();
            SnapshotPresetList& presets = manager->allPresets();

            // Use target_video_preset as both selected item index and chosen radio button index
            populateCapturePresetsList<CapturePresetsSelectMode::SINGLE>(
                [&](int i) -> CapturePreset& {
                return presets[i];
            }, (int)presets.size(), nullptr, config.target_video_preset);

            ImGui::Spacing(); 

            ImGui::Text("Codec:");
            if (ImGui::Combo("##codec", &config.record_format, CaptureFormatComboString()))
            {
                #if BITLOOP_FFMPEG_X265_ENABLED
                config.record_bitrate_mbps_range = recommended_bitrate_range_mbps(
                    config.getRecordResolution(),
                    config.record_fps,
                    config.getRecordFormat());
                #endif
            }

            ImGui::Spacing();
            ImGui::Spacing();
            ImGui::Text("Quality:");

            if (ImGui::SliderInt("##quality", &config.record_quality, 0, 100))
            {
                /// TODO: Apply quality to WebP

                #if BITLOOP_FFMPEG_ENABLED
                config.record_bitrate = (int64_t)(1000000.0 * choose_bitrate_mbps_from_range(config.record_bitrate_mbps_range, config.record_quality));
                #endif
            }

            #if BITLOOP_FFMPEG_ENABLED
            ImGui::Text("= %.1f Mbps", (double)config.record_bitrate / 1000000.0);
            #endif

            ImGui::Spacing();
            ImGui::Spacing();
            ImGui::Checkbox("Lossless", &config.record_lossless);

            if (config.record_lossless)
            {
                ImGui::Spacing();
                ImGui::Spacing();
                ImGui::Text("Lossless quality:");
                ImGui::SliderInt("##near_lossless", &config.record_near_lossless, 0, 100);
            }

            ImGui::Spacing();
            ImGui::Spacing();
            ImGui::Text("Frame Count (0 = No Limit):");
            if (ImGui::InputInt("##frame_count", &config.record_frame_count, 1))
            {
                config.record_frame_count = std::clamp(config.record_frame_count, 0, 10000000);
            }

            ImGui::EndTabBox();
        }

        if (ImGui::TabBox("Edit Presets"))
        {
            populateCapturePresetsEditor();

            ImGui::EndTabBox();
        }

        ImGui::EndTabBar();
    }
    

    //ImGui::EndChild();
    ImGui::PopStyleVar();

    ImGui::Text("Compiled:  %s %s", __DATE__, __TIME__);
}

BL_END_NS;
