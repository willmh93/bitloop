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


SDL_Window* window;
SDL_GLContext gl_context;
std::atomic<bool> done = false;


ToolbarButtonState play = { ImVec4(0.1f, 0.6f, 0.1f, 1.0f), ImVec4(1, 1, 1, 1), false };
ToolbarButtonState stop = { ImVec4(0.6f, 0.1f, 0.1f, 1.0f), ImVec4(1, 1, 1, 1), false };
ToolbarButtonState pause = { ImVec4(0.3f, 0.3f, 0.3f, 1.0f), ImVec4(1, 1, 1, 1), false };
ToolbarButtonState record = { ImVec4(0.8f, 0.0f, 0.0f, 1.0f), ImVec4(1, 1, 1, 1), false };

Canvas canvas;
ImDebugLog debug_log;

ProjectManager project_manager;
std::thread project_thread;
std::condition_variable data_ready_cv;
std::mutex data_mutex;

std::atomic<bool> project_thread_started = false;
std::atomic<bool> ready_to_render = false;
bool imgui_initialized = false;
bool update_docking_layout = false;

void update_imgui_styles()
{
    ImGuiStyle& style = ImGui::GetStyle();

    // Corners
    style.WindowRounding = 8.0f;
    style.ChildRounding = 8.0f;
    style.FrameRounding = 6.0f;
    style.FramePadding = ImVec2(8.0f, 5.0f);
    style.PopupRounding = 3.0f;
    style.ScrollbarRounding = 6.0f;
    style.GrabRounding = 2.0f;
    style.TabRounding = 6.0f;
    style.ScrollbarSize = Platform::get()->is_mobile_device() ? 30.0f : 14;

    // Colors
    ImVec4* colors = ImGui::GetStyle().Colors;
    colors[ImGuiCol_WindowBg] = ImVec4(0.19f, 0.20f, 0.21f, 0.85f);
    colors[ImGuiCol_TitleBg] = ImVec4(0.25f, 0.28f, 0.38f, 0.83f);
    colors[ImGuiCol_TitleBgActive] = ImVec4(0.32f, 0.43f, 0.63f, 0.87f);
    colors[ImGuiCol_Header] = ImVec4(0.44f, 0.62f, 0.85f, 0.45f);
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.45f, 0.69f, 0.90f, 0.80f);
    colors[ImGuiCol_HeaderActive] = ImVec4(0.53f, 0.68f, 0.87f, 0.80f);
    colors[ImGuiCol_TabHovered] = ImVec4(0.45f, 0.67f, 0.90f, 0.80f);
    colors[ImGuiCol_Tab] = ImVec4(0.34f, 0.47f, 0.68f, 0.79f);
    colors[ImGuiCol_TabSelected] = ImVec4(0.40f, 0.59f, 0.73f, 0.84f);
    colors[ImGuiCol_TabSelectedOverline] = ImVec4(0.53f, 0.72f, 0.87f, 0.80f);
    colors[ImGuiCol_TabDimmed] = ImVec4(0.28f, 0.41f, 0.57f, 0.82f);
    colors[ImGuiCol_TabDimmedSelected] = ImVec4(0.35f, 0.46f, 0.65f, 0.84f);

    style.ScaleAllSizes(Platform::get()->ui_scale_factor());
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


void show_toolbar()
{
    if (Platform::get()->is_mobile_device())
        return;

    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(10, 0));
    ImGui::RenderToolbar(play, stop, pause, record, 30.0f * Platform::get()->window_dpr());
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
            std::unique_lock<std::mutex> lock(data_mutex);
            project_manager.setActiveProject(node.project_info->sim_uid);
            project_manager.startProject();
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
                show_tree_node(node.children[child_i], i, depth+1);
            }
            ImGui::Unindent();
        }
    }
    ImGui::PopID();
}

void show_project_tree(bool expand_vertical)
{
    ImVec2 frameSize = ImVec2(0.0f, expand_vertical ? 0 : 150.0f); // Let height auto-expand
    ImVec4 bgColor = ImVec4(0.1f, 0.1f, 0.1f, 1.0f); // Custom background

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(4.0f, 4.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 0.0f);
    {
        ImGui::BeginChild("TreeFrame", frameSize, 0, ImGuiWindowFlags_AlwaysUseWindowPadding);

        ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 8.0f);
        ImGui::PushStyleColor(ImGuiCol_ChildBg, bgColor);

        ImGui::BeginChild("TreeFrameInner", ImVec2(0.0f, 0.0f), true);

        auto& tree = Project::projectTreeRootInfo();
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
        {
            std::unique_lock<std::mutex> lock(data_mutex);
            if (project_manager.getActiveProject())
                project_manager.process();
        }

        ready_to_render.store(true);
        data_ready_cv.notify_one();

        SDL_Delay(16);
    }
}

void init_simulation_thread()
{
    std::unique_lock<std::mutex> lock(data_mutex);
    project_manager.setSharedCanvas(&canvas);
    project_manager.setSharedDebugLog(&debug_log);
    project_thread = std::thread(sim_worker);
    project_thread_started.store(true);
}

void show_project_attributes()
{
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8, 8));
    ImGui::BeginChild("AttributesFrame", ImVec2(0, 0), 0, ImGuiWindowFlags_AlwaysUseWindowPadding);

    {
        std::unique_lock<std::mutex> lock(data_mutex);   // blocks the worker
        project_manager.populateAttributes();            // safe read
    }

    ImGui::EndChild();
    ImGui::PopStyleVar();
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

        ImGui::DockBuilderDockWindow("Projects", dock_sidebar);
        ImGui::DockBuilderDockWindow("Active", dock_sidebar);
        ImGui::DockBuilderDockWindow("Debug", dock_sidebar);
        //ImGui::DockBuilderDockWindow("Right Window", dock_right_id);
        //ImGui::DockBuilderDockWindow("Timeline", dock_top_id);
        ImGui::DockBuilderDockWindow("Viewport", dock_main_id);

        ImGui::DockBuilderFinish(dockspace_id);
    }

    int window_flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoDecoration;

    //if (Platform::is_mobile_device())
        window_flags |= ImGuiWindowFlags_NoMove;

    // Show windows
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));

    if (vertical_layout || Platform::get()->max_char_rows() < 40.0f)
    {
        // Collapse layout
        if (ImGui::Begin("Projects", nullptr, window_flags))
        {
            show_toolbar();
            show_project_tree(true);
        }
        ImGui::End();

        if (ImGui::Begin("Active", nullptr, window_flags))
        {
            show_toolbar();
            show_project_attributes();
        }
        ImGui::End();
    }
    else
    {
        // Show both windows
        if (ImGui::Begin("Projects", nullptr, window_flags))
        {
            show_toolbar();
            show_project_tree(false);
            show_project_attributes();
        }
        ImGui::End();
    }

    ImGui::PopStyleVar();
    
    ImGuiWindowClass wc{};
    wc.DockNodeFlagsOverrideSet = (int)ImGuiDockNodeFlags_NoTabBar | (int)ImGuiWindowFlags_NoDocking;
    ImGui::SetNextWindowClass(&wc);

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(3, 3));
    if (ImGui::Begin("Viewport"))
    {
        ImVec2 size = ImGui::GetContentRegionAvail();
        int width = static_cast<int>(size.x);
        int height = static_cast<int>(size.y);

        static bool done_first_size = false;
        
        canvas.resize(width, height);
        
        {
            std::unique_lock<std::mutex> lock(data_mutex);
            data_ready_cv.wait(lock, [] { return ready_to_render.load(); });

            if (!done_first_size)
            {
                done_first_size = true;
            }
            else if (project_thread_started.load())
            {
                // Launch initial simulation 1 frame late (background thread)
                if (!project_manager.getActiveProject())
                {
                    project_manager.setActiveProject(0);
                    project_manager.startProject();
                }
            }

            // Data mutex scope
            if (project_manager.getActiveProject())
            {
                data_ready_cv.wait(lock, [] { return ready_to_render.load(); });

                canvas.begin(0.05f, 0.05f, 0.1f, 1.0f);
                project_manager.draw();
                canvas.end();
            }

            ready_to_render.store(false);
        }

        ImGui::Image(canvas.texture(), ImVec2(
            static_cast<float>(canvas.width()), 
            static_cast<float>(canvas.height())),
            ImVec2(0.0f, 1.0f),   // UV top-left (flipped)
            ImVec2(1.0f, 0.0f)    // UV bottom-right);
        );
    }

    if (ImGui::Begin("Debug"))
    {
        debug_log.draw();
    }
    ImGui::End();

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
    while (SDL_PollEvent(&event)) {
        ImGui_ImplSDL2_ProcessEvent(&event);
        if (event.type == SDL_QUIT)
            done.store(true);
    }


    Platform::get()->update();

    glViewport(0, 0, 
        Platform::get()->drawable_width(), 
        Platform::get()->drawable_height()
    );

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL2_NewFrame();

    ImGuiIO& io = ImGui::GetIO();

    #ifdef __EMSCRIPTEN__
    io.DisplaySize = ImVec2(
        Platform::get()->drawable_width(),
        Platform::get()->drawable_height()
    );
    io.DisplayFramebufferScale = ImVec2(1.0f, 1.0f);
    #endif

    ImGui::NewFrame();

    // Draw simulation & refresh imgui drawlist
    show_ui();

    // Render
    ImGui::Render();
    
    glClearColor(0.1f, 0.0f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    SDL_GL_SwapWindow(window);
}

void reconfigure_ui()
{
    Platform::get()->update();

    static float last_dpr = -1.0f;
    float dpr = Platform::get()->window_dpr();

    if (fabs(dpr - last_dpr) > 0.01f)
    {
        // DPR changed
        last_dpr = Platform::get()->window_dpr();

        update_imgui_styles();

        // Update FreeType fonts
        ImGuiIO& io = ImGui::GetIO();
        io.Fonts->Clear();

        float base_pt = 16.0f;
        const char* font_path = Platform::get()->path("/data/fonts/DroidSans.ttf");
        ImFontConfig config;
        config.OversampleH = 3;
        config.OversampleV = 3;

        io.Fonts->AddFontFromFileTTF(font_path, base_pt * dpr * Platform::get()->font_scale(), &config);
        io.Fonts->FontBuilderFlags =
            ImGuiFreeTypeBuilderFlags_LightHinting | 
            ImGuiFreeTypeBuilderFlags_ForceAutoHint;

        ImGuiFreeType::GetBuilderForFreeType()->FontBuilder_Build(io.Fonts);
    }
}


#ifdef __EMSCRIPTEN__
EM_BOOL on_client_resized(int, const EmscriptenUiEvent* e, void* userData)
{
    Platform::get()->resized();
    return EM_TRUE;
}
#endif

int main(int argc, char* argv[])
{
    //DebugMessage("main() called");
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

        Platform::init(window);
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
