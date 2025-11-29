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

    bool Section(const char* name, bool open_by_default = false, float header_spacing=2.0f, float body_margin_top=2.0f);

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
    bool SliderAngle(const char* label, float* v_rad, float v_rad_min = 0.0, float v_rad_max = (float)std::numbers::pi * 2.0, int decimals = 1, ImGuiSliderFlags flags = 0);

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

    // Max nesting depth per window.
    static constexpr int kMaxDepth = 12;

    enum class BoxKind : unsigned char { Header, Tab, Labelled };

    struct CurrentBoxState
    {
        BoxKind kind{};
        ImDrawList* dl{};
        float pad{};
        float extra{};

        ImVec2 header_min{}, header_max{};
        ImVec2 start_cursor{};
        float  span_w{};

        float old_work_rect_max_x{};
        float old_content_rect_max_x{};

        int depth{};
        int content_ch{};

        // ---- Labelled box extras ----
        const char* label{};
        float label_pad_x{};
        ImVec2 text_sz{};
        float top_extra{};
    };

    inline std::vector<CurrentBoxState>& box_stack()
    {
        static thread_local std::vector<CurrentBoxState> s;
        return s;
    }

    struct DlCtx { int depth{}; int content_ch{}; bool split{}; };

    inline std::unordered_map<ImDrawList*, DlCtx>& box_ctx_map()
    {
        static thread_local std::unordered_map<ImDrawList*, DlCtx> m;
        return m;
    }

    inline void box_new_frame_guard()
    {
        static thread_local int last_frame = -1;
        const int frame = ImGui::GetFrameCount();
        if (frame != last_frame)
        {
            last_frame = frame;
            box_ctx_map().clear();

            // If you hit this, you're missing an End*Box() somewhere.
            IM_ASSERT(box_stack().empty() && "Unbalanced HeaderBox/TabBox (missing End...)");
            box_stack().clear();
        }
    }

    inline DlCtx& box_dlctx(ImDrawList* dl)
    {
        box_new_frame_guard();
        return box_ctx_map()[dl];
    }

    inline void box_ensure_split(ImDrawList* dl, DlCtx& c)
    {
        if (c.depth == 0 && !c.split)
        {
            const int channels = kMaxDepth + 1; // [0..kMaxDepth-1]=bg layers, [kMaxDepth]=content
            c.content_ch = kMaxDepth;
            dl->ChannelsSplit(channels);
            dl->ChannelsSetCurrent(c.content_ch);
            c.split = true;
        }
        else if (c.split)
        {
            dl->ChannelsSetCurrent(c.content_ch);
        }
    }

    inline void box_maybe_merge(ImDrawList* dl, DlCtx& c)
    {
        if (c.depth == 0 && c.split)
        {
            dl->ChannelsMerge();
            c.split = false;
        }
    }

    inline void box_begin_contents(CurrentBoxState& st)
    {
        // Move cursor into the inner padded area of the box
        ImGui::SetCursorScreenPos(ImVec2(st.start_cursor.x + st.pad,
            st.start_cursor.y + st.pad));

        // Shrink the current window's available width so everything inside
        // sees a reduced ContentRegionRect / WorkRect (right padding).
        ImGuiWindow* window = ImGui::GetCurrentWindow();
        st.old_work_rect_max_x = window->WorkRect.Max.x;
        st.old_content_rect_max_x = window->ContentRegionRect.Max.x;

        window->WorkRect.Max.x -= st.pad;
        window->ContentRegionRect.Max.x -= st.pad;

        ImGui::BeginGroup();
    }

    inline void box_draw_bg_border(const CurrentBoxState& st, const ImVec2& outer_min, const ImVec2& outer_max)
    {
        ImDrawList* dl = st.dl;

        ImGuiStyle& style = ImGui::GetStyle();
        const float rounding = style.FrameRounding;
        const float t = (style.FrameBorderSize > 0.0f) ? style.FrameBorderSize : 1.0f;

        const ImU32 col_bg = ImGui::GetColorU32(ImGuiCol_TitleBg);
        const ImU32 col_border = ImGui::GetColorU32(ImGuiCol_Border);
        const ImDrawFlags rf = ImDrawFlags_RoundCornersBottomLeft | ImDrawFlags_RoundCornersBottomRight;

        const int bg_ch = (st.depth < kMaxDepth) ? st.depth : (kMaxDepth - 1);

        // Allow drawing a little into window padding (your extra)
        const ImVec2 win_min = ImGui::GetWindowPos();
        const ImVec2 win_max = win_min + ImGui::GetWindowSize();

        dl->PushClipRect(win_min, win_max, true);
        dl->ChannelsSetCurrent(bg_ch);
        dl->AddRectFilled(outer_min, outer_max, col_bg, rounding, rf);
        dl->AddRect(outer_min, outer_max, col_border, rounding, rf, t);
        dl->PopClipRect();

        dl->ChannelsSetCurrent(st.content_ch);
    }

    // -------------------------
    // CollapsingHeaderBox / EndCollapsingHeaderBox
    // -------------------------

    inline bool CollapsingHeaderBox(const char* id, bool open_by_default, float pad = bl::scale_size(10.0f), float extra = 3.0f)
    {
        ImDrawList* dl = ImGui::GetWindowDrawList();
        DlCtx& c = box_dlctx(dl);
        box_ensure_split(dl, c);

        const int my_depth = c.depth;
        c.depth++;

        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
        const bool open = ImGui::CollapsingHeader(id, open_by_default ? ImGuiTreeNodeFlags_DefaultOpen : 0);
        ImGui::PopStyleVar();

        const ImVec2 hmin = ImGui::GetItemRectMin();
        const ImVec2 hmax = ImGui::GetItemRectMax();

        float spacing = 0.5f;//  ImGui::GetStyle().ItemSpacing.y;
        ImGui::Dummy(ImVec2(0, spacing));

        if (!open)
        {
            c.depth--;
            box_maybe_merge(dl, c);
            return false;
        }

        CurrentBoxState st{};
        st.kind = BoxKind::Header;
        st.dl = dl;
        st.pad = pad;
        st.extra = extra;
        st.depth = my_depth;
        st.content_ch = c.content_ch;
        st.header_min = hmin;
        st.header_max = hmax;

        st.start_cursor = ImGui::GetCursorScreenPos();
        st.span_w = ImMax(0.0f, ImGui::GetContentRegionAvail().x + st.extra*2);

        box_stack().push_back(st);

        ImGui::PushID(id);
        box_begin_contents(box_stack().back());
        return true;
    }

    inline void EndCollapsingHeaderBox(float end_spacing=4.0f)
    {
        auto& s = box_stack();
        IM_ASSERT(!s.empty() && s.back().kind == BoxKind::Header);

        //ImGui::Dummy(ImVec2(0, bl::scale_size(1.0f)));
        ImGui::Dummy(ImVec2(0, 1.0f));

        CurrentBoxState st = s.back();
        s.pop_back();

        ImGui::EndGroup();

        // tight content bounds (group)
        const ImVec2 content_max = ImGui::GetItemRectMax();

        // RESTORE window rects so subsequent items use full width again
        {
            ImGuiWindow* window = ImGui::GetCurrentWindow();
            window->WorkRect.Max.x = st.old_work_rect_max_x;
            window->ContentRegionRect.Max.x = st.old_content_rect_max_x;
        }


        // outer frame (your old "midline of header" start)
        const float y0 = (st.header_min.y + st.header_max.y) * 0.5f;
        const float x0 = st.start_cursor.x - st.extra;
        const float x1 = x0 + st.span_w;

        ImVec2 outer_min(x0, y0);
        ImVec2 outer_max(x1, content_max.y + st.pad);

        // draw behind content
        box_draw_bg_border(st, outer_min, outer_max);

        // advance cursor below the box and restore X to normal content start
        ImGui::SetCursorScreenPos(ImVec2(st.start_cursor.x, outer_max.y));
        ImGui::Dummy(ImVec2(st.span_w, bl::scale_size(end_spacing)));

        ImGui::PopID();

        DlCtx& c = box_dlctx(st.dl);
        c.depth--;
        box_maybe_merge(st.dl, c);

        
    }

    // ---------------------
    // TabBox / EndTabBox
    // ---------------------

    inline bool TabBox(const char* id, float pad = bl::scale_size(10.0f), float extra = 3.0f)
    {
        ImDrawList* dl = ImGui::GetWindowDrawList();
        DlCtx& c = box_dlctx(dl);
        box_ensure_split(dl, c);

        const int my_depth = c.depth;
        c.depth++;

        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
        const bool open = ImGui::BeginTabItem(id);
        ImGui::PopStyleVar();

        const ImVec2 hmin = ImGui::GetItemRectMin();
        const ImVec2 hmax = ImGui::GetItemRectMax();

        if (!open)
        {
            c.depth--;
            box_maybe_merge(dl, c);
            return false;
        }

        CurrentBoxState st{};
        st.kind = BoxKind::Tab;
        st.dl = dl;
        st.pad = pad;
        st.extra = extra;
        st.depth = my_depth;
        st.content_ch = c.content_ch;
        st.header_min = hmin;
        st.header_max = hmax;

        st.start_cursor = ImGui::GetCursorScreenPos();
        st.span_w = ImMax(0.0f, ImGui::GetContentRegionAvail().x + st.extra * 2);

        box_stack().push_back(st);

        ImGui::PushID(id);
        box_begin_contents(box_stack().back());
        return true;
    }

    inline void EndTabBox()
    {
        auto& s = box_stack();
        IM_ASSERT(!s.empty() && s.back().kind == BoxKind::Tab);

        CurrentBoxState st = s.back();
        s.pop_back();

        ImGui::Dummy(ImVec2(0, 1.0f));

        ImGui::EndGroup();

        const ImVec2 content_min = ImGui::GetItemRectMin();
        const ImVec2 content_max = ImGui::GetItemRectMax();

        // RESTORE window rects
        {
            ImGuiWindow* window = ImGui::GetCurrentWindow();
            window->WorkRect.Max.x = st.old_work_rect_max_x;
            window->ContentRegionRect.Max.x = st.old_content_rect_max_x;
        }

        // Finish the tab item AFTER we grabbed the group rect (otherwise "last item" changes)
        ImGui::EndTabItem();

        const float x0 = st.start_cursor.x - st.extra;
        const float x1 = x0 + st.span_w;

        ImVec2 outer_min(x0, content_min.y - st.pad);
        ImVec2 outer_max(x1, content_max.y + st.pad);

        box_draw_bg_border(st, outer_min, outer_max);

        ImGui::SetCursorScreenPos(ImVec2(st.start_cursor.x, outer_max.y));
        ImGui::Dummy(ImVec2(st.span_w, 0.0f));

        ImGui::PopID();

        DlCtx& c = box_dlctx(st.dl);
        c.depth--;
        box_maybe_merge(st.dl, c);
    }

    // ---------------------
    // LabelledBox / EndLabelledBox
    // ---------------------

    inline void BeginLabelledBox(const char* label = nullptr,
        float pad = bl::scale_size(13.0f),
        float label_pad_x = bl::scale_size(10.0f))
    {
        ImDrawList* dl = ImGui::GetWindowDrawList();
        DlCtx& c = box_dlctx(dl);
        box_ensure_split(dl, c);

        const int my_depth = c.depth;
        c.depth++;

        // Measure label height and reserve vertical space for the title.
        ImVec2 label_sz = (label && *label) ? ImGui::CalcTextSize(label) : ImVec2(0.0f, 0.0f);
        if (label && *label)
            ImGui::Dummy(ImVec2(0.0f, label_sz.y)); // space above frame for the title

        // Full available width at this point (inside parent box / window).
        ImVec2 start_screen = ImGui::GetCursorScreenPos();
        float span_w = ImMax(0.0f, ImGui::GetContentRegionAvail().x);

        CurrentBoxState st{};
        st.kind = BoxKind::Labelled;
        st.dl = dl;
        st.pad = pad;
        st.extra = 0.0f;
        st.depth = my_depth;
        st.content_ch = c.content_ch;
        st.start_cursor = start_screen;
        st.span_w = span_w;

        st.label = label;
        st.label_pad_x = label_pad_x;
        st.text_sz = label_sz;
        st.top_extra = (label_sz.y > 0.0f) ? (label_sz.y * 0.5f) : 0.0f;

        // Clamp inner width while we're inside this labelled box.
        {
            ImGuiWindow* window = ImGui::GetCurrentWindow();
            st.old_work_rect_max_x = window->WorkRect.Max.x;
            st.old_content_rect_max_x = window->ContentRegionRect.Max.x;

            window->WorkRect.Max.x -= pad;
            window->ContentRegionRect.Max.x -= pad;
        }

        box_stack().push_back(st);

        ImGui::PushID(label ? label : "LabelledBox");

        // Move cursor inside the box (inner padding + title offset) and begin a group.
        ImVec2 p = ImGui::GetCursorScreenPos();
        ImGui::SetCursorScreenPos(ImVec2(p.x + pad, p.y + pad + st.top_extra));
        ImGui::BeginGroup();
    }

    inline void EndLabelledBox()
    {
        auto& s = box_stack();
        IM_ASSERT(!s.empty() && s.back().kind == BoxKind::Labelled);

        CurrentBoxState st = s.back();
        s.pop_back();

        ImGui::EndGroup();

        // Tight content bounds of the inner group.
        ImVec2 content_min = ImGui::GetItemRectMin();
        ImVec2 content_max = ImGui::GetItemRectMax();

        // Outer frame around content.
        ImVec2 outer_min = content_min - ImVec2(st.pad, st.pad + st.top_extra);
        ImVec2 outer_max = content_max + ImVec2(st.pad, st.pad);

        // Force frame to span the entire parent width.
        outer_min.x = st.start_cursor.x;
        outer_max.x = st.start_cursor.x + st.span_w;

        // Restore window rects so siblings see the original available width.
        {
            ImGuiWindow* window = ImGui::GetCurrentWindow();
            window->WorkRect.Max.x = st.old_work_rect_max_x;
            window->ContentRegionRect.Max.x = st.old_content_rect_max_x;
        }

        ImGuiStyle& style = ImGui::GetStyle();
        float rounding = style.FrameRounding;
        float t = (style.FrameBorderSize > 0.0f) ? style.FrameBorderSize : 1.0f;
        ImU32 col_bg = ImGui::GetColorU32(ImGuiCol_TitleBg);
        ImU32 col_border = ImGui::GetColorU32(ImGuiCol_Border);
        ImU32 col_text = ImGui::GetColorU32(ImGuiCol_Text);

        ImDrawList* dl = st.dl;

        // Draw frame in background channel for this depth.
        const int bg_ch = (st.depth < kMaxDepth) ? st.depth : (kMaxDepth - 1);
        dl->ChannelsSetCurrent(bg_ch);
        dl->AddRectFilled(outer_min, outer_max, col_bg, rounding);
        dl->AddRect(outer_min, outer_max, col_border, rounding, 0, t);

        // Label pill and text in the shared content channel.
        dl->ChannelsSetCurrent(st.content_ch);
        if (st.label && *st.label)
        {
            float x_text = outer_min.x + st.pad + st.label_pad_x;
            float y_line = outer_min.y; // top border y
            float text_margin_x = 8.0f;
            float text_margin_y = 4.0f;

            ImVec2 label_min(
                x_text - text_margin_x,
                y_line - st.text_sz.y * 0.5f - t - text_margin_y
            );
            ImVec2 label_max(
                x_text + st.text_sz.x + text_margin_x,
                y_line + st.text_sz.y * 0.5f + t + text_margin_y
            );

            dl->AddRectFilled(label_min, label_max, col_bg);
            dl->AddRect(label_min, label_max, col_border);
            dl->AddText(ImVec2(x_text, y_line - st.text_sz.y * 0.5f), col_text, st.label);
        }

        // Advance cursor below the frame.
        ImGui::SetCursorScreenPos(ImVec2(st.start_cursor.x, outer_max.y));
        ImGui::Dummy(ImVec2(st.span_w, 0.0f));

        ImGui::PopID();

        DlCtx& c = box_dlctx(st.dl);
        c.depth--;
        box_maybe_merge(st.dl, c);
    }
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
