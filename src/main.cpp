#define _DISABLE_CONSTEXPR_MUTEX_CONSTRUCTOR 
#include "main.h"

/// emscripten
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <emscripten/html5.h>
#else
#define SDL_MAIN_HANDLED
#endif

/// Windows
#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#endif

/// ImGui
#include "imgui_custom.h"
#include "backends/imgui_impl_sdl2.h"
#include "backends/imgui_impl_opengl3.h"

/// SDL2
#include <SDL2/SDL.h>

/// OpenGL
#ifdef __EMSCRIPTEN__
#include <GLES3/gl3.h>
#else
#include "glad.h"
#endif


/// Standard library
#include <vector>
#include <memory>
#include <thread>
#include <atomic>
#include <mutex>

#include "platform.h"
#include "project.h"
#include "project_manager.h"
#include "nano_canvas.h"

#include "debug.h"
#include "imgui_debug_ui.h"

#define AGRESSIVE_THREAD_LOGGING false

SDL_Window* window;
SDL_GLContext gl_context;
std::atomic<bool> done{ false };
std::atomic<bool> editing_ui{ false };

ToolbarButtonState play = { ImVec4(0.1f, 0.6f, 0.1f, 1.0f), ImVec4(1, 1, 1, 1), false };
ToolbarButtonState stop = { ImVec4(0.6f, 0.1f, 0.1f, 1.0f), ImVec4(1, 1, 1, 1), false };
ToolbarButtonState pause = { ImVec4(0.3f, 0.3f, 0.3f, 1.0f), ImVec4(1, 1, 1, 1), false };
ToolbarButtonState record = { ImVec4(0.8f, 0.0f, 0.0f, 1.0f), ImVec4(1, 1, 1, 1), false };

Canvas canvas;
ImDebugLog debug_log;

ProjectManager project_manager;
std::thread project_thread;

std::condition_variable cv;
std::mutex working_mutex;
std::mutex state_mutex;
bool frame_ready = false;
bool frame_consumed = false;
bool processing_frame = false;
std::atomic<bool> populating_attributes{ false };

std::atomic<bool> project_thread_started{ false };
std::atomic<bool> gui_frame_running{ false };
std::atomic<bool> copy_requested{ false };

std::condition_variable cv_updating_live_buffer;
std::atomic<bool> updating_live_buffer{ false };
std::mutex live_buffer_mutex;

std::condition_variable cv_updating_shadow;
std::atomic<bool> updating_shadow{ false };
std::mutex shadow_mutex;

bool imgui_initialized = false;
bool update_docking_layout = false;

std::vector<SDL_Event> input_event_queue;

void update_imgui_styles()
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
    style.ScrollbarSize = Platform()->is_mobile() ? 30.0f : 14.0f;

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

    style.ScaleAllSizes(PlatformManager::get()->ui_scale_factor());
}

void imgui_init()
{
    ImGui::LoadIniSettingsFromMemory("");
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    update_imgui_styles();

    imgui_initialized = true;
}

void DrawSymbol(ImDrawList* drawList, ImVec2 pos, ImVec2 size, const char* symbol, ImU32 color)
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

bool SymbolButton(const char* id, const char* symbol, const ToolbarButtonState& state, ImVec2 size)
{
    ImGui::PushStyleColor(ImGuiCol_Button, state.bgColor);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, state.bgColor);
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, state.bgColor);
    bool pressed = ImGui::Button(id, size);
    ImGui::PopStyleColor(3);

    ImDrawList* drawList = ImGui::GetWindowDrawList();
    ImVec2 p = ImGui::GetItemRectMin();
    ImVec2 sz = ImGui::GetItemRectSize();
    DrawSymbol(drawList, p, sz, symbol, ImGui::ColorConvertFloat4ToU32(state.symbolColor));

    return pressed;
}

void show_toolbar()
{
    //if (PlatformManager::get()->is_mobile())
    //    return;

    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(10, 0));

    float size = ScaleSize(30.0f);
    ImVec2 avail = ImGui::GetContentRegionAvail();
    ImVec2 toolbarSize(avail.x, size); // Outer frame size
    ImVec4 frameColor = ImVec4(0,0,0,0); // Toolbar background color


    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 0.0f);
    ImGui::PushStyleColor(ImGuiCol_ChildBg, frameColor);

    ImGui::BeginChild("ToolbarFrame", toolbarSize, false,
        ImGuiWindowFlags_AlwaysUseWindowPadding | 
        ImGuiWindowFlags_NoScrollbar | 
        ImGuiWindowFlags_NoScrollWithMouse);

    // Layout the buttons inside the frame
    SymbolButton("##play", "play", play, ImVec2(size, size));
    ImGui::SameLine();
    SymbolButton("##stop", "stop", stop, ImVec2(size, size));
    ImGui::SameLine();
    SymbolButton("##pause", "pause", pause, ImVec2(size, size));

    #ifndef WEB_UI
    ImGui::SameLine();
    SymbolButton("##record", "record", record, ImVec2(size, size));
    #endif

    ImGui::EndChild();

    ImGui::PopStyleColor();
    ImGui::PopStyleVar(2);

    ImGui::PopStyleVar();
}

void show_tree_node(ProjectInfoNode& node, int& i, int depth)
{
    ImGui::PushID(i++);
    if (node.project_info)
    {
        // Leaf project node
        if (ImGui::Button(node.name.c_str()))
        {
            // Wait for worker to finish processing the frame
            std::lock(working_mutex, state_mutex);
            std::unique_lock<std::mutex> lock1(working_mutex, std::adopt_lock);
            std::lock_guard<std::mutex> lock2(state_mutex, std::adopt_lock);

            cv.wait(lock1, []() { return !processing_frame; });

            //std::unique_lock<std::mutex> lock(state_mutex);
            DebugPrint("show_tree_node::setActiveProject()");
            project_manager.setActiveProject(node.project_info->sim_uid);

            DebugPrint("show_tree_node::startProject()");
            project_manager.startProject();
            project_manager.updateShadowAttributes(); // thread-safe?
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
                show_tree_node(node.children[child_i], i, depth + 1);
            }
            ImGui::Unindent();
        }
    }
    ImGui::PopID();
}

void show_project_tree(bool expand_vertical)
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
            show_tree_node(tree.children[child_i], i, depth);
        }

        ImGui::EndChild();
        ImGui::PopStyleColor();
        ImGui::PopStyleVar();
    }
    ImGui::EndChild();
    ImGui::PopStyleVar(2);
}

void sim_worker()
{
    while (!done.load())
    {
        // Wait for GUI to consume previous frame
        {
            std::unique_lock<std::mutex> lock(state_mutex);
            cv.wait(lock, [] { return frame_consumed || done.load(); });
        }

        DebugPrintEx(AGRESSIVE_THREAD_LOGGING, "");
        DebugPrintEx(AGRESSIVE_THREAD_LOGGING, "=============================================================");
        DebugPrintEx(AGRESSIVE_THREAD_LOGGING, "====================== NEW WORKER FRAME =====================");

        // Do heavy work while we draw the previously rendered frame on GUI thread
        {
            std::lock_guard<std::mutex> lock(working_mutex);

            // ===== Update live values =====
            // (in case any inputs were changed since last .process())
            {
                /// Make sure we can't alter the shadow buffer while we copy them to live buffer
                std::lock_guard<std::mutex> live_buffer_lock(live_buffer_mutex);

                updating_live_buffer.store(true);

                DebugPrintEx(AGRESSIVE_THREAD_LOGGING, "");
                DebugPrintEx(AGRESSIVE_THREAD_LOGGING, "------------------------------------");
                DebugPrintEx(AGRESSIVE_THREAD_LOGGING, "---- BEGIN updateLiveAttributes ----");
                if (project_manager.getActiveProject())
                    project_manager.updateLiveAttributes();
                DebugPrintEx(AGRESSIVE_THREAD_LOGGING, "---- END updateLiveAttributes ----");
                DebugPrintEx(AGRESSIVE_THREAD_LOGGING, "------------------------------------");
                DebugPrintEx(AGRESSIVE_THREAD_LOGGING, "");

                updating_live_buffer.store(false, std::memory_order_release);

                // Inform GUI thread that 'updating_live_buffer' changed
                cv_updating_live_buffer.notify_one();
            }

            DebugPrintEx(AGRESSIVE_THREAD_LOGGING, "");
            DebugPrintEx(AGRESSIVE_THREAD_LOGGING, "------------------------------------");
            DebugPrintEx(AGRESSIVE_THREAD_LOGGING, "--------- BEGIN PROCESSING ---------");

            processing_frame = true;
            for (SDL_Event& e : input_event_queue)
                project_manager._onEvent(e);
            input_event_queue.clear();

            if (project_manager.getActiveProject())
                project_manager.process();
            processing_frame = false;

            DebugPrintEx(AGRESSIVE_THREAD_LOGGING, "---------- END PROCESSING ----------");
            DebugPrintEx(AGRESSIVE_THREAD_LOGGING, "------------------------------------");
            DebugPrintEx(AGRESSIVE_THREAD_LOGGING, "");

            // todo: if (shadow_attributes_updated)
            if (!editing_ui.load())
            {
                /// todo: Wait for an existing ui_update to finish first?

                /// While we update the shadow buffer, we should NOT permit ui updates (force a small stall)
                std::unique_lock<std::mutex> shadow_lock(shadow_mutex);
                updating_shadow.store(true);

                DebugPrintEx(AGRESSIVE_THREAD_LOGGING, "");
                DebugPrintEx(AGRESSIVE_THREAD_LOGGING, "------------------------------------");
                DebugPrintEx(AGRESSIVE_THREAD_LOGGING, "--- BEGIN updateShadowAttributes ---");
                if (project_manager.getActiveProject())
                    project_manager.updateShadowAttributes();
                DebugPrintEx(AGRESSIVE_THREAD_LOGGING, "---- END updateShadowAttributes ----");
                DebugPrintEx(AGRESSIVE_THREAD_LOGGING, "------------------------------------");
                DebugPrintEx(AGRESSIVE_THREAD_LOGGING, "");

                updating_shadow.store(false, std::memory_order_release);
                cv_updating_shadow.notify_one();
            }
            else
            {
                DebugPrintEx(AGRESSIVE_THREAD_LOGGING, "<<<<< ImGui Item Active - SKIPPED updateShadowAttributes() >>>>>");
            }
        }
        // Flag ready to draw
        {
            std::lock_guard<std::mutex> lock(state_mutex);
            frame_ready = true;
            frame_consumed = false;
        }

        DebugPrintEx(AGRESSIVE_THREAD_LOGGING, "====================== END WORKER FRAME =====================");
        DebugPrintEx(AGRESSIVE_THREAD_LOGGING, "=============================================================");
        DebugPrintEx(AGRESSIVE_THREAD_LOGGING, "");

        // Wake GUI
        cv.notify_one();
        ///cv_updating_live_buffer.notify_one();

        // Wait for GUI to draw
        {
            std::unique_lock<std::mutex> lock(state_mutex);
            cv.wait(lock, [] { return frame_consumed || done.load(); });
        }

        SDL_Delay(16);
    }
}

void init_simulation_thread()
{
    std::unique_lock<std::mutex> lock(state_mutex);
    project_manager.setSharedCanvas(&canvas);
    project_manager.setSharedDebugLog(&debug_log);
    project_thread = std::thread(sim_worker);
    project_thread_started.store(true);
}

void show_project_attributes(bool show_ui)
{
    std::unique_lock<std::mutex> lock(state_mutex);

    ImGui::BeginPaddedRegion(ScaleSize(10.0f));

    {
        std::unique_lock<std::mutex> wait_live_buffer_lock(live_buffer_mutex);
        cv_updating_live_buffer.wait(wait_live_buffer_lock, []() {
            return !updating_live_buffer.load();
        });
    }

    // Stall until we've finished copying live buffer to shadow buffer (on worker thread)
    std::unique_lock<std::mutex> shadow_lock(shadow_mutex);
    cv_updating_shadow.wait(shadow_lock, []() 
    {
        return !updating_shadow.load();
    });

    DebugPrintEx(AGRESSIVE_THREAD_LOGGING, "");
    DebugPrintEx(AGRESSIVE_THREAD_LOGGING, "------------------------------------");
    DebugPrintEx(AGRESSIVE_THREAD_LOGGING, "--- BEGIN populateAttributes ---");
    populating_attributes.store(true);
    project_manager.populateAttributes(show_ui);
    populating_attributes.store(false);
    DebugPrintEx(AGRESSIVE_THREAD_LOGGING, "---- END populateAttributes ----");
    DebugPrintEx(AGRESSIVE_THREAD_LOGGING, "------------------------------------");
    DebugPrintEx(AGRESSIVE_THREAD_LOGGING, "");

    editing_ui.store(ImGui::IsAnyItemActive(), std::memory_order_release);

    ImGui::EndPaddedRegion();
}

void show_ui()
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

    bool vertical_layout = (last_viewport_size.x < last_viewport_size.y);

    // Build initial layout (once)
    static bool initialized = false;
    static bool done_first_focus = false;
    if (!initialized || update_docking_layout)
    {
        initialized = true;
        update_docking_layout = false;

        if (ImGui::GetMainViewport()->Size.x <= 0 ||
            ImGui::GetMainViewport()->Size.y <= 0)
        {
            return;
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

        //ImGuiID dock_right_id = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Right, 0.3f, nullptr, &dock_main_id);
        //ImGuiID dock_top_id = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Up, 0.07f, nullptr, &dock_main_id);

        //ImGui::DockBuilderDockWindow("Right Window", dock_right_id);
        //ImGui::DockBuilderDockWindow("Timeline", dock_top_id);
        ImGui::DockBuilderDockWindow("Projects", dock_sidebar);
        ImGui::DockBuilderDockWindow("Active", dock_sidebar);
        ImGui::DockBuilderDockWindow("Debug", dock_sidebar);
        ImGui::DockBuilderDockWindow("Viewport", dock_main_id);

        ImGui::DockBuilderFinish(dockspace_id);
    }

    int window_flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoDecoration;
    window_flags |= ImGuiWindowFlags_NoMove;

    // Show windows
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));

    // Determine if we are ready to draw *before* populating simulation imgui attributes
    bool need_draw = false;
    {
        std::lock_guard<std::mutex> g(state_mutex);
        need_draw = frame_ready;
    }

    if (vertical_layout || PlatformManager::get()->max_char_rows() < 40.0f)
    {
        // Collapse layout
        if (ImGui::Begin("Projects", nullptr, window_flags))
        {
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ScaleSize(8.0f, 8.0f));
            ImGui::BeginChild("AttributesFrame", ImVec2(0, 0), 0, ImGuiWindowFlags_AlwaysUseWindowPadding);
            show_toolbar();
            ImGui::Dummy(ScaleSize(0, 6));

            show_project_tree(true);
            
            ImGui::EndChild();
            ImGui::PopStyleVar();
        }
        ImGui::End();

        bool showing_active_project_window = ImGui::Begin("Active", nullptr, window_flags);
        if (showing_active_project_window)
        {
            // Only add padding after toolbar to inner-child
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ScaleSize(8.0f, 8.0f));
            ImGui::BeginChild("AttributesFrameInner", ImVec2(0, 0), 0, ImGuiWindowFlags_AlwaysUseWindowPadding);
            show_toolbar();
            ImGui::Dummy(ScaleSize(0, 6));

            show_project_attributes(showing_active_project_window); // Always call (in case sim does any unusual setup here)

            ImGui::EndChild();
            ImGui::PopStyleVar();
        }

        ImGui::End();
    }
    else
    {
        // Show both windows
        bool showing_both_windows = ImGui::Begin("Projects", nullptr, window_flags);
        if (showing_both_windows)
        {
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8, 8));
            ImGui::BeginChild("AttributesFrame", ImVec2(0, 0), 0, ImGuiWindowFlags_AlwaysUseWindowPadding);
            show_toolbar();
            ImGui::Dummy(ImVec2(0, 6));

            show_project_tree(false);
            show_project_attributes(showing_both_windows); // Always call (in case sim does any unusual setup here)

            ImGui::EndChild();
            ImGui::PopStyleVar();
        }

        ImGui::End();
    }

    static bool first_frame = true; // SetWindowFocus doesn't switch focused tab unless second frame
    if (initialized && !done_first_focus && !first_frame)
    {
        done_first_focus = true;
        ImGui::SetWindowFocus("Projects");
    }
    first_frame = false;

    ImGui::PopStyleVar();

    ImGuiWindowClass wc{};
    wc.DockNodeFlagsOverrideSet = (int)ImGuiDockNodeFlags_NoTabBar | (int)ImGuiWindowFlags_NoDocking;
    ImGui::SetNextWindowClass(&wc);

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(3, 3));
    bool viewport_visible = ImGui::Begin("Viewport");

    // Always process viewport, even if not visible
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
        else if (project_thread_started.load())
        {
            // Launch initial simulation 1 frame late (background thread)
            if (!project_manager.getActiveProject())
            {
                DebugPrint("show_ui::setActiveProject()");
                project_manager.setActiveProject(0);
                DebugPrint("show_ui::startProject()");
                project_manager.startProject();
                project_manager.updateShadowAttributes(); // Thread safe?

                // Kick-start work-render-work-render loop
                need_draw = true;
            }
        }

        if (need_draw)
        {
            // Stop the worker and draw the fresh frame
            std::unique_lock<std::mutex> lock(state_mutex);

            canvas.begin(0.05f, 0.05f, 0.1f, 1.0f);
            project_manager.draw();
            canvas.end();

            frame_ready = false;
            frame_consumed = true;
            lock.unlock();

            // Let worker continue
            cv.notify_one();
        }
        else if (resized)
        {
            canvas.begin(0.05f, 0.05f, 0.1f, 1.0f);
            canvas.setFillStyle(0, 0, 0);
            canvas.fillRect(0, 0, (float)canvas.width(), (float)canvas.height());
            canvas.end();
        }

        // Draw cached (or freshly generated) frame
        ImGui::Image(canvas.texture(), ImVec2(
            static_cast<float>(canvas.width()),
            static_cast<float>(canvas.height())),
            ImVec2(0.0f, 1.0f),   // UV top-left (flipped)
            ImVec2(1.0f, 0.0f)    // UV bottom-right);
        );
    }
    ImGui::End();

    if (ImGui::Begin("Debug"))
    {
        debug_log.draw();
    }
    ImGui::End();

    ImGui::PopStyleVar();


    ///static bool demo_open = true;
    ///ImGui::ShowDemoWindow(&demo_open);

    /// Debugging
    {
        // Debug Log
        //debug_log.log("%d", rand());
        //debug_log.Draw();

        // Debug DPI
        //dpiDebugInfo();
    }
}

void main_loop()
{
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        ImGui_ImplSDL2_ProcessEvent(&event);
        switch (event.type)
        {
            case SDL_QUIT:
            {
                done.store(true);
                cv.notify_all();
            }
            break;

            default:
                input_event_queue.push_back(event);
            break;
        }
    }


    PlatformManager::get()->update();

    glViewport(0, 0,
        PlatformManager::get()->drawable_width(),
        PlatformManager::get()->drawable_height()
    );

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL2_NewFrame();

    ImGuiIO& io = ImGui::GetIO();

    #ifdef __EMSCRIPTEN__
    io.DisplaySize = ImVec2(
        PlatformManager::get()->drawable_width(),
        PlatformManager::get()->drawable_height()
    );
    io.DisplayFramebufferScale = ImVec2(1.0f, 1.0f);
    #endif

    DebugPrintEx(AGRESSIVE_THREAD_LOGGING, "");
    DebugPrintEx(AGRESSIVE_THREAD_LOGGING, "=============================================================");
    DebugPrintEx(AGRESSIVE_THREAD_LOGGING, "======================== NEW GUI FRAME ======================");

    // New ImGui frame
    gui_frame_running.store(true, std::memory_order_release);
    ImGui::NewFrame();

    // Draw simulation & refresh imgui drawlist
    show_ui();

    // Render
    ImGui::Render();
    gui_frame_running.store(false, std::memory_order_release);

    DebugPrintEx(AGRESSIVE_THREAD_LOGGING, "======================== END GUI FRAME ======================");
    DebugPrintEx(AGRESSIVE_THREAD_LOGGING, "=============================================================");
    DebugPrintEx(AGRESSIVE_THREAD_LOGGING, "");
    

    glClearColor(0.1f, 0.0f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    SDL_GL_SwapWindow(window);
}

void reconfigure_ui()
{
    PlatformManager::get()->update();

    static float last_dpr = -1.0f;
    float _dpr = PlatformManager::get()->dpr();

    if (fabs(_dpr - last_dpr) > 0.01f)
    {
        // DPR changed
        last_dpr = PlatformManager::get()->dpr();

        update_imgui_styles();

        // Update FreeType fonts
        ImGuiIO& io = ImGui::GetIO();
        io.Fonts->Clear();

        float base_pt = 16.0f;
        const char* font_path = PlatformManager::get()->path("/data/fonts/DroidSans.ttf");
        ImFontConfig config;
        config.OversampleH = 3;
        config.OversampleV = 3;

        io.Fonts->AddFontFromFileTTF(font_path, base_pt * _dpr * PlatformManager::get()->font_scale(), &config);
        io.Fonts->FontBuilderFlags =
            ImGuiFreeTypeBuilderFlags_LightHinting |
            ImGuiFreeTypeBuilderFlags_ForceAutoHint;

        ImGuiFreeType::GetBuilderForFreeType()->FontBuilder_Build(io.Fonts);
    }
}


#ifdef __EMSCRIPTEN__
EM_BOOL on_client_resized(int, const EmscriptenUiEvent* e, void* userData)
{
    PlatformManager::get()->resized();
    return EM_TRUE;
}
#endif

int main(int argc, char* argv[])
{
    //DebugPrint("main() called");
    //std::unique_lock<std::mutex> lock(data_mutex);

    // SDL Window setup
    {
        //SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0"); // "0" = nearest pixel sampling, NO bilinear
        //SDL_SetHint(SDL_HINT_VIDEO_HIGHDPI_DISABLED, "0");

        SDL_Init(SDL_INIT_VIDEO);

        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
        SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
        SDL_GL_SetAttribute(SDL_GL_FRAMEBUFFER_SRGB_CAPABLE, 0);
        ///SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
        ///SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
        /// 

        // Detect canvas size changes (webassembly)
        #ifdef __EMSCRIPTEN__
        int fb_w, fb_h;
        emscripten_get_canvas_element_size("#canvas", &fb_w, &fb_h);
        #else
        int fb_w = 1280;
        int fb_h = 720;
        #endif

        window = SDL_CreateWindow(
            "bitloop",
            SDL_WINDOWPOS_CENTERED,
            SDL_WINDOWPOS_CENTERED,
            fb_w, fb_h,
            SDL_WINDOW_OPENGL |
            SDL_WINDOW_RESIZABLE |
            SDL_WINDOW_MAXIMIZED |
            SDL_WINDOW_ALLOW_HIGHDPI
        );

        PlatformManager::init(window);
    }

    // OpenGL setup
    {
        gl_context = SDL_GL_CreateContext(window);
        SDL_GL_MakeCurrent(window, gl_context);

        #ifndef __EMSCRIPTEN__
        // If desktop build, load GLAD functions
        if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress))
        {
            fprintf(stderr, "Failed to initialize GLAD\n");
            return 1;
        }

        // Make colours consistent on all platforms
        glDisable(GL_FRAMEBUFFER_SRGB);
        #endif

        SDL_GL_SetSwapInterval(1);
    }

    #ifdef __EMSCRIPTEN__
    on_client_resized(0, nullptr, window);
    emscripten_set_resize_callback(
        EMSCRIPTEN_EVENT_TARGET_WINDOW,
        window,
        false,
        on_client_resized);
    #endif

    // ImGui setup
    {
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGui_ImplSDL2_InitForOpenGL(window, gl_context);

        // Initialize imgui & nanovg
        #ifdef __EMSCRIPTEN__
        ImGui_ImplOpenGL3_Init("#version 300 es");
        #else
        ImGui_ImplOpenGL3_Init();
        #endif

        reconfigure_ui();
        imgui_init();
    }

    canvas.create();


    init_simulation_thread();
    //debug_log.Log("Simulation started");

    // Start main ui loop (main thread)
    {
        #ifdef __EMSCRIPTEN__
        {
            emscripten_set_main_loop(main_loop, 0, true);
        }
        #else
        {
            // Desktop build, handle main loop and shutdown
            while (!done.load())
            {
                main_loop();
                SDL_Delay(16);
            }

            if (project_thread_started.load())
                project_thread.join();

            // Shutdown
            {
                ImGui_ImplOpenGL3_Shutdown();
                ImGui_ImplSDL2_Shutdown();
                ImGui::DestroyContext();

                SDL_GL_DeleteContext(gl_context);
                SDL_DestroyWindow(window);
                SDL_Quit();
            }
        }
        #endif
    }

    return 0;
}

#ifdef _WIN32
int WINAPI CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    return main(__argc, __argv);
}
#endif
