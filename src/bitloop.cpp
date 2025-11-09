/*
 *                 __    _ __  __                
 *                / /_  /_/ /_/ /___  ____  ____ 
 *               / __ \/ / __/ / __ \/ __ \/ __ \
 *              / /_/ / / /_/ / /_/ / /_/ / /_/ /
 *             /_____/_/\__/_/\____/\____/ ____/ 
 *                                      /_/      
 *              
 *  Copyright (C) 2025 Will Hemsworth
 * 
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 * 
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU Lesser General Public License for more details.
 * 
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/>.
 * 
 *  * 
 * ================================================
 * =============  Framework features  =============
 * ================================================
 * 
 * Goals:
 *  ✔ Rapid prototyping of scientific simulations, visualizations, and 
 *     other ideas (games, art, etc.)
 *  ✔ Optimized for high performance
 *  ✔ Seamless video encoding with FFmpeg (desktop-only for now)
 * 
 * Engine:
 *  ✔ Cross-platform (Linux:✔  Windows:✔  Emscripten:✔  macOS:✘  Android:✘  iPhone:✘)
 *  ✔ Takes advantage of latest C++23 features
 *  ✔ Multithreaded ImGui support for non-blocking UI input (updates applied at beginning of each frame)
 *  ✔ SDL3 for window/input handling
 *  ✔ Rich set helpers and 3rd party libraries for scientific simulations
 *  ✔ 128-bit floating-point support for Camera/World
 *  ✘ Timeline support with integrated scripting
 * 
 * Tools:
 *  ✔ Command-line tool for creating new projects (e.g. after bootstrapping: "bitloop new project")
 * 
 * Simulations:
 *  ✔ Modular nested project support
 *  ✔ Multi-viewport support
 *  ✔ Multiple scenes per project (which can be mounted to any number of viewports)
 * 
 * Graphics:
 *  ✔ High-DPI support
 *  ✔ NanoVG wrapper with 128-bit coordinate support (JS Canvas-like API)
 *  ✔ Easy switching between world/screen space rendering
 * 
 * Examples:
 *  ✔ Individual simulations can be compiled as standalone-apps for all supported platforms
 *  ✘ Collection of examples for different difficulty levels
 * 
 */

#include <bitloop/util/f128.h>

#include <memory>

#include <bitloop/core/project.h>

#ifdef __EMSCRIPTEN__
#define SDL_MAIN_HANDLED
#include <GLES3/gl3.h>
#include <bitloop/platform/emscripten_browser_clipboard.h>
#else
#include "glad/glad.h"
#endif

#include "stb_image.h"

/// ImGui
#include <bitloop/imguix/imgui_custom.h>
#include "imgui_impl_sdl3.h"
#include "imgui_impl_opengl3.h"

/// Project files
#include <bitloop/core/project_worker.h>
#include <bitloop/core/main_window.h>

using namespace bl;

SDL_Window* window = nullptr;
SharedSync shared_sync;
bool simulated_imgui_paste = false;


void gui_loop()
{
    // ======== Poll SDL events ========
    ImGuiIO& io = ImGui::GetIO();

    SDL_Event e;
    while (SDL_PollEvent(&e))
    {
        ImGui_ImplSDL3_ProcessEvent(&e);

        //bool imguiWantsMouse = io.WantCaptureMouse;
        bool imguiWantsKeyboard = io.WantCaptureKeyboard;

        switch (e.type)
        {
            case SDL_EVENT_QUIT:
                shared_sync.quit();
                break;

            case SDL_EVENT_WINDOW_RESIZED:
            case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
                platform()->resized();
                break;

            case SDL_EVENT_MOUSE_BUTTON_DOWN:
            case SDL_EVENT_MOUSE_BUTTON_UP:
            case SDL_EVENT_MOUSE_MOTION:
                project_worker()->queueEvent(e);
                break;

            case SDL_EVENT_MOUSE_WHEEL:
                // Project ignores scroll events when mouse is over ImGui
                if (main_window()->viewportHovered())
                    project_worker()->queueEvent(e);
                break;

            case SDL_EVENT_KEY_DOWN:
            case SDL_EVENT_KEY_UP:
            case SDL_EVENT_TEXT_INPUT:
                // Project ignores key events when ImGui input active
                if (!imguiWantsKeyboard) 
                    project_worker()->queueEvent(e);
                break;

            default:
                project_worker()->queueEvent(e);
                break;
        }
    }

    platform()->update();

    // ======== Prepare frame ========
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::GetIO().DisplaySize = (ImVec2)platform()->fbo_size();
    ImGui::GetIO().DisplayFramebufferScale = ImVec2(1.0f, 1.0f);
    ImGui::NewFrame();

    // ======== Draw window ========
    main_window()->populateUI();

    // ======== Render ========
    ImGui::Render();
    glClearColor(0.1f, 0.0f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    SDL_GL_SwapWindow(window);

    #ifdef __EMSCRIPTEN__
    // Release simulated paste keys
    if (simulated_imgui_paste) {
        simulated_imgui_paste = false;
        ImGui::GetIO().AddKeyEvent(ImGuiKey_ModCtrl, false);
        ImGui::GetIO().AddKeyEvent(ImGuiKey_V, false);
    }
    #endif
}

#ifdef __EMSCRIPTEN__
std::string clipboard_content;  // this stores the content for our internal clipboard
bool simulatedImguiPaste = false;

char const* get_content_for_imgui(ImGuiContext*)
{
    /// Callback for imgui, to return clipboard content
    return clipboard_content.c_str();
}

void set_content_from_imgui(ImGuiContext*, char const* text)
{
    /// Callback for imgui, to set clipboard content
    clipboard_content = text;
    emscripten_browser_clipboard::copy(clipboard_content);  // send clipboard data to the browser
}
void clipboard_paste_callback(std::string&& paste_data, void* callback_data)
{
    clipboard_content = std::move(paste_data);
    ImGui::GetIO().AddKeyEvent(ImGuiKey_ModCtrl, true);
    ImGui::GetIO().AddKeyEvent(ImGuiKey_V, true);
    simulated_imgui_paste = true;
}
#endif

int bitloop_main(int, char* [])
{
    // ======== SDL Window setup ========
    {
        SDL_Init(SDL_INIT_VIDEO);
        SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
        SDL_GL_SetAttribute(SDL_GL_FRAMEBUFFER_SRGB_CAPABLE, 0);

        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);

        int fb_w = 1280, fb_h = 720;

        #ifdef __EMSCRIPTEN__
        emscripten_get_canvas_element_size("#canvas", &fb_w, &fb_h);
        #endif

        blPrint() << "Creating window...\n";

        const char *window_name = 
            #ifdef BL_DEBUG
            "bitloop (debug)";
            #else
            "bitloop";
            #endif


        window = SDL_CreateWindow(window_name, fb_w, fb_h, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_MAXIMIZED | SDL_WINDOW_HIGH_PIXEL_DENSITY);

        if (!window) {
            blPrint() << "SDL_CreateWindow failed: " << SDL_GetError() << "\n";
            return 1;
        }

        // Set icon
        std::string icon_path = bl::platform()->path("/data/icon/app.png");
        int w = 0, h = 0, comp = 0;
        unsigned char* icon_rgba = stbi_load(icon_path.c_str(), &w, &h, &comp, 4);
        if (icon_rgba) {
            SDL_Surface* icon = SDL_CreateSurfaceFrom(w, h, SDL_PIXELFORMAT_RGBA32, icon_rgba, w * 4);
            SDL_SetWindowIcon(window, icon);
            SDL_DestroySurface(icon);
        }
    }

    SDL_GLContext gl_context = nullptr;

    // ======== OpenGL setup ========
    {
        gl_context = SDL_GL_CreateContext(window);
        SDL_GL_MakeCurrent(window, gl_context);
        SDL_GL_SetSwapInterval(1); // enforces 60fps / v-sync

        #ifndef __EMSCRIPTEN__
        if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress)) {
            blPrint() << "Failed to initialize GLAD\n";
            return 1;
        }

        // Make colours consistent on desktop build
        glDisable(GL_FRAMEBUFFER_SRGB);
        #endif

    }

    auto _platform_manager = std::make_unique<PlatformManager>(window);
    auto _main_window      = std::make_unique<MainWindow>(shared_sync);
    auto _project_worker   = std::make_unique<ProjectWorker>(shared_sync, _main_window->getRecordManager());

    // ======== ImGui setup ========
    {
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImPlot::CreateContext();
        ImGui_ImplSDL3_InitForOpenGL(window, gl_context);
        ImGui_ImplOpenGL3_Init();

        #ifdef __EMSCRIPTEN__
        emscripten_browser_clipboard::paste(clipboard_paste_callback);

        ImGuiPlatformIO& platform_io = ImGui::GetPlatformIO();
        platform_io.Platform_GetClipboardTextFn = get_content_for_imgui;
        platform_io.Platform_SetClipboardTextFn = set_content_from_imgui;
        #endif
    }

    // ======== Init window & start worker thread ========
    {
        platform()->init();
        main_window()->init();
        project_worker()->startWorker();
    }


    // ======== Start main gui loop ========
    {
        #ifdef __EMSCRIPTEN__
        emscripten_set_main_loop(gui_loop, 0, true);
        #else
        {
            while (!shared_sync.quitting.load())
            {
                gui_loop();
                SDL_Delay(0);
            }

            // Gracefully exit
            project_worker()->end();

            ImGui_ImplOpenGL3_Shutdown();
            ImGui_ImplSDL3_Shutdown();
            ImGui::DestroyContext();
            SDL_GL_DestroyContext(gl_context);
            SDL_DestroyWindow(window);
            SDL_Quit();
        }
        #endif
    }
    return 0;
}
