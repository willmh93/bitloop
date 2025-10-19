#pragma once

/// SDL3
#include <SDL3/SDL.h>
#include <SDL3/SDL_video.h>
#include <SDL3/SDL_touch.h>

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

/// OpenGL
#ifdef __EMSCRIPTEN__
#include <GLES3/gl3.h>
#else
#include "glad/glad.h"
#endif

#include <filesystem>
#include <functional>

#include <bitloop/core/debug.h>
#include <bitloop/core/types.h>
#include "platform_macros.h"

BL_BEGIN_NS

class PlatformManager
{
    static PlatformManager *singleton;

    SDL_Window* window = nullptr;

    double css_w = 0, css_h = 0;
    int gl_w=0, gl_h = 0;
    int win_w = 0, win_h = 0;
    int fb_w = 0, fb_h = 0;

    bool is_mobile_device = false;

public:

    static constexpr PlatformManager* instance()
    {
        return singleton;
    }

    PlatformManager(SDL_Window* _window)
    {
        singleton = this;
        window = _window;
        //init();
    }

    void init();

    // Required function calls
    void update();
    void resized();

    [[nodiscard]] SDL_Window* sdl_window() { return window; }

    [[nodiscard]] int gl_width()      { return gl_w; }
    [[nodiscard]] int gl_height()     { return gl_h; }
    [[nodiscard]] int fbo_width()     { return fb_w; }
    [[nodiscard]] int fbo_height()    { return fb_h; }
    [[nodiscard]] int window_width()  { return win_w; }
    [[nodiscard]] int window_height() { return win_h; }

    [[nodiscard]] IVec2 fbo_size()    { return {fb_w, fb_h}; }

    // Device Info
    ///[[nodiscard]] float dpi() { return _dpi; }

    #ifdef DEBUG_SIMULATE_DPR
    [[nodiscard]] float dpr() { return DEBUG_SIMULATE_DPR; }
    #else
    [[nodiscard]] float dpr() { return (float)gl_w / (float)win_w; }
    #endif

    [[nodiscard]] bool device_vertical();
    void device_orientation(int* orientation_angle, int* orientation_index = nullptr);
    bool device_orientation_changed(std::function<void(int, int)> onChanged);
    
    // Platform detection
    [[nodiscard]] bool is_mobile();
    [[nodiscard]] bool is_desktop_native();
    [[nodiscard]] bool is_desktop_browser();

    // Scale
    [[nodiscard]] float font_scale();
    [[nodiscard]] float ui_scale_factor(float extra_mobile_mult=1.0f);

    // Layout helpers
    [[nodiscard]] float line_height();
    [[nodiscard]] float input_height();
    [[nodiscard]] float max_char_rows();
    [[nodiscard]] float max_char_cols();

    // File system helpers
    [[nodiscard]] std::filesystem::path executable_dir();
    [[nodiscard]] std::filesystem::path resource_root();
    [[nodiscard]] std::string path(std::string_view virtual_path);


    // URL helpers
    #ifdef __EMSCRIPTEN__
private:
    char*        _url_get_base();
    char*        _url_get_string(const char* k);
                 
public:          
    int          url_has(const char* k);
                 
    std::string  url_get_base();
    std::string  url_get_string(const char* k);
    double       url_get_number(const char* k, double fallback);
                 
    void         url_set_string(const char* key, const char* value, int use_hash = 0, int replace = 1);
    void         url_set_number(const char* key, double value, int use_hash = 0, int replace = 1);
    void         url_unset(const char* key, int use_hash = 0, int replace = 1);

    void         download_blob(const void* data, size_t size, const char* filename, const char* mime);
    void         download_blob(const bytebuf& buf, const char* filename, const char* mime);
    void         download_snapshot_webp(const bytebuf& buf, const char* filename);
    #endif
};

[[nodiscard]] constexpr PlatformManager* platform()
{
    return PlatformManager::instance();
}

[[nodiscard]] inline float scale_size(float length)
{
    return PlatformManager::instance()->dpr() * length;
}

[[nodiscard]] inline double scale_size(double length)
{
    return static_cast<double>(PlatformManager::instance()->dpr()) * length;
}

[[nodiscard]] inline int scale_size(int length)
{
    return static_cast<int>(PlatformManager::instance()->dpr() * static_cast<float>(length));
}

BL_END_NS
