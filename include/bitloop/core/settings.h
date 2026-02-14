#pragma once

#include <bitloop/core/capture_manager.h>
#include <bitloop/core/snapshot_presets.h>
#include <bitloop/imguix/imguix.h>

BL_BEGIN_NS;

class MainWindow;

// Saveable settings
struct SettingsConfig
{
    SnapshotPresetManager snapshot_preset_manager;

    std::string capture_dir;

    bool    preview_mode = false;
    bool    fixed_time_delta = false;

    int     default_ssaa = 1;
    float   default_sharpen = 0.0f;

    CaptureFormatSnapshot snapshot_format = CaptureFormatSnapshot::WEBP_SNAPSHOT; // CaptureFormat
    CaptureFormatVideo    record_format =
    #if BITLOOP_FFMPEG_ENABLED
        CaptureFormatVideo::x264;
    #else
        CaptureFormatVideo::WEBP_VIDEO;
    #endif

    int     record_fps = 60;
    int     record_frame_count = 0;
    int     record_quality = 100;
    bool    record_lossless = true;
    int     record_near_lossless = 100;

    bool    show_fps = false;

    SnapshotPresetHashMap target_image_presets;
    int                   target_video_preset = 0; // idx (todo: should probably be preset hash)

    #if BITLOOP_FFMPEG_ENABLED
    int64_t        record_bitrate = 128000000ll;
    BitrateRange   record_bitrate_mbps_range{ 1, 1000 };
    #endif

    SettingsConfig()
    {
        const SnapshotPresetList& capture_presets = snapshot_preset_manager.allPresets();
        target_image_presets[capture_presets["fhd"]->hashedAlias()] = true;
    }

    SnapshotPresetList enabledImagePresets() const
    {
        const SnapshotPresetList& capture_presets = snapshot_preset_manager.allPresets();
        SnapshotPresetList ret;
        for (const auto& p : capture_presets)
        {
            if (target_image_presets.count(p.hashedAlias()) > 0)
                ret.add(p);
        }
        ret.updateLookup();
        return ret;
    }

    CapturePreset enabledVideoPreset() const
    {
        const SnapshotPresetList& capture_presets = snapshot_preset_manager.allPresets();
        return capture_presets.at(target_video_preset);
    }

    CaptureFormat getRecordFormat() const { return (CaptureFormat)record_format; }
    CaptureFormat getSnapshotFormat() const { return (CaptureFormat)snapshot_format; }
    IVec2         getRecordResolution() const { return enabledVideoPreset().getResolution(); }
};

// Settings UI
class SettingsPanel
{
    MainWindow* main_window;

    SettingsConfig config;

    // input buffers (avoids direct access to CapturePreset)
    char   input_name[64]{};
    char   input_alias[32]{};
    IVec2  input_resolution;
    int    input_ssaa = 1;
    float  input_sharpen = 0.0f;

    // revert back to old alias if attempting to set to an existing alias
    char revert_alias[32]{};

    int selected_capture_preset = 0;
    int selected_image_preset = -1;
    bool selected_preset_is_video = false;
    

public:

    SettingsPanel(MainWindow* w) : main_window(w)
    {}

    void init();
    void setInputPreset(CapturePreset* p)
    {
        if (!p) return;
        input_resolution = p->getResolution();
        input_ssaa = p->getSSAA();
        input_sharpen = p->getSharpening();
        strcpy(input_name, p->name_cstr());
        strcpy(input_alias, p->alias_cstr());
        strcpy(revert_alias, p->alias_cstr());
    }

    SettingsConfig& getConfig() { return config; }
    const SettingsConfig& getConfig() const { return config; }

    // image or video capture preset index, depending on visible tab
    int getSelectedPresetIndex() const {
        if (selected_preset_is_video)
            return config.target_video_preset;
        else
            return selected_image_preset;
    }

    void populateCapturePresetsEditor();
    void populateSettings();
};

enum struct CapturePresetsSelectMode
{
    NONE,
    MULTI,
    SINGLE
};

// flexible preset picker
// > getPresetRefFromIdx:   items provided by callback, e.g. [&](int i)->CapturePreset& { return presets[i]; }
// > count:                 number of items provided by callback
// > enabled_preset:        enabled presets provided by hash lookup
//   - Useful if a sim saves a list of "valid" render targets but can't guarantee they exist in the provided list
// > returns idx when selection changes (otherwise -1)
template<CapturePresetsSelectMode select_mode=CapturePresetsSelectMode::NONE, typename Callback>
int populateCapturePresetsList(
    Callback&& getPresetRefFromIdx, int count,
    SnapshotPresetHashMap* enabled_presets,
    int& selected_capture_preset)
{
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(ImGui::GetStyle().ItemSpacing.x, 2.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(ImGui::GetStyle().FramePadding.x, 1.0f));

    int changed_selected_index = -1;

    // dim listbox bg so checkboxes are still visible
    ImVec4 lb = ImGui::GetStyleColorVec4(ImGuiCol_FrameBg);
    lb.x *= 0.85f;
    lb.y *= 0.85f;
    lb.z *= 0.85f;
    lb.w = 1.0f;

    ImGui::PushStyleColor(ImGuiCol_FrameBg, lb);

    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
    const bool open = ImGui::BeginListBox("##Presets", ImVec2(0.0f, 0.0f));

    // pop immediately so the listbox contents keep normal FrameBg
    ImGui::PopStyleColor(1);

    if (open)
    {
        ImGuiListClipper clipper;
        clipper.Begin(count);

        while (clipper.Step())
        {
            for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; ++i)
            {
                ImGui::PushID(i);

                CapturePreset& preset = getPresetRefFromIdx(i);
                bool enabled = enabled_presets && enabled_presets->count(preset.hashedAlias()) > 0;
                bool was_enabled = enabled;

                // Checkbox at start of row
                ImGui::AlignTextToFramePadding();
                if constexpr (select_mode == CapturePresetsSelectMode::SINGLE)
                {
                    ImGui::RadioButton("##enabled", &selected_capture_preset, i);
                    ImGui::SameLine();
                }
                else if constexpr (select_mode == CapturePresetsSelectMode::MULTI)
                {
                    ImGui::Checkbox("##enabled", &enabled);
                    ImGui::SameLine();
                }

                // dim disabled rows
                if (!enabled)
                    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.6f);

                const bool is_selected = (selected_capture_preset == i);
                if (ImGui::Selectable(preset.description(), is_selected, ImGuiSelectableFlags_SpanAvailWidth))
                {
                    selected_capture_preset = i;
                    changed_selected_index = i;
                }

                if (!enabled)
                    ImGui::PopStyleVar();

                if constexpr (select_mode == CapturePresetsSelectMode::MULTI)
                {
                    if (enabled_presets)
                    {
                        if (!was_enabled && enabled) (*enabled_presets)[preset.hashedAlias()] = true;
                        if (was_enabled && !enabled) enabled_presets->erase(preset.hashedAlias());
                    }
                }

                ImGui::PopID();
            }
        }

        ImGui::EndListBox();
    }
    ImGui::PopStyleVar(2);

    ImGui::Spacing();

    if (enabled_presets)
    {
        auto enable_idx = [&](int i) {
            bl::hash_t hash = getPresetRefFromIdx(i).hashedAlias();
            if (enabled_presets->count(hash) == 0)
                (*enabled_presets)[hash] = true;
        };

        auto disable_idx = [&](int i) {
            bl::hash_t hash = getPresetRefFromIdx(i).hashedAlias();
            if (enabled_presets->count(hash) > 0)
                enabled_presets->erase(hash);
        };

        auto enable_hash = [&](bl::hash_t hash) {
            if (enabled_presets->count(hash) == 0)
                (*enabled_presets)[hash] = true;
        };

        auto disable_hash = [&](bl::hash_t hash) {
            if (enabled_presets->count(hash) > 0)
                enabled_presets->erase(hash);
        };

        auto is_enabled = [&](bl::hash_t hash) {
            return enabled_presets->count(hash) > 0;
        };

        auto set_enabled = [&](bl::hash_t hash, bool b) {
            if (b) {
                if (enabled_presets->count(hash) == 0)
                    (*enabled_presets)[hash] = true;
            }
            else if (!b) {
                if (enabled_presets->count(hash) > 0)
                    enabled_presets->erase(hash);
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
                CapturePreset& p = getPresetRefFromIdx(i);
                if (pred(p)) enable_hash(p.hashedAlias());
            }
        };

        auto disable_if = [&](auto&& pred)
        {
            for (int i = 0; i < count; i++) {
                CapturePreset& p = getPresetRefFromIdx(i);
                if (pred(p)) disable_hash(p.hashedAlias());
            }
        };

        auto set_only = [&](auto&& pred)
        {
            for (int i = 0; i < count; i++) {
                CapturePreset& p = getPresetRefFromIdx(i);
                if (pred(p)) enable_hash(p.hashedAlias());
                else         disable_hash(p.hashedAlias());
            }
        };

        auto max_dim = [](const CapturePreset& p) { return (p.width() > p.height()) ? p.width() : p.height(); };
        auto min_dim = [](const CapturePreset& p) { return (p.width() < p.height()) ? p.width() : p.height(); };

        auto is_square = [&](const CapturePreset& p) { return p.width() == p.height(); };
        auto is_portrait = [&](const CapturePreset& p) { return p.height() > p.width(); };
        auto is_landscape = [&](const CapturePreset& p) { return p.width() > p.height(); };

        auto aspect = [&](const CapturePreset& p) -> float
        {
            const float w = (float)((p.width() > 0) ? p.width() : 1);
            const float h = (float)((p.height() > 0) ? p.height() : 1);
            return w / h;
        };

        // Heuristics:
        // - 8K: any preset with max dimension >= 7680
        // - 4K: max dimension >= 3840 but < 7680, and min dimension >= ~2000 to avoid 3840x1080 counting as 4K
        auto is_8k = [&](const CapturePreset& p)
        {
            return max_dim(p) >= 7680;
        };

        auto is_4k = [&](const CapturePreset& p)
        {
            const int md = max_dim(p);
            const int nd = min_dim(p);
            return (md >= 3840) && (md < 7680) && (nd >= 2000);
        };

        auto is_1440_class = [&](const CapturePreset& p)
        {
            const int md = max_dim(p);
            const int nd = min_dim(p);
            return (md >= 2560) && (md < 3840) && (nd >= 1440);
        };

        auto is_1080_class = [&](const CapturePreset& p)
        {
            const int md = max_dim(p);
            const int nd = min_dim(p);
            return (md >= 1920) && (md < 2560) && (nd >= 1080);
        };

        auto is_ultrawide = [&](const CapturePreset& p)
        {
            return is_landscape(p) && aspect(p) >= 2.2f && aspect(p) < 3.0f;
        };

        auto is_super_ultrawide = [&](const CapturePreset& p)
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
                for (int i = 0; i < count; i++) {
                    CapturePreset& p = getPresetRefFromIdx(i);
                    set_enabled(p.hashedAlias(), !is_enabled(p.hashedAlias()));
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
