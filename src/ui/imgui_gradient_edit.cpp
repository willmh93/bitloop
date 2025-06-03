//
//  imgui_color_gradient.cpp
//  imgui extension
//
//  Created by David Gallardo on 11/06/16.
//  - Modified for my own use


#include "imgui_gradient_edit.h"

static const float GRADIENT_BAR_WIDGET_HEIGHT = 25;
static const float GRADIENT_BAR_EDITOR_HEIGHT = 40;
static const float GRADIENT_MARK_DELETE_DIFFY = 40;

int ImGradient::uid_counter = 0;

void ImGradient::clear() noexcept
{
    m_marks.clear();
}

ImGradient::ImGradient()
{
    addMark(0.0f, ImColor(0.0f, 0.0f, 0.0f));
    addMark(1.0f, ImColor(1.0f, 1.0f, 1.0f));
}

ImGradient& ImGradient::operator=(const ImGradient& rhs)
{
    m_marks = rhs.m_marks;
    dragging_uid = rhs.dragging_uid;
    selected_uid = rhs.selected_uid;
    memcpy(m_cachedValues, rhs.m_cachedValues, sizeof(m_cachedValues));

    return *this;
}

void ImGradient::addMark(float position, ImColor color)
{
    position = ImClamp(position, 0.0f, 1.0f);

    ImGradientMark m;
    m.uid = uid_counter++;
    m.position = position;
    m.color[0] = color.Value.x;
    m.color[1] = color.Value.y;
    m.color[2] = color.Value.z;
    m.color[3] = 1.0f;

    m_marks.push_back(m);
    refreshCache();
}

void ImGradient::removeMark(int uid)
{
    auto it = std::find_if(m_marks.begin(), m_marks.end(), [uid](ImGradientMark& m) {
        return m.uid == uid;
    });

    if (it != m_marks.end())
        m_marks.erase(it);

    refreshCache();
}

void ImGradient::getColorAt(float position, float* color) const
{
    position = ImClamp(position, 0.0f, 1.0f);
    int idx = static_cast<int>(position * (CACHE_SIZE - 1)) * 3;
    color[0] = m_cachedValues[idx + 0];
    color[1] = m_cachedValues[idx + 1];
    color[2] = m_cachedValues[idx + 2];
}

void ImGradient::getColorAt(double position, float* color) const
{
    position = ImClamp(position, 0.0, 1.0);
    int idx = static_cast<int>(position * (CACHE_SIZE - 1)) * 3;
    color[0] = m_cachedValues[idx + 0];
    color[1] = m_cachedValues[idx + 1];
    color[2] = m_cachedValues[idx + 2];
}

bool ImGradient::operator==(const ImGradient& rhs) const
{
    if (m_marks.size() != rhs.m_marks.size()) return false;
    if (dragging_uid != rhs.dragging_uid) return false;
    if (selected_uid != rhs.selected_uid) return false;

    auto it1 = m_marks.begin();
    auto it2 = rhs.m_marks.begin();

    while (it1 != m_marks.end())
    {
        const ImGradientMark& a = *it1;
        const ImGradientMark& b = *it2;

        if (std::abs(a.position - b.position) > kEps) return false;

        for (int i = 0; i < 3; ++i)
            if (std::abs(a.color[i] - b.color[i]) > kEps) return false;

        ++it1; ++it2;
    }
    return true;
}


// Assumes m_marks is kept sorted by .position in ascending order
void ImGradient::computeColorAt(float position, float* color) const
{
    // No marks -> return black
    if (m_marks.empty()) {
        color[0] = color[1] = color[2] = 0.0f;
        return;
    }

    // One mark -> its color
    if (m_marks.size() == 1) {
        const ImGradientMark& m = m_marks.front();
        color[0] = m.color[0];
        color[1] = m.color[1];
        color[2] = m.color[2];
        return;
    }

    // Put position in [0,1)
    position = fmodf(position, 1.0f);
    if (position < 0.0f) position += 1.0f;

    const ImGradientMark& first = m_marks.front();
    const ImGradientMark& last = m_marks.back();

    const ImGradientMark* lower = nullptr; // mark at or before position
    const ImGradientMark* upper = nullptr; // first mark after position

    // Walk once through the list to find the first mark with pos > position
    for (auto it = m_marks.begin(); it != m_marks.end(); ++it) 
    {
        const ImGradientMark& m = *it;
        if (m.position > position) {
            upper = &m;

            // previous mark becomes lower (wrap if we are at begin)
            if (it == m_marks.begin()) {
                lower = &last; // wrap pair: (last, first)
            }
            else {
                auto prev = it;
                --prev;
                lower = &*prev;
            }
            break;
        }
    }

    // If upper not found, position is after the last mark
    if (!upper) {
        lower = &last;
        upper = &first; // wrap pair: (last, first)
    }

    // Interpolate with wrap awareness
    float lowPos = lower->position;
    float upPos = upper->position;
    float p = position;

    // When the pair crosses 1->0, shift upper (and p if needed) by +1
    if (upPos <= lowPos) {
        upPos += 1.0f;
        if (p < lowPos) p += 1.0f;
    }

    float t = (p - lowPos) / (upPos - lowPos);

    for (int i = 0; i < 3; ++i)
        color[i] = (1.0f - t) * lower->color[i] + t * upper->color[i];
}



void ImGradient::refreshCache()
{
    std::sort(m_marks.begin(), m_marks.end(), [](const ImGradientMark& a, const ImGradientMark& b)
    { 
        return a.position < b.position;
    });

    for (int i = 0; i < CACHE_SIZE; ++i)
        computeColorAt(i / ((float)(CACHE_SIZE)-1.0f), &m_cachedValues[i * 3]);
}


/*ImGradient::ImGradient()
{
    addMark(0.0f, ImColor(0.0f, 0.0f, 0.0f));
    addMark(1.0f, ImColor(1.0f, 1.0f, 1.0f));
}

ImGradient::~ImGradient()
{
    for (ImGradientMark* mark : m_marks)
    {
        delete mark;
    }
}

void ImGradient::addMark(float position, ImColor const color)
{
    position = ImClamp(position, 0.0f, 1.0f);
    ImGradientMark* newMark = new ImGradientMark();
    newMark->position = position;
    newMark->color[0] = color.Value.x;
    newMark->color[1] = color.Value.y;
    newMark->color[2] = color.Value.z;

    m_marks.push_back(newMark);

    refreshCache();
}

void ImGradient::removeMark(ImGradientMark* mark)
{
    m_marks.remove(mark);
    refreshCache();
}

void ImGradient::getColorAt(float position, float* color) const
{
    position = ImClamp(position, 0.0f, 1.0f);
    int cachePos = (int)(position * 255.0f);
    cachePos *= 3;
    color[0] = m_cachedValues[cachePos + 0];
    color[1] = m_cachedValues[cachePos + 1];
    color[2] = m_cachedValues[cachePos + 2];
}

void ImGradient::computeColorAt(float position, float* color) const
{
    position = ImClamp(position, 0.0f, 1.0f);

    ImGradientMark* lower = nullptr;
    ImGradientMark* upper = nullptr;

    for (ImGradientMark* mark : m_marks)
    {
        if (mark->position < position)
        {
            if (!lower || lower->position < mark->position)
            {
                lower = mark;
            }
        }

        if (mark->position >= position)
        {
            if (!upper || upper->position > mark->position)
            {
                upper = mark;
            }
        }
    }

    if (upper && !lower)
    {
        lower = upper;
    }
    else if (!upper && lower)
    {
        upper = lower;
    }
    else if (!lower && !upper)
    {
        color[0] = color[1] = color[2] = 0;
        return;
    }

    if (upper == lower)
    {
        color[0] = upper->color[0];
        color[1] = upper->color[1];
        color[2] = upper->color[2];
    }
    else
    {
        float distance = upper->position - lower->position;
        float delta = (position - lower->position) / distance;

        //lerp
        color[0] = ((1.0f - delta) * lower->color[0]) + ((delta)*upper->color[0]);
        color[1] = ((1.0f - delta) * lower->color[1]) + ((delta)*upper->color[1]);
        color[2] = ((1.0f - delta) * lower->color[2]) + ((delta)*upper->color[2]);
    }
}

void ImGradient::refreshCache()
{
    m_marks.sort([](const ImGradientMark* a, const ImGradientMark* b) { return a->position < b->position; });

    for (int i = 0; i < 256; ++i)
    {
        computeColorAt(i / 255.0f, &m_cachedValues[i * 3]);
    }
}*/



namespace ImGui
{
    static void DrawGradientBar(ImGradient* gradient,
        struct ImVec2 const& bar_pos,
        float maxWidth,
        float height)
    {
        ImVec4 colorA = { 1,1,1,1 };
        ImVec4 colorB = { 1,1,1,1 };
        float prevX = bar_pos.x;
        float barBottom = bar_pos.y + height;
        ImGradientMark* prevMark = nullptr;
        ImDrawList* draw_list = ImGui::GetWindowDrawList();

        draw_list->AddRectFilled(ImVec2(bar_pos.x - 2, bar_pos.y - 2),
            ImVec2(bar_pos.x + maxWidth + 2, barBottom + 2),
            IM_COL32(100, 100, 100, 255));

        if (gradient->getMarks().size() == 0)
        {
            draw_list->AddRectFilled(ImVec2(bar_pos.x, bar_pos.y),
                ImVec2(bar_pos.x + maxWidth, barBottom),
                IM_COL32(255, 255, 255, 255));

        }

        ImU32 colorAU32 = 0;
        ImU32 colorBU32 = 0;

        float color_start[4], color_end[4];
        gradient->getColorAt(0.0f, color_start);
        gradient->getColorAt(1.0f, color_end);

        for (auto markIt = gradient->getMarks().begin(); markIt != gradient->getMarks().end(); ++markIt)
        {
            ImGradientMark& mark = *markIt;

            float from = prevX;
            float to = prevX = bar_pos.x + mark.position * maxWidth;

            if (prevMark == nullptr)
            {
                colorA.x = color_start[0]; //mark->color[0];
                colorA.y = color_start[1]; //mark->color[1];
                colorA.z = color_start[2]; //mark->color[2];
            }
            else
            {
                colorA.x = prevMark->color[0];
                colorA.y = prevMark->color[1];
                colorA.z = prevMark->color[2];
            }

            colorB.x = mark.color[0];
            colorB.y = mark.color[1];
            colorB.z = mark.color[2];

            colorAU32 = ImGui::ColorConvertFloat4ToU32(colorA);
            colorBU32 = ImGui::ColorConvertFloat4ToU32(colorB);

            if (mark.position > 0.0)
            {
                draw_list->AddRectFilledMultiColor(ImVec2(from, bar_pos.y),
                    ImVec2(to, barBottom),
                    colorAU32, colorBU32, colorBU32, colorAU32);
            }

            prevMark = &mark;
        }


        colorB.x = color_end[0];
        colorB.y = color_end[1];
        colorB.z = color_end[2];

        colorAU32 = colorBU32;
        colorBU32 = ImGui::ColorConvertFloat4ToU32(colorB);

        if (prevMark && prevMark->position < 1.0)
        {
            draw_list->AddRectFilledMultiColor(ImVec2(prevX, bar_pos.y),
                ImVec2(bar_pos.x + maxWidth, barBottom),
                colorAU32, colorBU32, colorBU32, colorAU32);
                //colorBU32, colorBU32, colorBU32, colorBU32);
        }

        ImGui::SetCursorScreenPos(ImVec2(bar_pos.x, bar_pos.y + height + 10.0f));
    }

    static void DrawGradientMarks(ImGradient* gradient,
        struct ImVec2 const& bar_pos,
        float maxWidth,
        float height,
        float mark_scale)
    {
        ImVec4 colorA = { 1,1,1,1 };
        ImVec4 colorB = { 1,1,1,1 };
        float barBottom = bar_pos.y + height;
        ImGradientMark* prevMark = nullptr;
        ImDrawList* draw_list = ImGui::GetWindowDrawList();
        ImU32 colorAU32 = 0;
        ImU32 colorBU32 = 0;

        int i = 0;
        for (auto markIt = gradient->getMarks().begin(); markIt != gradient->getMarks().end(); ++markIt)
        {
            ImGradientMark& mark = *markIt;

            //if (!selectedMark)
            if (!gradient->hasSelectedMark())
            {
                gradient->setSelectedMark(mark);
            }

            float to = bar_pos.x + mark.position * maxWidth;

            if (prevMark == nullptr)
            {
                colorA.x = mark.color[0];
                colorA.y = mark.color[1];
                colorA.z = mark.color[2];
            }
            else
            {
                colorA.x = prevMark->color[0];
                colorA.y = prevMark->color[1];
                colorA.z = prevMark->color[2];
            }

            colorB.x = mark.color[0];
            colorB.y = mark.color[1];
            colorB.z = mark.color[2];

            colorAU32 = ImGui::ColorConvertFloat4ToU32(colorA);
            colorBU32 = ImGui::ColorConvertFloat4ToU32(colorB);

            draw_list->AddTriangleFilled(
                ImVec2(to, bar_pos.y + (height - 6.0f * mark_scale)),
                ImVec2(to - 6.0f * mark_scale, barBottom),
                ImVec2(to + 6.0f * mark_scale, barBottom), 
                IM_COL32(100, 100, 100, 255));

            draw_list->AddRectFilled(
                ImVec2(to - 6.0f * mark_scale, barBottom),
                ImVec2(to + 6.0f * mark_scale, bar_pos.y + (height + (12.0f * mark_scale))),
                IM_COL32(100, 100, 100, 255), 1.0f);

            draw_list->AddRectFilled(
                ImVec2(to - 5.0f * mark_scale, bar_pos.y + (height + 1)),
                ImVec2(to + 5.0f * mark_scale, bar_pos.y + (height + (11.0f * mark_scale))),
                IM_COL32(0, 0, 0, 255), 1.0f);

            //if (selectedMark == &mark)
            if (gradient->selectedMarkUID() == mark.uid)
            {
                draw_list->AddTriangleFilled(
                    ImVec2(to, bar_pos.y + (height - 3.0f * mark_scale)),
                    ImVec2(to - 4.0f * mark_scale, barBottom + 1),
                    ImVec2(to + 4.0f * mark_scale, barBottom + 1), 
                    IM_COL32(0, 255, 0, 255));

                draw_list->AddRect(
                    ImVec2(to - 5.0f * mark_scale, bar_pos.y + (height + 1)),
                    ImVec2(to + 5.0f * mark_scale, bar_pos.y + (height + 11.0f * mark_scale)),
                    IM_COL32(0, 255, 0, 255), 1.0f);
            }

            draw_list->AddRectFilledMultiColor(
                ImVec2(to - 3.0f * mark_scale, bar_pos.y + (height + (3.0f * mark_scale))),
                ImVec2(to + 3.0f * mark_scale, bar_pos.y + (height + (9.0f * mark_scale))),
                colorBU32, colorBU32, colorBU32, colorBU32);

            ImGui::SetCursorScreenPos(ImVec2(to - 6.0f * mark_scale, barBottom));

            char mark_id[16];
            ImFormatString(mark_id, 16, "mark_%d", i);
            ImGui::InvisibleButton(mark_id, ImVec2(12.0f * mark_scale, 12.0f * mark_scale));

            i++;

            if (ImGui::IsItemHovered())
            {
                if (ImGui::IsMouseClicked(0))
                {
                    //selectedMark = mark;
                    //draggingMark = mark;
                    gradient->setSelectedMark(mark);
                    gradient->setDraggingMark(mark);
                }
            }


            prevMark = &mark;
        }

        ImGui::SetCursorScreenPos(ImVec2(bar_pos.x, bar_pos.y + height + 20.0f * mark_scale));
    }

    bool GradientButton(ImGradient* gradient, float dpr)
    {
        if (!gradient) return false;

        ImVec2 widget_pos = ImGui::GetCursorScreenPos();
        // ImDrawList* draw_list = ImGui::GetWindowDrawList();

        float maxWidth = ImMax(250.0f* dpr, ImGui::GetContentRegionAvail().x - 100.0f* dpr);
        bool clicked = ImGui::InvisibleButton("gradient_bar", ImVec2(maxWidth, GRADIENT_BAR_WIDGET_HEIGHT));

        DrawGradientBar(gradient, widget_pos, maxWidth, GRADIENT_BAR_WIDGET_HEIGHT);

        return clicked;
    }

    bool GradientEditor(
        ImGradient* gradient,
        float bar_scale,
        float mark_scale)
    {
        if (!gradient) return false;

        bool modified = false;

        ImVec2 bar_pos = ImGui::GetCursorScreenPos();
        bar_pos.x += 10 * bar_scale;
        float maxWidth = ImGui::GetContentRegionAvail().x - 20 * bar_scale;
        float barBottom = bar_pos.y + GRADIENT_BAR_EDITOR_HEIGHT * bar_scale;

        if (maxWidth < 100)
            maxWidth = 100;

        ImGui::InvisibleButton("gradient_editor_bar", ImVec2(maxWidth, GRADIENT_BAR_EDITOR_HEIGHT * bar_scale));

        if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(0))
        {
            float pos = (ImGui::GetIO().MousePos.x - bar_pos.x) / maxWidth;

            float newMarkCol[4];
            gradient->getColorAt(pos, newMarkCol);


            gradient->addMark(pos, ImColor(newMarkCol[0], newMarkCol[1], newMarkCol[2]));
        }

        DrawGradientBar(gradient, bar_pos, maxWidth, GRADIENT_BAR_EDITOR_HEIGHT* bar_scale);
        DrawGradientMarks(gradient, bar_pos, maxWidth, GRADIENT_BAR_EDITOR_HEIGHT* bar_scale, mark_scale);

        if (!ImGui::IsMouseDown(0) && gradient->hasDraggingMark())
        {
            gradient->clearDraggingMark();
        }

        if (ImGui::IsMouseDragging(0) && gradient->hasDraggingMark())
        {
            float increment = ImGui::GetIO().MouseDelta.x / maxWidth;
            bool insideZone = (ImGui::GetIO().MousePos.x > bar_pos.x) &&
                (ImGui::GetIO().MousePos.x < bar_pos.x + maxWidth);

            ImGradientMark* draggingMark = gradient->getDraggingMark();

            if (increment != 0.0f && insideZone)
            {
                draggingMark->position += increment;
                draggingMark->position = ImClamp(draggingMark->position, 0.0f, 1.0f);
                gradient->refreshCache();
                modified = true;
            }

            float diffY = ImGui::GetIO().MousePos.y - barBottom;

            if (diffY >= GRADIENT_MARK_DELETE_DIFFY * bar_scale)
            {
                gradient->removeMark(draggingMark->uid);
                gradient->clearDraggingMark();
                gradient->clearSelectedMark();
                //draggingMark = nullptr;
                //selectedMark = nullptr;
                modified = true;
            }
        }

        if (!gradient->hasSelectedMark() && gradient->getMarks().size() > 0)
        {
            gradient->setSelectedMark(gradient->getMarks().front());
            //selectedMark = &gradient->getMarks().front();
        }

        if (gradient->hasSelectedMark())
        {
            ImGradientMark* selectedMark = gradient->getSelectedMark();
            bool colorModified = ImGui::ColorPicker3("Color", selectedMark->color, 
                ImGuiColorEditFlags_NoLabel | ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoSidePreview);
            //bool colorModified = ImGui::ColorPicker3(selectedMark-color);

            if (selectedMark && colorModified)
            {
                modified = true;
                gradient->refreshCache();
            }
        }

        return modified;
    }
};
