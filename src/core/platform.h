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

class PlatformManager
{
    static PlatformManager *singleton;

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

    static constexpr PlatformManager* get()
    {
        return singleton;
    }

    PlatformManager(SDL_Window* _window)
    {
        singleton = this;
        window = _window;
        init();
    }

    void init();

    // Required function calls
    void update();
    void resized();

    [[nodiscard]] int gl_width()      { return gl_w; }
    [[nodiscard]] int gl_height()     { return gl_h; }
    [[nodiscard]] int fbo_width()     { return fb_w; }
    [[nodiscard]] int fbo_height()    { return fb_h; }
    [[nodiscard]] int window_width()  { return win_w; }
    [[nodiscard]] int window_height() { return win_h; }

    // Device Info
    [[nodiscard]] float dpi() { return _dpi; }

    #ifdef DEBUG_SIMULATE_DPR
    float dpr() { return DEBUG_SIMULATE_DPR; }
    #else
    float dpr() { return (float)gl_w / (float)win_w; }
    #endif

    [[nodiscard]] bool device_vertical();
    void device_orientation(int* orientation_angle, int* orientation_index = nullptr);
    bool device_orientation_changed(std::function<void(int, int)> onChanged);
    
    // Platform detection
    [[nodiscard]] bool is_mobile();
    [[nodiscard]] bool is_desktop_native();
    [[nodiscard]] bool is_desktop_browser();
    [[nodiscard]] bool is_touch_device();
    [[nodiscard]] float touch_accuracy();

    // Scale
    [[nodiscard]] float font_scale();
    [[nodiscard]] float ui_scale_factor(float extra_mobile_mult=1.0f);

    // Window
    [[nodiscard]] float window_width_inches();
    [[nodiscard]] float window_height_inches();
    [[nodiscard]] float window_size_inches();

    // Layout helpers
    [[nodiscard]] float line_height();
    [[nodiscard]] float input_height();
    [[nodiscard]] float max_char_rows();
    [[nodiscard]] float max_char_cols();

    // File system helpers
    [[nodiscard]] const char* path(const char* virtual_path);
};

[[nodiscard]] constexpr PlatformManager* Platform()
{
    return PlatformManager::get();
}

[[nodiscard]] inline float ScaleSize(float length)
{
    return PlatformManager::get()->dpr() * length;
}

[[nodiscard]] inline double ScaleSize(double length)
{
    return static_cast<double>(PlatformManager::get()->dpr()) * length;
}

[[nodiscard]] inline int ScaleSize(int length)
{
    return static_cast<int>(PlatformManager::get()->dpr() * static_cast<float>(length));
}