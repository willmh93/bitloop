#include <bitloop/imguix/imgui_custom.h>
#include <bitloop/util/math_util.h>

#define NANOSVG_IMPLEMENTATION
#define NANOSVGRAST_IMPLEMENTATION
#include "nanosvg/nanosvg.h"
#include "nanosvg/nanosvgrast.h"
#include "stb_image.h"

bool LoadPixelsRGBA(const char* path, int* outW, int* outH, std::vector<unsigned char>& outPixels)
{
    int w, h, n;
    unsigned char* data = stbi_load(bl::platform()->path(path).c_str(), &w, &h, &n, 4);  // force RGBA
    if (!data) return false;

    outPixels.assign(data, data + (w * h * 4));
    stbi_image_free(data);
    *outW = w;
    *outH = h;
    return true;
}

GLuint CreateGLTextureRGBA8(const void* pixels, int w, int h)
{
    GLuint tex = 0;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glTexImage2D(GL_TEXTURE_2D, 0,
        GL_RGBA, // internal format
        w, h, 0,
        GL_RGBA, // format
        GL_UNSIGNED_BYTE,
        pixels);

    glBindTexture(GL_TEXTURE_2D, 0);
    return tex;
}

GLuint loadSVG(const char* path, int outputWidth, int outputHeight)
{
    // Step 1: Parse SVG
    NSVGimage* image = nsvgParseFromFile(bl::platform()->path(path).c_str(), "px", 96.0f); // 96 dpi, or 72 dpi depending on your needs
    if (!image) {
        printf("Could not open SVG image: %s\n", path);
        return 0;
    }

    // Step 2: Create a rasterizer
    NSVGrasterizer* rast = nsvgCreateRasterizer();
    if (!rast) {
        nsvgDelete(image);
        printf("Could not init rasterizer\n");
        return 0;
    }

    // Step 3: Allocate buffer for the output
    int stride = outputWidth * 4;  // 4 bytes per pixel (RGBA)
    unsigned char* img = (unsigned char*)malloc(outputWidth * outputHeight * 4);
    if (!img) {
        nsvgDeleteRasterizer(rast);
        nsvgDelete(image);
        printf("Could not allocate image buffer\n");
        return 0;
    }

    // Clear buffer to transparent
    memset(img, 0, outputWidth * outputHeight * 4);

    // Step 4: Rasterize
    float scaleX = (float)outputWidth / image->width;
    float scaleY = (float)outputHeight / image->height;
    float scale = (scaleX < scaleY) ? scaleX : scaleY;  // Keep aspect ratio

    nsvgRasterize(rast, image, 0, 0, scale, img, outputWidth, outputHeight, stride);

    // Step 5: Create OpenGL texture
    GLuint tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, outputWidth, outputHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, img);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // Cleanup
    free(img);
    nsvgDeleteRasterizer(rast);
    nsvgDelete(image);

    return tex;
}

void DestroyTexture(GLuint tex)
{
    glDeleteTextures(1, &tex);
}


std::ostream& operator<<(std::ostream& os, const ImSpline::Spline& spline)
{
    (void)spline;
    return os << "Spline";
}

std::ostream& operator<<(std::ostream& os, const ImGradient& gradient)
{
    (void)gradient;
    return os << "Gradient";
}

// f128 replacements

struct ParsedPrintf {
    bool has_width = false;
    bool width_from_arg = false;
    int  width = 0;

    bool has_precision = false;
    bool precision_from_arg = false; // true for "%.*f"
    int  precision = 0;              // valid if has_precision && !precision_from_arg

    char spec = '\0';                // conversion specifier ('f','g','e','a', etc.)
};

static const char* ParsePrintfSpec(const char* s, ParsedPrintf& out)
{
    if (!s || *s != '%') return nullptr;
    const char* p = s + 1;

    // 1) Flags
    while (*p == '-' || *p == '+' || *p == ' ' || *p == '#' || *p == '0') ++p;

    // 2) Width
    if (*p == '*') { out.has_width = true; out.width_from_arg = true; ++p; }
    else if (std::isdigit((unsigned char)*p)) {
        out.has_width = true;
        long w = 0;
        while (std::isdigit((unsigned char)*p)) {
            w = w * 10 + (*p - '0');
            if (w > INT_MAX) { w = INT_MAX; }
            ++p;
        }
        out.width = (int)w;
    }

    // 3) Precision
    if (*p == '.') {
        ++p;
        out.has_precision = true;
        if (*p == '*') { out.precision_from_arg = true; ++p; }
        else {
            long pr = 0;
            bool any = false;
            while (std::isdigit((unsigned char)*p)) {
                any = true;
                pr = pr * 10 + (*p - '0');
                if (pr > INT_MAX) { pr = INT_MAX; }
                ++p;
            }
            // printf rule: "." with no digits -> precision = 0
            out.precision = any ? (int)pr : 0;
        }
    }

    // 4) Length modifiers (skip)
    if (*p == 'h') { ++p; if (*p == 'h') ++p; }
    else if (*p == 'l') { ++p; if (*p == 'l') ++p; }
    else if (*p == 'L' || *p == 'z' || *p == 't' || *p == 'j') { ++p; }

    // 5) Conversion spec
    if (*p == '\0') return nullptr;
    out.spec = *p++;
    return p;
}

static bool FindAndParseNextSpec(const char* fmt, ParsedPrintf& out)
{
    if (!fmt) return false;
    for (const char* p = fmt; *p; ++p) {
        if (*p != '%') continue;
        if (p[1] == '%') { ++p; continue; } // "%%" escape
        const char* after = ParsePrintfSpec(p, out);
        return after != nullptr;
    }
    return false;
}

static int ImParseFormatPrecision128(const char* fmt, int default_precision, bool* from_arg = nullptr)
{
    ParsedPrintf pf;
    if (!FindAndParseNextSpec(fmt, pf)) {
        if (from_arg) *from_arg = false;
        return default_precision;
    }
    if (!pf.has_precision) {
        if (from_arg) *from_arg = false;
        return default_precision;
    }
    if (pf.precision_from_arg) {
        if (from_arg) *from_arg = true;
        return default_precision; // caller should read it from variadic args
    }
    if (from_arg) *from_arg = false;
    return pf.precision; // un-capped; up to INT_MAX
}



// ---- helpers for SliderDouble_InvLog ----------------------------------------------------

    // --- Logarithmic "toward max" helpers ---------------------------------------
template<typename TYPE, typename SIGNEDTYPE, typename FLOATTYPE>
static inline float ScaleRatioFromValue_TowardMax(ImGuiDataType, TYPE v, TYPE v_min, TYPE v_max, float epsilon)
{
    if (v_min == v_max)
        return 0.0f;

    // Keep orientation info, but operate with (v1 <= v2) internally.
    bool flipped = (v_max < v_min);
    if (flipped)
        ImSwap(v_min, v_max);

    const TYPE v_clamped = ImClamp(v, v_min, v_max);

    // u = distance-to-max, range [0 .. Umax]
    const FLOATTYPE Umax = (FLOATTYPE)(v_max - v_min);
    const FLOATTYPE u = (FLOATTYPE)v_max - (FLOATTYPE)v_clamped;

    // Avoid degenerate log bases
    const FLOATTYPE u_min_f = (FLOATTYPE)epsilon;                     // avoid log(0)
    const FLOATTYPE u_max_f = (Umax < (FLOATTYPE)epsilon) ? (FLOATTYPE)epsilon : Umax;

    float t_u;
    if (u <= u_min_f)              // exactly at max (or inside epsilon)
        t_u = 0.0f;
    else if (u >= u_max_f)         // exactly at min (or beyond)
        t_u = 1.0f;
    else
        t_u = (float)(ImLog(u / u_min_f) / ImLog(u_max_f / u_min_f));

    // Convert back: small u (near max) -> t near 1
    float t = 1.0f - t_u;

    // Restore original orientation if the user gave a reversed range
    return flipped ? (1.0f - t) : t;
}

template<typename TYPE, typename SIGNEDTYPE, typename FLOATTYPE>
static inline TYPE ScaleValueFromRatio_TowardMax(ImGuiDataType data_type, float t, TYPE v_min, TYPE v_max, float epsilon)
{
    // Clamp extremes explicitly (avoids epsilon effects at ends)
    if (t <= 0.0f || v_min == v_max)
        return v_min;
    if (t >= 1.0f)
        return v_max;

    bool flipped = (v_max < v_min);
    if (flipped)
        ImSwap(v_min, v_max);

    // t_with_flip maintains the caller's orientation
    float t_with_flip = flipped ? (1.0f - t) : t;

    // Work in u-space (distance-to-max). We want fine control near v_max,
    // so apply standard log in u, then convert back v = v_max - u.
    const FLOATTYPE Umax = (FLOATTYPE)(v_max - v_min);
    const FLOATTYPE u_min_f = (FLOATTYPE)epsilon;
    const FLOATTYPE u_max_f = (Umax < (FLOATTYPE)epsilon) ? (FLOATTYPE)epsilon : Umax;

    // t_u grows from 0 at u_min_f (near v_max) to 1 at u_max_f (near v_min)
    const float t_u = 1.0f - t_with_flip;

    // u = epsilon * pow(Umax/epsilon, t_u)
    const FLOATTYPE u = (FLOATTYPE)(u_min_f * ImPow(u_max_f / u_min_f, (FLOATTYPE)t_u));

    FLOATTYPE v_f = (FLOATTYPE)v_max - u;

    // Restore orientation
    TYPE v_out = (TYPE)v_f;
    if (flipped)
    {
        // When flipped, v_min/v_max were swapped; no extra transform needed here
        // because t_with_flip already handled orientation.
    }

    // For integer types, mirror the behavior of imgui's rounding in linear mode:
    const bool is_floating_point = (data_type == ImGuiDataType_Float) || (data_type == ImGuiDataType_Double);
    if (!is_floating_point)
    {
        // Match integer semantics: nearest integer within range.
        if (v_out < v_min) v_out = v_min;
        if (v_out > v_max) v_out = v_max;
    }
    return v_out;
}

// --- Slider behavior using the "toward max" mapping --------------------------
template<typename TYPE, typename SIGNEDTYPE, typename FLOATTYPE>
bool SliderBehaviorT_TowardMax(const ImRect& bb, ImGuiID id, ImGuiDataType data_type,
    TYPE* v, const TYPE v_min, const TYPE v_max,
    const char* format, ImGuiSliderFlags flags, ImRect* out_grab_bb)
{
    using namespace ImGui;

    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;

    const ImGuiAxis axis = (flags & ImGuiSliderFlags_Vertical) ? ImGuiAxis_Y : ImGuiAxis_X;
    const bool is_floating_point = (data_type == ImGuiDataType_Float) || (data_type == ImGuiDataType_Double);
    const float v_range_f = (float)(v_min < v_max ? v_max - v_min : v_min - v_max);

    // Geometry
    const float grab_padding = 2.0f;
    const float slider_sz = (bb.Max[axis] - bb.Min[axis]) - grab_padding * 2.0f;
    float grab_sz = style.GrabMinSize;
    if (!is_floating_point && v_range_f >= 0.0f)
        grab_sz = ImMax(slider_sz / (v_range_f + 1), style.GrabMinSize);
    grab_sz = ImMin(grab_sz, slider_sz);
    const float slider_usable_sz = slider_sz - grab_sz;
    const float slider_usable_pos_min = bb.Min[axis] + grab_padding + grab_sz * 0.5f;
    const float slider_usable_pos_max = bb.Max[axis] - grab_padding - grab_sz * 0.5f;

    // Epsilon picked from the display precision (like imgui's log)
    const int decimal_precision = is_floating_point ? ImParseFormatPrecision128(format, 3) : 1;
    const float epsilon = ImPow(0.1f, (float)decimal_precision);

    bool value_changed = false;

    if (g.ActiveId == id)
    {
        bool set_new_value = false;
        float clicked_t = 0.0f;

        if (g.ActiveIdSource == ImGuiInputSource_Mouse)
        {
            if (!g.IO.MouseDown[0])
            {
                ClearActiveID();
            }
            else
            {
                const float mouse_abs_pos = g.IO.MousePos[axis];
                if (g.ActiveIdIsJustActivated)
                {
                    float grab_t = ScaleRatioFromValue_TowardMax<TYPE, SIGNEDTYPE, FLOATTYPE>(data_type, *v, v_min, v_max, epsilon);
                    if (axis == ImGuiAxis_Y)
                        grab_t = 1.0f - grab_t;
                    const float grab_pos = ImLerp(slider_usable_pos_min, slider_usable_pos_max, grab_t);
                    const bool clicked_around_grab = (mouse_abs_pos >= grab_pos - grab_sz * 0.5f - 1.0f) && (mouse_abs_pos <= grab_pos + grab_sz * 0.5f + 1.0f);
                    g.SliderGrabClickOffset = (clicked_around_grab && is_floating_point) ? mouse_abs_pos - grab_pos : 0.0f;
                }
                if (slider_usable_sz > 0.0f)
                    clicked_t = ImSaturate((mouse_abs_pos - g.SliderGrabClickOffset - slider_usable_pos_min) / slider_usable_sz);
                if (axis == ImGuiAxis_Y)
                    clicked_t = 1.0f - clicked_t;
                set_new_value = true;
            }
        }
        else if (g.ActiveIdSource == ImGuiInputSource_Keyboard || g.ActiveIdSource == ImGuiInputSource_Gamepad)
        {
            if (g.ActiveIdIsJustActivated)
            {
                g.SliderCurrentAccum = 0.0f;
                g.SliderCurrentAccumDirty = false;
            }

            float input_delta = (axis == ImGuiAxis_X) ? GetNavTweakPressedAmount(axis) : -GetNavTweakPressedAmount(axis);
            if (input_delta != 0.0f)
            {
                const bool tweak_slow = IsKeyDown((g.NavInputSource == ImGuiInputSource_Gamepad) ? ImGuiKey_NavGamepadTweakSlow : ImGuiKey_NavKeyboardTweakSlow);
                const bool tweak_fast = IsKeyDown((g.NavInputSource == ImGuiInputSource_Gamepad) ? ImGuiKey_NavGamepadTweakFast : ImGuiKey_NavKeyboardTweakFast);
                if (decimal_precision > 0)
                {
                    input_delta /= 100.0f;
                    if (tweak_slow)
                        input_delta /= 10.0f;
                }
                else
                {
                    if ((v_range_f >= -100.0f && v_range_f <= 100.0f && v_range_f != 0.0f) || tweak_slow)
                        input_delta = ((input_delta < 0.0f) ? -1.0f : +1.0f) / v_range_f;
                    else
                        input_delta /= 100.0f;
                }
                if (tweak_fast)
                    input_delta *= 10.0f;

                g.SliderCurrentAccum += input_delta;
                g.SliderCurrentAccumDirty = true;
            }

            if (g.NavActivatePressedId == id && !g.ActiveIdIsJustActivated)
            {
                ClearActiveID();
            }
            else if (g.SliderCurrentAccumDirty)
            {
                float old_t = ScaleRatioFromValue_TowardMax<TYPE, SIGNEDTYPE, FLOATTYPE>(data_type, *v, v_min, v_max, epsilon);
                float new_t = ImSaturate(old_t + g.SliderCurrentAccum);

                // Convert to value, then back to t to see how much we actually moved
                TYPE v_new = ScaleValueFromRatio_TowardMax<TYPE, SIGNEDTYPE, FLOATTYPE>(data_type, new_t, v_min, v_max, epsilon);

                if (is_floating_point && !(flags & ImGuiSliderFlags_NoRoundToFormat))
                    v_new = RoundScalarWithFormatT<TYPE>(format, data_type, v_new);

                float realized_t = ScaleRatioFromValue_TowardMax<TYPE, SIGNEDTYPE, FLOATTYPE>(data_type, v_new, v_min, v_max, epsilon);

                float delta = g.SliderCurrentAccum;
                g.SliderCurrentAccum -= (delta > 0) ? ImMin(realized_t - old_t, delta)
                    : ImMax(realized_t - old_t, delta);
                g.SliderCurrentAccumDirty = false;

                clicked_t = new_t;
                set_new_value = true;
            }
        }

        if (set_new_value)
            if ((g.LastItemData.ItemFlags & ImGuiItemFlags_ReadOnly) || (flags & ImGuiSliderFlags_ReadOnly))
                set_new_value = false;

        if (set_new_value)
        {
            TYPE v_new = ScaleValueFromRatio_TowardMax<TYPE, SIGNEDTYPE, FLOATTYPE>(data_type, clicked_t, v_min, v_max, epsilon);

            if (is_floating_point && !(flags & ImGuiSliderFlags_NoRoundToFormat))
                v_new = RoundScalarWithFormatT<TYPE>(format, data_type, v_new);

            if (*v != v_new)
            {
                *v = v_new;
                value_changed = true;
            }
        }
    }

    // Grab rect
    if (slider_sz < 1.0f)
    {
        *out_grab_bb = ImRect(bb.Min, bb.Min);
    }
    else
    {
        float grab_t = ScaleRatioFromValue_TowardMax<TYPE, SIGNEDTYPE, FLOATTYPE>(data_type, *v, v_min, v_max, epsilon);
        if (axis == ImGuiAxis_Y)
            grab_t = 1.0f - grab_t;
        const float grab_pos = ImLerp(slider_usable_pos_min, slider_usable_pos_max, grab_t);
        if (axis == ImGuiAxis_X)
            *out_grab_bb = ImRect(grab_pos - grab_sz * 0.5f, bb.Min.y + grab_padding, grab_pos + grab_sz * 0.5f, bb.Max.y - grab_padding);
        else
            *out_grab_bb = ImRect(bb.Min.x + grab_padding, grab_pos - grab_sz * 0.5f, bb.Max.x - grab_padding, grab_pos + grab_sz * 0.5f);
    }

    return value_changed;
}

bool SliderBehavior_TowardMax(const ImRect& bb, ImGuiID id, ImGuiDataType data_type, void* p_v, const void* p_min, const void* p_max, const char* format, ImGuiSliderFlags flags, ImRect* out_grab_bb)
{
    // Read imgui.cpp "API BREAKING CHANGES" section for 1.78 if you hit this assert.
    IM_ASSERT((flags == 1 || (flags & ImGuiSliderFlags_InvalidMask_) == 0) && "Invalid ImGuiSliderFlags flags! Has the legacy 'float power' argument been mistakenly cast to flags? Call function with ImGuiSliderFlags_Logarithmic flags instead.");
    IM_ASSERT((flags & ImGuiSliderFlags_WrapAround) == 0); // Not supported by SliderXXX(), only by DragXXX()

    switch (data_type)
    {
    case ImGuiDataType_S8: { ImS32 v32 = (ImS32) * (ImS8*)p_v;  bool r = SliderBehaviorT_TowardMax<ImS32, ImS32, float>(bb, id, ImGuiDataType_S32, &v32, *(const ImS8*)p_min, *(const ImS8*)p_max, format, flags, out_grab_bb); if (r) *(ImS8*)p_v = (ImS8)v32;  return r; }
    case ImGuiDataType_U8: { ImU32 v32 = (ImU32) * (ImU8*)p_v;  bool r = SliderBehaviorT_TowardMax<ImU32, ImS32, float>(bb, id, ImGuiDataType_U32, &v32, *(const ImU8*)p_min, *(const ImU8*)p_max, format, flags, out_grab_bb); if (r) *(ImU8*)p_v = (ImU8)v32;  return r; }
    case ImGuiDataType_S16: { ImS32 v32 = (ImS32) * (ImS16*)p_v; bool r = SliderBehaviorT_TowardMax<ImS32, ImS32, float>(bb, id, ImGuiDataType_S32, &v32, *(const ImS16*)p_min, *(const ImS16*)p_max, format, flags, out_grab_bb); if (r) *(ImS16*)p_v = (ImS16)v32; return r; }
    case ImGuiDataType_U16: { ImU32 v32 = (ImU32) * (ImU16*)p_v; bool r = SliderBehaviorT_TowardMax<ImU32, ImS32, float>(bb, id, ImGuiDataType_U32, &v32, *(const ImU16*)p_min, *(const ImU16*)p_max, format, flags, out_grab_bb); if (r) *(ImU16*)p_v = (ImU16)v32; return r; }
    case ImGuiDataType_S32:
        //IM_ASSERT(*(const ImS32*)p_min >= IM_S32_MIN / 2 && *(const ImS32*)p_max <= IM_S32_MAX / 2);
        return SliderBehaviorT_TowardMax<ImS32, ImS32, float >(bb, id, data_type, (ImS32*)p_v, *(const ImS32*)p_min, *(const ImS32*)p_max, format, flags, out_grab_bb);
    case ImGuiDataType_U32:
        //IM_ASSERT(*(const ImU32*)p_max <= IM_U32_MAX / 2);
        return SliderBehaviorT_TowardMax<ImU32, ImS32, float >(bb, id, data_type, (ImU32*)p_v, *(const ImU32*)p_min, *(const ImU32*)p_max, format, flags, out_grab_bb);
    case ImGuiDataType_S64:
        //IM_ASSERT(*(const ImS64*)p_min >= IM_S64_MIN / 2 && *(const ImS64*)p_max <= IM_S64_MAX / 2);
        return SliderBehaviorT_TowardMax<ImS64, ImS64, double>(bb, id, data_type, (ImS64*)p_v, *(const ImS64*)p_min, *(const ImS64*)p_max, format, flags, out_grab_bb);
    case ImGuiDataType_U64:
        //IM_ASSERT(*(const ImU64*)p_max <= IM_U64_MAX / 2);
        return SliderBehaviorT_TowardMax<ImU64, ImS64, double>(bb, id, data_type, (ImU64*)p_v, *(const ImU64*)p_min, *(const ImU64*)p_max, format, flags, out_grab_bb);
    case ImGuiDataType_Float:
        //IM_ASSERT(*(const float*)p_min >= -FLT_MAX / 2.0f && *(const float*)p_max <= FLT_MAX / 2.0f);
        return SliderBehaviorT_TowardMax<float, float, float >(bb, id, data_type, (float*)p_v, *(const float*)p_min, *(const float*)p_max, format, flags, out_grab_bb);
    case ImGuiDataType_Double:
        //IM_ASSERT(*(const double*)p_min >= -DBL_MAX / 2.0f && *(const double*)p_max <= DBL_MAX / 2.0f);
        return SliderBehaviorT_TowardMax<double, double, double>(bb, id, data_type, (double*)p_v, *(const double*)p_min, *(const double*)p_max, format, flags, out_grab_bb);
    case ImGuiDataType_COUNT: break;
    }
    IM_ASSERT(0);
    return false;
}

bool SliderScalar_TowardMax(const char* label, ImGuiDataType data_type, void* p_data, const void* p_min, const void* p_max, const char* format, ImGuiSliderFlags flags)
{
    using namespace ImGui;

    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return false;

    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;
    const ImGuiID id = window->GetID(label);
    const float w = CalcItemWidth();

    const ImVec2 label_size = CalcTextSize(label, NULL, true);
    const ImRect frame_bb(window->DC.CursorPos, window->DC.CursorPos + ImVec2(w, label_size.y + style.FramePadding.y * 2.0f));
    const ImRect total_bb(frame_bb.Min, frame_bb.Max + ImVec2(label_size.x > 0.0f ? style.ItemInnerSpacing.x + label_size.x : 0.0f, 0.0f));

    const bool temp_input_allowed = (flags & ImGuiSliderFlags_NoInput) == 0;
    ItemSize(total_bb, style.FramePadding.y);
    if (!ItemAdd(total_bb, id, &frame_bb, temp_input_allowed ? ImGuiItemFlags_Inputable : 0))
        return false;

    // Default format string when passing NULL
    if (format == NULL)
        format = DataTypeGetInfo(data_type)->PrintFmt;

    const bool hovered = ItemHoverable(frame_bb, id, g.LastItemData.ItemFlags);
    bool temp_input_is_active = temp_input_allowed && TempInputIsActive(id);
    if (!temp_input_is_active)
    {
        // Tabbing or CTRL+click on Slider turns it into an input box
        const bool clicked = hovered && IsMouseClicked(0, ImGuiInputFlags_None, id);
        const bool make_active = (clicked || g.NavActivateId == id);
        if (make_active && clicked)
            SetKeyOwner(ImGuiKey_MouseLeft, id);
        if (make_active && temp_input_allowed)
            if ((clicked && g.IO.KeyCtrl) || (g.NavActivateId == id && (g.NavActivateFlags & ImGuiActivateFlags_PreferInput)))
                temp_input_is_active = true;

        // Store initial value (not used by main lib but available as a convenience but some mods e.g. to revert)
        if (make_active)
            memcpy(&g.ActiveIdValueOnActivation, p_data, DataTypeGetInfo(data_type)->Size);

        if (make_active && !temp_input_is_active)
        {
            SetActiveID(id, window);
            SetFocusID(id, window);
            FocusWindow(window);
            g.ActiveIdUsingNavDirMask |= (1 << ImGuiDir_Left) | (1 << ImGuiDir_Right);
        }
    }

    if (temp_input_is_active)
    {
        // Only clamp CTRL+Click input when ImGuiSliderFlags_ClampOnInput is set (generally via ImGuiSliderFlags_AlwaysClamp)
        const bool clamp_enabled = (flags & ImGuiSliderFlags_ClampOnInput) != 0;
        return TempInputScalar(frame_bb, id, label, data_type, p_data, format, clamp_enabled ? p_min : NULL, clamp_enabled ? p_max : NULL);
    }

    // Draw frame
    const ImU32 frame_col = GetColorU32(g.ActiveId == id ? ImGuiCol_FrameBgActive : hovered ? ImGuiCol_FrameBgHovered : ImGuiCol_FrameBg);
    RenderNavCursor(frame_bb, id);
    RenderFrame(frame_bb.Min, frame_bb.Max, frame_col, true, g.Style.FrameRounding);

    // Slider behavior
    ImRect grab_bb;
    const bool value_changed = SliderBehavior_TowardMax(frame_bb, id, data_type, p_data, p_min, p_max, format, flags, &grab_bb);
    if (value_changed)
        MarkItemEdited(id);

    // Render grab
    if (grab_bb.Max.x > grab_bb.Min.x)
        window->DrawList->AddRectFilled(grab_bb.Min, grab_bb.Max, GetColorU32(g.ActiveId == id ? ImGuiCol_SliderGrabActive : ImGuiCol_SliderGrab), style.GrabRounding);

    // Display value using user-provided display format so user can add prefix/suffix/decorations to the value.
    char value_buf[64];
    const char* value_buf_end = value_buf + DataTypeFormatString(value_buf, IM_ARRAYSIZE(value_buf), data_type, p_data, format);
    if (g.LogEnabled)
        LogSetNextTextDecoration("{", "}");
    RenderTextClipped(frame_bb.Min, frame_bb.Max, value_buf, value_buf_end, NULL, ImVec2(0.5f, 0.5f));

    if (label_size.x > 0.0f)
        RenderText(ImVec2(frame_bb.Max.x + style.ItemInnerSpacing.x, frame_bb.Min.y + style.FramePadding.y), label);

    IMGUI_TEST_ENGINE_ITEM_INFO(id, label, g.LastItemData.StatusFlags | (temp_input_allowed ? ImGuiItemStatusFlags_Inputable : 0));
    return value_changed;
}


/*

// ---- helpers for f128 ----------------------------------------------------

static inline f128 ImAbs128(f128 x) { return x < 0 ? -x : x; }
static inline f128 ImLog128(f128 x) { using std::log; return log(x); }
static inline f128 ImPow128(f128 a, f128 b) { using std::pow; return pow(a, b); }

// Round to N decimals based on printf-like format, similar to ImGui RoundScalarWithFormatT
static inline f128 RoundScalarWithFormat128(const char* format, f128 v)
{
    int prec = ImParseFormatPrecision(format, 3);
    if (prec <= 0) return v;
    // pow10 using f128
    f128 p = 1;
    for (int i = 0; i < prec; ++i) p = p * 10;
    // round(v * p) / p
    double t = (double)(v * p);            // use double rounding for speed/stability of UI step
    t = (t >= 0.0) ? floor(t + 0.5) : ceil(t - 0.5);
    return f128(t) / p;
}

// Clamp for f128
static inline f128 ImClamp128(f128 v, f128 mn, f128 mx)
{
    if (v < mn) return mn;
    if (v > mx) return mx;
    return v;
}

// ---- parametric conversions (same as ImGui but specialized) ----------------

// value -> [0,1] ratio on slider
static float ScaleRatioFromValue_flt128(f128 v, f128 v_min, f128 v_max, bool is_logarithmic, float logarithmic_zero_epsilon, float zero_deadzone_halfsize)
{
    if (v_min == v_max) return 0.0f;

    f128 v_clamped = (v_min < v_max) ? ImClamp128(v, v_min, v_max) : ImClamp128(v, v_max, v_min);

    if (is_logarithmic)
    {
        bool flipped = v_max < v_min;
        if (flipped) { f128 tmp = v_min; v_min = v_max; v_max = tmp; }

        f128 eps = f128(logarithmic_zero_epsilon);
        f128 v_min_f = (ImAbs128(v_min) < eps) ? (v_min < 0 ? -eps : eps) : v_min;
        f128 v_max_f = (ImAbs128(v_max) < eps) ? (v_max < 0 ? -eps : eps) : v_max;

        if (v_min == 0 && v_max < 0) v_min_f = -eps;
        else if (v_max == 0 && v_min < 0) v_max_f = -eps;

        float result;
        if (v_clamped <= v_min_f)
            result = 0.0f;
        else if (v_clamped >= v_max_f)
            result = 1.0f;
        else if ((v_min * v_max) < 0) // crosses zero
        {
            float zero_point_center = (-(float)v_min) / ((float)v_max - (float)v_min);
            float zero_point_snap_L = zero_point_center - zero_deadzone_halfsize;
            float zero_point_snap_R = zero_point_center + zero_deadzone_halfsize;
            if (v == 0)
                result = zero_point_center;
            else if (v < 0)
                result = (1.0f - (float)(ImLog128(-(v_clamped) / eps) / ImLog128(-v_min_f / eps))) * zero_point_snap_L;
            else
                result = zero_point_snap_R + ((float)(ImLog128(v_clamped / eps) / ImLog128(v_max_f / eps)) * (1.0f - zero_point_snap_R));
        }
        else if ((v_min < 0) || (v_max < 0)) // entirely negative
            result = 1.0f - (float)(ImLog128(-(v_clamped) / -v_max_f) / ImLog128(-v_min_f / -v_max_f));
        else
            result = (float)(ImLog128(v_clamped / v_min_f) / ImLog128(v_max_f / v_min_f));

        return flipped ? (1.0f - result) : result;
    }
    else
    {
        // linear
        return (float)((double)(v_clamped - v_min) / (double)(v_max - v_min));
    }
}

// [0,1] ratio -> value
static f128 ScaleValueFromRatio_flt128(float t, f128 v_min, f128 v_max, bool is_logarithmic, float logarithmic_zero_epsilon, float zero_deadzone_halfsize)
{
    if (t <= 0.0f || v_min == v_max) return v_min;
    if (t >= 1.0f) return v_max;

    if (is_logarithmic)
    {
        f128 eps = f128(logarithmic_zero_epsilon);
        f128 v_min_f = (ImAbs128(v_min) < eps) ? (v_min < 0 ? -eps : eps) : v_min;
        f128 v_max_f = (ImAbs128(v_max) < eps) ? (v_max < 0 ? -eps : eps) : v_max;

        bool flipped = v_max < v_min;
        if (flipped) { f128 tmp = v_min_f; v_min_f = v_max_f; v_max_f = tmp; }
        float tf = flipped ? (1.0f - t) : t;

        if ((v_max == 0) && (v_min < 0))
            v_max_f = -eps;

        if ((v_min * v_max) < 0)
        {
            float zero_point_center = (-(float)ImMin((double)v_min, (double)v_max)) / ImAbs((float)v_max - (float)v_min);
            float zero_point_snap_L = zero_point_center - zero_deadzone_halfsize;
            float zero_point_snap_R = zero_point_center + zero_deadzone_halfsize;

            if (tf >= zero_point_snap_L && tf <= zero_deadzone_halfsize + zero_point_center)
                return 0;

            if (tf < zero_point_center)
                return -(eps * ImPow128(-v_min_f / eps, f128(1.0f - (tf / zero_point_snap_L))));
            else
                return (eps * ImPow128(v_max_f / eps, f128((tf - zero_point_snap_R) / (1.0f - zero_point_snap_R))));
        }
        else if ((v_min < 0) || (v_max < 0))
        {
            return -(-v_max_f * ImPow128(-v_min_f / -v_max_f, f128(1.0f - tf)));
        }
        else
        {
            return (v_min_f * ImPow128(v_max_f / v_min_f, f128(tf)));
        }
    }
    else
    {
        // linear
        return bl::Math::lerp(v_min, v_max, f128(t));
    }
}

// ---- behavior specialized to f128 ----------------------------------------

static bool SliderBehaviorFloat128(const ImRect& bb, ImGuiID id, f128* v, f128 v_min, f128 v_max, const char* format, ImGuiSliderFlags flags, ImRect* out_grab_bb)
{
    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;

    const ImGuiAxis axis = (flags & ImGuiSliderFlags_Vertical) ? ImGuiAxis_Y : ImGuiAxis_X;
    const bool is_logarithmic = (flags & ImGuiSliderFlags_Logarithmic) != 0;

    // geometry
    const float grab_padding = 2.0f;
    const float slider_sz = (bb.Max[axis] - bb.Min[axis]) - grab_padding * 2.0f;
    float grab_sz = style.GrabMinSize;
    grab_sz = ImMin(grab_sz, slider_sz);
    const float slider_usable_sz = slider_sz - grab_sz;
    const float slider_usable_pos_min = bb.Min[axis] + grab_padding + grab_sz * 0.5f;
    const float slider_usable_pos_max = bb.Max[axis] - grab_padding - grab_sz * 0.5f;

    float logarithmic_zero_epsilon = 0.0f;
    float zero_deadzone_halfsize = 0.0f;
    if (is_logarithmic)
    {
        int decimal_precision = ImParseFormatPrecision(format, 3);
        logarithmic_zero_epsilon = ImPow(0.1f, (float)decimal_precision);
        zero_deadzone_halfsize = (style.LogSliderDeadzone * 0.5f) / ImMax(slider_usable_sz, 1.0f);
    }

    bool value_changed = false;

    if (g.ActiveId == id)
    {
        bool set_new_value = false;
        float clicked_t = 0.0f;

        if (g.ActiveIdSource == ImGuiInputSource_Mouse)
        {
            if (!g.IO.MouseDown[0])
            {
                ImGui::ClearActiveID();
            }
            else
            {
                const float mouse_abs_pos = g.IO.MousePos[axis];
                if (g.ActiveIdIsJustActivated)
                {
                    float grab_t = ScaleRatioFromValue_flt128(*v, v_min, v_max, is_logarithmic, logarithmic_zero_epsilon, zero_deadzone_halfsize);
                    if (axis == ImGuiAxis_Y) grab_t = 1.0f - grab_t;
                    const float grab_pos = ImLerp(slider_usable_pos_min, slider_usable_pos_max, grab_t);
                    const bool clicked_around_grab = (mouse_abs_pos >= grab_pos - grab_sz * 0.5f - 1.0f) && (mouse_abs_pos <= grab_pos + grab_sz * 0.5f + 1.0f);
                    g.SliderGrabClickOffset = clicked_around_grab ? mouse_abs_pos - grab_pos : 0.0f;
                }
                if (slider_usable_sz > 0.0f)
                    clicked_t = ImSaturate((g.IO.MousePos[axis] - g.SliderGrabClickOffset - slider_usable_pos_min) / slider_usable_sz);
                if (axis == ImGuiAxis_Y)
                    clicked_t = 1.0f - clicked_t;
                set_new_value = true;
            }
        }
        else if (g.ActiveIdSource == ImGuiInputSource_Keyboard || g.ActiveIdSource == ImGuiInputSource_Gamepad)
        {
            if (g.ActiveIdIsJustActivated)
            {
                g.SliderCurrentAccum = 0.0f;
                g.SliderCurrentAccumDirty = false;
            }

            float input_delta = (axis == ImGuiAxis_X) ? ImGui::GetNavTweakPressedAmount(axis) : -ImGui::GetNavTweakPressedAmount(axis);
            if (input_delta != 0.0f)
            {
                const bool tweak_slow = ImGui::IsKeyDown((g.NavInputSource == ImGuiInputSource_Gamepad) ? ImGuiKey_NavGamepadTweakSlow : ImGuiKey_NavKeyboardTweakSlow);
                const bool tweak_fast = ImGui::IsKeyDown((g.NavInputSource == ImGuiInputSource_Gamepad) ? ImGuiKey_NavGamepadTweakFast : ImGuiKey_NavKeyboardTweakFast);
                int decimal_precision = ImParseFormatPrecision(format, 3);
                if (decimal_precision > 0)
                {
                    input_delta /= 100.0f;
                    if (tweak_slow) input_delta /= 10.0f;
                }
                else
                {
                    // Without a known integer step for f128, use % of range as above
                    input_delta /= 100.0f;
                }
                if (tweak_fast) input_delta *= 10.0f;

                g.SliderCurrentAccum += input_delta;
                g.SliderCurrentAccumDirty = true;
            }

            float delta = g.SliderCurrentAccum;
            if (g.NavActivatePressedId == id && !g.ActiveIdIsJustActivated)
            {
                ImGui::ClearActiveID();
            }
            else if (g.SliderCurrentAccumDirty)
            {
                clicked_t = ScaleRatioFromValue_flt128(*v, v_min, v_max, is_logarithmic, logarithmic_zero_epsilon, zero_deadzone_halfsize);

                if ((clicked_t >= 1.0f && delta > 0.0f) || (clicked_t <= 0.0f && delta < 0.0f))
                {
                    set_new_value = false;
                    g.SliderCurrentAccum = 0.0f;
                }
                else
                {
                    set_new_value = true;
                    float old_clicked_t = clicked_t;
                    clicked_t = ImSaturate(clicked_t + delta);

                    f128 v_new = ScaleValueFromRatio_flt128(clicked_t, v_min, v_max, is_logarithmic, logarithmic_zero_epsilon, zero_deadzone_halfsize);
                    if ((flags & ImGuiSliderFlags_NoRoundToFormat) == 0)
                        v_new = RoundScalarWithFormat128(format, v_new);
                    float new_clicked_t = ScaleRatioFromValue_flt128(v_new, v_min, v_max, is_logarithmic, logarithmic_zero_epsilon, zero_deadzone_halfsize);

                    if (delta > 0)
                        g.SliderCurrentAccum -= ImMin(new_clicked_t - old_clicked_t, delta);
                    else
                        g.SliderCurrentAccum -= ImMax(new_clicked_t - old_clicked_t, delta);
                }
                g.SliderCurrentAccumDirty = false;
            }
        }

        if (set_new_value)
        {
            if ((g.LastItemData.ItemFlags & ImGuiItemFlags_ReadOnly) || (flags & ImGuiSliderFlags_ReadOnly))
                set_new_value = false;
        }

        if (set_new_value)
        {
            f128 v_new = ScaleValueFromRatio_flt128(clicked_t, v_min, v_max, is_logarithmic, logarithmic_zero_epsilon, zero_deadzone_halfsize);
            if ((flags & ImGuiSliderFlags_NoRoundToFormat) == 0)
                v_new = RoundScalarWithFormat128(format, v_new);

            if (*v != v_new)
            {
                *v = v_new;
                value_changed = true;
            }
        }
    }

    // grab rect
    if (slider_sz < 1.0f)
    {
        *out_grab_bb = ImRect(bb.Min, bb.Min);
    }
    else
    {
        float grab_t = ScaleRatioFromValue_flt128(*v, v_min, v_max, is_logarithmic, logarithmic_zero_epsilon, zero_deadzone_halfsize);
        if (axis == ImGuiAxis_Y) grab_t = 1.0f - grab_t;
        const float grab_pos = ImLerp(slider_usable_pos_min, slider_usable_pos_max, grab_t);
        if (axis == ImGuiAxis_X)
            *out_grab_bb = ImRect(grab_pos - grab_sz * 0.5f, bb.Min.y + grab_padding, grab_pos + grab_sz * 0.5f, bb.Max.y - grab_padding);
        else
            *out_grab_bb = ImRect(bb.Min.x + grab_padding, grab_pos - grab_sz * 0.5f, bb.Max.x - grab_padding, grab_pos + grab_sz * 0.5f);
    }

    return value_changed;
}

*/

using bl::f128;
using bl::f128;

static const float DRAG_MOUSE_THRESHOLD_FACTOR = 0.50f;

static inline f128 ImAbs128(f128 x) { return x < 0 ? -x : x; }
static inline f128 ImLog128(f128 x) { using std::log; return log(x); }
static inline f128 ImPow128(f128 a, f128 b) { using std::pow; return pow(a, b); }
static inline f128 ImClamp128(f128 v, f128 mn, f128 mx)
{
    if (v < mn) return mn;
    if (v > mx) return mx;
    return v;
}

// Round to decimal precision parsed from printf-like format (precision only)
static inline f128 RoundScalarWithFormat128(const char* fmt, f128 v) {
    if (!fmt) return v;
    const char* s = ImParseFormatFindStart(fmt);
    if (s[0] != '%' || s[1] == '%') return v;
    int prec = ImParseFormatPrecision128(s, 3);
    if (prec <= 0) return v;
    return round_to_decimals(v, prec);
}

// value -> [0,1] ratio on drag/slider (log-support)
static float ScaleRatioFromValue_flt128(f128 v, f128 v_min, f128 v_max, bool is_logarithmic, float logarithmic_zero_epsilon)
{
    if (v_min == v_max) return 0.0f;

    f128 v_clamped = (v_min < v_max) ? ImClamp128(v, v_min, v_max) : ImClamp128(v, v_max, v_min);

    if (is_logarithmic)
    {
        bool flipped = v_max < v_min;
        if (flipped) { f128 t = v_min; v_min = v_max; v_max = t; }

        f128 eps = f128(logarithmic_zero_epsilon);
        f128 vminf = (ImAbs128(v_min) < eps) ? (v_min < 0 ? -eps : eps) : v_min;
        f128 vmaxf = (ImAbs128(v_max) < eps) ? (v_max < 0 ? -eps : eps) : v_max;

        if (v_min == 0 && v_max < 0) vminf = -eps;
        else if (v_max == 0 && v_min < 0) vmaxf = -eps;

        float res;
        if (v_clamped <= vminf) res = 0.0f;
        else if (v_clamped >= vmaxf) res = 1.0f;
        else if ((v_min * v_max) < 0)
        {
            // Crosses zero: split portions (deadzone not used for drags)
            float zc = (-(float)v_min) / ((float)v_max - (float)v_min);
            if (v == 0) res = zc;
            else if (v < 0)
                res = (1.0f - (float)(ImLog128(-(v_clamped) / eps) / ImLog128(-vminf / eps))) * zc;
            else
                res = zc + ((float)(ImLog128(v_clamped / eps) / ImLog128(vmaxf / eps)) * (1.0f - zc));
        }
        else if ((v_min < 0) || (v_max < 0))
            res = 1.0f - (float)(ImLog128(-(v_clamped) / -vmaxf) / ImLog128(-vminf / -vmaxf));
        else
            res = (float)(ImLog128(v_clamped / vminf) / ImLog128(vmaxf / vminf));

        return flipped ? (1.0f - res) : res;
    }
    // linear
    return (float)((double)(v_clamped - v_min) / (double)(v_max - v_min));
}

// [0,1] ratio -> value (log-support)
static f128 ScaleValueFromRatio_flt128(float t, f128 v_min, f128 v_max, bool is_logarithmic, float logarithmic_zero_epsilon)
{
    if (t <= 0.0f || v_min == v_max) return v_min;
    if (t >= 1.0f) return v_max;

    if (is_logarithmic)
    {
        f128 eps = f128(logarithmic_zero_epsilon);
        f128 vminf = (ImAbs128(v_min) < eps) ? (v_min < 0 ? -eps : eps) : v_min;
        f128 vmaxf = (ImAbs128(v_max) < eps) ? (v_max < 0 ? -eps : eps) : v_max;

        bool flipped = v_max < v_min;
        if (flipped) { f128 t2 = vminf; vminf = vmaxf; vmaxf = t2; }
        float tf = flipped ? (1.0f - t) : t;

        if ((v_max == 0) && (v_min < 0))
            vmaxf = -eps;

        if ((v_min * v_max) < 0)
        {
            float zc = (-(float)ImMin((double)v_min, (double)v_max)) / ImAbs((float)v_max - (float)v_min);
            if (tf < zc)
                return -(eps * ImPow128(-vminf / eps, 1.0f - (tf / zc)));
            else if (tf > zc)
                return (eps * ImPow128(vmaxf / eps, (tf - zc) / (1.0f - zc)));
            else
                return 0;
        }
        else if ((v_min < 0) || (v_max < 0))
            return -(-vmaxf * ImPow128(-vminf / -vmaxf, 1.0f - tf));
        else
            return (vminf * ImPow128(vmaxf / vminf, tf));
    }

    // linear
    return bl::Math::lerp(v_min, v_max, f128(t));
}

static int DataTypeCompareFlt128(const f128* lhs, const f128* rhs)
{
    if (*lhs < *rhs) return -1;
    if (*lhs > *rhs) return +1;
    return 0;
}

static bool TempInputFlt128(const ImRect& bb, ImGuiID id, const char* label, f128* p_data, const char* format, const f128* p_clamp_min, const f128* p_clamp_max)
{
    using namespace ImGui;

    // FIXME: May need to clarify display behavior if format doesn't contain %.
    // "%d" -> "%d" / "There are %d items" -> "%d" / "items" -> "%d" (fallback). Also see #6405
    ImGuiContext& g = *GImGui;
    //const ImGuiDataTypeInfo* type_info = DataTypeGetInfo(data_type);
    char fmt_buf[64];
    char data_buf[128];
    format = ImParseFormatTrimDecorations(format, fmt_buf, IM_ARRAYSIZE(fmt_buf));
    if (format[0] == 0)
        format = "%f";

    //DataTypeFormatString(data_buf, IM_ARRAYSIZE(data_buf), data_type, p_data, format);
    const int precision = ImParseFormatPrecision128(format ? format : "%.3f", 3);
    std::string s = to_string(*p_data, precision);
    strcpy(data_buf, s.c_str());

    ImStrTrimBlanks(data_buf);

    ImGuiInputTextFlags flags = ImGuiInputTextFlags_AutoSelectAll | (ImGuiInputTextFlags)ImGuiInputTextFlags_LocalizeDecimalPoint;
    g.LastItemData.ItemFlags |= ImGuiItemFlags_NoMarkEdited; // Because TempInputText() uses ImGuiInputTextFlags_MergedItem it doesn't submit a new item, so we poke LastItemData.
    bool value_changed = false;
    if (TempInputText(bb, id, label, data_buf, IM_ARRAYSIZE(data_buf), flags))
    {
        // Backup old value
        size_t data_type_size = sizeof(f128);
        f128 data_backup;
        memcpy(&data_backup, p_data, data_type_size);

        // Apply new value (or operations) then clamp
        ///DataTypeApplyFromText(data_buf, data_type, p_data, format, NULL);

        f128 v_new;
        const char* endp = nullptr;
        bool ok = parse_flt128(data_buf, v_new, &endp);
        // accept if parse succeeded and consumed something meaningful
        if (ok && endp != data_buf)
        {
            if (p_clamp_min || p_clamp_max) v_new = ImClamp128(v_new, *p_clamp_min, *p_clamp_max);
            // optional: keep full precision; if you must, round for *display* only
            bool value_changed = (*p_data != v_new);
            *p_data = v_new;

            //if (changed) MarkItemEdited(id);

            g.LastItemData.ItemFlags &= ~ImGuiItemFlags_NoMarkEdited;
            value_changed = memcmp(&data_backup, p_data, data_type_size) != 0;
            if (value_changed)
                MarkItemEdited(id);
        }

        ///if (p_clamp_min || p_clamp_max)
        ///{
        ///    if (p_clamp_min && p_clamp_max && DataTypeCompareFlt128(p_clamp_min, p_clamp_max) > 0)
        ///        ImSwap(p_clamp_min, p_clamp_max);
        ///    *p_data = ImClamp128(*p_data, *p_clamp_min, *p_clamp_max);
        ///}
        ///
        ///// Only mark as edited if new value is different
        ///g.LastItemData.ItemFlags &= ~ImGuiItemFlags_NoMarkEdited;
        ///value_changed = memcmp(&data_backup, p_data, data_type_size) != 0;
        ///if (value_changed)
        ///    MarkItemEdited(id);
    }
    return value_changed;
}

//
// ---- Behavior core for f128 drag ----
//

static bool DragBehaviorFloat128(ImGuiID id, f128* v, f128 v_speed, f128 v_min, f128 v_max, const char* format, ImGuiSliderFlags flags)
{
    using namespace ImGui;
    ImGuiContext& g = *GImGui;
    //if (g.ActiveId != id) return false;

    // Clear active state when mouse is released (or when nav says deactivate)
    if (g.ActiveId == id)
    {
        if (g.ActiveIdSource == ImGuiInputSource_Mouse && !g.IO.MouseDown[0])
            ClearActiveID();
        else if ((g.ActiveIdSource == ImGuiInputSource_Keyboard || g.ActiveIdSource == ImGuiInputSource_Gamepad)
            && g.NavActivatePressedId == id && !g.ActiveIdIsJustActivated)
            ClearActiveID();
    }
    else
        return false;

    if ((g.LastItemData.ItemFlags & ImGuiItemFlags_ReadOnly) || (flags & ImGuiSliderFlags_ReadOnly))
        return false;

    const ImGuiAxis axis = (flags & ImGuiSliderFlags_Vertical) ? ImGuiAxis_Y : ImGuiAxis_X;
    const bool is_bounded = (v_min < v_max) || ((v_min == v_max) && ((v_min != 0) || (flags & ImGuiSliderFlags_ClampZeroRange)));
    const bool is_wrapped = is_bounded && (flags & ImGuiSliderFlags_WrapAround);
    const bool is_logarithmic = (flags & ImGuiSliderFlags_Logarithmic) != 0;

    // Default speed if not provided and there’s a sane range
    if (v_speed == 0 && is_bounded)
    {
        // g.DragSpeedDefaultRatio is float; map it into f128 range
        f128 range = v_max - v_min;

        if (range > 0 && range < std::numeric_limits<f128>::max())
            v_speed = range * f128(g.DragSpeedDefaultRatio);
        else
            v_speed = 1;
    }

    // Mouse / Nav input -> accumulate float delta, then scale to f128
    float adjust_delta_f = 0.0f;
    if (g.ActiveIdSource == ImGuiInputSource_Mouse && ImGui::IsMousePosValid() && ImGui::IsMouseDragPastThreshold(0, g.IO.MouseDragThreshold * DRAG_MOUSE_THRESHOLD_FACTOR))
    {
        adjust_delta_f = g.IO.MouseDelta[axis];
        if (g.IO.KeyAlt && !(flags & ImGuiSliderFlags_NoSpeedTweaks))   adjust_delta_f *= 1.0f / 100.0f;
        if (g.IO.KeyShift && !(flags & ImGuiSliderFlags_NoSpeedTweaks)) adjust_delta_f *= 10.0f;
    }
    else if (g.ActiveIdSource == ImGuiInputSource_Keyboard || g.ActiveIdSource == ImGuiInputSource_Gamepad)
    {
        const bool tweak_slow = ImGui::IsKeyDown((g.NavInputSource == ImGuiInputSource_Gamepad) ? ImGuiKey_NavGamepadTweakSlow : ImGuiKey_NavKeyboardTweakSlow);
        const bool tweak_fast = ImGui::IsKeyDown((g.NavInputSource == ImGuiInputSource_Gamepad) ? ImGuiKey_NavGamepadTweakFast : ImGuiKey_NavKeyboardTweakFast);
        float tweak_factor = (flags & ImGuiSliderFlags_NoSpeedTweaks) ? 1.0f : (tweak_slow ? 0.1f : (tweak_fast ? 10.0f : 1.0f));
        adjust_delta_f = ImGui::GetNavTweakPressedAmount(axis) * tweak_factor;
    }

    if (axis == ImGuiAxis_Y) adjust_delta_f = -adjust_delta_f;

    // For logarithmic, scale into parametric [0,1] space
    if (is_logarithmic)
    {
        float log_eps = 0.0f;
        // Estimate epsilon from format precision (default 3)
        int prec = ImParseFormatPrecision128(format ? format : "%.3f", 3);
        log_eps = ImPow(0.1f, (float)prec);

        const bool just_activated = g.ActiveIdIsJustActivated;
        const bool pushing_out =
            is_bounded && !is_wrapped &&
            ((*v >= v_max && adjust_delta_f > 0.0f) || (*v <= v_min && adjust_delta_f < 0.0f));
        if (just_activated || pushing_out)
        {
            g.DragCurrentAccum = 0.0f;
            g.DragCurrentAccumDirty = false;
        }
        else if (adjust_delta_f != 0.0f)
        {
            g.DragCurrentAccum += (float)(f128(adjust_delta_f) / std::max(f128::eps(), abs(v_max - v_min)));
            g.DragCurrentAccumDirty = true;
        }

        if (!g.DragCurrentAccumDirty) return false;

        float old_param = ScaleRatioFromValue_flt128(*v, v_min, v_max, true, log_eps);
        float new_param = ImSaturate(old_param + g.DragCurrentAccum);
        f128 v_new = ScaleValueFromRatio_flt128(new_param, v_min, v_max, true, log_eps);

        if (!(flags & ImGuiSliderFlags_NoRoundToFormat))
            v_new = RoundScalarWithFormat128(format, v_new);

        // preserve remainder
        float new_param_after_round = ScaleRatioFromValue_flt128(v_new, v_min, v_max, true, log_eps);
        g.DragCurrentAccum -= (new_param_after_round - old_param);
        g.DragCurrentAccumDirty = false;

        // wrapping/clamp
        if (is_bounded && !is_wrapped)
            v_new = ImClamp128(v_new, v_min, v_max);

        if (*v == v_new)
            return false;
        *v = v_new;
        return true;
    }
    else
    {
        const bool just_activated = g.ActiveIdIsJustActivated;
        const bool pushing_out =
            is_bounded && !is_wrapped &&
            ((*v >= v_max && adjust_delta_f > 0.0f) || (*v <= v_min && adjust_delta_f < 0.0f));

        if (just_activated || pushing_out)
        {
            g.DragCurrentAccum = 0.0f;
            g.DragCurrentAccumDirty = false;
        }
        else if (adjust_delta_f != 0.0f)
        {
            g.DragCurrentAccum += adjust_delta_f;
            g.DragCurrentAccumDirty = true;
        }

        if (!g.DragCurrentAccumDirty) return false;

        f128 v_cur = *v + (f128(g.DragCurrentAccum) * v_speed);
        if (!(flags & ImGuiSliderFlags_NoRoundToFormat))
            v_cur = RoundScalarWithFormat128(format, v_cur);

        // preserve remainder after rounding
        g.DragCurrentAccum -= (float)((double)((v_cur - *v) / v_speed));
        g.DragCurrentAccumDirty = false;

        // lose negative zero
        if (v_cur == f128(-0)) v_cur = 0;

        // clamp/wrap
        if (is_bounded)
        {
            if (is_wrapped)
            {
                f128 range = v_max - v_min;
                if (range != 0)
                {
                    // wrap into [v_min, v_max]
                    while (v_cur < v_min) v_cur = v_cur + range;
                    while (v_cur > v_max) v_cur = v_cur - range;
                }
            }
            else
            {
                v_cur = ImClamp128(v_cur, v_min, v_max);
            }
        }

        if (*v == v_cur) return false;
        *v = v_cur;
        return true;
    }
}


namespace ImGui
{
    bool Section(const char* name, bool open_by_default, float header_spacing, float body_margin_top)
    {
        ImGui::Dummy(bl::scale_size(0.0f, header_spacing));
        bool ret = ImGui::CollapsingHeader(name, open_by_default ? ImGuiTreeNodeFlags_DefaultOpen : 0);
        if (ret) ImGui::Dummy(bl::scale_size(0.0f, body_margin_top));
        return ret;
    }

    bool ResetBtn(const char* id)
    {
        static int size = (int)bl::platform()->line_height();
        static int reset_icon = loadSVG("/data/icon/reset.svg", size, size);
        return ImGui::ImageButton(id, reset_icon, ImVec2((float)size, (float)size));
    }

    bool InlResetBtn(const char* id)
    {
        ImGui::SameLine();
        bool ret = ResetBtn(id);
        return ret;
    }

    bool SliderDouble(const char* label, double* v, double v_min, double v_max, const char* format, ImGuiSliderFlags flags)
    {
        return SliderScalar(label, ImGuiDataType_Double, v, &v_min, &v_max, format, flags);
    }

    bool DragDouble(const char* label, double* v, double v_speed, double v_min, double v_max, const char* format, ImGuiSliderFlags flags)
    {
        return DragScalar(label, ImGuiDataType_Double, v, (float)v_speed, &v_min, &v_max, format, flags);
    }

    bool SliderDouble2(const char* label, double v[2], double v_min, double v_max, const char* format, ImGuiSliderFlags flags)
    {
        return SliderScalarN(label, ImGuiDataType_Double, v, 2, &v_min, &v_max, format, flags);
    }

    bool DragDouble2(const char* label, double v[2], double v_speed, double v_min, double v_max, const char* format, ImGuiSliderFlags flags)
    {
        return DragScalarN(label, ImGuiDataType_Double, v, 2, (float)v_speed, &v_min, &v_max, format, flags);
    }

    /*template<typename TYPE, typename SIGNEDTYPE, typename FLOATTYPE>
    static inline float ScaleRatioFromValue_TowardMax(ImGuiDataType, TYPE v, TYPE v_min, TYPE v_max, float epsilon)
    {
        if (v_min == v_max)
            return 0.0f;

        bool flipped = (v_max < v_min);
        if (flipped)
            ImSwap(v_min, v_max);

        const TYPE v_clamped = ImClamp(v, v_min, v_max);

        // u = distance to max
        const FLOATTYPE Umax = (FLOATTYPE)(v_max - v_min);
        const FLOATTYPE u = (FLOATTYPE)v_max - (FLOATTYPE)v_clamped;

        // Choose k (shape) to mirror stock intensity where possible.
        FLOATTYPE k;
        if (v_min > (TYPE)0 && v_max > (TYPE)0)
        {
            k = (FLOATTYPE)v_min;                  // matches log(v_max/v_min)
        }
        else if (v_min < (TYPE)0 && v_max < (TYPE)0)
        {
            k = (FLOATTYPE)(-(FLOATTYPE)v_max);    // matches log(|v_min|/|v_max|)
        }
        else
        {
            // Crosses zero: pick smaller magnitude side, clamp by epsilon
            const FLOATTYPE am = (FLOATTYPE)ImAbs((FLOATTYPE)v_min);
            const FLOATTYPE aM = (FLOATTYPE)ImAbs((FLOATTYPE)v_max);
            FLOATTYPE s = (am > (FLOATTYPE)0 && aM > (FLOATTYPE)0) ? ImMin(am, aM) : (am > (FLOATTYPE)0 ? am : aM);
            if (s <= (FLOATTYPE)0) s = (FLOATTYPE)1;
            k = s;
        }
        k = ImMax(k, (FLOATTYPE)epsilon);

        const FLOATTYPE denom = ImLog((FLOATTYPE)1 + Umax / k);
        float t;
        if (denom <= (FLOATTYPE)0)
        {
            // Fallback to linear if degenerate
            t = (Umax > (FLOATTYPE)0) ? (float)(1.0 - (double)(u / Umax)) : 0.0f;
        }
        else
        {
            const FLOATTYPE t_u = ImLog((FLOATTYPE)1 + u / k) / denom; // 0..1
            t = 1.0f - (float)ImClamp(t_u, (FLOATTYPE)0, (FLOATTYPE)1);
        }

        return flipped ? (1.0f - t) : t;
    }

    template<typename TYPE, typename SIGNEDTYPE, typename FLOATTYPE>
    static inline TYPE ScaleValueFromRatio_TowardMax(ImGuiDataType data_type, float t, TYPE v_min, TYPE v_max, float epsilon)
    {
        using namespace ImGui;

        if (t <= 0.0f || v_min == v_max)
            return v_min;
        if (t >= 1.0f)
            return v_max;

        bool flipped = (v_max < v_min);
        if (flipped)
            ImSwap(v_min, v_max);

        float t_with_flip = flipped ? (1.0f - t) : t;

        const FLOATTYPE Umax = (FLOATTYPE)(v_max - v_min);

        // Choose k (shape) consistent with the value->ratio function.
        FLOATTYPE k;
        if (v_min > (TYPE)0 && v_max > (TYPE)0)
        {
            k = (FLOATTYPE)v_min;
        }
        else if (v_min < (TYPE)0 && v_max < (TYPE)0)
        {
            k = (FLOATTYPE)(-(FLOATTYPE)v_max);
        }
        else
        {
            const FLOATTYPE am = (FLOATTYPE)ImAbs((FLOATTYPE)v_min);
            const FLOATTYPE aM = (FLOATTYPE)ImAbs((FLOATTYPE)v_max);
            FLOATTYPE s = (am > (FLOATTYPE)0 && aM > (FLOATTYPE)0) ? ImMin(am, aM) : (am > (FLOATTYPE)0 ? am : aM);
            if (s <= (FLOATTYPE)0) s = (FLOATTYPE)1;
            k = s;
        }
        k = ImMax(k, (FLOATTYPE)epsilon);

        const FLOATTYPE denom = ImLog((FLOATTYPE)1 + Umax / k);

        FLOATTYPE v_f;
        if (denom <= (FLOATTYPE)0)
        {
            // Linear fallback if degenerate
            v_f = (FLOATTYPE)ImLerp(v_min, v_max, t_with_flip);
        }
        else
        {
            const FLOATTYPE t_u = (FLOATTYPE)(1.0f - t_with_flip); // 0..1
            const FLOATTYPE R1 = ImPow((FLOATTYPE)1 + Umax / k, t_u);
            const FLOATTYPE u = k * (R1 - (FLOATTYPE)1);
            v_f = (FLOATTYPE)v_max - u;
        }

        TYPE v_out = (TYPE)v_f;

        // Keep integer types inside range (rounding to format is handled by caller for floats)
        const bool is_floating_point = (data_type == ImGuiDataType_Float) || (data_type == ImGuiDataType_Double);
        if (!is_floating_point)
        {
            if (v_out < v_min) v_out = v_min;
            if (v_out > v_max) v_out = v_max;
        }

        return v_out;
    }

    // --- Slider behavior using the "toward max" mapping --------------------------
    template<typename TYPE, typename SIGNEDTYPE, typename FLOATTYPE>
    bool SliderBehaviorT_TowardMax(const ImRect& bb, ImGuiID id, ImGuiDataType data_type,
        TYPE* v, const TYPE v_min, const TYPE v_max,
        const char* format, ImGuiSliderFlags flags, ImRect* out_grab_bb)
    {
        ImGuiContext& g = *GImGui;
        const ImGuiStyle& style = g.Style;

        const ImGuiAxis axis = (flags & ImGuiSliderFlags_Vertical) ? ImGuiAxis_Y : ImGuiAxis_X;
        const bool is_floating_point = (data_type == ImGuiDataType_Float) || (data_type == ImGuiDataType_Double);
        const float v_range_f = (float)(v_min < v_max ? v_max - v_min : v_min - v_max);

        // Geometry
        const float grab_padding = 2.0f;
        const float slider_sz = (bb.Max[axis] - bb.Min[axis]) - grab_padding * 2.0f;
        float grab_sz = style.GrabMinSize;
        if (!is_floating_point && v_range_f >= 0.0f)
            grab_sz = ImMax(slider_sz / (v_range_f + 1), style.GrabMinSize);
        grab_sz = ImMin(grab_sz, slider_sz);
        const float slider_usable_sz = slider_sz - grab_sz;
        const float slider_usable_pos_min = bb.Min[axis] + grab_padding + grab_sz * 0.5f;
        const float slider_usable_pos_max = bb.Max[axis] - grab_padding - grab_sz * 0.5f;

        // Epsilon picked from the display precision (like imgui's log)
        const int decimal_precision = is_floating_point ? ImParseFormatPrecision(format, 3) : 1;
        const float epsilon = ImPow(0.1f, (float)decimal_precision);

        bool value_changed = false;

        if (g.ActiveId == id)
        {
            bool set_new_value = false;
            float clicked_t = 0.0f;

            if (g.ActiveIdSource == ImGuiInputSource_Mouse)
            {
                if (!g.IO.MouseDown[0])
                {
                    ClearActiveID();
                }
                else
                {
                    const float mouse_abs_pos = g.IO.MousePos[axis];
                    if (g.ActiveIdIsJustActivated)
                    {
                        float grab_t = ScaleRatioFromValue_TowardMax<TYPE, SIGNEDTYPE, FLOATTYPE>(data_type, *v, v_min, v_max, epsilon);
                        if (axis == ImGuiAxis_Y)
                            grab_t = 1.0f - grab_t;
                        const float grab_pos = ImLerp(slider_usable_pos_min, slider_usable_pos_max, grab_t);
                        const bool clicked_around_grab = (mouse_abs_pos >= grab_pos - grab_sz * 0.5f - 1.0f) && (mouse_abs_pos <= grab_pos + grab_sz * 0.5f + 1.0f);
                        g.SliderGrabClickOffset = (clicked_around_grab && is_floating_point) ? mouse_abs_pos - grab_pos : 0.0f;
                    }
                    if (slider_usable_sz > 0.0f)
                        clicked_t = ImSaturate((mouse_abs_pos - g.SliderGrabClickOffset - slider_usable_pos_min) / slider_usable_sz);
                    if (axis == ImGuiAxis_Y)
                        clicked_t = 1.0f - clicked_t;
                    set_new_value = true;
                }
            }
            else if (g.ActiveIdSource == ImGuiInputSource_Keyboard || g.ActiveIdSource == ImGuiInputSource_Gamepad)
            {
                if (g.ActiveIdIsJustActivated)
                {
                    g.SliderCurrentAccum = 0.0f;
                    g.SliderCurrentAccumDirty = false;
                }

                float input_delta = (axis == ImGuiAxis_X) ? GetNavTweakPressedAmount(axis) : -GetNavTweakPressedAmount(axis);
                if (input_delta != 0.0f)
                {
                    const bool tweak_slow = IsKeyDown((g.NavInputSource == ImGuiInputSource_Gamepad) ? ImGuiKey_NavGamepadTweakSlow : ImGuiKey_NavKeyboardTweakSlow);
                    const bool tweak_fast = IsKeyDown((g.NavInputSource == ImGuiInputSource_Gamepad) ? ImGuiKey_NavGamepadTweakFast : ImGuiKey_NavKeyboardTweakFast);
                    if (decimal_precision > 0)
                    {
                        input_delta /= 100.0f;
                        if (tweak_slow)
                            input_delta /= 10.0f;
                    }
                    else
                    {
                        if ((v_range_f >= -100.0f && v_range_f <= 100.0f && v_range_f != 0.0f) || tweak_slow)
                            input_delta = ((input_delta < 0.0f) ? -1.0f : +1.0f) / v_range_f;
                        else
                            input_delta /= 100.0f;
                    }
                    if (tweak_fast)
                        input_delta *= 10.0f;

                    g.SliderCurrentAccum += input_delta;
                    g.SliderCurrentAccumDirty = true;
                }

                if (g.NavActivatePressedId == id && !g.ActiveIdIsJustActivated)
                {
                    ClearActiveID();
                }
                else if (g.SliderCurrentAccumDirty)
                {
                    float old_t = ScaleRatioFromValue_TowardMax<TYPE, SIGNEDTYPE, FLOATTYPE>(data_type, *v, v_min, v_max, epsilon);
                    float new_t = ImSaturate(old_t + g.SliderCurrentAccum);

                    // Convert to value, then back to t to see how much we actually moved
                    TYPE v_new = ScaleValueFromRatio_TowardMax<TYPE, SIGNEDTYPE, FLOATTYPE>(data_type, new_t, v_min, v_max, epsilon);

                    if (is_floating_point && !(flags & ImGuiSliderFlags_NoRoundToFormat))
                        v_new = RoundScalarWithFormatT<TYPE>(format, data_type, v_new);

                    float realized_t = ScaleRatioFromValue_TowardMax<TYPE, SIGNEDTYPE, FLOATTYPE>(data_type, v_new, v_min, v_max, epsilon);

                    float delta = g.SliderCurrentAccum;
                    g.SliderCurrentAccum -= (delta > 0) ? ImMin(realized_t - old_t, delta)
                        : ImMax(realized_t - old_t, delta);
                    g.SliderCurrentAccumDirty = false;

                    clicked_t = new_t;
                    set_new_value = true;
                }
            }

            if (set_new_value)
                if ((g.LastItemData.ItemFlags & ImGuiItemFlags_ReadOnly) || (flags & ImGuiSliderFlags_ReadOnly))
                    set_new_value = false;

            if (set_new_value)
            {
                TYPE v_new = ScaleValueFromRatio_TowardMax<TYPE, SIGNEDTYPE, FLOATTYPE>(data_type, clicked_t, v_min, v_max, epsilon);

                if (is_floating_point && !(flags & ImGuiSliderFlags_NoRoundToFormat))
                    v_new = RoundScalarWithFormatT<TYPE>(format, data_type, v_new);

                if (*v != v_new)
                {
                    *v = v_new;
                    value_changed = true;
                }
            }
        }

        // Grab rect
        if (slider_sz < 1.0f)
        {
            *out_grab_bb = ImRect(bb.Min, bb.Min);
        }
        else
        {
            float grab_t = ScaleRatioFromValue_TowardMax<TYPE, SIGNEDTYPE, FLOATTYPE>(data_type, *v, v_min, v_max, epsilon);
            if (axis == ImGuiAxis_Y)
                grab_t = 1.0f - grab_t;
            const float grab_pos = ImLerp(slider_usable_pos_min, slider_usable_pos_max, grab_t);
            if (axis == ImGuiAxis_X)
                *out_grab_bb = ImRect(grab_pos - grab_sz * 0.5f, bb.Min.y + grab_padding, grab_pos + grab_sz * 0.5f, bb.Max.y - grab_padding);
            else
                *out_grab_bb = ImRect(bb.Min.x + grab_padding, grab_pos - grab_sz * 0.5f, bb.Max.x - grab_padding, grab_pos + grab_sz * 0.5f);
        }

        return value_changed;
    }*/

   
    bool SliderDouble_InvLog(const char* label, double* v, double v_min, double v_max, const char* format, ImGuiSliderFlags flags)
    {
        return SliderScalar_TowardMax(label, ImGuiDataType_Double, v, &v_min, &v_max, format, flags);
    }

    bool DragFloat128(const char* label, f128* v, f128 v_speed, f128 v_min, f128 v_max, const char* format, ImGuiSliderFlags flags)
    {
        ImGuiWindow* window = GetCurrentWindow();
        if (window->SkipItems)
            return false;
        
        ImGuiContext& g = *GImGui;
        const ImGuiStyle& style = g.Style;
        const ImGuiID id = window->GetID(label);
        const float w = CalcItemWidth();

        const ImVec2 label_size = CalcTextSize(label, NULL, true);
        const ImRect frame_bb(window->DC.CursorPos, window->DC.CursorPos + ImVec2(w, label_size.y + style.FramePadding.y * 2.0f));
        const ImRect total_bb(frame_bb.Min, frame_bb.Max + ImVec2(label_size.x > 0.0f ? style.ItemInnerSpacing.x + label_size.x : 0.0f, 0.0f));

        const bool temp_input_allowed = (flags & ImGuiSliderFlags_NoInput) == 0;
        ItemSize(total_bb, style.FramePadding.y);
        if (!ItemAdd(total_bb, id, &frame_bb, temp_input_allowed ? ImGuiItemFlags_Inputable : 0))
            return false;

        if (format == NULL)
            format = "%f";

        ///if (g.ActiveId == id)
        ///{
        ///    if (g.ActiveIdSource == ImGuiInputSource_Mouse && !g.IO.MouseDown[0])
        ///        ClearActiveID();
        ///    else if ((g.ActiveIdSource == ImGuiInputSource_Keyboard || g.ActiveIdSource == ImGuiInputSource_Gamepad)
        ///        && g.NavActivatePressedId == id && !g.ActiveIdIsJustActivated)
        ///        ClearActiveID();
        ///}

        const bool hovered = ItemHoverable(frame_bb, id, g.LastItemData.ItemFlags);
        bool temp_input_is_active = temp_input_allowed && TempInputIsActive(id);
        if (!temp_input_is_active)
        {
            const bool clicked = hovered && IsMouseClicked(0, ImGuiInputFlags_None, id);
            const bool double_clicked = (hovered && g.IO.MouseClickedCount[0] == 2 && TestKeyOwner(ImGuiKey_MouseLeft, id));
            const bool make_active = (clicked || double_clicked || g.NavActivateId == id);
            if (make_active && (clicked || double_clicked))
                SetKeyOwner(ImGuiKey_MouseLeft, id);
            if (make_active && temp_input_allowed)
                if ((clicked && g.IO.KeyCtrl) || double_clicked || (g.NavActivateId == id && (g.NavActivateFlags & ImGuiActivateFlags_PreferInput)))
                    temp_input_is_active = true;

            if (g.IO.ConfigDragClickToInputText && temp_input_allowed && !temp_input_is_active)
                if (g.ActiveId == id && hovered && g.IO.MouseReleased[0] && !IsMouseDragPastThreshold(0, g.IO.MouseDragThreshold * DRAG_MOUSE_THRESHOLD_FACTOR))
                {
                    g.NavActivateId = id;
                    g.NavActivateFlags = ImGuiActivateFlags_PreferInput;
                    temp_input_is_active = true;
                }

            //if (make_active)
            //    memcpy(&g.ActiveIdValueOnActivation, v, sizeof(f128));

            if (make_active && !temp_input_is_active)
            {
                SetActiveID(id, window);
                SetFocusID(id, window);
                FocusWindow(window);
                g.ActiveIdUsingNavDirMask = (1 << ImGuiDir_Left) | (1 << ImGuiDir_Right);
            }
        }

        // --- Text input mode (double-click / Ctrl+Click) ---
        if (temp_input_is_active)
        {
            const bool clamp_enabled = (flags & ImGuiSliderFlags_ClampOnInput) != 0;
            //const int precision = ImParseFormatPrecision(format ? format : "%.3f", 3);

            return TempInputFlt128(frame_bb, id, label, v, format, clamp_enabled ? &v_min : NULL, clamp_enabled ? &v_max : NULL);
        }

        // Draw frame
        const ImU32 frame_col = GetColorU32(g.ActiveId == id ? ImGuiCol_FrameBgActive : hovered ? ImGuiCol_FrameBgHovered : ImGuiCol_FrameBg);
        RenderNavCursor(frame_bb, id);
        RenderFrame(frame_bb.Min, frame_bb.Max, frame_col, true, style.FrameRounding);

        // Drag behavior (active-id path uses internal accumulators)
        const bool value_changed = DragBehaviorFloat128(id, v, v_speed, v_min, v_max, format ? format : "%.3f", flags);
        if (value_changed)
            MarkItemEdited(id);

        // Display value using your to_string(f128)
        std::string value_s = to_string(*v, ImParseFormatPrecision128(format, 3), true);
        if (g.LogEnabled) LogSetNextTextDecoration("{", "}");
        RenderTextClipped(frame_bb.Min, frame_bb.Max, value_s.c_str(), NULL, NULL, ImVec2(0.5f, 0.5f));

        if (label_size.x > 0.0f)
            RenderText(ImVec2(frame_bb.Max.x + style.ItemInnerSpacing.x, frame_bb.Min.y + style.FramePadding.y), label);

        IMGUI_TEST_ENGINE_ITEM_INFO(id, label, g.LastItemData.StatusFlags | (temp_input_allowed ? ImGuiItemStatusFlags_Inputable : 0));
        return value_changed;
    }

    bool SliderAngle(const char* label, double* v_rad, double v_rad_min, double v_rad_max, const char* format, ImGuiSliderFlags flags)
    {
        double v_deg = (*v_rad * 180.0 / std::numbers::pi);

        bool changed = ImGui::SliderDouble(label, &v_deg,
            (v_rad_min * 180.0 / std::numbers::pi),
            (v_rad_max * 180.0 / std::numbers::pi),
            format, flags);

        if (changed)
            *v_rad = (v_deg * std::numbers::pi / 180.0);

        return changed;
    }

    bool SliderAngle(const char* label, double* v_rad, double v_rad_min, double v_rad_max, int decimals, ImGuiSliderFlags flags)
    {
        char format[8];
        snprintf(format, 8, "%%.%df\xC2\xB0", decimals);
        return SliderAngle(label, v_rad, v_rad_min, v_rad_max, format, flags);
    }

    bool RevertableSliderDouble(const char* label, double* v, double* initial, double v_min, double v_max, const char* format, ImGuiSliderFlags flags)
    {
        bool ret = false;
        ImGui::PushItemWidth(ImGui::CalcItemWidth() - bl::platform()->line_height());
        ret |= SliderScalar(label, ImGuiDataType_Double, v, &v_min, &v_max, format, flags);
        ImGui::PopItemWidth();

        ImGui::PushID("reset_");
        if (*v != *initial && InlResetBtn(label))
        {
            *v = *initial;
            ret |= true;
        }
        ImGui::PopID();
        return ret;
    }

    bool RevertableDragDouble(const char* label, double* v, double* initial, double v_speed, double v_min, double v_max, const char* format, ImGuiSliderFlags flags)
    {
        bool ret = false;

        ImGui::PushItemWidth(ImGui::CalcItemWidth() - bl::platform()->line_height());
        ret |= DragScalar(label, ImGuiDataType_Double, v, (float)v_speed, &v_min, &v_max, format, flags);
        ImGui::PopItemWidth();

        ImGui::PushID("reset_");
        if (*v != *initial && InlResetBtn(label))
        {
            *v = *initial;
            ret |= true;
        }
        ImGui::PopID();
        return ret;
    }

    bool RevertableDragFloat128(const char* label, f128* v, f128* initial, f128 v_speed, f128 v_min, f128 v_max, const char* format, ImGuiSliderFlags flags)
    {
        bool ret = false;

        ImGui::PushItemWidth(ImGui::CalcItemWidth() - bl::platform()->line_height());
        ret |= DragFloat128(label, v, v_speed, v_min, v_max, format, flags);
        ImGui::PopItemWidth();

        ImGui::PushID("reset_");
        if (*v != *initial && InlResetBtn(label))
        {
            *v = *initial;
            ret |= true;
        }
        ImGui::PopID();
        return ret;
    }

    bool RevertableSliderDouble2(const char* label, double v[2], double initial[2], double v_min, double v_max, const char* format, ImGuiSliderFlags flags)
    {
        bool ret = false;

        ImGui::PushItemWidth(ImGui::CalcItemWidth() - bl::platform()->line_height());
        ret |= SliderScalarN(label, ImGuiDataType_Double, v, 2, &v_min, &v_max, format, flags);
        ImGui::PopItemWidth();

        ImGui::PushID("reset_");
        if ((v[0] != initial[0] || v[1] != initial[1]) && InlResetBtn(label))
        {
            v[0] = initial[0];
            v[1] = initial[1];
            ret |= true;
        }
        ImGui::PopID();
        return ret;
    }

    bool RevertableSliderAngle(const char* label, double* v_rad, double* initial, double v_rad_min, double v_rad_max, const char* format, ImGuiSliderFlags flags)
    {
        bool ret = false;
        ImGui::PushItemWidth(ImGui::CalcItemWidth() - bl::platform()->line_height());
        ret |= SliderAngle(label, v_rad, v_rad_min, v_rad_max, format, flags);
        ImGui::PopItemWidth();

        ImGui::PushID("reset_");
        if (*v_rad != *initial && InlResetBtn(label))
        {
            *v_rad = *initial;
            ret |= true;
        }
        ImGui::PopID();
        return ret;
    }

    bool RevertableSliderAngle(const char* label, double* v_rad, double* initial, double v_rad_min, double v_rad_max, int decimals, ImGuiSliderFlags flags)
    {
        char format[8];
        snprintf(format, 8, "%%.%df\xC2\xB0", decimals);
        return RevertableSliderAngle(label, v_rad, initial, v_rad_min, v_rad_max, format, flags);
    }

    float old_work_rect_width = 0.0f;
    float old_conent_rect_width = 0.0f;
    float region_padding = 0.0f;

    void BeginPaddedRegion(float padding) 
    {
        //const float fullW = ImGui::GetContentRegionAvail().x;
        ImVec2 p0 = ImGui::GetCursorScreenPos() + ImVec2(padding, padding);
        ImGui::SetCursorScreenPos(p0);
        ImGui::BeginGroup();

        region_padding = padding;

        old_work_rect_width = ImGui::GetCurrentWindow()->WorkRect.Max.x;
        old_conent_rect_width = ImGui::GetCurrentWindow()->ContentRegionRect.Max.x;
        ImGui::GetCurrentWindow()->WorkRect.Max.x -= padding;
        ImGui::GetCurrentWindow()->ContentRegionRect.Max.x -= padding;
        ImGui::PushTextWrapPos(old_conent_rect_width - padding);
    }
    void EndPaddedRegion()
    {
        ImGui::PopTextWrapPos();
        ImGui::GetCurrentWindow()->ContentRegionRect.Max.x = old_conent_rect_width;
        ImGui::GetCurrentWindow()->WorkRect.Max.x = old_work_rect_width;

        ImGui::EndGroup();
        ImVec2 p1 = ImGui::GetItemRectMax() + ImVec2(region_padding, region_padding);
        ImGui::Dummy(ImVec2(region_padding, region_padding));
        region_padding = 0.0f;
    }

    GroupBox::GroupBox(const char* id, const char* _label, float _pad, float _label_pad_x)
        : label(_label), pad(_pad), label_pad_x(_label_pad_x)
    {
        ImVec2 label_height = ImGui::CalcTextSize(label);
        ImGui::Dummy(ImVec2(0, label_height.y)); // space for title

        // measure full available width of the current row/column
        start_screen = ImGui::GetCursorScreenPos();
        span_w = ImMax(0.0f, ImGui::GetContentRegionAvail().x);

        ImGui::PushID(id);
        dl = ImGui::GetWindowDrawList();
        text_sz = (label && *label) ? label_height : ImVec2(0, 0);
        top_extra = (text_sz.y > 0.0f) ? text_sz.y * 0.5f : 0.0f;

        dl->ChannelsSplit(2); // 0 = bg/border, 1 = contents/label
        dl->ChannelsSetCurrent(1);

        ImVec2 p = ImGui::GetCursorScreenPos();
        ImGui::SetCursorScreenPos(ImVec2(p.x + pad, p.y + pad + top_extra)); // inner + title
        ImGui::BeginGroup();
    }

    GroupBox::~GroupBox()
    {
        ImGui::EndGroup();

        // content bounds (tight)
        content_min = ImGui::GetItemRectMin();
        content_max = ImGui::GetItemRectMax();

        // outer frame
        ImVec2 outer_min = content_min - ImVec2(pad, pad + top_extra);
        ImVec2 outer_max = content_max + ImVec2(pad, pad);

        // force the frame to span the entire parent width
        outer_min.x = start_screen.x;
        outer_max.x = start_screen.x + span_w;

        // draw bg/border behind contents
        ImGuiStyle& style = ImGui::GetStyle();
        float rounding = style.FrameRounding;
        float t = style.FrameBorderSize > 0 ? style.FrameBorderSize : 1.0f;
        ImU32 col_bg = ImGui::GetColorU32(ImGuiCol_TitleBg);
        ImU32 col_border = ImGui::GetColorU32(ImGuiCol_Border);
        ImU32 col_text = ImGui::GetColorU32(ImGuiCol_Text);

        // draw bg + full border
        dl->ChannelsSetCurrent(0);
        dl->AddRectFilled(outer_min, outer_max, col_bg, rounding);
        dl->AddRect(outer_min, outer_max, col_border, rounding, 0, t);

        // foreground: title bg + text (channel 1)
        dl->ChannelsSetCurrent(1);
        if (label && *label)
        {
            float x_text = outer_min.x + pad + label_pad_x;
            float y_line = outer_min.y; // top border y
            float text_margin_x = 8.0f;
            float text_margin_y = 4.0f;

            ImVec2 label_min(x_text - text_margin_x, y_line - text_sz.y * 0.5f - t - text_margin_y);
            ImVec2 label_max(x_text + text_sz.x + text_margin_x, y_line + text_sz.y * 0.5f + t + text_margin_y);

            // title background (kept inside frame; no special clipping needed)
            dl->AddRectFilled(label_min, label_max, col_bg);
            dl->AddRect(label_min, label_max, col_border);

            dl->AddText(ImVec2(x_text, y_line - text_sz.y * 0.5f), col_text, label);
        }

        dl->ChannelsMerge();

        ImGui::SetCursorScreenPos(ImVec2(start_screen.x, outer_max.y));
        ImGui::Dummy(ImVec2(span_w, 0.0f));

        ImGui::PopID();
    }

}
