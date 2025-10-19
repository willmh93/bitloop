#include <SDL3/SDL_dialog.h>

#include <bitloop/platform/platform.h>
#include <bitloop/core/main_window.h>
#include <bitloop/core/project_worker.h>
#include <imgui_stdlib.h>

#ifndef __EMSCRIPTEN__
#include <filesystem>
#include <fstream>
#endif

#include <regex>


BL_BEGIN_NS

MainWindow* MainWindow::singleton = nullptr;

ImDebugLog project_log;
ImDebugLog debug_log;



void ImDebugPrint(const char* fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    debug_log.vlog(fmt, ap);
    va_end(ap);
}

#ifndef __EMSCRIPTEN__
std::string getPreferredCapturesDirectory()
{
    std::filesystem::path path = platform()->executable_dir();
    std::filesystem::path trimmed;

    // todo: Make sure this is threadsafe
    // 
    // should be name of root CMake project, not the launched project?
    // They're usually the same on startup, so it should work in practice

    if (!project_worker()->getActiveProject())
    {
        DebugBreak();
    }

    std::string active_sim_name = project_worker()->getActiveProject()->getProjectInfo()->name;
       
    for (const auto& part : path) {
        trimmed /= part;
        if (!active_sim_name.empty() && part == active_sim_name) break;
        if (part == "bitloop") break;
    }
    if (!trimmed.empty() && trimmed.filename() == "bitloop")
        path = trimmed;

    path /= "captures";

    return path.lexically_normal().string();
}

static int find_max_clip_index(const std::filesystem::path& directory_path, const char* name, const char* extension)
{
    int max_clip_index = 0;
    std::error_code io_error;
    std::string regex = "^" + std::string(name) + "(\\d+)\\" + extension + "$";
    std::regex filename_pattern(regex, std::regex::icase);
    //std::regex filename_pattern("^clip(\\d+)\\.mp4$", std::regex::icase);

    for (const auto& dir_entry : std::filesystem::directory_iterator(
        directory_path, std::filesystem::directory_options::skip_permission_denied, io_error))
    {
        if (io_error) break; // stop on catastrophic error
        if (!dir_entry.is_regular_file()) continue;

        const std::string filename = dir_entry.path().filename().string();
        std::smatch match;
        if (std::regex_match(filename, match, filename_pattern)) {
            try {
                const int clip_index = std::stoi(match[1].str());
                if (clip_index > max_clip_index) max_clip_index = clip_index;
            }
            catch (...) {
                // ignore invalid numbers
            }
        }
    }
    return max_clip_index;
}

static std::filesystem::path make_next_clip_path(const std::filesystem::path& capture_dir, const char* name, const char* extension)
{
    std::error_code io_error;
    std::filesystem::create_directories(capture_dir, io_error); // no-op if exists
    const int next_index = find_max_clip_index(capture_dir, name, extension) + 1;
    return (name + std::to_string(next_index) + extension);
}

static bool ensure_parent_directories_exist(const std::filesystem::path& file_path, std::error_code& ec)
{
    ec.clear();
    const std::filesystem::path parent = file_path.parent_path();
    if (parent.empty()) return true; // nothing to create
    return std::filesystem::create_directories(parent, ec);
}

static inline bool is_x265(const char* s)
{
    if (!s) return false;
    std::string t(s); for (auto& c : t) c = (char)tolower(c);
    return (t == "x265" || t == "hevc" || t == "h265");
}

static inline double lerp_log(double a, double b, double t) {
    return std::exp(std::log(a) + t * (std::log(b) - std::log(a)));
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

void MainWindow::init()
{
    ImGui::LoadIniSettingsFromMemory("");

    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    // Always initializes window on first call
    checkChangedDPR();

    canvas.create(platform()->dpr());
    //canvas.create(5.0);
    //canvas.create(0.75);

    #ifndef __EMSCRIPTEN__
    bitrate_mbps_range = recommended_bitrate_range_mbps(record_resolution, record_fps, VideoCodecFromCaptureFormat((CaptureFormat)record_format));
    record_bitrate = (int64_t)(1000000.0 * choose_bitrate_mbps_from_range(bitrate_mbps_range, record_quality));
    #endif
}

void MainWindow::checkChangedDPR()
{
    platform()->update();

    static float last_dpr = -1.0f;
    if (std::fabs(platform()->dpr() - last_dpr) > 0.01f)
    {
        // DPR changed
        last_dpr = platform()->dpr();

        initStyles();
        initFonts();
    }
}

void MainWindow::initStyles()
{
    ImGuiStyle& style = ImGui::GetStyle();

    // Corners
    style.WindowRounding = 8.0f;
    style.ChildRounding = 8.0f;
    style.FrameRounding = 6.0f;
    style.FramePadding = ImVec2(8.0f, 5.0f);
    style.PopupRounding = 3.0f;
    style.GrabRounding = 2.0f;
    style.TabRounding = 6.0f;
    style.ScrollbarRounding = platform()->is_mobile() ? 12.0f : 6.0f;
    style.ScrollbarSize = platform()->is_mobile() ? 30.0f : 20.0f;

    // Colors
    ImVec4* colors = ImGui::GetStyle().Colors;

    // Base colors for a pleasant and modern dark theme with dark accents
    colors[ImGuiCol_Text] = ImVec4(0.92f, 0.93f, 0.94f, 1.00f);                  // Light grey text for readability
    colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.52f, 0.54f, 1.00f);          // Subtle grey for disabled text
    colors[ImGuiCol_WindowBg] = ImVec4(0.14f, 0.14f, 0.16f, 1.00f);              // Dark background with a hint of blue
    colors[ImGuiCol_ChildBg] = ImVec4(0.16f, 0.16f, 0.18f, 1.00f);               // Slightly lighter for child elements
    colors[ImGuiCol_PopupBg] = ImVec4(0.18f, 0.18f, 0.20f, 1.00f);               // Popup background
    colors[ImGuiCol_Border] = ImVec4(0.28f, 0.29f, 0.30f, 0.60f);                // Soft border color
    colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);          // No border shadow
    colors[ImGuiCol_FrameBg] = ImVec4(0.20f, 0.22f, 0.24f, 1.00f);               // Frame background
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.22f, 0.24f, 0.26f, 1.00f);        // Frame hover effect
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.24f, 0.26f, 0.28f, 1.00f);         // Active frame background
    colors[ImGuiCol_TitleBg] = ImVec4(0.14f, 0.14f, 0.16f, 1.00f);               // Title background
    colors[ImGuiCol_TitleBgActive] = ImVec4(0.16f, 0.16f, 0.18f, 1.00f);         // Active title background
    colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.14f, 0.14f, 0.16f, 1.00f);      // Collapsed title background
    colors[ImGuiCol_MenuBarBg] = ImVec4(0.20f, 0.20f, 0.22f, 1.00f);             // Menu bar background
    colors[ImGuiCol_ScrollbarBg] = ImVec4(0.16f, 0.16f, 0.18f, 1.00f);           // Scrollbar background
    colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.24f, 0.26f, 0.28f, 1.00f);         // Dark accent for scrollbar grab
    colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.28f, 0.30f, 0.32f, 1.00f);  // Scrollbar grab hover
    colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.32f, 0.34f, 0.36f, 1.00f);   // Scrollbar grab active
    colors[ImGuiCol_CheckMark] = ImVec4(0.46f, 0.56f, 0.66f, 1.00f);             // Dark blue checkmark
    colors[ImGuiCol_SliderGrab] = ImVec4(0.36f, 0.46f, 0.56f, 1.00f);            // Dark blue slider grab
    colors[ImGuiCol_SliderGrabActive] = ImVec4(0.40f, 0.50f, 0.60f, 1.00f);      // Active slider grab
    colors[ImGuiCol_Button] = ImVec4(0.24f, 0.34f, 0.44f, 1.00f);                // Dark blue button
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.28f, 0.38f, 0.48f, 1.00f);         // Button hover effect
    colors[ImGuiCol_ButtonActive] = ImVec4(0.32f, 0.42f, 0.52f, 1.00f);          // Active button
    colors[ImGuiCol_Header] = ImVec4(0.24f, 0.34f, 0.44f, 1.00f);                // Header color similar to button
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.28f, 0.38f, 0.48f, 1.00f);         // Header hover effect
    colors[ImGuiCol_HeaderActive] = ImVec4(0.32f, 0.42f, 0.52f, 1.00f);          // Active header
    colors[ImGuiCol_Separator] = ImVec4(0.28f, 0.29f, 0.30f, 1.00f);             // Separator color
    colors[ImGuiCol_SeparatorHovered] = ImVec4(0.46f, 0.56f, 0.66f, 1.00f);      // Hover effect for separator
    colors[ImGuiCol_SeparatorActive] = ImVec4(0.46f, 0.56f, 0.66f, 1.00f);       // Active separator
    colors[ImGuiCol_ResizeGrip] = ImVec4(0.36f, 0.46f, 0.56f, 1.00f);            // Resize grip
    colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.40f, 0.50f, 0.60f, 1.00f);     // Hover effect for resize grip
    colors[ImGuiCol_ResizeGripActive] = ImVec4(0.44f, 0.54f, 0.64f, 1.00f);      // Active resize grip
    colors[ImGuiCol_Tab] = ImVec4(0.20f, 0.22f, 0.24f, 1.00f);                   // Inactive tab
    colors[ImGuiCol_TabHovered] = ImVec4(0.28f, 0.38f, 0.48f, 1.00f);            // Hover effect for tab
    colors[ImGuiCol_TabActive] = ImVec4(0.24f, 0.34f, 0.44f, 1.00f);             // Active tab color
    colors[ImGuiCol_TabUnfocused] = ImVec4(0.20f, 0.22f, 0.24f, 1.00f);          // Unfocused tab
    colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.24f, 0.34f, 0.44f, 1.00f);    // Active but unfocused tab
    colors[ImGuiCol_PlotLines] = ImVec4(0.46f, 0.56f, 0.66f, 1.00f);             // Plot lines
    colors[ImGuiCol_PlotLinesHovered] = ImVec4(0.46f, 0.56f, 0.66f, 1.00f);      // Hover effect for plot lines
    colors[ImGuiCol_PlotHistogram] = ImVec4(0.36f, 0.46f, 0.56f, 1.00f);         // Histogram color
    colors[ImGuiCol_PlotHistogramHovered] = ImVec4(0.40f, 0.50f, 0.60f, 1.00f);  // Hover effect for histogram
    colors[ImGuiCol_TableHeaderBg] = ImVec4(0.20f, 0.22f, 0.24f, 1.00f);         // Table header background
    colors[ImGuiCol_TableBorderStrong] = ImVec4(0.28f, 0.29f, 0.30f, 1.00f);     // Strong border for tables
    colors[ImGuiCol_TableBorderLight] = ImVec4(0.24f, 0.25f, 0.26f, 1.00f);      // Light border for tables
    colors[ImGuiCol_TableRowBg] = ImVec4(0.20f, 0.22f, 0.24f, 1.00f);            // Table row background
    colors[ImGuiCol_TableRowBgAlt] = ImVec4(0.22f, 0.24f, 0.26f, 1.00f);         // Alternate row background
    colors[ImGuiCol_TextSelectedBg] = ImVec4(0.24f, 0.34f, 0.64f, 0.85f);        // Selected text background
    colors[ImGuiCol_DragDropTarget] = ImVec4(0.46f, 0.56f, 0.66f, 0.90f);        // Drag and drop target
    colors[ImGuiCol_NavHighlight] = ImVec4(0.46f, 0.56f, 0.66f, 1.00f);          // Navigation highlight
    colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f); // Windowing highlight
    colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);     // Dim background for windowing
    colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);      // Dim background for modal windows

    style.ScaleAllSizes(platform()->ui_scale_factor());
}

void MainWindow::initFonts()
{
    // Update FreeType fonts
    ImGuiIO& io = ImGui::GetIO();
    
    float base_pt = 16.0f;
    std::string font_path = platform()->path("/data/fonts/DroidSans.ttf");
    std::string font_path_mono = platform()->path("/data/fonts/UbuntuMono.ttf");
    ImFontConfig config;
    config.OversampleH = 3;
    config.OversampleV = 3;
    
    io.Fonts->Clear();
    main_font = io.Fonts->AddFontFromFileTTF(font_path.c_str(), base_pt * platform()->dpr() * platform()->font_scale(), &config);
    mono_font = io.Fonts->AddFontFromFileTTF(font_path_mono.c_str(), base_pt * platform()->dpr() * platform()->font_scale(), &config);
    io.Fonts->Build();
}

void MainWindow::populateProjectUI()
{
    ImGui::BeginPaddedRegion(scale_size(10.0f));
    project_worker()->populateAttributes();
    ImGui::EndPaddedRegion();
}

void MainWindow::queueBeginRecording()
{
    queueMainWindowCommand({ MainWindowCommandType::BEGIN_RECORDING });
}

void MainWindow::queueEndRecording()
{
    queueMainWindowCommand({ MainWindowCommandType::END_RECORDING });
}

void MainWindow::beginRecording()
{
    assert(!capture_manager.isRecording());
    assert(!capture_manager.isSnapshotting());

    // Don't capture until next frame starts (and sets this true)
    capture_manager.setCaptureEnabled(false);

    #ifndef __EMSCRIPTEN__
    std::filesystem::path capture_dir = getPreferredCapturesDirectory();

    std::string active_sim_name = project_worker()->getActiveProject()->getProjectInfo()->name;

    // Organize by project name
    capture_dir /= active_sim_name;

    switch ((CaptureFormat)record_format)
    {
        case CaptureFormat::x264:
        case CaptureFormat::x265:
            capture_dir /= "video";
            capture_dir /= make_next_clip_path(capture_dir, "clip", ".mp4");
            break;

        case CaptureFormat::WEBP_VIDEO:
            capture_dir /= "animation";
            capture_dir /= make_next_clip_path(capture_dir, "clip", ".webp");
    }

    std::error_code ec;
    ensure_parent_directories_exist(capture_dir, ec);

    capture_manager.startCapture(
        (CaptureFormat)record_format,
        capture_dir.string(),
        record_resolution,
        snapshot_supersample_factor,
        snapshot_sharpen,
        record_fps,
        record_frame_count,
        record_bitrate,
        (float)record_quality,
        record_lossless,
        record_near_lossless,
        true);
    #else
    capture_manager.startCapture(
        (CaptureFormat)record_format,
        record_resolution,
        snapshot_supersample_factor,
        snapshot_sharpen,
        record_fps,
        record_frame_count,
        0,
        record_quality,
        record_lossless,
        record_near_lossless,
        true);
    #endif
}



void MainWindow::beginSnapshot()
{
    assert(!capture_manager.isRecording());
    assert(!capture_manager.isSnapshotting());

    // Don't capture until next frame starts (and sets this true)
    capture_manager.setCaptureEnabled(false);

    #ifndef __EMSCRIPTEN__
    std::string active_sim_name = project_worker()->getActiveProject()->getProjectInfo()->name;

    // Organize by project name
    std::filesystem::path dir = getPreferredCapturesDirectory();
    dir /= active_sim_name;
    dir /= "image";

    std::filesystem::path filename = dir;
    filename /= make_next_clip_path(dir, "snap", ".webp");

    std::error_code ec;
    ensure_parent_directories_exist(filename, ec);

    capture_manager.startCapture(
        (CaptureFormat)snapshot_format,
        filename.string(),
        snapshot_resolution,
        snapshot_supersample_factor,
        snapshot_sharpen);
    #else
    capture_manager.startCapture(
        (CaptureFormat)snapshot_format,
        snapshot_resolution,
        snapshot_supersample_factor,
        snapshot_sharpen);
    #endif
}

void MainWindow::endRecording()
{
    record.toggled = false;

    if (capture_manager.isRecording())
        capture_manager.finalizeCapture();
}

void MainWindow::handleCommand(MainWindowCommandEvent e)
{
    // Handle commands which can be initiated by both GUI thread & project-worker thread
    switch (e.type)
    {
    case MainWindowCommandType::ON_STARTED_PROJECT:
    {
        // received from project-worker once project succesfully started
        play.enabled = false;
        stop.enabled = true;
        pause.enabled = true;
        snapshot.enabled = true;

        static bool first = true;
        if (first)
        {
            // set initial preferred folder once
            #ifndef __EMSCRIPTEN__
            capture_dir = getPreferredCapturesDirectory();
            #endif

            first = false;
        }
    }
    break;

    case MainWindowCommandType::ON_STOPPED_PROJECT:
        if (capture_manager.isRecording())
            capture_manager.finalizeCapture();

        stop.enabled = false;
        pause.enabled = false;
        play.enabled = true;
        record.toggled = false;
        snapshot.enabled = false;

        break;

    case MainWindowCommandType::ON_PAUSED_PROJECT:
        pause.enabled = false;
        play.enabled = true;
        break;


    case MainWindowCommandType::BEGIN_RECORDING:
        beginRecording();
        break;

    case MainWindowCommandType::END_RECORDING:
        endRecording();
        break;
    }
}

/// ======== Toolbar ========

void MainWindow::drawToolbarButton(ImDrawList* drawList, ImVec2 pos, ImVec2 size, const char* symbol, ImU32 color)
{
    float cx = pos.x + size.x * 0.5f;
    float cy = pos.y + size.y * 0.5f;
    float r = ImMin(size.x, size.y) * 0.25f;

    if (strcmp(symbol, "play") == 0) {
        ImVec2 p1(cx - r * 0.6f, cy - r);
        ImVec2 p2(cx - r * 0.6f, cy + r);
        ImVec2 p3(cx + r, cy);
        drawList->AddTriangleFilled(p1, p2, p3, color);
    }
    else if (strcmp(symbol, "stop") == 0) {
        drawList->AddRectFilled(ImVec2(cx - r, cy - r), ImVec2(cx + r, cy + r), color);
    }
    else if (strcmp(symbol, "pause") == 0) {
        float w = r * 0.4f;
        drawList->AddRectFilled(ImVec2(cx - r, cy - r), ImVec2(cx - r + w, cy + r), color);
        drawList->AddRectFilled(ImVec2(cx + r - w, cy - r), ImVec2(cx + r, cy + r), color);
    }
    else if (strcmp(symbol, "record") == 0) {
        drawList->AddCircleFilled(ImVec2(cx, cy), r, color);
    }
    else if (strcmp(symbol, "snapshot") == 0) {
        cy += r * 0.2f;

        // Scales with the button rect
        float u = ImMin(size.x, size.y);
        float t = ImClamp(u * 0.06f, 1.0f, 3.0f);   // stroke thickness
        float round = u * 0.10f;                    // body corner radius

        // Camera body
        float body_w = u * 0.62f;
        float body_h = u * 0.46f;
        ImVec2 body_min(cx - body_w * 0.5f, cy - body_h * 0.5f);
        ImVec2 body_max(cx + body_w * 0.5f, cy + body_h * 0.5f);
        drawList->AddRect(body_min, body_max, color, round, 0, t);

        // Top hump (viewfinder housing)
        float hump_w = u * 0.26f;
        float hump_h = u * 0.12f;
        ImVec2 hump_min(body_min.x + u * 0.06f, body_min.y - hump_h + t * 0.5f);
        ImVec2 hump_max(hump_min.x + hump_w, body_min.y + t * 0.5f);
        drawList->AddRect(hump_min, hump_max, color, round * 0.5f, 0, t);

        // Lens (outer ring)
        float lens_r = u * 0.14f;
        drawList->AddCircle(ImVec2(cx, cy), lens_r, color, 0, t);

        // Lens inner accent (tiny dot)
        drawList->AddCircleFilled(ImVec2(cx + lens_r * 0.35f, cy - lens_r * 0.35f),
            ImMax(1.0f, u * 0.02f), color);

        // Shutter button nub (small filled circle on top-right)
        ImVec2 nub(hump_max.x + u * 0.04f, hump_min.y + (hump_h * 0.35f));
        drawList->AddCircleFilled(nub, ImMax(1.0f, u * 0.02f), color);
    }
}

bool MainWindow::toolbarButton(const char* id, const char* symbol, const ToolbarButtonState& state, ImVec2 size, float inactive_alpha)
{
    auto icon_col =
        state.enabled ?
        state.symbolColor :
        ImVec4(state.symbolColor.x, state.symbolColor.y, state.symbolColor.z, inactive_alpha);

    auto bg_col =
        state.enabled ?
        state.bgColor :
        ImVec4(state.bgColor.x, state.bgColor.y, state.bgColor.z, inactive_alpha);

    if (state.toggled)
        bg_col = state.bgColorToggled;

    ImGui::PushStyleColor(ImGuiCol_Button, bg_col);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, bg_col);
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, bg_col);
    bool pressed = ImGui::Button(id, size);
    ImGui::PopStyleColor(3);

    ImDrawList* drawList = ImGui::GetWindowDrawList();
    ImVec2 p = ImGui::GetItemRectMin();
    ImVec2 sz = ImGui::GetItemRectSize();

    drawToolbarButton(drawList, p, sz, symbol, ImGui::ColorConvertFloat4ToU32(icon_col));

    if (state.enabled)
        return pressed;

    return false;
}

void MainWindow::populateToolbar()
{
    if (platform()->is_mobile())
        return;

    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(10, 0));

    ImGui::Spacing();
    ImGui::Spacing();

    float size = scale_size(30.0f);
    ImVec2 avail = ImGui::GetContentRegionAvail();
    ImVec2 toolbarSize(avail.x, size); // Outer frame size
    ImVec4 frameColor = ImVec4(0, 0, 0, 0); // Toolbar background color


    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 0.0f);
    ImGui::PushStyleColor(ImGuiCol_ChildBg, frameColor);

    ImGui::BeginChild("ToolbarFrame", toolbarSize, false,
        ImGuiWindowFlags_AlwaysUseWindowPadding |
        ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoScrollWithMouse);

    // Layout the buttons inside the frame
    if (toolbarButton("##play", "play", play, ImVec2(size, size)))
    {
        // immediately disable play button, handle other button states in handleCommand(..)
        // in response to the project-worker succesfully switching project.
        play.enabled = false;

        // begin recording immediately if record button "on" so we start capturing
        // from the very first frame
        if (record.toggled)
            beginRecording();

        project_worker()->startProject();
    }

    ImGui::SameLine();
    if (toolbarButton("##pause", "pause", pause, ImVec2(size, size)))
    {
        // immediately disable pause button, handle other button states in handleCommand(..)
        // in response to the project-worker succesfully switching project.
        pause.enabled = false;

        project_worker()->pauseProject();
    }

    ImGui::SameLine();
    if (toolbarButton("##stop", "stop", stop, ImVec2(size, size)))
    {
        // immediately disable stop/pause buttons, handle other button states in handleCommand(..)
        // in response to the project-worker succesfully switching project.
        stop.enabled = false;
        pause.enabled = false;

        project_worker()->stopProject();
    }

    if (!platform()->is_mobile())
    {
        ImGui::SameLine();
        if (toolbarButton("##record", "record", record, ImVec2(size, size)))
        {
            if (!play.enabled)
            {
                // sim already running
                if (!record.toggled)
                {
                    // Start recording
                    record.toggled = true;
                    beginRecording();
                }
                else
                {
                    record.toggled = false;
                    capture_manager.finalizeCapture();
                }
            }
            else
            {
                // sim not started yet, but toggle state so it auto-begins
                // recording on project started
                record.toggled = !record.toggled;
            }
        }
    }

    ImGui::SameLine();
    if (toolbarButton("##snapshot", "snapshot", snapshot, ImVec2(size, size)))
    {
        snapshot.toggled = true;
        beginSnapshot();
    }

    // Always save capture output on main GUI thread (for Emscripten)
    if (capture_manager.isCaptureToMemoryComplete())
    {
        bytebuf data;
        capture_manager.takeCompletedCaptureFromMemory(data);

        #ifdef __EMSCRIPTEN__
        platform()->download_snapshot_webp(data, "snapshot.webp");
        #else
        std::ofstream out(capture_manager.filename(), std::ios::out | std::ios::binary);
        out.write((const char*)data.data(), data.size());
        out.close();
        #endif
    }

    ImGui::EndChild();

    ImGui::PopStyleColor();
    ImGui::PopStyleVar(2);

    ImGui::PopStyleVar();
    ImGui::Spacing();
    ImGui::Spacing();
}

/// ======== Project Tree ========

void MainWindow::populateProjectTreeNodeRecursive(ProjectInfoNode& node, int& i, int depth)
{
    //if (!node.visible)
    //    return;

    ImGui::PushID(i++);
    if (node.project_info)
    {
        // Leaf project node
        if (ImGui::Button(node.name.c_str()))
        {
            project_worker()->setActiveProject(node.project_info->sim_uid);
            project_worker()->startProject();
        }
    }
    else
    {
        int flags = ImGuiTreeNodeFlags_DefaultOpen;
        if (ImGui::CollapsingHeader(node.name.c_str(), flags))
        {
            ImGui::Indent();
            for (size_t child_i = 0; child_i < node.children.size(); child_i++)
            {
                populateProjectTreeNodeRecursive(node.children[child_i], i, depth + 1);
            }
            ImGui::Unindent();
        }
    }
    ImGui::PopID();
}

void MainWindow::populateProjectTree(bool expand_vertical)
{
    if (ProjectBase::projectInfoList().size() <= 1)
        return;

    ImVec2 frameSize = ImVec2(0.0f, expand_vertical ? 0 : 170.0f); // Let height auto-expand
    ImVec4 bg = ImGui::GetStyleColorVec4(ImGuiCol_ChildBg);
    //ImVec4 dim_bg = bg * 0.9f;
    ImVec4 dim_bg = ImVec4(bg.x * 0.9f, bg.y * 0.9f, bg.x * 0.9f, bg.w * 0.9f);
    dim_bg.w = bg.w;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 0.0f);
    {
        ImGui::BeginChild("TreeFrame", frameSize, 0, ImGuiWindowFlags_AlwaysUseWindowPadding);

        ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 8.0f);
        ImGui::PushStyleColor(ImGuiCol_ChildBg, dim_bg);

        ImGui::BeginChild("TreeFrameInner", ImVec2(0.0f, 0.0f), true);

        auto& tree = ProjectBase::projectTreeRootInfo();
        int i = 0;
        for (size_t child_i = 0; child_i < tree.children.size(); child_i++)
        {
            int depth = 0;
            populateProjectTreeNodeRecursive(tree.children[child_i], i, depth);
        }

        ImGui::EndChild();
        ImGui::PopStyleColor();
        ImGui::PopStyleVar();
    }
    ImGui::EndChild();
    ImGui::PopStyleVar(2);
}

/// ======== Main Window populate ========

bool MainWindow::manageDockingLayout()
{
    // Create a fullscreen dockspace
    ImGuiWindowFlags dockspace_flags =
        ImGuiWindowFlags_NoDocking |
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoBringToFrontOnFocus |
        ImGuiWindowFlags_NoNavFocus |
        ImGuiWindowFlags_NoBackground;

    const ImGuiViewport* viewport = ImGui::GetMainViewport();

    ImGui::SetNextWindowPos(viewport->Pos);
    ImGui::SetNextWindowSize(viewport->Size);
    ImGui::SetNextWindowViewport(viewport->ID);

    // Make main dock space that fills the entire imgui viewport
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::Begin("MainDockSpaceHost", nullptr, dockspace_flags);
    ImGui::PopStyleVar(3);

    ImGuiID dockspace_id = ImGui::GetID("MyDockspace");
    ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_None);
    ImGui::End();

    // Detect viewport resize events (in case we need to switch layout)
    static ImVec2 last_viewport_size = ImVec2(0, 0);
    ImVec2 current_viewport_size = ImGui::GetMainViewport()->Size;
    if (current_viewport_size.x != last_viewport_size.x ||
        current_viewport_size.y != last_viewport_size.y)
    {
        update_docking_layout = true;
        last_viewport_size = current_viewport_size;
    }

    // Determine landscape/portrait layout
    vertical_layout = (last_viewport_size.x < last_viewport_size.y);

    // Build initial layout (once)
    if (!initialized || update_docking_layout)
    {
        initialized = true;
        update_docking_layout = false;

        if (ImGui::GetMainViewport()->Size.x <= 0 ||
            ImGui::GetMainViewport()->Size.y <= 0)
        {
            return false;
        }

        // Remove previous dockspace and rebuild
        ImGui::DockBuilderRemoveNode(dockspace_id);
        ImGui::DockBuilderAddNode(dockspace_id, ImGuiDockNodeFlags_DockSpace);
        ImGui::DockBuilderSetNodeSize(dockspace_id, ImGui::GetMainViewport()->Size);

        ImGuiID dock_main_id = dockspace_id;
        ImGuiID dock_sidebar = ImGui::DockBuilderSplitNode(
            dock_main_id,
            vertical_layout ? ImGuiDir_Down : ImGuiDir_Right,
            vertical_layout ? 0.4f : 0.25f,
            nullptr,
            &dock_main_id
        );

        if (ImGuiDockNode* n = ImGui::DockBuilderGetNode(dock_sidebar))
            n->LocalFlags |= ImGuiDockNodeFlags_AutoHideTabBar;
        //if (ImGuiDockNode* n = ImGui::DockBuilderGetNode(dock_main_id))
        //    n->LocalFlags |= ImGuiDockNodeFlags_AutoHideTabBar;

        ImGui::DockBuilderDockWindow("Projects",    dock_sidebar);  // dock to sidebar
        ImGui::DockBuilderDockWindow("Active",      dock_sidebar);  // dock to sidebar
        ImGui::DockBuilderDockWindow("Debug",       dock_sidebar);  // dock to sidebar
        ImGui::DockBuilderDockWindow("Settings",    dock_sidebar);  // dock to sidebar
        ImGui::DockBuilderDockWindow("Viewport",    dock_main_id);  // dock to window
        
        ImGui::DockBuilderFinish(dockspace_id);
    }
    return true;
}

bool MainWindow::focusWindow(const char* id)
{
    // SetWindowFocus doesn't switch focused tab unless second frame, so 
    // never call on first frame and wait for the next call
    static bool first_frame = true;
    bool ret = false;
    if (initialized && !first_frame)
    {
        ImGui::SetWindowFocus(id);
        ret = true;
    }
    first_frame = false;
    return ret;
}

void ScrollWhenDraggingOnVoid(const ImVec2& delta)
{
    ImGuiContext& g = *ImGui::GetCurrentContext();
    ImGuiWindow* window = g.CurrentWindow;
    bool hovered = false;
    bool held = false;
    ImGuiID id = window->GetID("##scrolldraggingoverlay");
    ImGui::KeepAliveID(id);

    static float vy = 0.0;
    static float scroll_amount = 0.0;

    

    //if (g.HoveredId == 0) // If nothing hovered so far in the frame (not same as IsAnyItemHovered()!)
        ImGui::ButtonBehavior(window->Rect(), id, &hovered, &held, ImGuiButtonFlags_MouseButtonLeft);
    //if (held && delta.x != 0.0f)
    //    ImGui::SetScrollX(window, window->Scroll.x + delta.x);

    if (held) vy = delta.y;
    scroll_amount = vy;
    vy *= 0.93f;

    if (/*held && */scroll_amount != 0.0f)
        ImGui::SetScrollY(window, window->Scroll.y + scroll_amount);
}

static void SwipeScrollWindow(float decay = 0.93f, float drag_threshold = scale_size(20.0f))
{
    if (!platform()->is_mobile())
        return;

    ImGuiContext& g = *ImGui::GetCurrentContext();
    ImGuiIO& io = g.IO;
    ImGuiWindow* window = g.CurrentWindow;
    if (!window) return;

    // State - TODO: global for now, make per-window
    static bool   scrolling = false;
    static ImVec2 press_pos = ImVec2(0, 0);
    static float  vy = 0.0f;

    const bool hovered_anyhow = ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);
    const bool down = io.MouseDown[0];
    const bool up = !down;

    // Begin potential gesture
    if (!scrolling && hovered_anyhow && ImGui::IsMouseClicked(0))
    {
        press_pos = io.MousePos;
        vy = 0.0f;
    }

    // Decide when to take over
    if (!scrolling && down && hovered_anyhow)
    {
        const float dy = io.MousePos.y - press_pos.y;
        if (fabsf(dy) >= drag_threshold)
        {
            // Take over: cancel the currently active item so it won't trigger on release
            if (g.ActiveId != 0)
                ImGui::ClearActiveID();

            // Optionally set ourselves as active (not strictly required but keeps ID bookkeeping tidy)
            ImGuiID id = window->GetID("##swipe_scroll_captor");
            ImGui::SetActiveID(id, window);

            scrolling = true;
        }
    }

    // While dragging, update velocity and scroll
    if (scrolling && down)
    {
        vy = -io.MouseDelta.y; // swipe up -> scroll down; adjust sign to taste
        if (vy != 0.0f)
            ImGui::SetScrollY(window, window->Scroll.y + vy);
    }

    // On release, keep inertia; while idle, decay velocity
    if (scrolling && up)
    {
        // free active id if we grabbed it
        if (g.ActiveId != 0 && g.ActiveId == window->GetID("##swipe_scroll_captor"))
            ImGui::ClearActiveID();

        // keep scrolling with decay until velocity dies
        if (fabsf(vy) < 0.01f)
        {
            vy = 0.0f;
            scrolling = false;
        }
    }

    // Apply inertial decay when not dragging
    if (vy != 0.0f && !down)
    {
        ImGui::SetScrollY(window, window->Scroll.y + vy);
        vy *= decay;
        if (fabsf(vy) < 0.01f)
            vy = 0.0f;
    }
}


void MainWindow::populateCollapsedLayout()
{
    // Collapse layout
    if (ImGui::Begin("Projects", nullptr, window_flags))
    {
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, scale_size(6.0f, 6.0f));
        ImGui::BeginChild("AttributesFrame", ImVec2(0, 0), 0, ImGuiWindowFlags_AlwaysUseWindowPadding);
        populateToolbar();

        populateProjectTree(true);
        SwipeScrollWindow();

        ImGui::EndChild();
        ImGui::PopStyleVar();
    }
    ImGui::End();
    
    if (ImGui::Begin("Active", nullptr, window_flags))
    {
        // Only add padding after toolbar to inner-child
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, scale_size(8.0f, 8.0f));
        ImGui::BeginChild("AttributesFrameInner", ImVec2(0, 0), 0, ImGuiWindowFlags_AlwaysUseWindowPadding);
        populateToolbar();
        ImGui::Dummy(scale_size(0, 6));

        populateProjectUI();
        SwipeScrollWindow();

        ImGui::EndChild();
        ImGui::PopStyleVar();
    }

    ImGui::End();
}

void MainWindow::populateExpandedLayout()
{

    // Show both windows
    if (ImGui::Begin("Projects", nullptr, window_flags))
    {
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, scale_size(6, 6));
        ImGui::BeginChild("AttributesFrame", ImVec2(0, 0), 0, ImGuiWindowFlags_AlwaysUseWindowPadding);
        populateToolbar();

        populateProjectTree(false);
        populateProjectUI();

        SwipeScrollWindow();

        ImGui::EndChild();
        ImGui::PopStyleVar();
    }

    ImGui::End();
}

void MainWindow::populateViewport()
{
    if (!command_queue.empty())
    {
        // We don't call populateAttributes() if holding shadow_buffer_mutex,
        // meaning we won't loop over scenes here while processing project commands
        std::vector<MainWindowCommandEvent> commands;
        {
            std::lock_guard<std::mutex> lock(command_mutex);
            commands = std::move(command_queue);
        }

        if (!commands.empty()) {
            for (auto& e : commands)
                handleCommand(e);
        }
    }

    // Always process viewport, even if not visible
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::Begin("Viewport");
    {
        ImVec2 client_size = ImGui::GetContentRegionAvail();

        // set canvas size to viewport size by default
        IVec2 canvas_size(client_size);

        // switch to record resolution if recording
        if (capture_manager.isRecording() || capture_manager.isSnapshotting())
            canvas_size = capture_manager.srcResolution();

        bool resized = canvas.resize(canvas_size.x, canvas_size.y);
        if (resized)
        {
            canvas.begin(0.05f, 0.05f, 0.1f, 1.0f);
            canvas.setFillStyle(0, 0, 0);
            canvas.fillRect(0, 0, (float)canvas.fboWidth(), (float)canvas.fboHeight());
            canvas.end();
        }

        if (!done_first_size)
        {
            done_first_size = true;
        }
        else if (shared_sync.project_thread_started)
        {
            // Launch startup simulation 1 frame late once we have a valid canvas size
            if (!project_worker()->getActiveProject())
            {
                auto first_project = ProjectBase::projectInfoList().front();
                project_worker()->setActiveProject(first_project->sim_uid);
                project_worker()->startProject();

                // Kick-start work-render-work-render loop
                need_draw = true;
            }
        }

        if (need_draw)
        {
            // Draw the fresh frame
            {
                std::unique_lock<std::mutex> lock(shared_sync.state_mutex);

                canvas.begin(0.05f, 0.05f, 0.1f, 1.0f);
                project_worker()->draw();
                canvas.end();

                // Even if not recording, behave as though we are for testing purposes
                captured_last_frame = encode_next_sim_frame;

                if (capture_manager.isRecording() || capture_manager.isSnapshotting())
                {
                    if (encode_next_sim_frame)
                    {
                        canvas.readPixels(frame_data);
                        capture_manager.encodeFrame(frame_data.data());
                    }
                }

                // Force worker to tell us when it wants to encode a new frame
                encode_next_sim_frame = false;

                shared_sync.frame_ready_to_draw = false;
                shared_sync.frame_consumed = true;
            }

            // Let worker continue
            shared_sync.cv.notify_one();
        }

        ///else if (resized)
        ///{
        ///    canvas.begin(0.05f, 0.05f, 0.1f, 1.0f);
        ///    canvas.setFillStyle(0, 0, 0);
        ///    canvas.fillRect(0, 0, (float)canvas.fboWidth(), (float)canvas.fboHeight());
        ///    canvas.end();
        ///}

        ImVec2 canvas_pos = ImGui::GetCursorScreenPos();
        ImVec2 image_size = (ImVec2)canvas_size;

        if (capture_manager.isRecording() || capture_manager.isSnapshotting())
        {
            float w  = (float)client_size.x, h  = (float)client_size.y; // render size
            float rw = (float)canvas_size.x, rh = (float)canvas_size.y; // client size
            float sw = w, sh = h, ox = 0, oy = 0;

            float client_aspect = (w / h);
            float render_aspect = (rw / rh);

            if (render_aspect > client_aspect)
            {
                sh = h * (client_aspect / render_aspect); // Render aspect is too wide
                oy = 0.5f * (client_size.y - sh);         // Center vertically
            }
            else
            {
                sw = w * (render_aspect / client_aspect); // Render aspect is too tall
                ox = 0.5f * (client_size.x - sw);         // Center horizontally
            }

            canvas_pos = { ox, oy };
            image_size = { sw, sh };
        }


        // Draw cached (or freshly generated) frame
        ImGui::SetCursorScreenPos(canvas_pos);
        ImGui::Image(canvas.texture(), image_size, ImVec2(0,1), ImVec2(1,0));

        //ImGui::SetItemAllowOverlap();              // allow next item to overlap this one
        //ImGui::SetCursorScreenPos(canvas_pos);
        //ImGui::Image(overlay.texture(), client_size, ImVec2(0.0f, 1.0f), ImVec2(1.0f, 0.0f));

        viewport_hovered = ImGui::IsWindowHovered();
    }
    ImGui::End();
    ImGui::PopStyleVar();
}

static void on_folder_chosen(
    [[maybe_unused]] void* userdata,
    const char* const* filelist,
    [[maybe_unused]] int filter)
{
    if (!filelist) { SDL_Log("Dialog error"); return; }
    if (!*filelist) { SDL_Log("User canceled"); return; }
    blPrint() << "Chosen folder: " << filelist[0]; // first (and only) entry for folder dialog
}

void MainWindow::populateRecordOptions()
{
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, scale_size(6, 6));
    ImGui::BeginChild("RecordingFrame", ImVec2(0, 0), 0, ImGuiWindowFlags_AlwaysUseWindowPadding);

    // Paths
    #ifndef __EMSCRIPTEN__
    {
        ImGui::GroupBox box("dir_box", "Paths");
        ImGui::Text("Media Output Directory:");
        ImGui::InputText("##folder", &capture_dir, ImGuiInputTextFlags_EnterReturnsTrue);
        ImGui::SameLine();
        if (ImGui::Button("..."))
        {
            SDL_ShowOpenFolderDialog(on_folder_chosen, NULL, platform()->sdl_window(), capture_dir.c_str(), false);
        }
    }
    #endif

    //ImGui::SeparatorText("Image Capture");

    // Global Options
    {
        //ImGui::GroupBox box("img_resolution", "Snapshot Resolution");
        ImGui::GroupBox box("shared_options", "Global Capture Options");

        ImGui::Checkbox("Delta-Time Multiplier", &use_delta_time_mult);

        ImGui::Spacing();
        ImGui::Spacing();
        ImGui::Text("FPS:");
        if (ImGui::InputInt("##fps", &record_fps, 1))
        {
            record_fps = std::clamp(record_fps, 1, 100);

            #ifndef __EMSCRIPTEN__
            bitrate_mbps_range = recommended_bitrate_range_mbps(record_resolution, record_fps, VideoCodecFromCaptureFormat((CaptureFormat)record_format));
            record_bitrate = (int64_t)(1000000.0 * choose_bitrate_mbps_from_range(bitrate_mbps_range, record_quality));
            #endif
        }

        ImGui::Spacing();
        ImGui::Spacing();
        ImGui::Text("Supersample Factor (SSAA)");
        ImGui::SliderInt("##ssaa", &snapshot_supersample_factor, 1, 4, "%dx");
        //if (ImGui::InputInt("##ssaa", &snapshot_supersample_factor, 1, 1))
        //    snapshot_supersample_factor = std::clamp(snapshot_supersample_factor, 1, 3);

        ImGui::Spacing(); ImGui::Spacing();
        ImGui::Text("Sharpen");
        ImGui::SliderFloat("##sharpen", &snapshot_sharpen, 0.0f, 1.0f, "%.1f");
    }

    // Image Resolution
    {
        //ImGui::GroupBox box("img_resolution", "Snapshot Resolution");
        ImGui::GroupBox box("img_options", "Image Capture");

        ImGui::Text("X:"); ImGui::SameLine();
        if (ImGui::InputInt("##img_res_x", &snapshot_resolution.x, 10)) {
            snapshot_resolution.x = std::clamp(snapshot_resolution.x, 16, 16382);
            snapshot_resolution.x = (snapshot_resolution.x & ~1);
        }
        ImGui::Text("Y:"); ImGui::SameLine();
        if (ImGui::InputInt("##img_res_y", &snapshot_resolution.y, 10)) {
            snapshot_resolution.y = std::clamp(snapshot_resolution.y, 16, 16382);
            snapshot_resolution.y = (snapshot_resolution.y & ~1);
        }
    }

    //ImGui::SeparatorText("Video Capture");

    // Video Resolution
    {
        //ImGui::GroupBox box("vid_resolution", "Video Resolution");
        ImGui::GroupBox box("vid_options", "Video Capture");

        ImGui::Spacing();

        ImGui::Text("Codec:");
        if (ImGui::Combo("##codec", &record_format, "H.264 (x264)\0H.265 / HEVC (x265)\0WebP Animation\0"))
        {
            #ifndef __EMSCRIPTEN__
            bitrate_mbps_range = recommended_bitrate_range_mbps(record_resolution, record_fps, VideoCodecFromCaptureFormat((CaptureFormat)record_format));
            record_bitrate = (int64_t)(1000000.0 * choose_bitrate_mbps_from_range(bitrate_mbps_range, record_quality));
            #endif
        }

        ImGui::Spacing();
        ImGui::Spacing();
        ImGui::Spacing();

        #ifndef __EMSCRIPTEN__
        if (record_format != (int)CaptureFormat::WEBP_VIDEO)
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

            struct PresetList { const Preset* p; int x264_count;  int x265_count; };
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

            // selection state
            static int sel_aspect = -1;
            static int sel_tier = -1;
            static bool sel_aspect_was_portrait = false;

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
                PresetList L = get_preset_list(sel_aspect);
                if (!L.p || sel_tier >= L.x265_count) return; // safety
                out.x = set_even(L.p[sel_tier].w);
                out.y = set_even(L.p[sel_tier].h);
                bitrate_mbps_range = recommended_bitrate_range_mbps(record_resolution, record_fps, VideoCodecFromCaptureFormat((CaptureFormat)record_format));
                record_bitrate = (int64_t)(1000000.0 * choose_bitrate_mbps_from_range(bitrate_mbps_range, record_quality));
            };

            // auto-select from current resolution
            auto try_select_from_resolution = [&](int w, int h)
            {
                sel_aspect = -1;
                sel_tier = -1;

                bitrate_mbps_range = recommended_bitrate_range_mbps(record_resolution, record_fps, VideoCodecFromCaptureFormat((CaptureFormat)record_format));
                record_bitrate = (int64_t)(1000000.0 * choose_bitrate_mbps_from_range(bitrate_mbps_range, record_quality));

                for (int ai = 0; ai < IM_ARRAYSIZE(aspects); ++ai) {
                    PresetList L = get_preset_list(ai);
                    for (int ti = 0; ti < L.x265_count; ++ti) {
                        if (L.p[ti].w == w && L.p[ti].h == h) {
                            sel_aspect = ai;
                            sel_tier = ti;
                            return;
                        }
                    }
                }

            };

            bool changed = false;

            ImGui::Text("X:"); ImGui::SameLine();
            if (ImGui::InputInt("##vid_res_x", &record_resolution.x, 10)) {
                record_resolution.x = std::clamp(record_resolution.x, 16, 16384);
                record_resolution.x = (record_resolution.x & ~1); // force even
                changed = true;
            }
            ImGui::Text("Y:"); ImGui::SameLine();
            if (ImGui::InputInt("##vid_res_y", &record_resolution.y, 10)) {
                record_resolution.y = std::clamp(record_resolution.y, 16, 16384);
                record_resolution.y = (record_resolution.y & ~1); // force even
                changed = true;
            }

            static bool first = true;
            if (first) { try_select_from_resolution(record_resolution.x, record_resolution.y); first = false; }
            if (changed) try_select_from_resolution(record_resolution.x, record_resolution.y);

            // Helper: choose the closest tier (by height) to current H for a given aspect list
            auto pick_closest_tier = [&](int aspect_index, int current_h) -> int
            {
                PresetList preset_list = get_preset_list(aspect_index);
                if (!preset_list.p || preset_list.x265_count == 0) return -1;
                int best = 0;
                int best_d = INT_MAX;
                for (int i = 0; i < preset_list.x265_count; ++i) {
                    int h = preset_list.p[i].h; // works for portrait lists too (we stored exact WxH)
                    int d = std::abs(h - current_h);
                    if (d < best_d) { best_d = d; best = i; }
                }
                return best;
            };

            ImGui::Spacing();
            ImGui::Spacing();
            ImGui::TextUnformatted("Aspect:");
            for (int i = 0; i < (int)IM_ARRAYSIZE(aspects); ++i) {
                bool is_selected = (sel_aspect == i);
                int pushed = pushSelected(is_selected);
                if (ImGui::Button(aspects[i].label)) {
                    int old_aspect = sel_aspect;
                    sel_aspect = i;

                    // Get new list
                    PresetList new_preset_list = get_preset_list(sel_aspect);

                    // If switching portrait <-> landscape or tier is out of range, choose a tier
                    bool switched_portrait =
                        (old_aspect >= 0) &&
                        (aspects[old_aspect].portrait != aspects[sel_aspect].portrait);

                    bool tier_invalid = (sel_tier < 0 || !new_preset_list.p || sel_tier >= new_preset_list.x265_count);

                    if (switched_portrait || tier_invalid)
                    {
                        // Prefer closest tier to current height; fallback to 0
                        int current_h = record_resolution.y;
                        int t = pick_closest_tier(sel_aspect, current_h);
                        sel_tier = (t >= 0 ? t : 0);
                    }
                    else
                    {
                        int valid_tier_count = (record_format == (int)CaptureFormat::x264) ?
                            new_preset_list.x264_count :
                            new_preset_list.x265_count;


                        // Clamp to new list length just in case
                        sel_tier = (sel_tier >= valid_tier_count) ? (valid_tier_count - 1) : sel_tier;
                    }

                    // Apply the (aspect, tier) to actually change WxH
                    apply_selection(record_resolution);
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
                bool unsupported = (record_format == (int)CaptureFormat::x264) && preset_list.p[i].HEVC_only;
                if (unsupported) ImGui::BeginDisabled();
                if (ImGui::Button(preset_list.p[i].label)) {
                    sel_aspect = ai;
                    sel_tier = i;
                    apply_selection(record_resolution);
                }
                if (unsupported) ImGui::EndDisabled();
                if (pushed) ImGui::PopStyleColor(pushed);
                if ((i + 1) % per_row != 0 && (i + 1) < preset_list.x265_count) ImGui::SameLine(0, 6);
            }
        }
        else
        #endif
        {
            ImGui::Text("X:"); ImGui::SameLine();
            if (ImGui::InputInt("##vid_res_x", &record_resolution.x, 10)) {
                record_resolution.x = std::clamp(record_resolution.x, 16, 1024);
                record_resolution.x = (record_resolution.x & ~1); // force even
            }
            ImGui::Text("Y:"); ImGui::SameLine();
            if (ImGui::InputInt("##vid_res_y", &record_resolution.y, 10)) {
                record_resolution.y = std::clamp(record_resolution.y, 16, 1024);
                record_resolution.y = (record_resolution.y & ~1); // force even
            }

            ImGui::Spacing();
            ImGui::Spacing();
        }

        ImGui::Spacing();
        ImGui::Spacing();
        ImGui::Text("Quality:");

        if (ImGui::SliderInt("##quality", &record_quality, 0, 100))
        {
            /// TODO: Apply quality to WebP

            #ifndef __EMSCRIPTEN__
            record_bitrate = (int64_t)(1000000.0 * choose_bitrate_mbps_from_range(bitrate_mbps_range, record_quality));
            #endif
        }

        #ifndef __EMSCRIPTEN__
        ImGui::Text("= %.1f Mbps", (double)record_bitrate / 1000000.0);
        #endif

        ImGui::Spacing();
        ImGui::Spacing();
        ImGui::Checkbox("Lossless", &record_lossless);

        if (record_lossless)
        {
            ImGui::Spacing();
            ImGui::Spacing();
            ImGui::Text("Lossless quality:");
            ImGui::SliderInt("##near_lossless", &record_near_lossless, 0, 100);
        }

        ImGui::Spacing();
        ImGui::Spacing();
        ImGui::Text("Frame Count (0 = No Limit):");
        if (ImGui::InputInt("##frame_count", &record_frame_count, 1))
        {
            record_frame_count = std::clamp(record_frame_count, 0, 10000000);
        }
    }
   
    ImGui::EndChild();
    ImGui::PopStyleVar();
}

void threadsDebugInfo()
{
    ImGui::Begin("Threads Info", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
    ImGui::Text("BSL Threads: %d", (int)Thread::pool().get_thread_count());
    ImGui::End();
}

void MainWindow::populateUI()
{
    if (!manageDockingLayout())
        return;


    // Determine if we are ready to draw *before* populating simulation imgui attributes
    {
        std::lock_guard<std::mutex> g(shared_sync.state_mutex);
        need_draw = shared_sync.frame_ready_to_draw;
    }

    bool collapse_layout = vertical_layout || platform()->max_char_rows() < 40.0f;

    if (ProjectBase::projectInfoList().size() <= 1)
        collapse_layout = false;


    // ==== Allow project to populate UI / draw nanovg overlay ====


    // Shadow buffer is now up-to-date and free to access (while worker does processing)
    {
        // Draw sidebar
        {
            // Stall until we've finishing copying the shadow buffer to the live buffer (on worker thread)
            shared_sync.wait_until_live_buffer_updated();
            std::unique_lock<std::mutex> shadow_lock(shared_sync.shadow_buffer_mutex);

            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));

            if (collapse_layout)
                populateCollapsedLayout();
            else
                populateExpandedLayout();

            if (!done_first_focus && focusWindow(collapse_layout ? "Active" : "Projects"))
                done_first_focus = true;

            ImGui::PopStyleVar();
        }

        // Draw viewport
        {
            ImGuiWindowClass wc{};
            wc.DockNodeFlagsOverrideSet =
                (int)ImGuiDockNodeFlags_NoTabBar |
                (int)ImGuiWindowFlags_NoDocking;

            ImGui::SetNextWindowClass(&wc);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(3, 3));

            populateViewport();
        }
    }

    ImGui::Begin("Settings"); // Begin Debug Window
    {
        populateRecordOptions();
    }
    ImGui::End();

    #if defined BL_DEBUG && defined DEBUG_INCLUDE_LOG_TABS
    ImGui::Begin("Debug"); // Begin Debug Window
    {
        if (ImGui::BeginTabBar("DebugTabs"))
        {

            if (ImGui::BeginTabItem("Project Log"))
            {
                project_log.draw();
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Global Log"))
            {
                debug_log.draw();
                ImGui::EndTabItem();
            }

            ImGui::EndTabBar();
        }
    }
    ImGui::End(); // End Debug Window
    #endif

    ImGui::PopStyleVar();

    //static bool demo_open = true;
    //ImGui::ShowDemoWindow(&demo_open);

    /// Debugging
    {
        // Debug Log
        //project_log.draw();

        // Debug DPI
        //dpiDebugInfo();

        //threadsDebugInfo();

        /*ImGuiID active_id = ImGui::GetActiveID();

        ImGui::Begin("Viewport Info", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
        ImGui::Text("Viewport hovered: %d", viewport_hovered ?1:0);
        ImGui::Text("active_id: %d", active_id);
        //ImGui::Text("Viewport: {%.0f, %.0f, %.0f, %.0f}",
        //    viewport_rect.Min.x,
        //    viewport_rect.Min.y,
        //    viewport_rect.Max.x,
        //    viewport_rect.Max.y
        //);
        ImGui::End();*/
    }
}

BL_END_NS
