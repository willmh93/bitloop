#pragma once

#include <bitloop/core/capture_manager.h>
#include <bitloop/core/snapshot_presets.h>
#include <bitloop/imguix/imgui_custom.h>

BL_BEGIN_NS;

class MainWindow;

// Saveable settings
struct SettingsConfig
{
    SnapshotPresetManager snapshot_preset_manager;

    std::string capture_dir;

    bool    fixed_time_delta = false;

    int     default_ssaa = 1;
    float   default_sharpen = 0.0f;

    int     snapshot_format = (int)CaptureFormat::WEBP_SNAPSHOT; // CaptureFormat
    int     record_format = (int)CaptureFormat::x264;

    int     record_fps = 60;
    int     record_frame_count = 0;
    IVec2   record_resolution{ 1920, 1080 };
    int     record_quality = 100;
    bool    record_lossless = true;
    int     record_near_lossless = 100;

    #if BITLOOP_FFMPEG_ENABLED
    int64_t        record_bitrate = 128000000ll;
    BitrateRange   record_bitrate_mbps_range{ 1, 1000 };
    #endif

    CaptureFormat getRecordFormat() const { return (CaptureFormat)record_format; }
    CaptureFormat getSnapshotFormat() const { return (CaptureFormat)snapshot_format; }
};

// Settings UI
class SettingsPanel
{
    MainWindow* main_window;

    SettingsConfig config;

    // temporary buffers (prevents accepting values when duplicate detected)
    char input_name[64]{};
    char input_alias[32]{};
    char revert_name[64]{};
    char revert_alias[32]{};
    int selected_capture_preset = -1;

public:

    SettingsPanel(MainWindow* w) : main_window(w)
    {}

    void init();

    SettingsConfig& getConfig() { return config; }
    const SettingsConfig& getConfig() const { return config; }

    void populateCapturePresets();
    void populateResolutionOptions(IVec2& targ_res, bool& first, int& sel_aspect, int& sel_tier, u64 max_pixels);
    void populateSettings();
};

// returns idx when selection changes (otherwise -1)
template<typename Callback>
int populateCapturePresetsList(
    Callback&& getPresetRefFromIdx, int count,
    std::unordered_map<bl::hash_t, bool>& enabled_presets,
    int& selected_capture_preset)
{
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(ImGui::GetStyle().ItemSpacing.x, 2.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(ImGui::GetStyle().FramePadding.x, 1.0f));

    int changed_selected_index = -1;

    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
    if (ImGui::BeginListBox("##Presets", ImVec2(0.0f, 0.0f)))
    {
        ImGuiListClipper clipper;
        //clipper.Begin(presets.size());
        clipper.Begin(count);

        while (clipper.Step())
        {
            for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; ++i)
            {
                ImGui::PushID(i);

                //SnapshotPreset& preset = presets[i];
                SnapshotPreset& preset = getPresetRefFromIdx(i);
                bool enabled = enabled_presets.count(preset.hashed_name) > 0;
                bool was_enabled = enabled;

                // Checkbox at start of row
                ImGui::AlignTextToFramePadding();
                ImGui::Checkbox("##enabled", &enabled);
                ImGui::SameLine();

                // dim disabled rows
                ///if (!preset.enabled)
                if (!enabled)
                    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.6f);

                const bool is_selected = (selected_capture_preset == i);
                if (ImGui::Selectable(preset.list_name.c_str(), is_selected, ImGuiSelectableFlags_SpanAvailWidth))
                {
                    selected_capture_preset = i;
                    changed_selected_index = i;
                }

                ///if (!preset.enabled)
                if (!enabled)
                    ImGui::PopStyleVar();

                if (!was_enabled && enabled) enabled_presets[preset.hashed_name] = true;
                if (was_enabled && !enabled) enabled_presets.erase(preset.hashed_name);

                ImGui::PopID();
            }
        }

        ImGui::EndListBox();
    }
    ImGui::PopStyleVar(2);

    ImGui::Spacing();

    {
        auto enable_idx = [&](int i) {
            bl::hash_t hash = getPresetRefFromIdx(i).hashed_name;
            if (enabled_presets.count(hash) == 0)
                enabled_presets[hash] = true;
        };

        auto disable_idx = [&](int i) {
            bl::hash_t hash = getPresetRefFromIdx(i).hashed_name;
            if (enabled_presets.count(hash) > 0)
                enabled_presets.erase(hash);
        };

        auto enable_hash = [&](bl::hash_t hash) {
            if (enabled_presets.count(hash) == 0)
                enabled_presets[hash] = true;
        };

        auto disable_hash = [&](bl::hash_t hash) {
            if (enabled_presets.count(hash) > 0)
                enabled_presets.erase(hash);
        };

        auto is_enabled = [&](bl::hash_t hash) {
            return enabled_presets.count(hash) > 0;
        };

        auto set_enabled = [&](bl::hash_t hash, bool b) {
            if (b) {
                if (enabled_presets.count(hash) == 0)
                    enabled_presets[hash] = true;
            }
            else if (!b) {
                if (enabled_presets.count(hash) > 0)
                    enabled_presets.erase(hash);
            }
        };

        auto set_all = [&](bool v)
        {
            for (int i = 0; i < count; i++) {
                if (v) enable_idx(i);
                else   disable_idx(i);
            }
        };

        auto enable_if = [&](auto&& pred)
        {
            for (int i = 0; i < count; i++) {
                SnapshotPreset& p = getPresetRefFromIdx(i);
                if (pred(p)) enable_hash(p.hashed_name);
                //if (pred(p)) p.enabled = true;
            }
            //for (auto& p : presets) if (pred(p)) p.enabled = true;
        };

        auto disable_if = [&](auto&& pred)
        {
            for (int i = 0; i < count; i++) {
                SnapshotPreset& p = getPresetRefFromIdx(i);
                if (pred(p)) disable_hash(p.hashed_name);
                //if (pred(p)) p.enabled = false;
            }
            //for (auto& p : presets) if (pred(p)) p.enabled = false;
        };

        auto set_only = [&](auto&& pred)
        {
            //for (auto& p : presets) p.enabled = pred(p);
            for (int i = 0; i < count; i++) {
                SnapshotPreset& p = getPresetRefFromIdx(i);
                if (pred(p)) enable_hash(p.hashed_name);
                else         disable_hash(p.hashed_name);
                //p.enabled = pred(p);
            }
        };

        auto max_dim = [](const SnapshotPreset& p) { return (p.size.x > p.size.y) ? p.size.x : p.size.y; };
        auto min_dim = [](const SnapshotPreset& p) { return (p.size.x < p.size.y) ? p.size.x : p.size.y; };

        auto is_square = [&](const SnapshotPreset& p) { return p.size.x == p.size.y; };
        auto is_portrait = [&](const SnapshotPreset& p) { return p.size.y > p.size.x; };
        auto is_landscape = [&](const SnapshotPreset& p) { return p.size.x > p.size.y; };

        auto aspect = [&](const SnapshotPreset& p) -> float
        {
            const float w = (float)((p.size.x > 0) ? p.size.x : 1);
            const float h = (float)((p.size.y > 0) ? p.size.y : 1);
            return w / h;
        };

        // Heuristics:
        // - 8K: any preset with max dimension >= 7680
        // - 4K: max dimension >= 3840 but < 7680, and min dimension >= ~2000 to avoid 3840x1080 counting as 4K
        auto is_8k = [&](const SnapshotPreset& p)
        {
            return max_dim(p) >= 7680;
        };

        auto is_4k = [&](const SnapshotPreset& p)
        {
            const int md = max_dim(p);
            const int nd = min_dim(p);
            return (md >= 3840) && (md < 7680) && (nd >= 2000);
        };

        auto is_1440_class = [&](const SnapshotPreset& p)
        {
            const int md = max_dim(p);
            const int nd = min_dim(p);
            return (md >= 2560) && (md < 3840) && (nd >= 1440);
        };

        auto is_1080_class = [&](const SnapshotPreset& p)
        {
            const int md = max_dim(p);
            const int nd = min_dim(p);
            return (md >= 1920) && (md < 2560) && (nd >= 1080);
        };

        auto is_ultrawide = [&](const SnapshotPreset& p)
        {
            return is_landscape(p) && aspect(p) >= 2.2f && aspect(p) < 3.0f;
        };

        auto is_super_ultrawide = [&](const SnapshotPreset& p)
        {
            return is_landscape(p) && aspect(p) >= 3.0f;
        };

        // Small dropdown trigger (arrow button). Use SmallButton("...") if you prefer.
        if (ImGui::ArrowButton("##preset_actions", ImGuiDir_Down))
            ImGui::OpenPopup("##preset_actions_popup");
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort))
            ImGui::SetTooltip("Preset batch actions");

        if (ImGui::BeginPopup("##preset_actions_popup"))
        {
            if (ImGui::MenuItem("Disable All")) set_all(false);
            if (ImGui::MenuItem("Enable All"))  set_all(true);

            if (ImGui::MenuItem("Invert Enabled"))
            {
                //for (auto& p : presets) p.enabled = !p.enabled;
                for (int i = 0; i < count; i++) {
                    SnapshotPreset& p = getPresetRefFromIdx(i);
                    set_enabled(p.hashed_name, !is_enabled(p.hashed_name));
                    //p.enabled = !p.enabled;
                }
            }

            ImGui::Separator();

            if (ImGui::BeginMenu("Enable all (additive)"))
            {
                if (ImGui::MenuItem("Enable all 1080 class")) enable_if(is_1080_class);
                if (ImGui::MenuItem("Enable all 1440 class")) enable_if(is_1440_class);
                if (ImGui::MenuItem("Enable all 4K"))         enable_if(is_4k);
                if (ImGui::MenuItem("Enable all 8K"))         enable_if(is_8k);

                ImGui::Separator();

                if (ImGui::MenuItem("Enable all Square"))          enable_if(is_square);
                if (ImGui::MenuItem("Enable all Portrait"))        enable_if(is_portrait);
                if (ImGui::MenuItem("Enable all Ultrawide"))       enable_if(is_ultrawide);
                if (ImGui::MenuItem("Enable all Super ultrawide")) enable_if(is_super_ultrawide);

                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Enable only (exclusive)"))
            {
                if (ImGui::MenuItem("Only 1080 class")) set_only(is_1080_class);
                if (ImGui::MenuItem("Only 1440 class")) set_only(is_1440_class);
                if (ImGui::MenuItem("Only 4K"))         set_only(is_4k);
                if (ImGui::MenuItem("Only 8K"))         set_only(is_8k);

                ImGui::Separator();

                if (ImGui::MenuItem("Only Square"))          set_only(is_square);
                if (ImGui::MenuItem("Only Portrait"))        set_only(is_portrait);
                if (ImGui::MenuItem("Only Landscape"))       set_only(is_landscape);
                if (ImGui::MenuItem("Only Ultrawide"))       set_only(is_ultrawide);
                if (ImGui::MenuItem("Only Super ultrawide")) set_only(is_super_ultrawide);

                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Disable by category"))
            {
                if (ImGui::MenuItem("Disable all 4K")) disable_if(is_4k);
                if (ImGui::MenuItem("Disable all 8K")) disable_if(is_8k);
                if (ImGui::MenuItem("Disable all Ultrawide")) disable_if(is_ultrawide);
                ImGui::EndMenu();
            }

            ImGui::EndPopup();
        }
    }
    return changed_selected_index;
}


BL_END_NS;
