/// emscripten
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <emscripten/html5.h>
#else
#define SDL_MAIN_HANDLED
#endif

/// Windows
#ifdef _WIN32
#include <windows.h>
#endif

/// ImGui
#include "imgui.h"
#include "imgui_internal.h"
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

/// NanoVG
//#include "nanovg.h"
//#include "nanovg_gl.h"

#include "project.h"
#include "nanovg_canvas.h"

#include "TestSim.h"

SDL_Window* window;
SDL_GLContext gl_context;
int client_width = 0, client_height = 0;
bool done = false;

Canvas canvas;

Project* active_project = new TestSim();
std::thread project_thread;

std::condition_variable data_ready_cv;
std::mutex data_mutex;
bool ready_to_render = false;

int calculate_heavy_prime(int target_index) {
    int count = 0;
    int num = 1;
    while (count < target_index) {
        num++;
        bool is_prime = true;
        for (int i = 2; i * i <= num; ++i) {
            if (num % i == 0) {
                is_prime = false;
                break;
            }
        }
        if (is_prime)
            ++count;
    }
    return num;
}

void sim_worker()
{
    while (!done)
    {
        //int prime = calculate_heavy_prime(40000 + (rand() % 40000));

        {
            std::unique_lock<std::mutex> lock(data_mutex);
            active_project->process();
        }

        ready_to_render = true;
        data_ready_cv.notify_one();

        SDL_Delay(16);
    }
}

void init_simulation_thread()
{
    project_thread = std::thread(sim_worker);
}

/*void create_fbo()
{
    if (fbo) glDeleteFramebuffers(1, &fbo);
    if (tex) glDeleteTextures(1, &tex);
    if (rbo) glDeleteRenderbuffers(1, &rbo);

    glGenFramebuffers(1, &fbo);
    glGenTextures(1, &tex);
    glGenRenderbuffers(1, &rbo);

    glBindTexture(GL_TEXTURE_2D, tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, fb_width, fb_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

    glBindRenderbuffer(GL_RENDERBUFFER, rbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, fb_width, fb_height);

    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex, 0);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void render_nanovg()
{
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glViewport(0, 0, fb_width, fb_height);
    glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    nvgBeginFrame(vg, fb_width, fb_height, 1.0f);
    nvgBeginPath(vg);
    nvgCircle(vg, fb_width / 2, fb_height / 2, 100);
    nvgFillColor(vg, nvgRGBA(0, 192, 255, 255));
    nvgFill(vg);
    nvgEndFrame(vg);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}*/

void set_imgui_styles()
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



    /*ImVec4* colors = ImGui::GetStyle().Colors;
    colors[ImGuiCol_Text] = ImVec4(0.90f, 0.90f, 0.90f, 1.00f);
    colors[ImGuiCol_TextDisabled] = ImVec4(0.60f, 0.60f, 0.60f, 1.00f);
    colors[ImGuiCol_WindowBg] = ImVec4(0.04f, 0.05f, 0.05f, 0.85f);
    colors[ImGuiCol_ChildBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_PopupBg] = ImVec4(0.11f, 0.11f, 0.14f, 0.92f);
    colors[ImGuiCol_Border] = ImVec4(0.50f, 0.50f, 0.50f, 0.50f);
    colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_FrameBg] = ImVec4(0.43f, 0.43f, 0.43f, 0.39f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.47f, 0.47f, 0.69f, 0.40f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.42f, 0.41f, 0.64f, 0.69f);
    colors[ImGuiCol_TitleBg] = ImVec4(0.27f, 0.27f, 0.54f, 0.83f);
    colors[ImGuiCol_TitleBgActive] = ImVec4(0.32f, 0.32f, 0.63f, 0.87f);
    colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.40f, 0.40f, 0.80f, 0.20f);
    colors[ImGuiCol_MenuBarBg] = ImVec4(0.40f, 0.40f, 0.55f, 0.80f);
    colors[ImGuiCol_ScrollbarBg] = ImVec4(0.20f, 0.25f, 0.30f, 0.60f);
    colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.40f, 0.40f, 0.80f, 0.30f);
    colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.40f, 0.40f, 0.80f, 0.40f);
    colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.41f, 0.39f, 0.80f, 0.60f);
    colors[ImGuiCol_CheckMark] = ImVec4(0.90f, 0.90f, 0.90f, 0.50f);
    colors[ImGuiCol_SliderGrab] = ImVec4(1.00f, 1.00f, 1.00f, 0.30f);
    colors[ImGuiCol_SliderGrabActive] = ImVec4(0.41f, 0.39f, 0.80f, 0.60f);
    colors[ImGuiCol_Button] = ImVec4(0.35f, 0.40f, 0.61f, 0.62f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.40f, 0.48f, 0.71f, 0.79f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.46f, 0.54f, 0.80f, 1.00f);
    colors[ImGuiCol_Header] = ImVec4(0.40f, 0.40f, 0.90f, 0.45f);
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.45f, 0.45f, 0.90f, 0.80f);
    colors[ImGuiCol_HeaderActive] = ImVec4(0.53f, 0.53f, 0.87f, 0.80f);
    colors[ImGuiCol_Separator] = ImVec4(0.50f, 0.50f, 0.50f, 0.60f);
    colors[ImGuiCol_SeparatorHovered] = ImVec4(0.60f, 0.60f, 0.70f, 1.00f);
    colors[ImGuiCol_SeparatorActive] = ImVec4(0.70f, 0.70f, 0.90f, 1.00f);
    colors[ImGuiCol_ResizeGrip] = ImVec4(1.00f, 1.00f, 1.00f, 0.10f);
    colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.78f, 0.82f, 1.00f, 0.60f);
    colors[ImGuiCol_ResizeGripActive] = ImVec4(0.78f, 0.82f, 1.00f, 0.90f);
    colors[ImGuiCol_InputTextCursor] = ImVec4(0.90f, 0.90f, 0.90f, 1.00f);
    colors[ImGuiCol_TabHovered] = ImVec4(0.45f, 0.45f, 0.90f, 0.80f);
    colors[ImGuiCol_Tab] = ImVec4(0.34f, 0.34f, 0.68f, 0.79f);
    colors[ImGuiCol_TabSelected] = ImVec4(0.40f, 0.40f, 0.73f, 0.84f);
    colors[ImGuiCol_TabSelectedOverline] = ImVec4(0.53f, 0.53f, 0.87f, 0.80f);
    colors[ImGuiCol_TabDimmed] = ImVec4(0.28f, 0.28f, 0.57f, 0.82f);
    colors[ImGuiCol_TabDimmedSelected] = ImVec4(0.35f, 0.35f, 0.65f, 0.84f);
    colors[ImGuiCol_TabDimmedSelectedOverline] = ImVec4(0.53f, 0.53f, 0.87f, 0.00f);
    colors[ImGuiCol_DockingPreview] = ImVec4(0.40f, 0.40f, 0.90f, 0.31f);
    colors[ImGuiCol_DockingEmptyBg] = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
    colors[ImGuiCol_PlotLines] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
    colors[ImGuiCol_PlotLinesHovered] = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
    colors[ImGuiCol_PlotHistogram] = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
    colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
    colors[ImGuiCol_TableHeaderBg] = ImVec4(0.27f, 0.27f, 0.38f, 1.00f);
    colors[ImGuiCol_TableBorderStrong] = ImVec4(0.31f, 0.31f, 0.45f, 1.00f);
    colors[ImGuiCol_TableBorderLight] = ImVec4(0.26f, 0.26f, 0.28f, 1.00f);
    colors[ImGuiCol_TableRowBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_TableRowBgAlt] = ImVec4(1.00f, 1.00f, 1.00f, 0.07f);
    colors[ImGuiCol_TextLink] = ImVec4(0.53f, 0.53f, 0.87f, 0.80f);
    colors[ImGuiCol_TextSelectedBg] = ImVec4(0.00f, 0.00f, 1.00f, 0.35f);
    colors[ImGuiCol_TreeLines] = ImVec4(0.50f, 0.50f, 0.50f, 0.50f);
    colors[ImGuiCol_DragDropTarget] = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
    colors[ImGuiCol_NavCursor] = ImVec4(0.45f, 0.45f, 0.90f, 0.80f);
    colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
    colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
    colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.20f, 0.20f, 0.20f, 0.35f);*/
}



void process_ui()
{
    // Create a fullscreen dockspace
    ImGuiWindowFlags window_flags =
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
    ImGui::Begin("MainDockSpaceHost", nullptr, window_flags);
    ImGui::PopStyleVar(3);

    ImGuiID dockspace_id = ImGui::GetID("MyDockspace");
    ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_None);
    ImGui::End();

    // Build initial layout (once)
    static bool initialized = false;
    if (!initialized)
    {
        initialized = true;

        ImGui::DockBuilderRemoveNode(dockspace_id); // clear any previous layout
        ImGui::DockBuilderAddNode(dockspace_id, ImGuiDockNodeFlags_DockSpace);

        ImGui::DockBuilderSetNodeSize(dockspace_id, ImGui::GetMainViewport()->Size);

        ImGuiID dock_main_id = dockspace_id;
        ImGuiID dock_left_id = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Left, 0.25f, nullptr, &dock_main_id);
        //ImGuiID dock_right_id = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Right, 0.3f, nullptr, &dock_main_id);
        ImGuiID dock_top_id = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Up, 0.12f, nullptr, &dock_main_id);


        ImGui::DockBuilderDockWindow("Options", dock_left_id);
        //ImGui::DockBuilderDockWindow("Right Window", dock_right_id);
        ImGui::DockBuilderDockWindow("Timeline", dock_top_id);
        ImGui::DockBuilderDockWindow("Viewport", dock_main_id);

        ImGui::DockBuilderFinish(dockspace_id);
    }

    // Show windows
    if (ImGui::Begin("Options"))
    {
        if (ImGui::TreeNodeEx("Tree Nodes", ImGuiTreeNodeFlags_DefaultOpen))
        {
            static ImGuiTreeNodeFlags base_flags = 
                ImGuiTreeNodeFlags_OpenOnArrow | 
                ImGuiTreeNodeFlags_OpenOnDoubleClick | 
                ImGuiTreeNodeFlags_SpanAvailWidth;

            static int selection_mask = (1 << 2);

            for (int i = 0; i < 5; i++)
            {
                ImGui::PushID(i);
                if (ImGui::TreeNodeEx("", ImGuiTreeNodeFlags_DefaultOpen, "Child Node %d", i))
                {
                    ImGui::TreePop();
                }
                ImGui::PopID();
            }
            ImGui::TreePop();
        }
    }
    ImGui::End();

    //if (ImGui::Begin("Right Window"))
    //    ImGui::Text("Hello from the right docked window!");
    //ImGui::End();

    if (ImGui::Begin("Timeline"))
    {

    }
    ImGui::End();

    //static ImVec2 lastViewportSize = ImVec2(0, 0);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(3,3));
    if (ImGui::Begin("Viewport"))
    {
        //ImVec2 size = ImGui::GetWindowSize();
        ImVec2 size = ImGui::GetContentRegionAvail();
        int width = static_cast<int>(size.x);
        int height = static_cast<int>(size.y);

        if (canvas.resize(width, height))
        {
            canvas.begin(0.05f, 0.05f, 0.1f, 1.0f);
            active_project->draw(&canvas);
            canvas.end();
        }

        //ImGui::Image((ImTextureID)(intptr_t)tex, ImVec2(fb_width, fb_height));
        ImGui::Image(canvas.texture(), ImVec2(canvas.width(), canvas.height()),
            ImVec2(0, 1),   // UV top-left (flipped)
            ImVec2(1, 0)    // UV bottom-right);
        );
    }

    ImGui::End();
    ImGui::PopStyleVar();

    static bool demo_open = true;
    ImGui::ShowDemoWindow(&demo_open);
}

void main_loop() 
{
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        ImGui_ImplSDL2_ProcessEvent(&event);
        if (event.type == SDL_QUIT)
            done = true;
    }

    //render_nanovg();

    if (active_project)
    {
        std::unique_lock<std::mutex> lock(data_mutex);
        data_ready_cv.wait(lock, [] { return ready_to_render; });

        canvas.begin(0.05f, 0.05f, 0.1f, 1.0f);
        active_project->draw(&canvas);
        canvas.end();

        ready_to_render = false;

        lock.unlock();
    }


    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();

    // Process imgui ui
    process_ui();

    ImGui::Render();
    glViewport(0, 0, client_width, client_height);
    glClearColor(0.1f, 0.0f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    SDL_GL_SwapWindow(window);
}

#ifdef __EMSCRIPTEN__
// Handle canvas resize (webassembly)
EM_BOOL on_client_resized(int eventType, const EmscriptenUiEvent* uiEvent, void* userData)
{
    emscripten_get_canvas_element_size("#canvas", &client_width, &client_height);
    SDL_SetWindowSize(window, client_width, client_height);
    glViewport(0, 0, client_width, client_height);
    return EM_TRUE;
}
#endif

int main(int argc, char* argv[])
{
    SDL_Init(SDL_INIT_VIDEO);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_FRAMEBUFFER_SRGB_CAPABLE, 0);

    // Detect canvas size changes (webassembly)
    #ifdef __EMSCRIPTEN__
    emscripten_get_canvas_element_size("#canvas", &client_width, &client_height);
    emscripten_set_resize_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, nullptr, false, on_client_resized);
    #else
    client_width = 1280;
    client_height = 720;
    #endif

    window = SDL_CreateWindow(
        "bitloop",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        client_width,
        client_height,
        SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_MAXIMIZED
    );

    gl_context = SDL_GL_CreateContext(window);
    SDL_GL_MakeCurrent(window, gl_context);

    #ifndef __EMSCRIPTEN__
    if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress)) 
    {
        fprintf(stderr, "Failed to initialize GLAD\n");
        return 1;
    }
    #endif

    #ifdef __WIN32__
    glDisable(GL_FRAMEBUFFER_SRGB);
    #endif

    SDL_GL_SetSwapInterval(1);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();

    ImGui::LoadIniSettingsFromMemory("");
    io.IniFilename = nullptr;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    ImGui_ImplSDL2_InitForOpenGL(window, gl_context);

    // Initialize imgui & nanovg
    #ifdef __EMSCRIPTEN__
    ImGui_ImplOpenGL3_Init("#version 300 es");
    //vg = nvgCreateGLES3(NVG_ANTIALIAS | NVG_STENCIL_STROKES);
    #else
    ImGui_ImplOpenGL3_Init();
    //vg = nvgCreateGL3(NVG_ANTIALIAS | NVG_STENCIL_STROKES);
    #endif

    canvas.create();
    //create_fbo();

    set_imgui_styles();

    // Launch simulation in background thread
    init_simulation_thread();

    // Start main ui loop (on main thread)
    #ifdef __EMSCRIPTEN__
    emscripten_set_main_loop(main_loop, 0, true);
    #else

    while (!done)
    {
        SDL_GetWindowSize(window, &client_width, &client_height);
        main_loop();
        SDL_Delay(16);
    }

    project_thread.join();

    // Desktop cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    SDL_GL_DeleteContext(gl_context);
    SDL_DestroyWindow(window);
    SDL_Quit();
    #endif

    return 0;
}

#ifdef _WIN32
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    return main(__argc, __argv);
}
#endif
