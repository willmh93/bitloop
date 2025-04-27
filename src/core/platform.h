#pragma once
#include <SDL2/SDL.h>
#include <functional>
#ifdef __EMSCRIPTEN__
#include <emscripten/html5.h>
#endif

class Platform
{
    SDL_Window* window;

    
    float dpr = 1.0f;
    float dpi = 96.0f;

    double css_w, css_h;
    int gl_w, gl_h;
    int win_w, win_h;
    int fb_w, fb_h;

    void update_device_dpi();

public:

    static Platform* get()
    {
        static Platform singleton;
        return &singleton;
    }

    static void init(SDL_Window* window)
    {
        Platform::get()->window = window;
    }

    // Required function calls
    void update();
    void resized();

    int drawable_width()  { return fb_w; }
    int drawable_height() { return fb_h; }
    int window_width()    { return win_w; }
    int window_height()   { return win_h; }

    // Device Info
    float device_dpi() { return dpi; }
    bool device_vertical();
    void device_orientation(int* orientation_angle, int* orientation_index = nullptr);
    bool device_orientation_changed(std::function<void(int, int)> onChanged);
    

    // Mobile/Touch
    int is_mobile_device();
    bool is_touch_device();
    float touch_accuracy();

    // Scale
    float font_scale();
    float ui_scale_factor();

    // Window
    float window_dpr();
    float window_width_inches();
    float window_height_inches();
    float window_size_inches();

    // Layout helpers
    float max_char_rows();
    float max_char_cols();

    // File system helpers
    const char* path(const char* virtual_path);
};
