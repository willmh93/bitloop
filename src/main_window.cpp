#include "platform.h"
#include "main_window.h"
#include "project_worker.h"


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

void MainWindow::init()
{
    ImGui::LoadIniSettingsFromMemory("");

    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    // Always initializes window on first call
    checkChangedDPR();

    canvas.create(Platform()->dpr());
}

void MainWindow::checkChangedDPR()
{
    Platform()->update();

    static float last_dpr = -1.0f;
    if (fabs(Platform()->dpr() - last_dpr) > 0.01f)
    {
        // DPR changed
        last_dpr = Platform()->dpr();

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
    style.ScrollbarRounding = Platform()->is_mobile() ? 12.0f : 6.0f;
    style.ScrollbarSize = Platform()->is_mobile() ? 30.0f : 20.0f;

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

    style.ScaleAllSizes(Platform()->ui_scale_factor());
}

void MainWindow::initFonts()
{
    // Update FreeType fonts
    ImGuiIO& io = ImGui::GetIO();

    float base_pt = 16.0f;
    const char* font_path = Platform()->path("/data/fonts/DroidSans.ttf");
    ImFontConfig config;
    config.OversampleH = 3;
    config.OversampleV = 3;

    io.Fonts->Clear();
    io.Fonts->AddFontFromFileTTF(font_path, base_pt * Platform()->dpr() * Platform()->font_scale(), &config);
    io.Fonts->FontBuilderFlags =
        ImGuiFreeTypeBuilderFlags_LightHinting |
        ImGuiFreeTypeBuilderFlags_ForceAutoHint;

    ImGuiFreeType::GetBuilderForFreeType()->FontBuilder_Build(io.Fonts);
}

void MainWindow::populateProjectUI()
{
    ImGui::BeginPaddedRegion(ScaleSize(10.0f));
  
    // Stall until we've finishing copying the shadow buffer to the live buffer (on worker thread)
    shared_sync.wait_until_live_buffer_updated();

    // Shadow buffer is now up-to-date free to access (while worker does processing)
    {
        std::unique_lock<std::mutex> shadow_lock(shared_sync.shadow_buffer_mutex);
        ProjectWorker::instance()->populateAttributes();
    }

    ImGui::EndPaddedRegion();
}

bool MainWindow::isEditingUI()
{
    ImGuiID active_id = ImGui::GetActiveID();

    static ImGuiID old_active_id = 0;
    ImGuiID lagged_active_id = active_id ? active_id : old_active_id;

    old_active_id = active_id;

    if (lagged_active_id == 0)
        return false;

    static ImGuiID viewport_id = ImHashStr("Viewport");
    return (lagged_active_id != ImGui::FindWindowByID(viewport_id)->MoveId);
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
}

bool MainWindow::toolbarButton(const char* id, const char* symbol, const ToolbarButtonState& state, ImVec2 size)
{
    ImGui::PushStyleColor(ImGuiCol_Button, state.bgColor);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, state.bgColor);
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, state.bgColor);
    bool pressed = ImGui::Button(id, size);
    ImGui::PopStyleColor(3);

    ImDrawList* drawList = ImGui::GetWindowDrawList();
    ImVec2 p = ImGui::GetItemRectMin();
    ImVec2 sz = ImGui::GetItemRectSize();
    drawToolbarButton(drawList, p, sz, symbol, ImGui::ColorConvertFloat4ToU32(state.symbolColor));

    return pressed;
}

void MainWindow::populateToolbar()
{
    //if (Platform()->is_mobile())
    //    return;

    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(10, 0));

    float size = ScaleSize(30.0f);
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
    toolbarButton("##play", "play", play, ImVec2(size, size));
    ImGui::SameLine();
    toolbarButton("##stop", "stop", stop, ImVec2(size, size));
    ImGui::SameLine();
    toolbarButton("##pause", "pause", pause, ImVec2(size, size));

    #ifndef WEB_UI
    ImGui::SameLine();
    toolbarButton("##record", "record", record, ImVec2(size, size));
    #endif

    ImGui::EndChild();

    ImGui::PopStyleColor();
    ImGui::PopStyleVar(2);

    ImGui::PopStyleVar();
}

/// ======== Project Tree ========

void MainWindow::populateProjectTreeNodeRecursive(ProjectInfoNode& node, int& i, int depth)
{
    ImGui::PushID(i++);
    if (node.project_info)
    {
        // Leaf project node
        if (ImGui::Button(node.name.c_str()))
        {
            ProjectWorker::instance()->setActiveProject(node.project_info->sim_uid);
            ProjectWorker::instance()->startProject();
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
    ImVec2 frameSize = ImVec2(0.0f, expand_vertical ? 0 : 170.0f); // Let height auto-expand
    ImVec4 bg = ImGui::GetStyleColorVec4(ImGuiCol_ChildBg);
    ImVec4 dim_bg = bg * 0.9f;
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

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::Begin("MainDockSpaceHost", nullptr, dockspace_flags);
    ImGui::PopStyleVar(3);

    ImGuiID dockspace_id = ImGui::GetID("MyDockspace");
    ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_None);
    ImGui::End();

    static ImVec2 last_viewport_size = ImVec2(0, 0);
    ImVec2 current_viewport_size = ImGui::GetMainViewport()->Size;

    if (current_viewport_size.x != last_viewport_size.x ||
        current_viewport_size.y != last_viewport_size.y)
    {
        update_docking_layout = true;
        last_viewport_size = current_viewport_size;
    }

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

        ImGui::DockBuilderRemoveNode(dockspace_id); // clear any previous layout
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

        ImGui::DockBuilderDockWindow("Projects", dock_sidebar);
        ImGui::DockBuilderDockWindow("Active", dock_sidebar);
        ImGui::DockBuilderDockWindow("Debug Log", dock_sidebar);
        ImGui::DockBuilderDockWindow("Project Log", dock_sidebar);
        ImGui::DockBuilderDockWindow("Viewport", dock_main_id);

        ImGui::DockBuilderFinish(dockspace_id);
    }
    return true;
}

bool MainWindow::focusWindow(const char* id)
{
    static bool first_frame = true; // SetWindowFocus doesn't switch focused tab unless second frame
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
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ScaleSize(8.0f, 8.0f));
        ImGui::BeginChild("AttributesFrame", ImVec2(0, 0), 0, ImGuiWindowFlags_AlwaysUseWindowPadding);
        populateToolbar();
        ImGui::Dummy(ScaleSize(0, 6));

        populateProjectTree(true);

        ImGui::EndChild();
        ImGui::PopStyleVar();
    }
    ImGui::End();

    if (ImGui::Begin("Active", nullptr, window_flags))
    {
        // Only add padding after toolbar to inner-child
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ScaleSize(8.0f, 8.0f));
        ImGui::BeginChild("AttributesFrameInner", ImVec2(0, 0), 0, ImGuiWindowFlags_AlwaysUseWindowPadding);
        populateToolbar();
        ImGui::Dummy(ScaleSize(0, 6));

        populateProjectUI();

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
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8, 8));
        ImGui::BeginChild("AttributesFrame", ImVec2(0, 0), 0, ImGuiWindowFlags_AlwaysUseWindowPadding);
        populateToolbar();
        ImGui::Dummy(ImVec2(0, 6));

        populateProjectTree(false);
        populateProjectUI(); // Always call (in case sim does any unusual setup here)

        ImGui::EndChild();
        ImGui::PopStyleVar();
    }

    ImGui::End();
}

void MainWindow::populateViewport()
{
    // Always process viewport, even if not visible
    bool viewport_visible = ImGui::Begin("Viewport");
    {
        ImVec2 size = ImGui::GetContentRegionAvail();
        int width = static_cast<int>(size.x);
        int height = static_cast<int>(size.y);

        static bool done_first_size = false;
        bool resized = canvas.resize(width, height);


        if (!done_first_size)
        {
            done_first_size = true;
        }
        else if (shared_sync.project_thread_started)
        {
            // Launch initial simulation 1 frame late (background thread)
            if (!ProjectWorker::instance()->getActiveProject())
            {
                ProjectWorker::instance()->setActiveProject(ProjectBase::findProjectInfo("Mandelbrot Viewer")->sim_uid);
                ProjectWorker::instance()->startProject();

                // Kick-start work-render-work-render loop
                need_draw = true;
            }
        }

        if (need_draw)
        {
            // Stop the worker and draw the fresh frame
            {
                std::unique_lock<std::mutex> lock(shared_sync.state_mutex);

                canvas.begin(0.05f, 0.05f, 0.1f, 1.0f);
                ProjectWorker::instance()->draw();
                canvas.end();

                shared_sync.frame_ready = false;
                shared_sync.frame_consumed = true;
            }

            // Let worker continue
            shared_sync.cv.notify_one();
        }
        else if (resized)
        {
            canvas.begin(0.05f, 0.05f, 0.1f, 1.0f);
            canvas.setFillStyle(0, 0, 0);
            canvas.fillRect(0, 0, (float)canvas.fboWidth(), (float)canvas.fboHeight());
            canvas.end();
        }

        // Draw cached (or freshly generated) frame
        ImGui::Image(canvas.texture(), ImVec2(
            static_cast<float>(canvas.fboWidth()),
            static_cast<float>(canvas.fboHeight())),
            ImVec2(0.0f, 1.0f),   // UV top-left (flipped)
            ImVec2(1.0f, 0.0f)    // UV bottom-right);
        );

        shared_sync.editing_ui.store(isEditingUI(), std::memory_order_release);
    }
    ImGui::End();
}

void MainWindow::populateUI()
{
    if (!manageDockingLayout())
        return;

    // Show windows
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));

    // Determine if we are ready to draw *before* populating simulation imgui attributes
    {
        std::lock_guard<std::mutex> g(shared_sync.state_mutex);
        need_draw = shared_sync.frame_ready;
    }

    bool collapse_layout = vertical_layout || Platform()->max_char_rows() < 40.0f;
    if (collapse_layout)
        populateCollapsedLayout();
    else
        populateExpandedLayout();

    /*static bool first_frame = true; // SetWindowFocus doesn't switch focused tab unless second frame
    if (initialized && !done_first_focus && !first_frame)
    {
        done_first_focus = true;
        ImGui::SetWindowFocus(collapse_layout ? "Active" : "Projects");
    }
    first_frame = false;*/

    if (!done_first_focus && focusWindow(collapse_layout ? "Active" : "Projects"))
    {
        done_first_focus = true;
    }

    ImGui::PopStyleVar();

    ImGuiWindowClass wc{};
    wc.DockNodeFlagsOverrideSet =
        (int)ImGuiDockNodeFlags_NoTabBar |
        (int)ImGuiWindowFlags_NoDocking;

    ImGui::SetNextWindowClass(&wc);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(3, 3));


    populateViewport();

    #ifdef DEBUG_INCLUDE_LOG_TABS
    if (ImGui::Begin("Project Log"))
    {
        project_log.draw();
    }
    ImGui::End();
    if (ImGui::Begin("Debug Log"))
    {
        debug_log.draw();
    }
    ImGui::End();
    #endif

    ImGui::PopStyleVar();


    ///static bool demo_open = true;
    ///ImGui::ShowDemoWindow(&demo_open);

    /// Debugging
    {
        // Debug Log
        //project_log.log("%d", rand());
        //project_log.Draw();

        // Debug DPI
        //dpiDebugInfo();
    }
}