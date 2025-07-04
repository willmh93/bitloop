﻿#include <memory>
#ifdef __EMSCRIPTEN__
#define SDL_MAIN_HANDLED
#include <GLES3/gl3.h>
#else
#include "glad.h"
#endif

/// ImGui
#include "imgui_custom.h"
#include "backends/imgui_impl_sdl3.h"
#include "backends/imgui_impl_opengl3.h"

/// Project files
#include "platform.h"
#include "project.h"
#include "project_worker.h"
#include "main_window.h"

#include "simulations_registry.h"

SDL_Window* window = nullptr;
SharedSync shared_sync;

std::unordered_map<size_t, size_t> thread_map;

void gui_loop()
{
    // ======== Poll SDL events ========
    SDL_Event e;
    while (SDL_PollEvent(&e))
    {
        ImGui_ImplSDL3_ProcessEvent(&e);

        switch (e.type)
        {
            case SDL_EVENT_QUIT: shared_sync.quit(); break;
            case SDL_EVENT_WINDOW_RESIZED:
            case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
                Platform()->resized();
                break;
            default: ProjectWorker::instance()->queueEvent(e); break;
        }
    }

    Platform()->update();

    // ======== Prepare frame ========
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::GetIO().DisplaySize = ImVec2((float)Platform()->fbo_width(), (float)Platform()->fbo_height());
    ImGui::GetIO().DisplayFramebufferScale = ImVec2(1.0f, 1.0f);
    ImGui::NewFrame();

    // ======== Draw window ========
    MainWindow::instance()->populateUI();

    // ======== Render ========
    ImGui::Render();
    glClearColor(0.1f, 0.0f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    SDL_GL_SwapWindow(window);
}



int main(int, char* [])
{
    force_link_all_simulations();

    // ======== SDL Window setup ========
    {
        SDL_Init(SDL_INIT_VIDEO);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
        SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
        SDL_GL_SetAttribute(SDL_GL_FRAMEBUFFER_SRGB_CAPABLE, 0);

        int fb_w = 1280, fb_h = 720;

        #ifdef __EMSCRIPTEN__
        emscripten_get_canvas_element_size("#canvas", &fb_w, &fb_h);

        //SDL_SetHint(SDL_HINT_EMSCRIPTEN_KEYBOARD_ELEMENT, "#canvas"); // disabled, breaks imgui input

        EM_ASM({
            const cv = Module['canvas'];
            cv.setAttribute('tabindex', '0');
            cv.setAttribute('contenteditable', 'true');
            cv.addEventListener('mousedown', () => cv.focus());
        });
        #endif

        ///SDL_WindowProperties props = {
        ///    .title = "bitloop",
        ///    .x = SDL_WINDOWPOS_CENTERED,
        ///    .y = SDL_WINDOWPOS_CENTERED,
        ///    .width = fb_w,
        ///    .height = fb_h,
        ///    .flags = SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_MAXIMIZED | SDL_WINDOW_HIGH_PIXEL_DENSITY
        ///};

        window = SDL_CreateWindow("bitloop", fb_w, fb_h, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_MAXIMIZED | SDL_WINDOW_HIGH_PIXEL_DENSITY);

        //window = SDL_CreateWindow("bitloop", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, fb_w, fb_h,
        //    SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_MAXIMIZED | SDL_WINDOW_HIGH_PIXEL_DENSITY);

    }

    SDL_GLContext gl_context;

    // ======== OpenGL setup ========
    {
        gl_context = SDL_GL_CreateContext(window);
        SDL_GL_MakeCurrent(window, gl_context);
        SDL_GL_SetSwapInterval(1); // enforces 60fps / v-sync

        #ifndef __EMSCRIPTEN__
        if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress))
            return 1; // Failed to initialize GLAD

        // Make colours consistent on with desktop build
        glDisable(GL_FRAMEBUFFER_SRGB);
        #endif
    }

    auto platform_manager = std::make_unique<PlatformManager>(window);
    auto project_worker   = std::make_unique<ProjectWorker>(shared_sync);
    auto main_window      = std::make_unique<MainWindow>(shared_sync);

    // ======== ImGui setup ========
    {
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGui_ImplSDL3_InitForOpenGL(window, gl_context);

        #ifdef __EMSCRIPTEN__
        ImGui_ImplOpenGL3_Init("#version 300 es");
        #else
        ImGui_ImplOpenGL3_Init();
        #endif
    }

    // ======== Init Window & Start worker thread ========
    {
        MainWindow::instance()->init();
        ProjectWorker::instance()->startWorker();
    }

    // ======== Start main ui loop ========
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
            ProjectWorker::instance()->end();

            ImGui_ImplOpenGL3_Shutdown();
            ImGui_ImplSDL3_Shutdown();
            ImGui::DestroyContext();
            ///SDL_GL_DeleteContext(gl_context);
            SDL_GL_DestroyContext(gl_context);
            SDL_DestroyWindow(window);
            SDL_Quit();
        }
        #endif
    }
    return 0;
}

#ifdef _WIN32
int WINAPI CALLBACK WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
    return main(__argc, __argv); // Redirect entry point for WIN32 build
}
#endif