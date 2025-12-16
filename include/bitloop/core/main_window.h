#pragma once

#include <vector>

#include <bitloop/core/threads.h>
#include <bitloop/core/project.h>
#include <bitloop/core/capture_manager.h>
#include <bitloop/core/settings.h>

#include <bitloop/imguix/imgui_custom.h>
#include <bitloop/nanovgx/nano_canvas.h>

BL_BEGIN_NS;

extern ImDebugLog project_log;
extern ImDebugLog debug_log;

struct ToolbarButtonState 
{
    ImVec4 bgColor;
    ImVec4 bgColorToggled;
    ImVec4 symbolColor;

    bool enabled = false;
    bool toggled = false;

    int  blink_timer = 0;
    bool blinking = false;
};

enum struct MainWindowCommandType
{
    ON_PLAY_PROJECT,
    ON_STOPPED_PROJECT,
    ON_PAUSED_PROJECT,
    BEGIN_SNAPSHOT,
    TAKE_ACTIVE_PRESET_SNAPSHOT,
    BEGIN_RECORDING,
    END_RECORDING
};

struct MainWindowCommand_SnapshotPayload
{
    std::string path; // path format (relative to project capture dir, no extension)
    SnapshotPresetList presets; // which presets to target
};

struct MainWindowCommandEvent
{
    MainWindowCommandType type;
    std::any payload;
    MainWindowCommandEvent(MainWindowCommandType t) : type(t) {}

    template<typename T>
    MainWindowCommandEvent(MainWindowCommandType t, T&& d) : type(t), payload(d) {}
};

class MainWindow
{
    static MainWindow* singleton;

    std::mutex command_mutex;
    std::vector<MainWindowCommandEvent> command_queue;

    ImFont* main_font = nullptr;
    ImFont* mono_font = nullptr;

    bool initialized = false;
    bool done_first_size = false;
    bool done_first_focus = false;
    bool update_docking_layout = false;
    bool vertical_layout = false;
    bool sidebar_visible = true;

    bool need_draw = false;

    //ImRect viewport_rect;
    ImVec2 client_size{};
    bool viewport_hovered = false;

    ToolbarButtonState play     = { ImVec4(0.3f, 0.3f, 0.3f, 1.0f), ImVec4(0.1f, 0.6f, 0.1f, 1.0f), ImVec4(0.4f, 1.0f, 0.4f, 1.0f), true };
    ToolbarButtonState pause    = { ImVec4(0.3f, 0.3f, 0.3f, 1.0f), ImVec4(0.3f, 0.3f, 0.3f, 1.0f), ImVec4(1.0f, 1.0f, 1.0f, 1.0f), false };
    ToolbarButtonState stop     = { ImVec4(0.3f, 0.3f, 0.3f, 1.0f), ImVec4(0.6f, 0.1f, 0.1f, 1.0f), ImVec4(1.0f, 0.0f, 0.0f, 1.0f), false };
    ToolbarButtonState record   = { ImVec4(0.3f, 0.3f, 0.3f, 1.0f), ImVec4(0.8f, 0.0f, 0.0f, 1.0f), ImVec4(1.0f, 0.2f, 0.0f, 1.0f), true };
    ToolbarButtonState snapshot = { ImVec4(0.3f, 0.3f, 0.3f, 1.0f), ImVec4(0.8f, 0.0f, 0.0f, 1.0f), ImVec4(0.8f, 0.8f, 0.8f, 1.0f), true };

    CaptureManager capture_manager;
    

    SettingsPanel    settings_panel;
    CaptureConfig    config;

    SnapshotPresetList enabled_capture_presets;
    bool               is_snapshotting = false; // remains true for whole batch
    int                active_capture_preset = 0;
    std::string        active_capture_rel_proj_path;
                                     
    ///bool          window_capture = false;
    bool             encode_next_sim_frame = false;
    bool             captured_last_frame = false;

    Canvas canvas;
    std::vector<uint8_t> frame_data; // intermediate buffer to read canvas pixels before encoding

    SharedSync& shared_sync;

    const int window_flags =
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoDecoration |
        ImGuiWindowFlags_NoMove;

public:

    static constexpr MainWindow* instance() { 
        return singleton; 
    }

    MainWindow(SharedSync& _shared_sync) :
        shared_sync(_shared_sync),
        settings_panel(this)
    {
        singleton = this;
    }

    /// todo: switch to refs?
    [[nodiscard]] Canvas* getCanvas() { return &canvas; }
    [[nodiscard]] CaptureManager* getRecordManager() { return &capture_manager; }
    [[nodiscard]] SettingsConfig* getSettingsConfig() { return &settings_panel.getConfig(); }
    [[nodiscard]] const SettingsConfig* getSettingsConfig() const { return &settings_panel.getConfig(); }
    [[nodiscard]] SnapshotPresetManager* getSnapshotPresetManager() { return &settings_panel.getConfig().snapshot_preset_manager; }

    [[nodiscard]] bool isSnapshotting() const
    {
        return is_snapshotting;
    }

    int getFPS() const { return getSettingsConfig()->record_fps; }
    void setFixedFrameTimeDelta(bool b) { getSettingsConfig()->fixed_time_delta = b; }
    bool isFixedFrameTimeDelta() const { return getSettingsConfig()->fixed_time_delta; }

    void queueBeginSnapshot(const SnapshotPresetList& presets, const char* relative_filename = nullptr);
    void queueBeginRecording();
    void queueEndRecording();

    void beginRecording();

    // e.g. for a given preset 'backgrounds/seahorse_valley' may create missing dirs and return:
    //     'captures/Mandelbrot/backgrounds/seahorse_valley_1920x1080.webp'
    std::string prepareFullCapturePath(const SnapshotPreset& preset, const char* relative_filepath_noext);

    void beginSnapshot(const char* rel_proj_path, IVec2 res, int ssaa=1, float sharpen=0.0f);
    void beginSnapshot(const SnapshotPreset& preset, const char* relative_filepath_noext);

    // relative to project capture dir - if nullptr, name chosen automatically, e.g. For a given preset:
    void beginSnapshotList(const SnapshotPresetList& presets, const char* rel_proj_path);

    void endRecording();
    void checkCaptureComplete();

    void captureFrame(bool b) { encode_next_sim_frame = b; }
    bool capturingNextFrame() const { return encode_next_sim_frame; }
    bool capturedLastFrame() const { return captured_last_frame; }

    void init();
    void checkChangedDPR();

    void initStyles();
    void initFonts();

    ImFont* mainFont() { return main_font; }
    ImFont* monoFont() { return mono_font; }


    //ImRect viewportRect() { return viewport_rect; }
    bool viewportHovered() const { return viewport_hovered; }
    IVec2 viewportSize() const { return (IVec2)client_size; }

    void setSidebarVisible(bool b) { sidebar_visible = b; }

    /// Thread-safe message queue
    void queueMainWindowCommand(MainWindowCommandEvent e) {
        std::lock_guard<std::mutex> lock(command_mutex);
        command_queue.push_back(MainWindowCommandEvent(e));
    }

    template<typename T>
    void queueMainWindowCommand(MainWindowCommandEvent e, T&& data) {
        std::lock_guard<std::mutex> lock(command_mutex);
        command_queue.push_back(MainWindowCommandEvent(e, std::make_any(data)));
    }

    private: void handleCommand(MainWindowCommandEvent e);

public:

    /// Toolbar
    void drawToolbarButton(ImDrawList* drawList, ImVec2 pos, ImVec2 size, const char* symbol, ImU32 color);
    bool toolbarButton(const char* id, const char* symbol, const ToolbarButtonState& state, ImVec2 size, float inactive_alpha = 0.25f);
    void populateToolbar();

    /// Project Tree
    void populateProjectTreeNodeRecursive(ProjectInfoNode& node, int& i, int depth);
    void populateProjectTree(bool expand_vertical);
    void populateProjectUI();
    void populateOverlay();
    
    /// Main Window populate
    bool manageDockingLayout();
    bool focusWindow(const char* id);

    void populateCollapsedLayout();
    void populateViewport();
    
    void populateExpandedLayout();
    void populateUI();
};

[[nodiscard]] constexpr MainWindow* main_window()
{
    return MainWindow::instance();
}

BL_END_NS;
