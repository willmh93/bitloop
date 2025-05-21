#pragma once

/// SDL2
#include <SDL2/SDL.h>

/// emscripten
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <emscripten/html5.h>
#endif

/// Windows
#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#endif

#include <functional>

#include "debug.h"
#include "types.h"
//#include "imgui_custom.h"

class PlatformManager
{
    SDL_Window* window;

    
    //float _dpr = 1.0f;
    float _dpi = 96.0f;

    double css_w, css_h;
    int gl_w, gl_h;
    int win_w, win_h;
    int fb_w, fb_h;

    bool is_mobile_device = false;

    void update_device_dpi();

public:

    static PlatformManager* get()
    {
        static PlatformManager singleton;
        return &singleton;
    }

    static void prepare(SDL_Window* window)
    {
        get()->window = window;
        get()->init();
    }

    void init();

    // Required function calls
    void update();
    void resized();

    int gl_width()      { return gl_w; }
    int gl_height()     { return gl_h; }
    int fbo_width()     { return fb_w; }
    int fbo_height()    { return fb_h; }
    int window_width()  { return win_w; }
    int window_height() { return win_h; }

    // Device Info
    float dpi() { return _dpi; }

    #ifdef DEBUG_SIMULATE_DPR
    float dpr() { return DEBUG_SIMULATE_DPR; }
    #else
    float dpr() { return (float)gl_w / (float)win_w; }
    #endif

    bool device_vertical();
    void device_orientation(int* orientation_angle, int* orientation_index = nullptr);
    bool device_orientation_changed(std::function<void(int, int)> onChanged);
    
    // Platform detection
    bool is_mobile();
    bool is_desktop_native();
    bool is_desktop_browser();
    bool is_touch_device();
    float touch_accuracy();

    // Scale
    float font_scale();
    float ui_scale_factor(float extra_mobile_mult=1.0f);

    // Window
    float window_width_inches();
    float window_height_inches();
    float window_size_inches();

    // Layout helpers
    float max_char_rows();
    float max_char_cols();

    // File system helpers
    const char* path(const char* virtual_path);
};

inline PlatformManager* Platform()
{
    return PlatformManager::get();
}

inline float ScaleSize(float length)
{
    return PlatformManager::get()->dpr() * length;
}