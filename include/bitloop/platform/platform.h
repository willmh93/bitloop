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
    float win_dpr = 1.0f;

    bool is_mobile_device = false;


    GLuint offscreen_fbo = 0;
    GLuint offscreen_color = 0;
    GLuint offscreen_depth = 0;
    int    offscreen_w = 0;
    int    offscreen_h = 0;

public:

    static constexpr PlatformManager* instance()
    {
        return singleton;
    }

    PlatformManager(SDL_Window* _window)
    {
        singleton = this;
        window = _window;
    }

    // Required function calls
    void init();
    void update();
    void resized();

    [[nodiscard]] SDL_Window* sdl_window() const { return window; }

    [[nodiscard]] int gl_width()      const { return gl_w; }
    [[nodiscard]] int gl_height()     const { return gl_h; }
    [[nodiscard]] int fbo_width()     const { return fbo_size().x; }
    [[nodiscard]] int fbo_height()    const { return fbo_size().y; }
    [[nodiscard]] int window_width()  const { return win_w; }
    [[nodiscard]] int window_height() const { return win_h; }

    [[nodiscard]] IVec2 gl_size()     const { return {gl_w, gl_h}; }
    [[nodiscard]] IVec2 window_size() const { return {win_w, win_h}; }

    // Device Info

    [[nodiscard]] float dpr() const { return win_dpr; }

    [[nodiscard]] bool offscreen_active() const
    {
        #ifdef BL_SIMULATED_DEVICE
        return true;
        #else
        return false;
        #endif
    }

    // what the app should render to (offscreen, or direct)
    [[nodiscard]] IVec2 fbo_size() const
    {
        #ifdef BL_SIMULATED_DEVICE
        return {offscreen_w, offscreen_h};
        #else
        return {fb_w, fb_h};
        #endif
    }

    // input scaling to fbo-space
    [[nodiscard]] float input_scale_x() const { return (gl_w > 0) ? (float)fbo_size().x / (float)gl_w : 1.0f; }
    [[nodiscard]] float input_scale_y() const { return (gl_h > 0) ? (float)fbo_size().y / (float)gl_h : 1.0f; }

    void imgui_fix_offscreen_mouse_position();
    void upscale_mouse_event_to_offscreen(SDL_Event& e);
    void convert_mouse_to_touch(SDL_Event& e);

    // begin gl (for offscreen, or direct)
    void gl_begin_frame();
    void gl_end_frame();

    [[nodiscard]] bool device_vertical();
    void device_orientation(int* orientation_angle, int* orientation_index = nullptr);
    bool device_orientation_changed(std::function<void(int, int)> onChanged);
    
    // Platform detection
    [[nodiscard]] bool is_mobile() const;
    [[nodiscard]] bool is_desktop_native() const;
    [[nodiscard]] bool is_desktop_browser() const;

    // Scale
    [[nodiscard]] float font_scale() const;
    [[nodiscard]] float thumbScale(float extra_mobile_mult=1.0f) const;

    // Layout helpers
    [[nodiscard]] float line_height() const;
    [[nodiscard]] float input_height() const;
    [[nodiscard]] float max_char_rows() const;
    [[nodiscard]] float max_char_cols() const;

    // File system helpers
    [[nodiscard]] std::filesystem::path executable_dir() const;
    [[nodiscard]] std::filesystem::path resource_root() const;
    [[nodiscard]] std::string path(std::string_view virtual_path) const;


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
