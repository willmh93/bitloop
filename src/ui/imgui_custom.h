#pragma once
#include <unordered_map>
#include <numbers>
#include <algorithm>
#include <stdexcept>
#include <functional>

constexpr double pi = std::numbers::pi;

#define IMGUI_ENABLE_FREETYPE
#define IMGUI_DEFINE_MATH_OPERATORS

#include "platform.h"

#include "imgui_internal.h"
#include "misc/freetype/imgui_freetype.h"

// Custom components
#include "imgui_splines.h"
#include "imgui_gradient_edit.h"
#include "imgui_log.h"
#include "imgui_debug_ui.h"

inline ImVec2 ScaleSize(const ImVec2& size)
{
    float dpr = PlatformManager::get()->dpr();
    return ImVec2(size.x * dpr, size.y * dpr);
}

inline ImVec2 ScaleSize(int w, int h)
{
    float dpr = PlatformManager::get()->dpr();
    return ImVec2((float)w * dpr, (float)h * dpr);
}

inline ImVec2 ScaleSize(float w, float h)
{
    float dpr = PlatformManager::get()->dpr();
    return ImVec2(w * dpr, h * dpr);
}

namespace ImGui
{
    struct StartValue { void* initial; int size; };
    static std::unordered_map<void*, StartValue> starting_map;
    static void updatePointerValues()
    {
        for (auto& item : starting_map)
            memcpy(item.first, item.second.initial, item.second.size);
    }

    IMGUI_API bool SliderDouble(const char* label, double* v, double v_min, double v_max, const char* format = "%.6f", ImGuiSliderFlags flags = 0);     // adjust format to decorate the value with a prefix or a suffix for in-slider labels or unit display.
    IMGUI_API bool DragDouble(const char* label, double* v, double v_speed = 1.0, double v_min = 0.0, double v_max = 0.0, const char* format = "%.6f", ImGuiSliderFlags flags = 0);     // If v_min >= v_max we have no bound
    IMGUI_API bool SliderDouble2(const char* label, double v[2], double v_min, double v_max, const char* format = "%.6f", ImGuiSliderFlags flags = 0);
    IMGUI_API bool DragDouble2(const char* label, double v[2], double v_speed = 1.0f, double v_min = 0.0f, double v_max = 0.0f, const char* format = "%.6f", ImGuiSliderFlags flags = 0);
    
    void BeginPaddedRegion(float padding);
    void EndPaddedRegion();
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