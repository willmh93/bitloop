#pragma once

#include <vector>
#include <bitloop/core/threads.h>
#include <bitloop/core/project.h>
#include <bitloop/core/record_manager.h>
#include <bitloop/imguix/imgui_custom.h>
#include <bitloop/nanovgx/nano_canvas.h>



BL_BEGIN_NS;

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




extern ImDebugLog project_log;
extern ImDebugLog debug_log;

enum struct MainWindowCommandType
{
    ON_STARTED_PROJECT,
    ON_STOPPED_PROJECT,
    ON_PAUSED_PROJECT,
    BEGIN_RECORDING,
    END_RECORDING
};


struct MainWindowCommandEvent
{
    MainWindowCommandType type;
};

class MainWindow
{
    static MainWindow* singleton;

    std::mutex command_mutex;
    std::vector<MainWindowCommandEvent> command_queue;

    void handleCommand(MainWindowCommandEvent e);

    ImFont* main_font = nullptr;
    ImFont* mono_font = nullptr;

    bool initialized = false;
    bool done_first_size = false;
    bool done_first_focus = false;
    bool update_docking_layout = false;
    bool vertical_layout = false;

    bool need_draw = false;

    //ImRect viewport_rect;
    bool viewport_hovered = false;

    ToolbarButtonState play     = { ImVec4(0.3f, 0.3f, 0.3f, 1.0f), ImVec4(0.1f, 0.6f, 0.1f, 1.0f), ImVec4(0.4f, 1.0f, 0.4f, 1.0f), true };
    ToolbarButtonState pause    = { ImVec4(0.3f, 0.3f, 0.3f, 1.0f), ImVec4(0.3f, 0.3f, 0.3f, 1.0f), ImVec4(1.0f, 1.0f, 1.0f, 1.0f), false };
    ToolbarButtonState stop     = { ImVec4(0.3f, 0.3f, 0.3f, 1.0f), ImVec4(0.6f, 0.1f, 0.1f, 1.0f), ImVec4(1.0f, 0.0f, 0.0f, 1.0f), false };
    ToolbarButtonState record   = { ImVec4(0.3f, 0.3f, 0.3f, 1.0f), ImVec4(0.8f, 0.0f, 0.0f, 1.0f), ImVec4(1.0f, 0.2f, 0.0f, 1.0f), true };
    ToolbarButtonState snapshot = { ImVec4(0.3f, 0.3f, 0.3f, 1.0f), ImVec4(0.8f, 0.0f, 0.0f, 1.0f), ImVec4(0.8f, 0.8f, 0.8f, 1.0f), true };

    // Video Capture
    CaptureManager capture_manager;
    std::vector<uint8_t> frame_data;


    #ifndef __EMSCRIPTEN__
    std::string   capture_dir;
    int64_t       record_bitrate = 128000000ll;
    BitrateRange  bitrate_mbps_range{ 1, 1000 };
    #endif

    CaptureConfig config;

    int           record_fps = 60;
                  
    int           record_format = (int)CaptureFormat::x264;
    int           record_frame_count = 0;
    IVec2         record_resolution{ 1920, 1080 };
    int           record_quality = 100;
    bool          record_lossless = true;
    int           record_near_lossless = 100;

    ///bool window_capture = false;
    bool          encode_next_sim_frame = false;
    bool          captured_last_frame = false;

    // Image Capture
    IVec2         snapshot_resolution{ 1920, 1080 };
    int           snapshot_format = (int)CaptureFormat::WEBP_SNAPSHOT; // CaptureFormat
    int           snapshot_supersample_factor = 1;
    float         snapshot_sharpen = 0.25f;
                
    bool          use_delta_time_mult = true;

    Canvas canvas;

    SharedSync& shared_sync;

    const int window_flags =
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoDecoration |
        ImGuiWindowFlags_NoMove;

public:

    static constexpr MainWindow* instance() { 
        return singleton; 
    }

    MainWindow(SharedSync& _shared_sync) : shared_sync(_shared_sync) {
        singleton = this;
    }

    [[nodiscard]] Canvas* getCanvas() { return &canvas; }
    [[nodiscard]] CaptureManager* getRecordManager() { return &capture_manager; }

    int getFPS() const { return record_fps; }
    bool useDeltaTimeMultiplier() const { return use_delta_time_mult; }

    void queueBeginRecording();
    void queueEndRecording();

    void beginRecording();
    void beginSnapshot();
    void endRecording();

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

    void queueMainWindowCommand(MainWindowCommandEvent e)
    {
        std::lock_guard<std::mutex> lock(command_mutex);
        command_queue.push_back(e);
    }

    /// Toolbar
    void drawToolbarButton(ImDrawList* drawList, ImVec2 pos, ImVec2 size, const char* symbol, ImU32 color);
    bool toolbarButton(const char* id, const char* symbol, const ToolbarButtonState& state, ImVec2 size, float inactive_alpha = 0.25f);
    void populateToolbar();

    /// Project Tree
    void populateProjectTreeNodeRecursive(ProjectInfoNode& node, int& i, int depth);
    void populateProjectTree(bool expand_vertical);
    void populateProjectUI();
    
    /// Main Window populate
    bool manageDockingLayout();
    bool focusWindow(const char* id);

    void populateCollapsedLayout();
    void populateViewport();
    void populateRecordOptions();
    void populateExpandedLayout();
    void populateUI();
};

[[nodiscard]] constexpr MainWindow* main_window()
{
    return MainWindow::instance();
}

BL_END_NS;
