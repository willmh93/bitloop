#include "platform.h"
#include "imgui_custom.h"

PlatformManager* PlatformManager::singleton = nullptr;

#ifdef __EMSCRIPTEN__
EM_JS(int, _is_mobile_device, (), {
  // 1. Prefer UA-Client-Hints when they exist (Chromium >= 89)
  if (navigator.userAgentData && navigator.userAgentData.mobile !== undefined) {
    return navigator.userAgentData.mobile ? 1 : 0;
  }

  // 2. Pointer/hover media-queries – quick and cheap
  if (window.matchMedia('(pointer: coarse)').matches &&
      !window.matchMedia('(hover: hover)').matches) {
    return 1;
  }

  // 3. Fallback to a light regex on the UA string
  return /Android|webOS|iPhone|iPad|iPod|BlackBerry|IEMobile|Opera Mini|Windows Phone/i
           .test(navigator.userAgent) ? 1 : 0;
});
#else
int _is_mobile_device()
{
    return 0;
}
#endif

void PlatformManager::init()
{
    is_mobile_device = _is_mobile_device();
}

void PlatformManager::update()
{
    SDL_GL_GetDrawableSize(window, &gl_w, &gl_h);
    SDL_GetWindowSize(window, &win_w, &win_h);

    //_dpr = (float)gl_w / (float)win_w;
    update_device_dpi();
}

void PlatformManager::resized()
{
    #ifdef __EMSCRIPTEN__
    emscripten_get_element_css_size("#canvas", &css_w, &css_h);
    float device_dpr = emscripten_get_device_pixel_ratio();
    fb_w = css_w * device_dpr;
    fb_h = css_h * device_dpr;
    SDL_SetWindowSize(window, fb_w, fb_h);
    emscripten_set_canvas_element_size("#canvas", fb_w, fb_h);
    #else
    SDL_GL_GetDrawableSize(window, &fb_w, &fb_h);
    #endif

    Platform()->update();
}

bool PlatformManager::device_vertical()
{
    #ifdef __EMSCRIPTEN__
    EmscriptenOrientationChangeEvent orientation;
    if (emscripten_get_orientation_status(&orientation) == EMSCRIPTEN_RESULT_SUCCESS)
        return abs(orientation.orientationAngle % 180) < 90;
    #endif

    return false;
}

void PlatformManager::device_orientation(int* orientation_angle, int* orientation_index)
{
    #ifdef __EMSCRIPTEN__
    EmscriptenOrientationChangeEvent orientation;
    if (emscripten_get_orientation_status(&orientation) == EMSCRIPTEN_RESULT_SUCCESS)
    {
        if (orientation_angle) *orientation_angle = orientation.orientationAngle;
        if (orientation_index) *orientation_index = orientation.orientationIndex;
    }
    #else
    if (orientation_angle) *orientation_angle = 0;
    if (orientation_index) *orientation_index = 0;
    #endif
}

#ifdef __EMSCRIPTEN__
std::function<void(int, int)> _onOrientationChangedCB;
EM_BOOL _orientationChanged(int type, const EmscriptenOrientationChangeEvent* e, void* data)
{
    if (_onOrientationChangedCB)
        _onOrientationChangedCB(e->orientationIndex, e->orientationAngle);
    return EM_TRUE;
}
#endif

bool PlatformManager::device_orientation_changed(std::function<void(int, int)> onChanged)
{
    #ifdef __EMSCRIPTEN__
    _onOrientationChangedCB = onChanged;
    EMSCRIPTEN_RESULT res = emscripten_set_orientationchange_callback(NULL, EM_FALSE, _orientationChanged);
    return (res == EMSCRIPTEN_RESULT_SUCCESS);
    #else
    return false;
    #endif
}

void PlatformManager::update_device_dpi()
{
    #ifdef __EMSCRIPTEN__
    _dpi = EM_ASM_DOUBLE({
        return window.devicePixelRatio * 96.0;
    });
    #else
    float ddpi, hdpi, vdpi;
    if (SDL_GetDisplayDPI(0, &ddpi, &hdpi, &vdpi) == 0)
        _dpi = ddpi;
    else
        _dpi = 96.0f;
    #endif
}

bool PlatformManager::is_touch_device()
{
    return SDL_GetNumTouchDevices() > 0;
}

bool PlatformManager::is_mobile()
{
    #ifdef DEBUG_SIMULATE_MOBILE
    return true;
    #else
    return is_mobile_device;
    #endif
}

bool PlatformManager::is_desktop_native()
{
    #if defined __EMSCRIPTEN__ || defined FORCE_WEB_UI
    return false;
    #else
    return true;
    #endif
}

bool PlatformManager::is_desktop_browser()
{
    #ifdef __EMSCRIPTEN__
    return !is_mobile(); // Assumed desktop browser if mobile screen not detected
    #else
    return false; // Native desktop application
    #endif
}

float PlatformManager::touch_accuracy()
{
    // Measure size of a thumb relative to screen size
    // 0 = poor accuracy (thumb on small screen)
    // 1 = perfect accuracy (pointer)
    return 1.0f - (1.0f / window_size_inches());
}

float PlatformManager::font_scale()
{
    return is_mobile() ? 1.3f : 1.0f;
    //return 1.0f / touch_accuracy(window);
    //return dpr(window) / touch_accuracy(window);
}


float PlatformManager::ui_scale_factor(float extra_mobile_mult)
{
    return is_mobile() ? (2.0f * extra_mobile_mult) : 1.0f;
}

float PlatformManager::window_width_inches()
{
    return win_w / _dpi;
}

float PlatformManager::window_height_inches()
{
    return win_h / _dpi;
}

float PlatformManager::window_size_inches()
{
    float diag_px = std::sqrt((float)(win_w * win_w + win_h * win_h));
    return diag_px / _dpi;
}

float PlatformManager::line_height()
{
    return ImGui::GetFontSize();
}

float PlatformManager::input_height()
{
    ImGuiIO& io = ImGui::GetIO();
    ImGuiStyle& s = ImGui::GetStyle();
    return io.DisplaySize.y / ImGui::GetFontSize() + s.FramePadding.y * 2.0f;
}

float PlatformManager::max_char_rows()
{
    ImGuiIO& io = ImGui::GetIO();
    return io.DisplaySize.y / ImGui::GetFontSize();
}

float PlatformManager::max_char_cols()
{
    ImGuiIO& io = ImGui::GetIO();
    return io.DisplaySize.x / ImGui::GetFontSize();
}


const char* PlatformManager::path(const char* virtual_path)
{
    #ifdef __EMSCRIPTEN__
    return virtual_path;
    #else
    return virtual_path + 1; // Skip first '/'
    #endif
}
