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

static inline bool is_x265(const char* s)
{
    if (!s) return false;
    std::string t(s); for (auto& c : t) c = (char)tolower(c);
    return (t == "x265" || t == "hevc" || t == "h265");
}

// Range depends ONLY on resolution, fps, codec
static inline BitrateRange recommended_bitrate_range_mbps(IVec2 res, int fps, const char* codec)
{
    int w = std::max(16, res.x);
    int h = std::max(16, res.y);
    fps = std::clamp(fps, 1, 120);

    // Broad coverage band for simple->extreme screen content
    // (bppf = bits per pixel per frame)
    const double bppf_min = 0.006; // very simple scenes
    const double bppf_max = 0.70;  // near-lossless-ish, extreme detail/motion

    // Codec efficiency: x265 typically ~30% less for same quality
    const double eff = is_x265(codec) ? 0.70 : 1.00;

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
        config.record_resolution,
        config.record_fps,
        VideoCodecFromCaptureFormat(config.getRecordFormat()));

    config.record_bitrate = (int64_t)(1000000.0 * choose_bitrate_mbps_from_range(config.record_bitrate_mbps_range, config.record_quality));
    #endif
}

void SettingsPanel::populateCapturePresets()
{
    SnapshotPresetManager* manager = main_window->getSnapshotPresetManager();
    SnapshotPresetList& presets = manager->allPresets();
    SnapshotPresetHashMap& enabled_presets = manager->enabledHashes();

    SnapshotPreset* selected_preset = presets.at_safe(selected_capture_preset);

    if (ImGui::Button("New capture target"))
    {
        SnapshotPreset new_preset;
        strcpy(new_preset.name, "unnamed");
        new_preset.size.set(1920, 1080);
        new_preset.updateCache();

        presets.add(new_preset);
        presets.updateLookup();
    }

    int changed_selected_index = populateCapturePresetsList([&](int i) -> SnapshotPreset& {
        return presets[i];
    }, (int)presets.size(), enabled_presets, selected_capture_preset);

    if (changed_selected_index >= 0)
    {
        selected_preset = presets.at_safe(selected_capture_preset);
        strcpy(input_alias, selected_preset->alias);
        strcpy(revert_alias, selected_preset->alias);
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

            if (ImGui::InputText("##preset_alias", input_alias, sizeof(selected_preset->alias),
                ImGuiInputTextFlags_CallbackCharFilter, FilterLowerNumUnderscore))
            {
                if (presets.isUniqueAliasChange(selected_capture_preset, input_alias))
                {
                    strcpy(selected_preset->alias, input_alias);
                    selected_preset->updateCache();
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
            if (ImGui::InputText("##preset_name", selected_preset->name, sizeof(selected_preset->name)))
            {
                selected_preset->updateCache();
            }

            // Width
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted("Width");
            ImGui::TableSetColumnIndex(1);
            ImGui::SetNextItemWidth(-FLT_MIN);
            if (ImGui::InputInt("##res_x", &selected_preset->size.x, 10))
            {
                selected_preset->size.x = std::clamp(selected_preset->size.x, 16, 16384);
                selected_preset->size.x = (selected_preset->size.x & ~1); // force even
                selected_preset->updateCache();
            }

            // Height
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted("Height");
            ImGui::TableSetColumnIndex(1);
            ImGui::SetNextItemWidth(-FLT_MIN);
            if (ImGui::InputInt("##res_y", &selected_preset->size.y, 10))
            {
                selected_preset->size.y = std::clamp(selected_preset->size.y, 16, 16384);
                selected_preset->size.y = (selected_preset->size.y & ~1); // force even
                selected_preset->updateCache();
            }

            // Supersampling
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::AlignTextToFramePadding();

            bool had_specific_ssaa = selected_preset->ssaa > 0;
            bool use_specific_ssaa = had_specific_ssaa;
            ImGui::Checkbox("Supersample", &use_specific_ssaa);

            if (!had_specific_ssaa && use_specific_ssaa)
                selected_preset->ssaa = 1; // enable
            else if (had_specific_ssaa && !use_specific_ssaa)
                selected_preset->ssaa = 0; // disable

            ImGui::TableSetColumnIndex(1);
            ImGui::SetNextItemWidth(-FLT_MIN);
            if (!use_specific_ssaa) ImGui::BeginDisabled();
            ImGui::SliderInt("##ssaa",
                use_specific_ssaa ? &selected_preset->ssaa : &config.default_ssaa,
                1, 8,
                use_specific_ssaa ? "%dx" : "Using global (%dx)");

            if (!use_specific_ssaa) ImGui::EndDisabled();

            // Sharpening
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::AlignTextToFramePadding();
            bool had_specific_sharpen = selected_preset->sharpen >= 0.0;
            bool use_specific_sharpen = had_specific_sharpen;
            ImGui::Checkbox("Sharpening", &use_specific_sharpen);
            if (!had_specific_sharpen && use_specific_sharpen)
                selected_preset->sharpen = 0.0f; // enable
            else if (had_specific_sharpen && !use_specific_sharpen)
                selected_preset->sharpen = -1.0f; // disable

            ImGui::TableSetColumnIndex(1);
            ImGui::SetNextItemWidth(-FLT_MIN);
            if (!use_specific_sharpen) ImGui::BeginDisabled();

            ImGui::SliderFloat("##sharpen",
                use_specific_sharpen ? &selected_preset->sharpen : &config.default_sharpen,
                0.0f, 1.0f,
                use_specific_sharpen ? "%.2f" : "Using global (%.2f)");

            if (!use_specific_sharpen) ImGui::EndDisabled();
            ImGui::EndTable();
        }
    }
}

void SettingsPanel::populateResolutionOptions(IVec2& targ_res, bool& first, int& sel_aspect, int& sel_tier, u64 max_pixels)
{
    struct Aspect { const char* label; bool portrait; };
    struct Preset { int w, h; const char* label; bool HEVC_only = false; };

    static const Aspect aspects[] = {
        {"16:9", false},
        {"21:9", false},
        {"32:9", false},
        {"4:3",  false},
        {"3:2",  false},
        {"1:1",  false},
        {"9:16", true },
    };

    // 16:9 — compact, common
    static const Preset presets_16_9[] = {
        {1280,  720, "720p"},
        {1920, 1080, "1080p"},
        {2560, 1440, "1440p"},
        {3840, 2160, "4K"},
        {5120, 2880, "5K", true},
        {7680, 4320, "8K", true},
    };

    // 21:9 — ultrawide
    static const Preset presets_21_9[] = {
        {2560, 1080, "FHD"},
        {3440, 1440, "QHD"},
        {3840, 1600, "WQHD+"},
        {5120, 2160, "5K2K", true},
    };

    // 32:9 — super-ultrawide (dual-monitor equiv.)
    static const Preset presets_32_9[] = {
        {3840, 1080, "FHD"},
        {5120, 1440, "QHD"},
        {7680, 2160, "8K-wide", true},
    };

    // 4:3 — classic VESA
    static const Preset presets_4_3[] = {
        {1024,  768, "XGA"},
        {1600, 1200, "UXGA"},
        {2048, 1536, "QXGA"},
    };

    // 3:2 — laptop/document
    static const Preset presets_3_2[] = {
        {1920, 1280, "1280p"},
        {2160, 1440, "1440p"},
        {3000, 2000, "3K"},
    };

    // 1:1 — square
    static const Preset presets_1_1[] = {
        {1080, 1080, "Square 1080"},
        {1440, 1440, "Square 1440"},
        {2160, 2160, "Square 2160"},
    };

    // 9:16 — vertical/social
    static const Preset presets_9_16[] = {
        {1080, 1920, "1080p"},
        {1440, 2560, "1440p"},
        {2160, 3840, "4K"},
    };

    struct PresetList { const Preset* preset; int x264_count;  int x265_count; };
    auto get_preset_list = [&](int aspect_index) -> PresetList {
        switch (aspect_index) {
        case 0: return { presets_16_9, 4, 6 };
        case 1: return { presets_21_9, 3, 4 };
        case 2: return { presets_32_9, 2, 3 };
        case 3: return { presets_4_3,  3, 3 };
        case 4: return { presets_3_2,  3, 3 };
        case 5: return { presets_1_1,  3, 3 };
        case 6: return { presets_9_16, 3, 3 };
        default: return { nullptr, 0 };
        }
    };

    // highlight colors
    static const ImVec4 sel = ImVec4(0.18f, 0.55f, 0.95f, 1.00f);
    static const ImVec4 sel_hover = ImVec4(0.22f, 0.62f, 1.00f, 1.00f);
    static const ImVec4 sel_active = ImVec4(0.14f, 0.45f, 0.85f, 1.00f);
    auto pushSelected = [](bool selected) {
        if (!selected) return 0;
        ImGui::PushStyleColor(ImGuiCol_Button, sel);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, sel_hover);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, sel_active);
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1, 1, 1, 1));
        return 4;
    };

    auto set_even = [](int v) { return v & ~1; };

    // apply current selection (bounds-checked)
    auto apply_selection = [&](IVec2& out) {
        if (sel_aspect < 0 || sel_tier < 0) return;
        PresetList preset_list = get_preset_list(sel_aspect);
        if (!preset_list.preset || sel_tier >= preset_list.x265_count) return; // safety
        out.x = set_even(preset_list.preset[sel_tier].w);
        out.y = set_even(preset_list.preset[sel_tier].h);
        //bitrate_mbps_range = recommended_bitrate_range_mbps(targ_res, record_fps, VideoCodecFromCaptureFormat((CaptureFormat)record_format));
        //record_bitrate = (int64_t)(1000000.0 * choose_bitrate_mbps_from_range(bitrate_mbps_range, record_quality));
    };

    // auto-select from current resolution
    auto try_select_from_resolution = [&](int w, int h)
    {
        sel_aspect = -1;
        sel_tier = -1;

        //bitrate_mbps_range = recommended_bitrate_range_mbps(targ_res, record_fps, VideoCodecFromCaptureFormat((CaptureFormat)record_format));
        //record_bitrate = (int64_t)(1000000.0 * choose_bitrate_mbps_from_range(bitrate_mbps_range, record_quality));

        for (int ai = 0; ai < IM_ARRAYSIZE(aspects); ++ai)
        {
            PresetList L = get_preset_list(ai);
            for (int ti = 0; ti < L.x265_count; ++ti) {
                if (L.preset[ti].w == w && L.preset[ti].h == h) {
                    sel_aspect = ai;
                    sel_tier = ti;
                    return;
                }
            }
        }

    };

    bool changed = false;

    ImGui::Text("W:"); ImGui::SameLine();
    if (ImGui::InputInt("##res_x", &targ_res.x, 10)) {
        targ_res.x = std::clamp(targ_res.x, 16, 16384);
        targ_res.x = (targ_res.x & ~1); // force even
        changed = true;
    }
    ImGui::Text("H:"); ImGui::SameLine();
    if (ImGui::InputInt("##res_y", &targ_res.y, 10)) {
        targ_res.y = std::clamp(targ_res.y, 16, 16384);
        targ_res.y = (targ_res.y & ~1); // force even
        changed = true;
    }

    //static bool first = true;
    if (first) { try_select_from_resolution(targ_res.x, targ_res.y); first = false; }
    if (changed) try_select_from_resolution(targ_res.x, targ_res.y);

    // Helper: choose the closest tier (by height) to current H for a given aspect list
    auto pick_closest_tier = [&](int aspect_index, int current_h) -> int
    {
        PresetList preset_list = get_preset_list(aspect_index);
        if (!preset_list.preset || preset_list.x265_count == 0) return -1;
        int best = 0;
        int best_d = INT_MAX;
        for (int i = 0; i < preset_list.x265_count; ++i) {
            int h = preset_list.preset[i].h; // works for portrait lists too (we stored exact WxH)
            int d = std::abs(h - current_h);
            if (d < best_d) { best_d = d; best = i; }
        }
        return best;
    };

    ImGui::Spacing();
    ImGui::Spacing();
    ImGui::TextUnformatted("Aspect:");
    for (int i = 0; i < (int)IM_ARRAYSIZE(aspects); ++i)
    {
        bool is_selected = (sel_aspect == i);
        int pushed = pushSelected(is_selected);

        if (ImGui::Button(aspects[i].label))
        {
            int old_aspect = sel_aspect;
            sel_aspect = i;

            // Get new list
            PresetList new_preset_list = get_preset_list(sel_aspect);

            // If switching portrait <-> landscape or tier is out of range, choose a tier
            bool switched_portrait =
                (old_aspect >= 0) &&
                (aspects[old_aspect].portrait != aspects[sel_aspect].portrait);

            bool tier_invalid = (sel_tier < 0 || !new_preset_list.preset || sel_tier >= new_preset_list.x265_count);

            if (switched_portrait || tier_invalid)
            {
                // Prefer closest tier to current height; fallback to 0
                int current_h = targ_res.y;
                int t = pick_closest_tier(sel_aspect, current_h);
                sel_tier = (t >= 0 ? t : 0);
            }
            else
            {
                int valid_tier_count = (config.getRecordFormat() == CaptureFormat::x264) ?
                    new_preset_list.x264_count :
                    new_preset_list.x265_count;


                // Clamp to new list length just in case
                sel_tier = (sel_tier >= valid_tier_count) ? (valid_tier_count - 1) : sel_tier;
            }

            // Apply the (aspect, tier) to actually change WxH
            apply_selection(targ_res);
        }
        if (pushed) ImGui::PopStyleColor(pushed);
        if (i + 1 < (int)IM_ARRAYSIZE(aspects)) ImGui::SameLine(0, 6);
    }

    ImGui::Spacing();
    ImGui::Spacing();
    ImGui::TextUnformatted("Resolution tier:");

    int ai = (sel_aspect >= 0) ? sel_aspect : 0;
    PresetList preset_list = get_preset_list(ai);

    // compact grid
    int per_row = 6;
    for (int i = 0; i < preset_list.x265_count; ++i) {
        bool is_selected = (sel_aspect == ai && sel_tier == i);
        int pushed = pushSelected(is_selected);
        bool unsupported = (config.getRecordFormat() == CaptureFormat::x264) && preset_list.preset[i].HEVC_only;

        //const PresetList& list = get_preset_list(ai);
        const Preset& preset = preset_list.preset[i];
        IVec2 res = { preset.w, preset.h };
        //u64 res_pixels = (u64)res.x * (u64)res.y * (u64)snapshot_supersample_factor;
        u64 res_pixels = (u64)res.x * (u64)res.y * (u64)config.default_ssaa;
        if (res_pixels > max_pixels)
            unsupported = true;

        if (unsupported) ImGui::BeginDisabled();
        if (ImGui::Button(preset_list.preset[i].label)) {
            sel_aspect = ai;
            sel_tier = i;
            apply_selection(targ_res);
        }
        if (unsupported) ImGui::EndDisabled();
        if (pushed) ImGui::PopStyleColor(pushed);
        if ((i + 1) % per_row != 0 && (i + 1) < preset_list.x265_count) ImGui::SameLine(0, 6);
    }
}

void SettingsPanel::populateSettings()
{
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, scale_size(6, 6));
    ImGui::BeginChild("RecordingFrame", ImVec2(0, 0), 0, ImGuiWindowFlags_AlwaysUseWindowPadding);

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

            ///#if BITLOOP_FFMPEG_ENABLED
            ///bitrate_mbps_range = recommended_bitrate_range_mbps(record_resolution, record_fps, VideoCodecFromCaptureFormat((CaptureFormat)record_format));
            ///record_bitrate = (int64_t)(1000000.0 * choose_bitrate_mbps_from_range(bitrate_mbps_range, record_quality));
            ///#endif
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

    // Image Resolution
    {
        ImGui::BeginLabelledBox("Image Capture");

        //static int sel_aspect = -1;
        //static int sel_tier = -1;
        //constexpr IVec2 pixels_8k{ 7680, 4320 };
        //static u64 max_pixels = (u64)pixels_8k.x * (u64)pixels_8k.y;
        //static bool first = true;
        ///populateResolutionOptions(snapshot_resolution, first, sel_aspect, sel_tier, max_pixels);

        populateCapturePresets();

        ImGui::EndLabelledBox();
    }

    //ImGui::SeparatorText("Video Capture");

    // Video Resolution
    if (!platform()->is_mobile())
    {
        ImGui::BeginLabelledBox("Video Capture");

        ImGui::Spacing();

        ImGui::Text("Codec:");
        ImGui::Combo("##codec", &config.record_format, "H.264 (x264)\0H.265 / HEVC (x265)\0WebP Animation\0");

        ImGui::Spacing();
        ImGui::Spacing();
        ImGui::Spacing();

        #if BITLOOP_FFMPEG_ENABLED
        if (config.record_format != (int)CaptureFormat::WEBP_VIDEO)
        {
            // ffmpeg video
            static int sel_aspect = -1;
            static int sel_tier = -1;

            constexpr IVec2 pixels_4k{ 3840, 2160 };
            constexpr IVec2 pixels_8k{ 7680, 4320 };
            u64 max_pixels;

            if (config.record_format == (int)CaptureFormat::x265)
                max_pixels = (u64)pixels_8k.x * (u64)pixels_8k.y;
            else
                max_pixels = (u64)pixels_4k.x * (u64)pixels_4k.y;

            static bool first = true;
            populateResolutionOptions(config.record_resolution, first, sel_aspect, sel_tier, max_pixels);

            config.record_bitrate_mbps_range = recommended_bitrate_range_mbps(config.record_resolution, config.record_fps, VideoCodecFromCaptureFormat(config.getRecordFormat()));
            config.record_bitrate = (i64)(1000000.0 * choose_bitrate_mbps_from_range(config.record_bitrate_mbps_range, config.record_quality));
        }
        else
            #endif
        {
            ImGui::Text("X:"); ImGui::SameLine();
            if (ImGui::InputInt("##vid_res_x", &config.record_resolution.x, 10)) {
                config.record_resolution.x = std::clamp(config.record_resolution.x, 16, 1024);
                config.record_resolution.x = (config.record_resolution.x & ~1); // force even
            }
            ImGui::Text("Y:"); ImGui::SameLine();
            if (ImGui::InputInt("##vid_res_y", &config.record_resolution.y, 10)) {
                config.record_resolution.y = std::clamp(config.record_resolution.y, 16, 1024);
                config.record_resolution.y = (config.record_resolution.y & ~1); // force even
            }

            ImGui::Spacing();
            ImGui::Spacing();
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

        ImGui::EndLabelledBox();
    }

    ImGui::EndChild();
    ImGui::PopStyleVar();
}

BL_END_NS;
