#include <bitloop/core/project.h>
#include <bitloop/imguix/imgui_custom.h>

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#elif defined(__APPLE__)
#include <mach-o/dyld.h>
#include <limits.h>
#elif defined(__linux__)
#include <unistd.h>
#include <limits.h>
#endif


BL_BEGIN_NS

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

    #ifdef __EMSCRIPTEN__
    EM_ASM({
        window.addEventListener('keydown', function(event)
        {
            if (event.ctrlKey && event.key == 'v')
                event.stopImmediatePropagation();
        }, true);
    });
    #endif
}

void PlatformManager::update()
{
    SDL_GetWindowSizeInPixels(window, &gl_w, &gl_h);
    SDL_GetWindowSize(window, &win_w, &win_h);
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
    SDL_GetWindowSizeInPixels(window, &fb_w, &fb_h);
    #endif

    platform()->update();
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

bool PlatformManager::device_orientation_changed(std::function<void(int, int)> onChanged [[maybe_unused]])
{
    #ifdef __EMSCRIPTEN__
    _onOrientationChangedCB = onChanged;
    EMSCRIPTEN_RESULT res = emscripten_set_orientationchange_callback(NULL, EM_FALSE, _orientationChanged);
    return (res == EMSCRIPTEN_RESULT_SUCCESS);
    #else
    return false;
    #endif
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
    return !is_mobile();
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

float PlatformManager::font_scale()
{
    return is_mobile() ? 1.3f : 1.0f;
}


float PlatformManager::ui_scale_factor(float extra_mobile_mult)
{
    return is_mobile() ? (2.0f * extra_mobile_mult) : 1.0f;
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

std::filesystem::path PlatformManager::executable_dir()
{
    #if defined(_WIN32)
    std::wstring buf(MAX_PATH, L'\0');
    DWORD len = GetModuleFileNameW(nullptr, buf.data(), (DWORD)buf.size());
    if (len == 0)
        throw std::runtime_error("GetModuleFileNameW failed");
    buf.resize(len);
    return std::filesystem::path(buf).parent_path();

    #elif defined(__APPLE__)
    char buf[PATH_MAX];
    uint32_t size = sizeof(buf);
    if (_NSGetExecutablePath(buf, &size) != 0)
        throw std::runtime_error("_NSGetExecutablePath failed");
    return std::filesystem::canonical(buf).parent_path();

    #elif defined(__linux__)
    char buf[PATH_MAX];
    ssize_t len = readlink("/proc/self/exe", buf, sizeof(buf) - 1);
    if (len == -1)
        throw std::runtime_error("readlink failed");
    buf[len] = '\0';
    return std::filesystem::canonical(buf).parent_path();

    #else // fallback
    return std::filesystem::current_path();
    #endif
}

std::filesystem::path PlatformManager::resource_root()
{
    #ifdef __EMSCRIPTEN__
    return "/";
    #else
    return executable_dir();
    #endif
}

std::string PlatformManager::path(std::string_view virtual_path)
{
    std::filesystem::path p = resource_root();

    if (!virtual_path.empty() && virtual_path.front() == '/')
        virtual_path.remove_prefix(1); // trim leading '/'

    p /= virtual_path; // join
    p = p.lexically_normal(); // clean up
    return p.string();
}

#ifdef __EMSCRIPTEN__
char* PlatformManager::_url_get_base() {
    return (char*)MAIN_THREAD_EM_ASM_PTR({
      const u = new URL(location.href);
      let path = u.pathname;
      if (path.endsWith('index.html')) path = path.slice(0, -'index.html'.length);
      if (path.length > 1 && path.endsWith('/')) path = path.slice(0, -1);
      const s = u.origin + path;
      const n = lengthBytesUTF8(s) + 1;
      const p = _malloc(n);
      stringToUTF8(s, p, n);
      return p;
    });
}

std::string PlatformManager::url_get_base()
{
    char* str = _url_get_base();
    std::string ret = str;
    free(str);
    return ret;
}

int PlatformManager::url_has(const char* k)
{
    return MAIN_THREAD_EM_ASM_INT({
      const key = UTF8ToString($0);
      const u = new URL(location.href);
      if (u.searchParams.has(key)) return 1;
      if (u.hash && u.hash.length > 1) {
        const h = new URLSearchParams(u.hash.substring(1));
        if (h.has(key)) return 1;
      }
      return 0;
    }, k);
}

// Gets a number param; returns fallback if missing/NaN
double PlatformManager::url_get_number(const char* k, double fallback)
{
    return MAIN_THREAD_EM_ASM_DOUBLE({
      const key = UTF8ToString($0);
      const fb = $1;
      const u = new URL(location.href);
      let v = u.searchParams.get(key);
      if (v === null && u.hash && u.hash.length > 1) {
        v = new URLSearchParams(u.hash.substring(1)).get(key);
      }
      const n = Number(v);
      return Number.isFinite(n) ? n : fb;
    }, k, fallback);
}

// Gets a string param; returns newly-allocated C string (caller free()), or 0 if missing
char* PlatformManager::_url_get_string(const char* k)
{
    return (char*)MAIN_THREAD_EM_ASM_PTR({
      const key = UTF8ToString($0);
      const u = new URL(location.href);
      let v = u.searchParams.get(key);
      if (v === null && u.hash && u.hash.length > 1) {
        v = new URLSearchParams(u.hash.substring(1)).get(key);
      }
      if (v === null) return 0;
      const len = lengthBytesUTF8(v) + 1;
      const p = _malloc(len);
      stringToUTF8(v, p, len);
      return p;
    }, k);
}


std::string PlatformManager::url_get_string(const char* k)
{
    char* str = _url_get_string(k);
    std::string ret = str;
    free(str);
    return ret;
}

// Set a string param in ?query (use_hash=0) or #hash (use_hash=1).
// replace=1 uses history.replaceState (good for live updates).
void PlatformManager::url_set_string(const char* key, const char* value, int use_hash, int replace)
{
  MAIN_THREAD_EM_ASM({
    const key = UTF8ToString($0);
    const val = UTF8ToString($1);
    const useHash = $2|0;
    const doReplace = $3|0;

    const u = new URL(location.href);
    const base = u.origin + u.pathname;

    const qs = u.searchParams;                     // ?query
    const hs = new URLSearchParams(u.hash ? u.hash.slice(1) : ""); // #hash

    if (useHash) hs.set(key, val); else qs.set(key, val);

    const q = qs.toString();
    const h = hs.toString();
    const newUrl = base + (q ? "?" + q : "") + (h ? "#" + h : "");
    if (doReplace) history.replaceState(null, "", newUrl);
    else           history.pushState(null, "", newUrl);
  }, key, value, use_hash, replace);
}

void PlatformManager::url_set_number(const char* key, double value, int use_hash, int replace) {
    char buf[64];
    std::snprintf(buf, sizeof(buf), "%.17g", value);
    url_set_string(key, buf, use_hash, replace);
}

// Remove a param
void PlatformManager::url_unset(const char* key, int use_hash, int replace)
{
    MAIN_THREAD_EM_ASM({
      const key = UTF8ToString($0);
      const useHash = $1 | 0;
      const doReplace = $2 | 0;

      const u = new URL(location.href);
      const base = u.origin + u.pathname;

      const qs = u.searchParams;
      const hs = new URLSearchParams(u.hash ? u.hash.slice(1) : "");

      if (useHash) hs.delete(key); else qs.delete(key);

      const q = qs.toString();
      const h = hs.toString();
      const newUrl = base + (q ? "?" + q : "") + (h ? "#" + h : "");
      if (doReplace) history.replaceState(null, "", newUrl);
      else           history.pushState(null, "", newUrl);
    }, key, use_hash, replace);
}


// Downloading files to browser
EM_JS(void, js_download_bytes, (const uint8_t* ptr, size_t size, const char* filename, const char* mime),
{
    const name = UTF8ToString(filename);
    const type = UTF8ToString(mime);

    // Copy bytes out of WASM memory
    const bytes = HEAPU8.slice(ptr, ptr + size);
    const blob = new Blob([bytes], { type });
    const url = URL.createObjectURL(blob);

    const a = document.createElement('a');
    a.href = url;
    a.download = name;
    document.body.appendChild(a);
    a.click();
    a.remove();
    URL.revokeObjectURL(url);
});

void PlatformManager::download_blob(const void* data, size_t size, const char* filename, const char* mime)
{
    js_download_bytes(static_cast<const uint8_t*>(data), size, filename, mime);
}

void PlatformManager::download_blob(const bytebuf& buf, const char* filename, const char* mime)
{
    js_download_bytes(buf.data(), buf.size(), filename, mime);
}

void PlatformManager::download_snapshot_webp(const bytebuf& buf, const char* filename)
{
    download_blob(buf.data(), buf.size(), filename, "image/webp");
}

#endif


BL_END_NS
