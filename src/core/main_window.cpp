#include <bitloop/platform/platform.h>
#include <bitloop/core/main_window.h>
#include <bitloop/core/project_worker.h>
#include <bitloop/imguix/imgui_debug_ui.h>
#include <imgui_stdlib.h>
#include <filesystem>

#ifndef __EMSCRIPTEN__
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
static bool path_contains(const std::filesystem::path& root, const std::filesystem::path& p)
{
    std::error_code ec1, ec2;
    std::filesystem::path pc    = std::filesystem::weakly_canonical(p, ec1);
    std::filesystem::path rootc = std::filesystem::weakly_canonical(root,ec2);

    if (ec1 || ec2)
    {
        pc = p.lexically_normal();
        rootc = root.lexically_normal();
    }

    if (pc == rootc) return true;
    std::filesystem::path rel = pc.lexically_relative(rootc);

    auto s = rel.native();
    return !s.empty() && !(s.size() >= 2 && s[0] == '.' && s[1] == '.');
}

std::string getPreferredCapturesDirectory() {
    std::filesystem::path path = platform()->executable_dir();
    std::filesystem::path project_root = std::filesystem::path(CMAKE_SOURCE_DIR).lexically_normal();

    // If executable dir lives inside the cmake project dir, clamp to root cmake project dir
    if (path_contains(project_root, path)) {
        path = project_root;
    } else {
        std::filesystem::path trimmed;
        if (!project_worker()->getCurrentProject()) {
            DebugBreak();
        }
        std::string active_sim_name =
            project_worker()->getCurrentProject()->getProjectInfo()->name;

        for (const auto& part : path) {
            trimmed /= part;
            if (!active_sim_name.empty() && part == active_sim_name) break;
        }
    }

    path /= "captures";
    return path.lexically_normal().string();
}

static inline char bl_tolower(char c) {
    return static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
}

static bool ends_with_ci(std::string_view s, std::string_view suffix)
{
    if (suffix.size() > s.size()) return false;
    s = s.substr(s.size() - suffix.size());
    for (size_t i = 0; i < suffix.size(); ++i)
        if (bl_tolower(s[i]) != bl_tolower(suffix[i])) return false;
    return true;
}

static bool starts_with_ci(std::string_view s, std::string_view prefix)
{
    if (prefix.size() > s.size()) return false;
    for (size_t i = 0; i < prefix.size(); ++i)
        if (bl_tolower(s[i]) != bl_tolower(prefix[i])) return false;
    return true;
}

static bool try_parse_indexed_name(
    std::string_view filename,
    std::string_view name,
    std::string_view alias,      // empty = no alias
    std::string_view extension,  // e.g. ".webp" (case-insensitive)
    int& out_index)
{
    // Must end with extension
    if (!ends_with_ci(filename, extension)) return false;

    // Strip extension
    std::string_view core = filename.substr(0, filename.size() - extension.size());

    // If alias is provided, must end with "_" + alias
    if (!alias.empty())
    {
        // Build the suffix "_alias" without allocating: check manually
        if (core.size() <= alias.size() + 1) return false;
        if (bl_tolower(core[core.size() - alias.size() - 1]) != '_') return false;

        std::string_view tail = core.substr(core.size() - alias.size(), alias.size());
        if (!starts_with_ci(tail, alias)) return false;

        core = core.substr(0, core.size() - (alias.size() + 1));
    }

    // Now core must be: name + digits
    if (!starts_with_ci(core, name)) return false;
    if (core.size() <= name.size()) return false;

    std::string_view digits = core.substr(name.size());
    for (char c : digits) if (c < '0' || c > '9') return false;

    int idx = 0;
    auto* b = digits.data();
    auto* e = digits.data() + digits.size();
    auto [ptr, ec] = std::from_chars(b, e, idx);
    if (ec != std::errc{} || ptr != e) return false;

    out_index = idx;
    return true;
}

static int find_max_clip_index(
    const std::filesystem::path& directory_path,
    const char* name,
    const char* alias,       // may be nullptr
    const char* extension)
{
    int max_clip_index = 0;

    const std::string_view sv_name = name ? std::string_view(name) : std::string_view();
    const std::string_view sv_alias = alias ? std::string_view(alias) : std::string_view();
    std::string_view sv_ext = extension ? std::string_view(extension) : std::string_view();

    // Be tolerant if caller passes "webp" instead of ".webp"
    std::string ext_storage;
    if (!sv_ext.empty() && sv_ext[0] != '.') {
        ext_storage.reserve(sv_ext.size() + 1);
        ext_storage.push_back('.');
        ext_storage.append(sv_ext);
        sv_ext = ext_storage;
    }

    std::error_code ec;
    std::filesystem::directory_iterator it(
        directory_path,
        std::filesystem::directory_options::skip_permission_denied,
        ec
    );
    const std::filesystem::directory_iterator end;

    for (; it != end; it.increment(ec))
    {
        if (ec) break; // stop on catastrophic error

        std::error_code ec2;
        if (!it->is_regular_file(ec2) || ec2) continue;

        const std::string filename = it->path().filename().string();

        int clip_index = 0;
        if (try_parse_indexed_name(filename, sv_name, sv_alias, sv_ext, clip_index))
            if (clip_index > max_clip_index) max_clip_index = clip_index;
    }

    return max_clip_index;
}

static std::filesystem::path make_next_clip_path(
    const std::filesystem::path& capture_dir,
    const char* name,
    const char* alias,      // may be nullptr
    const char* extension)
{
    std::error_code io_error;
    std::filesystem::create_directories(capture_dir, io_error); // no-op if exists

    const int next_index = find_max_clip_index(capture_dir, name, alias, extension) + 1;

    std::string filename;
    filename.reserve(
        (name ? std::char_traits<char>::length(name) : 0) +
        16 +
        (alias ? (1 + std::char_traits<char>::length(alias)) : 0) +
        (extension ? std::char_traits<char>::length(extension) : 0) + 1
    );

    if (name) filename += name;
    filename += std::to_string(next_index);
    if (alias && *alias) { filename += '_'; filename += alias; }

    if (extension && *extension) {
        if (extension[0] == '.') filename += extension;
        else { filename += '.'; filename += extension; }
    }

    return capture_dir / filename;
}

// Convenience overload (no alias)
static std::filesystem::path make_next_clip_path(
    const std::filesystem::path& capture_dir,
    const char* name,
    const char* extension)
{
    return make_next_clip_path(capture_dir, name, nullptr, extension);
}

static bool ensure_parent_directories_exist(const std::filesystem::path& file_path, std::error_code& ec)
{
    ec.clear();
    const std::filesystem::path parent = file_path.parent_path();
    if (parent.empty()) return true; // nothing to create
    return std::filesystem::create_directories(parent, ec);
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

    settings_panel.init();
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

    // base sizes
    style.WindowRounding = 8.0f;
    style.ChildRounding = 8.0f;
    style.FrameRounding = 6.0f;
    style.FramePadding = ImVec2(8.0f, 5.0f);
    style.PopupRounding = 3.0f;
    style.GrabRounding = 2.0f;
    style.TabRounding = 6.0f;

    style.ScrollbarRounding = 6.0f * platform()->thumbScale();
    style.ScrollbarSize = 20.0f * platform()->thumbScale(0.85f);
    style.GrabMinSize = 35.0f * platform()->thumbScale(0.85f);

    //style.ScrollbarRounding = platform()->is_mobile() ? 12.0f : 6.0f; // Extra scrollbar size for mobile
    //style.ScrollbarSize = platform()->is_mobile() ? 30.0f : 20.0f;    // Extra scrollbar size for mobile

    // update by dpr
    style.ScaleAllSizes(platform()->dpr());

    // colors
    ImVec4* colors = ImGui::GetStyle().Colors;

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
    if (!project_worker()->hasActiveProject())
        return;

    ImGui::BeginPaddedRegion(scale_size(3.0f));
    project_worker()->populateAttributes();
    ImGui::EndPaddedRegion();
}
void MainWindow::populateOverlay()
{
    project_worker()->populateOverlay();
}

void MainWindow::queueBeginSnapshot(const SnapshotPresetList& presets, const char* relative_filename)
{
    MainWindowCommand_SnapshotPayload payload;
    payload.path = relative_filename;
    payload.presets = presets;
    queueMainWindowCommand({ MainWindowCommandType::BEGIN_SNAPSHOT, payload });
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

    snapshot.enabled = false;
    //record.enabled = true; // keep record button enabled. Clicking again ends recording
    record.toggled = true;

    // Don't capture until next frame starts (and sets this true)
    capture_manager.setCaptureEnabled(false);

    #ifndef __EMSCRIPTEN__
    std::filesystem::path capture_dir = getPreferredCapturesDirectory();

    std::string active_sim_name = project_worker()->getCurrentProject()->getProjectInfo()->name;

    // Organize by project name
    capture_dir /= active_sim_name;

    switch ((CaptureFormat)getSettingsConfig()->getRecordFormat())
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

    int64_t bitrate = 0;
    #if BITLOOP_FFMPEG_ENABLED
    bitrate = getSettingsConfig()->record_bitrate;
    #endif

    capture_manager.startCapture(
        getSettingsConfig()->getRecordFormat(),
        capture_dir.string(),
        getSettingsConfig()->record_resolution,
        getSettingsConfig()->default_ssaa,
        getSettingsConfig()->default_sharpen,
        getSettingsConfig()->record_fps,
        getSettingsConfig()->record_frame_count,
        bitrate,
        (float)getSettingsConfig()->record_quality,
        getSettingsConfig()->record_lossless,
        getSettingsConfig()->record_near_lossless,
        true);
    #else
    capture_manager.startCapture(
        getSettingsConfig()->getRecordFormat(),
        getSettingsConfig()->record_resolution,
        getSettingsConfig()->default_ssaa,
        getSettingsConfig()->default_sharpen,
        getSettingsConfig()->record_fps,
        getSettingsConfig()->record_frame_count,
        0,
        getSettingsConfig()->record_quality,
        getSettingsConfig()->record_lossless,
        getSettingsConfig()->record_near_lossless,
        true);
    #endif
}
std::string MainWindow::prepareFullCapturePath(const SnapshotPreset& preset, const char* relative_filepath_noext)
{
    namespace fs = std::filesystem;

    fs::path rel_filepath = relative_filepath_noext;

    std::string base_filename      = rel_filepath.filename().string();     // e.g. "seahorse_valley"

    #ifdef __EMSCRIPTEN__
    std::string qualified_filename = (base_filename + '_') + preset.alias; // e.g. "seahorse_valley_1920x1080"
    return qualified_filename + ".webp";
    #else
    // Organize by project name
    fs::path dir = getPreferredCapturesDirectory();                                                            // e.g. "C:/dev/bitloop-gallery/captures/"
    dir /= project_worker()->getCurrentProject()->getProjectInfo()->name;                                      // e.g. "C:/dev/bitloop-gallery/captures/Mandelbrot/"
    dir /= "image";                                                                                            // e.g. "C:/dev/bitloop-gallery/captures/Mandelbrot/image/"
                                                                                               
    // relative to project capture dir                                                         
    fs::path relative_dir = dir / rel_filepath.parent_path();                                                  // e.g. "C:/dev/bitloop-gallery/captures/Mandelbrot/image/backgrounds/"
    fs::path filename = relative_dir / make_next_clip_path(relative_dir, base_filename.c_str(), preset.alias, ".webp"); // e.g. "C:/dev/bitloop-gallery/captures/Mandelbrot/image/backgrounds/seahorse_valley_1920x1080.webp"

    filename = filename.lexically_normal();

    std::error_code ec;
    ensure_parent_directories_exist(filename, ec);

    return filename.string();
    #endif
}
void MainWindow::beginSnapshot(const char* full_filepath, IVec2 res, int ssaa, float sharpen)
{
    assert(!capture_manager.isRecording());
    assert(!capture_manager.isSnapshotting());

    // Disable everything until snapshot complete
    snapshot.enabled = false;
    record.enabled = false;
    pause.enabled = false;
    stop.enabled = false; 

    snapshot.toggled = true;
    record.toggled = false;

    // Don't capture until *next* frame starts (when capture_enabled turns true)
    capture_manager.setCaptureEnabled(false);

    #ifndef __EMSCRIPTEN__
    capture_manager.startCapture(
        getSettingsConfig()->getSnapshotFormat(),
        full_filepath,
        res, ssaa, sharpen,
        0, 0, 0, 100.0f, true, 100, true);
    #else
    capture_manager.startCapture(
        getSettingsConfig()->getSnapshotFormat(),
        res, ssaa, sharpen,
        0, 0, 0, 100.0f, true, 100, true);
    #endif
}
void MainWindow::beginSnapshot(const SnapshotPreset& preset, const char* relative_filepath_noext)
{
    std::string full_path = prepareFullCapturePath(preset, relative_filepath_noext);
    int ssaa      = (preset.ssaa > 0)    ? preset.ssaa :    getSettingsConfig()->default_ssaa;
    float sharpen = (preset.sharpen > 0) ? preset.sharpen : getSettingsConfig()->default_sharpen;

    beginSnapshot(full_path.c_str(), preset.size, ssaa, sharpen);
}
void MainWindow::beginSnapshotList(const SnapshotPresetList& presets, const char* rel_proj_path)
{
    enabled_capture_presets = presets;
    is_snapshotting = true;
    active_capture_preset = 0;
    active_capture_rel_proj_path = rel_proj_path;

    beginSnapshot(enabled_capture_presets[active_capture_preset], active_capture_rel_proj_path.c_str());
}
void MainWindow::endRecording()
{
    // signal to capture_manager to finish, so isCaptureToMemoryComplete()==true below
    if (capture_manager.isRecording())
    {
        // called immediately when end recording signalled, even if current frame hasn't finished encoding
        record.toggled = false;

        // disable everything while finalizing capture
        play.enabled = false;
        pause.enabled = false;
        stop.enabled = false;
        record.enabled = false;
        snapshot.enabled = false;

        capture_manager.finalizeCapture();
    }
}
void MainWindow::checkCaptureComplete()
{
    bool captured_to_memory = false;
    if (capture_manager.handleCaptureComplete(captured_to_memory))
    {
        if (captured_to_memory)
        {
            // capture to memory is currently always webp... (still OR webp animation)
            bytebuf data;
            capture_manager.takeCompletedCaptureFromMemory(data);

            #ifdef __EMSCRIPTEN__
            platform()->download_snapshot_webp(data, "snapshot.webp");
            #else
            std::ofstream out(capture_manager.filename(), std::ios::out | std::ios::binary);
            out.write((const char*)data.data(), data.size());
            out.close();
            #endif

            // todo: When you include other snapshot types (PNG, JPEG, etc) use a more robust "is snapshot" check
            if (capture_manager.format() == CaptureFormat::WEBP_SNAPSHOT)
            {
                // Do we have another preset to capture?
                active_capture_preset++;
                if (active_capture_preset < enabled_capture_presets.size())
                {
                    // todo: maybe queue to begin capture at more suitable time
                    //beginSnapshot(enabled_capture_presets[active_capture_preset], active_capture_rel_proj_path.c_str());
                    queueMainWindowCommand({ MainWindowCommandType::TAKE_ACTIVE_PRESET_SNAPSHOT });
                }
                else
                {
                    is_snapshotting = false;
                }
            }
        }
        else
        {
            // ffmpeg saving to desk handled automatically by encoder...
        }

        // untoggle
        record.toggled = false;
        snapshot.toggled = false;

        bool project_active = project_worker()->hasActiveProject();

        if (project_active)
        {
            bool isPaused = project_worker()->getCurrentProject()->isPaused();

            // re-enable
            play.enabled = isPaused;
            pause.enabled = !isPaused;
            stop.enabled = true;
            record.enabled = true;
            snapshot.enabled = true;
        }
    }
}

void MainWindow::handleCommand(MainWindowCommandEvent e)
{
    // Handle commands which can be initiated by both GUI thread & project-worker thread
    switch (e.type)
    {
    case MainWindowCommandType::ON_PLAY_PROJECT:
    {
        /// note: could be starting OR resuming project

        // received from project-worker once project succesfully started
        play.enabled = false;
        stop.enabled = true;
        pause.enabled = true;

        record.enabled = true; // may already be recording, but enable in case we're starting new project
        snapshot.enabled = !capture_manager.isRecording(); // don't re-enable snapshot button if we're still recording (i.e. resuming)

        static bool first = true;
        if (first)
        {
            // set initial preferred folder once
            #ifndef __EMSCRIPTEN__
            getSettingsConfig()->capture_dir = getPreferredCapturesDirectory();
            #endif

            first = false;
        }
    }
    break;

    case MainWindowCommandType::ON_STOPPED_PROJECT:

        // set states here as well as on toolbar button click, in case
        // the project gets stopped in some other way.
        play.enabled = true;
        pause.enabled = false;

        stop.enabled = false;
        record.toggled = false;
        snapshot.enabled = false;

        // If recording, finalize
        endRecording();

        break;

    case MainWindowCommandType::ON_PAUSED_PROJECT:
        play.enabled = true;
        pause.enabled = false;
        snapshot.enabled = false; // only allow snapshot while project running
        break;

    case MainWindowCommandType::BEGIN_SNAPSHOT:
        if (e.payload.has_value())
        {
            MainWindowCommand_SnapshotPayload payload = std::any_cast<MainWindowCommand_SnapshotPayload>(e.payload);
            beginSnapshotList(payload.presets, payload.path.c_str()); // e.g. "backgrounds/seahorse_valley"
        }
        else
            beginSnapshotList(getSnapshotPresetManager()->enabledPresets(), "snap");
        break;

    case MainWindowCommandType::TAKE_ACTIVE_PRESET_SNAPSHOT:
    {
        beginSnapshot(enabled_capture_presets[active_capture_preset], active_capture_rel_proj_path.c_str());
    }
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
    //if (platform()->is_mobile())
    //    return;

    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(10, 0));

    ImGui::Spacing();
    ImGui::Spacing();

    float size = scale_size(36.0f);
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

        if (!project_worker()->getCurrentProject()->isPaused())
        {
            // not paused/resuming - begin recording immediately if record button "on" so we start capturing from the very first frame
            if (record.toggled)
                beginRecording();
        }

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
            //if (!play.enabled)
            if (project_worker()->hasActiveProject())
            {
                // sim already 'active' (and possibly in 'paused' state)
                if (!record.toggled)
                {
                    // Start recording
                    //record.toggled = true;
                    beginRecording();
                }
                else
                {
                    /// endRecording() signals finalize_requested=true, so recording finalizes in MainWindow::checkCaptureComplete()
                    endRecording();
                    //record.toggled = false;
                    //capture_manager.finalizeCapture();
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
        beginSnapshotList(getSnapshotPresetManager()->enabledPresets(), "snap");
    }

    ImGui::EndChild();

    ImGui::PopStyleColor();
    ImGui::PopStyleVar(3);

    ImGui::Dummy(scale_size(0, 6));
}

/// ======== Project Tree ========

void MainWindow::populateProjectTreeNodeRecursive(ProjectInfoNode& node, int& i, int depth)
{
    //if (!node.visible)
    //    return;
    ImGui::PushID(node.uid);
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
    ImVec4 dim_bg = ImVec4(bg.x * 0.8f, bg.y * 0.8f, bg.x * 0.8f, bg.w);

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 0.0f);

    {
        ImGui::BeginChild("TreeFrame", frameSize, 0, ImGuiChildFlags_AlwaysUseWindowPadding);

        ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, scale_size(6.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, scale_size(3.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(ImGui::GetStyle().FramePadding.x, platform()->thumbScale(0.7f) * scale_size(4.0f)));

        ImGui::PushStyleColor(ImGuiCol_ChildBg, dim_bg);
        ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.3f, 0.3f, 0.3f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.3f, 0.5f, 1.0f));

        const float margin = 6.0f;
        const ImVec2 win_pos = ImGui::GetWindowPos();
        ImVec2 clip_min = win_pos + ImGui::GetWindowContentRegionMin();
        ImVec2 clip_max = win_pos + ImGui::GetWindowContentRegionMax();

        clip_min.x += margin; clip_min.y += margin;
        clip_max.x -= margin; clip_max.y -= margin;

        {
            ImGui::BeginChild("TreeFrameInner", ImVec2(0.0f, 0.0f), true);

            ImGui::BeginPaddedRegion(scale_size(6.0f));
            ImGui::PushClipRect(clip_min, clip_max, true);

            auto& tree = ProjectBase::projectTreeRootInfo();
            int i = 0; // for unique ID's
            for (size_t child_i = 0; child_i < tree.children.size(); child_i++)
            {
                int depth = 0; // track recursion depth
                populateProjectTreeNodeRecursive(tree.children[child_i], i, depth);
            }

            ImGui::PopClipRect();
            ImGui::EndPaddedRegion();

            ImGui::EndChild();
        }


        ImGui::PopStyleColor(3);
        ImGui::PopStyleVar(3);

        ImGui::EndChild();
    }
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

void MainWindow::populateCollapsedLayout()
{
    // Collapse layout
    if (ImGui::Begin("Projects", nullptr, window_flags))
    {
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, scale_size(6.0f, 6.0f));
        ImGui::BeginChild("AttributesFrame", ImVec2(0, 0), 0, ImGuiWindowFlags_AlwaysUseWindowPadding);
        populateToolbar();

        populateProjectTree(true);
        ImGui::SwipeScrollWindow();

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
        

        populateProjectUI();
        ImGui::SwipeScrollWindow();

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

        ImGui::SwipeScrollWindow();

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
        client_size = ImGui::GetContentRegionAvail();

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
            if (!project_worker()->getCurrentProject())
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
                if (project_worker()->getCurrentProject() &&
                    !project_worker()->getCurrentProject()->isPaused())
                {
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
                }

                shared_sync.frame_ready_to_draw = false;
                shared_sync.frame_consumed = true;
            }

            // Let worker continue
            shared_sync.cv.notify_one();
        }

        // "canvas" refers to internal canvas dimensions
        // "image" in this context = ImGui image
        // "client" viewport size (max image size)
        ImVec2 image_pos = ImGui::GetCursorScreenPos();
        ImVec2 image_size;

        if (capture_manager.isRecording() || capture_manager.isSnapshotting())
        {
            float sw = client_size.x, sh = client_size.y; // calculates to image size (after scaling)
            float ox = 0, oy = 0;

            float client_aspect  = (client_size.x / client_size.y);
            float canvas_aspect = ((float)canvas_size.x / (float)canvas_size.y);

            if (canvas_aspect > client_aspect)
            {
                sh = client_size.y * (client_aspect / canvas_aspect); // Render aspect is too wide
                oy = 0.5f * (client_size.y - sh);                     // Center vertically
            }
            else
            {
                sw = client_size.x * (canvas_aspect / client_aspect); // Render aspect is too tall
                ox = 0.5f * (client_size.x - sw);                     // Center horizontally
            }

            image_pos = { ox, oy };
            image_size = { sw, sh };

            canvas.setClientRect(IRect((int)ox, (int)oy, (int)(ox + sw), (int)(oy + sh)));
        }
        else
        {
            image_size = (ImVec2)canvas_size;
            canvas.setClientRect(IRect(0, 0, canvas_size.x, canvas_size.y));
        }

        // Draw cached (or freshly generated) frame
        ImGui::SetCursorScreenPos(image_pos);
        ImGui::Image(canvas.texture(), image_size, ImVec2(0,1), ImVec2(1,0));

        viewport_hovered = ImGui::IsWindowHovered();
    }
    ImGui::End();
    ImGui::PopStyleVar();
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

            if (sidebar_visible)
            {
                if (collapse_layout)
                    populateCollapsedLayout();
                else
                    populateExpandedLayout();

                if (!done_first_focus && focusWindow(collapse_layout ? "Active" : "Projects"))
                    done_first_focus = true;
            }

            populateOverlay();

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

    if (sidebar_visible)
    {
        ImGui::Begin("Settings");
        {
            settings_panel.populateSettings();
        }
        ImGui::End();

        #ifdef DEBUG_INCLUDE_LOG_TABS
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

                if (ImGui::BeginTabItem("Display"))
                {
                    // Debug DPI
                    dpiDebugInfo();

                    ImGui::EndTabItem();
                }

                ImGui::EndTabBar();
            }
        }
        ImGui::End(); // End Debug Window
        #endif
    }

    ImGui::PopStyleVar();

    // Always save capture output on main GUI thread (for Emscripten)
    checkCaptureComplete();

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


