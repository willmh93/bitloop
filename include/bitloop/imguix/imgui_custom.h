#pragma once
#include <unordered_map>
#include <numbers>
#include <algorithm>
#include <stdexcept>
#include <functional>
#include <numbers>

constexpr double pi = std::numbers::pi;

#define IMGUI_DEFINE_MATH_OPERATORS

#include <bitloop/core/project.h>

#include <imgui_internal.h>
#include <imgui_freetype.h>
#include <imgui_stdlib.h>

#include "implot.h"

// Custom components
#include "imgui_splines.h"
#include "imgui_gradient_edit.h"
#include "imgui_log.h"
#include "imgui_debug_ui.h"

#ifdef __EMSCRIPTEN__
#define SDL_MAIN_HANDLED
#include <GLES3/gl3.h>
#else
#include "glad/glad.h"
#endif

// Image load helpers
bool LoadPixelsRGBA(const char* path, int* outW, int* outH, std::vector<unsigned char>& outPixels);
GLuint CreateGLTextureRGBA8(const void* pixels, int w, int h);
void DestroyTexture(GLuint tex);

inline ImTextureID ToImTextureID(GLuint tex)
{
    return (ImTextureID)(intptr_t)tex;
}

namespace bl 
{
    inline ImVec2 scale_size(const ImVec2& size)
    {
        float dpr = bl::platform()->dpr();
        return ImVec2(size.x * dpr, size.y * dpr);
    }

    inline ImVec2 scale_size(int w, int h)
    {
        float dpr = bl::platform()->dpr();
        return ImVec2((float)w * dpr, (float)h * dpr);
    }

    inline ImVec2 scale_size(float w, float h)
    {
        float dpr = bl::platform()->dpr();
        return ImVec2(w * dpr, h * dpr);
    }

    inline DVec2 scale_size(double w, double h)
    {
        float dpr = bl::platform()->dpr();
        return DVec2(w * dpr, h * dpr);
    }

    inline DVec2 scale_size(const DVec2& size)
    {
        float dpr = bl::platform()->dpr();
        return DVec2(size.x * dpr, size.y * dpr);
    }
}

std::ostream& operator<<(std::ostream& os, const ImSpline::Spline& spline);
std::ostream& operator<<(std::ostream& os, const ImGradient& gradient);

bool SliderBehavior_TowardMax(const ImRect& bb, ImGuiID id, ImGuiDataType data_type, void* p_v, const void* p_min, const void* p_max, const char* format, ImGuiSliderFlags flags, ImRect* out_grab_bb);


namespace ImGui
{
    using bl::f128;
    using bl::f128;

    struct StartValue { void* initial; int size; };
    static std::unordered_map<void*, StartValue> starting_map;
    static void updatePointerValues()
    {
        for (auto& item : starting_map)
            memcpy(item.first, item.second.initial, item.second.size);
    }

    bool Section(const char* name, bool open_by_default = false, float header_spacing=5.0f, float body_margin_top=2.0f);

    bool ResetBtn(const char* id);
    bool InlResetBtn(const char* id);

    
    bool SliderDouble(const char* label, double* v, double v_min, double v_max, const char* format = "%.6f", ImGuiSliderFlags flags = 0);
    bool DragDouble(const char* label, double* v, double v_speed = 1.0, double v_min = 0.0, double v_max = 0.0, const char* format = "%.6f", ImGuiSliderFlags flags = 0);
    bool SliderDouble2(const char* label, double v[2], double v_min, double v_max, const char* format = "%.6f", ImGuiSliderFlags flags = 0);
    bool DragDouble2(const char* label, double v[2], double v_speed = 1.0f, double v_min = 0.0f, double v_max = 0.0f, const char* format = "%.6f", ImGuiSliderFlags flags = 0);
    bool SliderDouble_InvLog(const char* label, double* v, double v_min, double v_max, const char* format, ImGuiSliderFlags flags = 0);
    bool DragFloat128(const char* label, f128* v, f128 v_speed=0, f128 v_min=0, f128 v_max=0, const char* format = "%.32f", ImGuiSliderFlags flags = 0);

    bool SliderAngle(const char* label, double* v_rad, double v_rad_min, double v_rad_max, const char* format = "%.1f\xC2\xB0", ImGuiSliderFlags flags = 0);
    bool SliderAngle(const char* label, double* v_rad, double v_rad_min = 0.0, double v_rad_max = std::numbers::pi*2.0, int decimals = 1, ImGuiSliderFlags flags = 0);

    bool RevertableSliderDouble(const char* label, double* v, double* initial, double v_min, double v_max, const char* format = "%.6f", ImGuiSliderFlags flags = 0);
    bool RevertableDragDouble(const char* label, double* v, double* initial, double v_speed, double v_min, double v_max, const char* format = "%.6f", ImGuiSliderFlags flags = 0);
    bool RevertableDragFloat128(const char* label, f128* v, f128* initial, f128 v_speed, f128 v_min, f128 v_max, const char* format = "%.6f", ImGuiSliderFlags flags = 0);
    bool RevertableSliderDouble2(const char* label, double v[2], double initial[2], double v_min, double v_max, const char* format = "%.6f", ImGuiSliderFlags flags = 0);

    bool RevertableSliderAngle(const char* label, double* v_rad, double* initial, double v_rad_min, double v_rad_max, const char* format = "%.1f\xC2\xB0", ImGuiSliderFlags flags = 0);
    bool RevertableSliderAngle(const char* label, double* v_rad, double* initial, double v_rad_min = 0.0, double v_rad_max = std::numbers::pi * 2.0, int decimals = 1, ImGuiSliderFlags flags = 0);

    void BeginPaddedRegion(float padding);
    void EndPaddedRegion();

    inline void IncreaseRequiredSpaceForLabel(float& w, const char* label, float pad_right = bl::scale_size(10.0f))
    {
        float lw = ImGui::CalcTextSize(label).x + pad_right;
        if (lw > w) w = lw;
    }

    inline void SetNextItemWidthForSpace(float w)
    {
        float spacing = ImGui::GetStyle().ItemInnerSpacing.x;
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - w - spacing);
    }

    inline void SetNextItemWidthForLabel(const char* label, float pad_right = bl::scale_size(10.0f))
    {
        float lw = ImGui::CalcTextSize(label).x + pad_right;
        float spacing = ImGui::GetStyle().ItemInnerSpacing.x;
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - lw - spacing);
    }

    struct GroupBox {
        ImDrawList* dl{};
        const char* label{};
        float pad, label_pad_x;
        ImVec2 text_sz{}, content_min{}, content_max{};
        float top_extra{};

        // NEW: capture starting x and the available row width
        ImVec2 start_screen{};   // left/top of the row after the Dummy()
        float  span_w{};         // width of parent content region

        GroupBox(const char* id, const char* label_ = "", float pad_ = bl::scale_size(13.0f), float label_pad_x_ = bl::scale_size(10.0f))
            : label(label_), pad(pad_), label_pad_x(label_pad_x_)
        {
            ImVec2 label_height = ImGui::CalcTextSize(label);
            ImGui::Dummy(ImVec2(0, label_height.y));        // space for title


            // NEW: measure full available width of the current row/column
            start_screen = ImGui::GetCursorScreenPos();
            span_w = ImMax(0.0f, ImGui::GetContentRegionAvail().x);

            ImGui::PushID(id);
            dl = ImGui::GetWindowDrawList();
            text_sz = (label && *label) ? label_height : ImVec2(0, 0);
            top_extra = (text_sz.y > 0.0f) ? text_sz.y * 0.5f : 0.0f;

            dl->ChannelsSplit(2);           // 0 = bg/border, 1 = contents/label
            dl->ChannelsSetCurrent(1);

            ImVec2 p = ImGui::GetCursorScreenPos();
            ImGui::SetCursorScreenPos(ImVec2(p.x + pad, p.y + pad + top_extra)); // inner + title clearance
            ImGui::BeginGroup();
        }

        ~GroupBox() {
            ImGui::EndGroup();
            

            // content bounds (tight)
            content_min = ImGui::GetItemRectMin();
            content_max = ImGui::GetItemRectMax();

            // outer frame
            ImVec2 outer_min = content_min - ImVec2(pad, pad + top_extra);
            ImVec2 outer_max = content_max + ImVec2(pad, pad);

            // NEW: force the frame to span the entire parent width
            outer_min.x = start_screen.x;                     // align left to row start
            outer_max.x = start_screen.x + span_w;            // extend to full available width

            // draw bg/border behind contents
            ImGuiStyle& style = ImGui::GetStyle();
            float rounding = style.FrameRounding;
            float t = style.FrameBorderSize > 0 ? style.FrameBorderSize : 1.0f;
            ImU32 col_bg = ImGui::GetColorU32(ImGuiCol_TitleBg);
            ImU32 col_border = ImGui::GetColorU32(ImGuiCol_Border);
            //ImU32 col_winbg = ImGui::GetColorU32(ImGuiCol_WindowBg);
            ImU32 col_text = ImGui::GetColorU32(ImGuiCol_Text);

            // Draw bg + full border first (channel 0)
            dl->ChannelsSetCurrent(0);
            dl->AddRectFilled(outer_min, outer_max, col_bg, rounding);
            dl->AddRect(outer_min, outer_max, col_border, rounding, 0, t);

            // Foreground: title bg + text (channel 1)
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

            // NEW: advance the layout below the box and reserve the full row width
            ImGui::SetCursorScreenPos(ImVec2(start_screen.x, outer_max.y));
            ImGui::Dummy(ImVec2(span_w, 0.0f));

            ImGui::PopID();

        }

        void IncreaseRequiredSpaceForLabel(float& w, const char* text, float pad_right=bl::scale_size(10.0f))
        {
            float lw = ImGui::CalcTextSize(text).x + pad_right;
            if (lw > w) w = lw;
        }

        void SetNextItemWidthForSpace(float w)
        {
            float spacing = ImGui::GetStyle().ItemInnerSpacing.x;
            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - w - spacing - pad);
        }
    };

}


// Global Initial(&var) helper method for only updating 'var' when 
// updatePointerValues() gets called.
//
// Works by passing a separate placeholder value to ImGui, and copying that data to
// the pointer when updatePointerValues() called

template<typename T>
T* Initial(T* ptr)
{
    if (ImGui::starting_map.count(ptr) == 0)
        ImGui::starting_map[ptr] = { new T(*ptr), sizeof(T) };

    return reinterpret_cast<T*>(ImGui::starting_map[ptr].initial);
}
