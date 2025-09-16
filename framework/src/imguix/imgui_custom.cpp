#include <bitloop/imguix/imgui_custom.h>

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
    
    bool SliderDouble_InvLog(const char* label, double* v, double v_min, double v_max, const char* format, ImGuiSliderFlags flags)
    {
        return SliderScalar_TowardMax(label, ImGuiDataType_Double, v, &v_min, &v_max, format, flags);
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
}
