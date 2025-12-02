//
//  imgui_color_gradient.cpp
//  imgui extension
//
//  Created by David Gallardo on 11/06/16.
//  - Modified for my own use


#include <bitloop/imguix/imgui_gradient_edit.h>
#include <cstdint>

static const float GRADIENT_BAR_WIDGET_HEIGHT = 25;
static const float GRADIENT_BAR_EDITOR_HEIGHT = 40;
static const float GRADIENT_MARK_DELETE_DIFFY = 40;

int ImGradient::uid_counter = 0;

void ImGradient::clear() noexcept
{
    m_marks.clear();
}

ImGradient::ImGradient(bool empty)
{
    if (!empty)
    {
        addMark(0.0f, ImColor(0.0f, 0.0f, 0.0f));
        addMark(1.0f, ImColor(1.0f, 1.0f, 1.0f));
    }
}

ImGradient& ImGradient::operator=(const ImGradient& rhs)
{
    m_marks = rhs.m_marks;
    dragging_uid = rhs.dragging_uid;
    selected_uid = rhs.selected_uid;
    memcpy(m_cachedValues, rhs.m_cachedValues, sizeof(m_cachedValues));
    memcpy(m_cachedColors, rhs.m_cachedColors, sizeof(m_cachedColors));
    _hash = rhs._hash;

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

void ImGradient::addMark(const ImGradientMark& mark)
{
    m_marks.push_back(mark);
    m_marks.back().uid = uid_counter++;
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

void ImGradient::getColorAtUnguarded(double position, float* color) const
{
    int idx = static_cast<int>(position * (CACHE_SIZE - 1)) * 3;
    color[0] = m_cachedValues[idx + 0];
    color[1] = m_cachedValues[idx + 1];
    color[2] = m_cachedValues[idx + 2];
}

// Serializing
        // 
        // Check if a character is a valid base64 character.
inline bool is_base64(unsigned char c) {
    return (isalnum(c) || (c == '+') || (c == '/'));
}

static const char* base64_chars =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz"
    "0123456789+/";

std::string base64_encode(const unsigned char* data, size_t len) {
    std::string encoded;
    encoded.reserve(((len + 2) / 3) * 4);

    size_t i = 0;
    while (i < len) {
        unsigned char a = data[i++];
        unsigned char b = (i < len ? data[i++] : 0);
        unsigned char c = (i < len ? data[i++] : 0);

        // Pack the three bytes into a 24-bit integer
        unsigned int triple = (a << 16) + (b << 8) + c;

        // Extract four 6-bit values and map them to base64 characters.
        encoded.push_back(base64_chars[(triple >> 18) & 0x3F]);
        encoded.push_back(base64_chars[(triple >> 12) & 0x3F]);
        encoded.push_back((i - 1 < len) ? base64_chars[(triple >> 6) & 0x3F] : '=');
        encoded.push_back((i < len) ? base64_chars[triple & 0x3F] : '=');
    }

    return encoded;
}

static unsigned int indexOf(char c)
{
    const char* pos = std::strchr(base64_chars, c);
    if (!pos)
        throw std::runtime_error("Invalid character in Base64 string");
    return static_cast<unsigned int>(pos - base64_chars);
}

std::vector<unsigned char> base64_decode(const std::string& encoded)
{
    size_t len = encoded.size();
    if (len % 4 != 0)
        throw std::runtime_error("Invalid Base64 string length");

    // Determine the number of padding characters.
    size_t padding = 0;
    if (len) {
        if (encoded[len - 1] == '=') padding++;
        if (encoded[len - 2] == '=') padding++;
    }

    std::vector<unsigned char> decoded;
    decoded.reserve((len / 4) * 3 - padding);

    // Process every 4 characters as a block.
    for (size_t i = 0; i < len; i += 4) {
        unsigned int sextet_a = (encoded[i] == '=') ? 0 : indexOf(encoded[i]);
        unsigned int sextet_b = (encoded[i + 1] == '=') ? 0 : indexOf(encoded[i + 1]);
        unsigned int sextet_c = (encoded[i + 2] == '=') ? 0 : indexOf(encoded[i + 2]);
        unsigned int sextet_d = (encoded[i + 3] == '=') ? 0 : indexOf(encoded[i + 3]);

        // Pack the 4 sextets into a 24-bit integer.
        unsigned int triple = (sextet_a << 18) | (sextet_b << 12) | (sextet_c << 6) | sextet_d;

        // Extract the original bytes from the 24-bit number.
        decoded.push_back((triple >> 16) & 0xFF);
        if (encoded[i + 2] != '=') decoded.push_back((triple >> 8) & 0xFF);
        if (encoded[i + 3] != '=') decoded.push_back(triple & 0xFF);
    }

    return decoded;
}

std::string ImGradient::serialize() const
{
    std::ostringstream oss(std::ios::binary);

    short num_marks = static_cast<short>(m_marks.size());
    oss.write(reinterpret_cast<const char*>(&num_marks), sizeof(short));

    for (short i = 0; i < num_marks; i++)
    {
        const ImGradientMark& mark = m_marks[i];
        float c[3];
        computeColorAt(mark.position, c);

        uint32_t u32 = 0xFF000000 |
            ((uint32_t)(c[0] * 255.0f)) |
            ((uint32_t)(c[1] * 255.0f) << 8) |
            ((uint32_t)(c[2] * 255.0f) << 16);

        oss.write(reinterpret_cast<const char*>(&mark.position), sizeof(float));
        oss.write(reinterpret_cast<const char*>(&u32), sizeof(uint32_t));
    }

    // Convert stream to string
    std::string binaryData = oss.str();

    // Encode as base64
    std::string base64_txt = base64_encode(
        reinterpret_cast<const unsigned char*>(binaryData.data()), binaryData.size());

    return base64_txt;
}

void ImGradient::deserialize(std::string txt)
{
    m_marks.clear();

    std::vector<unsigned char> buffer = base64_decode(txt);
    std::istringstream in(std::string(buffer.begin(), buffer.end()), std::ios::binary);

    // Read num_marks
    short num_marks;
    in.read(reinterpret_cast<char*>(&num_marks), sizeof(short));

    // Read marks
    for (short i = 0; i < num_marks; i++)
    {
        ImGradientMark mark;
        mark.uid = i;
        in.read(reinterpret_cast<char*>(&mark.position), sizeof(float));

        uint32_t u32;
        in.read(reinterpret_cast<char*>(&u32), sizeof(uint32_t));
        mark.color[0] = static_cast<float>(u32 & 0x000000FF) / 255.0f;
        mark.color[1] = static_cast<float>((u32 & 0x0000FF00) >> 8) / 255.0f;
        mark.color[2] = static_cast<float>((u32 & 0x00FF0000) >> 16) / 255.0f;
        mark.color[3] = static_cast<float>((u32 & 0xFF000000) >> 24) / 255.0f;
        m_marks.push_back(mark);
    }

    refreshCache();

    clearSelectedMark();
    clearDraggingMark();
}

bool ImGradient::operator==(const ImGradient& rhs) const
{
    return _hash == rhs._hash;
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
    if (position < 0.0f || position > 1.0f)
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
        if (m.position >= position) {
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
    {
        computeColorAt(i / ((float)(CACHE_SIZE)-1.0f), &m_cachedValues[i * 3]);

        uint32_t& u32 = m_cachedColors[i];

        // ABGR
        //u32 = 0xFFFF0000;
        u32 = 0xFF000000 |
              ((uint32_t)(m_cachedValues[i*3]   * 255.0f)) |
              ((uint32_t)(m_cachedValues[i*3+1] * 255.0f) << 8) |
              ((uint32_t)(m_cachedValues[i*3+2] * 255.0f) << 16);
    }

    updateHash();
}

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

        float h = dpr * GRADIENT_BAR_WIDGET_HEIGHT;
        float maxWidth = ImMax(250.0f* dpr, ImGui::GetContentRegionAvail().x - 100.0f* dpr);
        bool clicked = ImGui::InvisibleButton("gradient_bar", ImVec2(maxWidth, h));

        DrawGradientBar(gradient, widget_pos, maxWidth, h);

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
