#include "imgui_custom.h"



void DrawSymbol(ImDrawList* drawList, ImVec2 pos, ImVec2 size, const char* symbol, ImU32 color) 
{
    float cx = pos.x + size.x * 0.5f;
    float cy = pos.y + size.y * 0.5f;
    float r = ImMin(size.x, size.y) * 0.25f;

    if (strcmp(symbol, "play") == 0) {
        ImVec2 p1(cx - r * 0.6f, cy - r);
        ImVec2 p2(cx - r * 0.6f, cy + r);
        ImVec2 p3(cx + r, cy);
        drawList->AddTriangleFilled(p1, p2, p3, color);
    }
    else if (strcmp(symbol, "stop") == 0) {
        drawList->AddRectFilled(ImVec2(cx - r, cy - r), ImVec2(cx + r, cy + r), color);
    }
    else if (strcmp(symbol, "pause") == 0) {
        float w = r * 0.4f;
        drawList->AddRectFilled(ImVec2(cx - r, cy - r), ImVec2(cx - r + w, cy + r), color);
        drawList->AddRectFilled(ImVec2(cx + r - w, cy - r), ImVec2(cx + r, cy + r), color);
    }
    else if (strcmp(symbol, "record") == 0) {
        drawList->AddCircleFilled(ImVec2(cx, cy), r, color);
    }
}



namespace ImGui
{
    IMGUI_API bool SliderDouble(const char* label, double* v, double v_min, double v_max, const char* format, ImGuiSliderFlags flags)
    {
        return SliderScalar(label, ImGuiDataType_Double, v, &v_min, &v_max, format, flags);
    }

    IMGUI_API bool DragDouble(const char* label, double* v, double v_speed, double v_min, double v_max, const char* format, ImGuiSliderFlags flags)
    {
        return DragScalar(label, ImGuiDataType_Double, v, v_speed, &v_min, &v_max, format, flags);
    }

    IMGUI_API bool SliderDouble2(const char* label, double v[2], double v_min, double v_max, const char* format, ImGuiSliderFlags flags)
    {
        return SliderScalarN(label, ImGuiDataType_Double, v, 2, &v_min, &v_max, format, flags);
    }

    IMGUI_API bool DragDouble2(const char* label, double v[2], double v_speed, double v_min, double v_max, const char* format, ImGuiSliderFlags flags)
    {
        return DragScalarN(label, ImGuiDataType_Double, v, 2, v_speed, &v_min, &v_max, format, flags);
    }

    bool SymbolButton(const char* id, const char* symbol, const ToolbarButtonState& state, ImVec2 size) 
    {
        ImGui::PushStyleColor(ImGuiCol_Button, state.bgColor);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, state.bgColor);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, state.bgColor);
        bool pressed = ImGui::Button(id, size);
        ImGui::PopStyleColor(3);

        ImDrawList* drawList = ImGui::GetWindowDrawList();
        ImVec2 p = ImGui::GetItemRectMin();
        ImVec2 sz = ImGui::GetItemRectSize();
        DrawSymbol(drawList, p, sz, symbol, ImGui::ColorConvertFloat4ToU32(state.symbolColor));

        return pressed;
    }

    void RenderToolbar(
        const ToolbarButtonState& playState,
        const ToolbarButtonState& stopState,
        const ToolbarButtonState& pauseState,
        const ToolbarButtonState& recordState,
        float size) 
    {
        constexpr float icon_padding = 10;
        //constexpr float toolbar_padding = 0;

        //ImGui::Dummy(ImVec2(0, toolbar_padding));

        ImVec2 avail = GetContentRegionAvail();
        ImVec2 toolbarSize(avail.x, size + icon_padding*2); // Outer frame size
        ImVec4 frameColor = ImVec4(0.15f, 0.15f, 0.15f, 1.0f); // Toolbar background color


        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(icon_padding, icon_padding));
        ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 0.0f);
        ImGui::PushStyleColor(ImGuiCol_ChildBg, frameColor);

        ImGui::BeginChild("ToolbarFrame", toolbarSize, true, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

        

        // Layout the buttons inside the frame
        SymbolButton("##play", "play", playState, ImVec2(size, size));
        ImGui::SameLine();
        SymbolButton("##stop", "stop", stopState, ImVec2(size, size));
        ImGui::SameLine();
        SymbolButton("##pause", "pause", pauseState, ImVec2(size, size));
        ImGui::SameLine();
        SymbolButton("##record", "record", recordState, ImVec2(size, size));

        ImGui::EndChild();

        ImGui::PopStyleColor();
        ImGui::PopStyleVar(2);

        //ImGui::Dummy(ImVec2(0, toolbar_padding));
    }
}