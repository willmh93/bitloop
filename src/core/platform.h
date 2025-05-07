#pragma once
#include <SDL2/SDL.h>
#include <functional>
#ifdef __EMSCRIPTEN__
#include <emscripten/html5.h>
#endif

#include "debug.h"
#include "imgui_custom.h"

class PlatformManager
{
    SDL_Window* window;

    
    float _dpr = 1.0f;
    float _dpi = 96.0f;

    double css_w, css_h;
    int gl_w, gl_h;
    int win_w, win_h;
    int fb_w, fb_h;

    void update_device_dpi();

public:

    static PlatformManager* get()
    {
        static PlatformManager singleton;
        return &singleton;
    }

    static void init(SDL_Window* window)
    {
        PlatformManager::get()->window = window;
    }

    // Required function calls
    void update();
    void resized();

    int drawable_width()  { return fb_w; }
    int drawable_height() { return fb_h; }
    int window_width()    { return win_w; }
    int window_height()   { return win_h; }

    // Device Info
    float dpi() { return _dpi; }
    float dpr() { return (float)gl_w / (float)win_w; }

    bool device_vertical();
    void device_orientation(int* orientation_angle, int* orientation_index = nullptr);
    bool device_orientation_changed(std::function<void(int, int)> onChanged);
    

    // Mobile/Touch
    int is_mobile();
    bool is_touch_device();
    float touch_accuracy();

    // Scale
    float font_scale();
    float ui_scale_factor();

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

inline ImVec2 ScaleSize(const ImVec2& size)
{
    float dpr = PlatformManager::get()->dpr();
    return ImVec2(size.x * dpr, size.y * dpr);
}

inline ImVec2 ScaleSize(int w, int h)
{
    float dpr = PlatformManager::get()->dpr();
    return ImVec2((float)w * dpr, (float)h * dpr);
}

inline ImVec2 ScaleSize(float w, float h)
{
    float dpr = PlatformManager::get()->dpr();
    return ImVec2(w * dpr, h * dpr);
}