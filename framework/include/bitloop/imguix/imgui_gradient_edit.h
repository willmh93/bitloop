//
//  imgui_color_gradient.h
//  imgui extension
//
//  Created by David Gallardo on 11/06/16.

/*

 Usage:

 ::GRADIENT DATA::
 ImGradient gradient;

 ::BUTTON::
 if(ImGui::GradientButton(&gradient))
 {
    //set show editor flag to true/false
 }

 ::EDITOR::
 static ImGradientMark* draggingMark = nullptr;
 static ImGradientMark* selectedMark = nullptr;

 bool updated = ImGui::GradientEditor(&gradient, draggingMark, selectedMark);

 ::GET A COLOR::
 float color[3];
 gradient.getColorAt(0.3f, color); //position from 0 to 1

 ::MODIFY GRADIENT WITH CODE::
 gradient.getMarks().clear();
 gradient.addMark(0.0f, ImColor(0.2f, 0.1f, 0.0f));
 gradient.addMark(0.7f, ImColor(120, 200, 255));

 ::WOOD BROWNS PRESET::
 gradient.getMarks().clear();
 gradient.addMark(0.0f, ImColor(0xA0, 0x79, 0x3D));
 gradient.addMark(0.2f, ImColor(0xAA, 0x83, 0x47));
 gradient.addMark(0.3f, ImColor(0xB4, 0x8D, 0x51));
 gradient.addMark(0.4f, ImColor(0xBE, 0x97, 0x5B));
 gradient.addMark(0.6f, ImColor(0xC8, 0xA1, 0x65));
 gradient.addMark(0.7f, ImColor(0xD2, 0xAB, 0x6F));
 gradient.addMark(0.8f, ImColor(0xDC, 0xB5, 0x79));
 gradient.addMark(1.0f, ImColor(0xE6, 0xBF, 0x83));

 */

#pragma once

#include "imgui.h"
#include "imgui_internal.h"
//#include "types.h"

#include <vector>
#include <algorithm>
#include <cstring>
#include <string>
#include <sstream>
#include <utility>
#include <cstdint>

struct ImGradientMark
{
    int uid{};
    float color[4]{};
    float position{};
};


class ImGradient
{
public:
    ImGradient(bool empty=false);
    ImGradient& operator=(const ImGradient& rhs); // copy-assign

    /* API used by the editor ------------------------------------------------ */
    void   getColorAt(float position, float* color) const;
    void   getColorAt(double position, float* color) const;
    void   getColorAtUnguarded(double position, float* color) const;
    void   addMark(float position, ImColor color);
    void   addMark(const ImGradientMark& mark);
    void   removeMark(int uid);
    void   refreshCache();
    std::vector<ImGradientMark>& getMarks() { return m_marks; }
    const std::vector<ImGradientMark>& getMarks() const { return m_marks; }

    ImGradientMark* getSelectedMark() { return markFromUID(selected_uid); }
    void  setSelectedMark(const ImGradientMark &m) { selected_uid = m.uid; }
    int  selectedMarkUID() { return selected_uid; }
    bool hasSelectedMark() { return selected_uid >= 0 && getSelectedMark(); }
    void clearSelectedMark() { selected_uid = -1; }

    ImGradientMark* getDraggingMark() { return markFromUID(dragging_uid); }
    void  setDraggingMark(const ImGradientMark& m) { dragging_uid = m.uid; }
    int  draggingMarkUID() { return dragging_uid; }
    bool hasDraggingMark() { return dragging_uid >= 0 && getDraggingMark(); }
    void clearDraggingMark() { dragging_uid = -1; }

    std::string serialize() const;
    void deserialize(std::string txt);

    /* Comparison ------------------------------------------------------------ */
    bool operator==(const ImGradient& rhs) const;
    bool operator!=(const ImGradient& rhs) const { return !(*this == rhs); }

    /* Transformations ------------------------------------------------------- */

    static void lerp(ImGradient& out, const ImGradient& a, const ImGradient& b, float x)
    {
        // Return gradient with same number of marks as 'b'
        // If 'b' > marks than 'a': Find nearest mark to 'a' and split it up
        // If 'b' < marks than 'a': Find nearest mark to 'a' and merge there

        if (x <= 0.0f) { out = a; return; };
        if (x >= 1.0f) { out = b; return; };

        out.m_marks.clear();

        //ImGradient out(true);
        for (int i=0; i<=10; i++)
        {
            float t = static_cast<float>(i) / 10.0f;
            float colA[4], colB[4];
            a.computeColorAt(t, colA);
            b.computeColorAt(t, colB);

            ImGradientMark m;
            m.color[0] = colA[0] + (colB[0] - colA[0]) * x;
            m.color[1] = colA[1] + (colB[1] - colA[1]) * x;
            m.color[2] = colA[2] + (colB[2] - colA[2]) * x;
            m.color[3] = colA[3] + (colB[3] - colA[3]) * x;
            m.position = t;
            m.uid = out.uid_counter++;

            out.m_marks.push_back(m);
        }

        out.refreshCache();
        //return out;

        /*
        for (int i=0; i<a.m_marks.size(); i++) out.addMark(a.m_marks[i].position, );
        for (int i=0; i<b.m_marks.size(); i++) out.addMark(b.m_marks[i]);*/
    }

    /* Threading helpers ----------------------------------------------------- */
    static int uid_counter;

    ImGradientMark* markFromUID(int uid) const
    {
        if (uid < 0) return nullptr;
        for (size_t i=0; i<m_marks.size(); i++)
        {
            if (m_marks[i].uid == uid)
                return (ImGradientMark*)&m_marks[i];
        }

        return nullptr;
    }
    
    inline void unguardedRGBA(float position, uint32_t& c) const
    {
        c = m_cachedColors[(int)(position * CACHE_SIZE_M1)];
    }
    inline void unguardedRGBA(double position, uint32_t& c) const
    {
        c = m_cachedColors[(int)(position * CACHE_SIZE_M1)];
    }

private:
    static constexpr float kEps = 1e-6f;
    static constexpr int CACHE_SIZE = 512*6;
    static constexpr int CACHE_SIZE_M1 = CACHE_SIZE - 1;

    void   clear() noexcept;
    void   computeColorAt(float position, float* color) const;

    std::vector<ImGradientMark> m_marks;
    int dragging_uid = -1;
    int selected_uid = -1;
    float m_cachedValues[CACHE_SIZE * 3]{};
    uint32_t m_cachedColors[CACHE_SIZE]{};
};

/* ---------------------------- editor helpers ------------------------------ */
namespace ImGui
{
    bool GradientButton(ImGradient* gradient, float dpr);
    bool GradientEditor(ImGradient* gradient, float bar_scale, float mark_scale);
}

/*#include <list>

struct ImGradientMark
{
    float color[4];
    float position; //0 to 1
};

class ImGradient
{
public:
    ImGradient();
    ~ImGradient();

    void getColorAt(float position, float* color) const;
    void addMark(float position, ImColor const color);
    void removeMark(ImGradientMark* mark);
    void refreshCache();
    std::list<ImGradientMark*>& getMarks() { return m_marks; }

    ImGradient(const ImGradient& rhs)
    {
        operator=(rhs);
    }
    ImGradient(ImGradient&& rhs) noexcept
    {
        operator=(rhs);
    }
    ImGradient& operator =(const ImGradient& rhs)
    {
        if (this == &rhs)
            return *this;

        for (ImGradientMark* mark : m_marks)
            delete mark;
        m_marks.clear();

        for (ImGradientMark* mark : rhs.m_marks)
        {
            ImGradientMark* newMark = new ImGradientMark();
            memcpy(newMark, mark, sizeof(ImGradientMark));
            m_marks.push_back(newMark); //~ // <-- flicker is a clue, sync bug when you alter color
        }

        memcpy(m_cachedValues, rhs.m_cachedValues, sizeof(m_cachedValues));
        return *this;
    }

    bool operator ==(const ImGradient& rhs)
    {
        if (m_marks.size() != rhs.m_marks.size())
            return false;
        auto it1 = m_marks.begin();
        auto it2 = rhs.m_marks.begin();
        for (size_t i = 0; i < m_marks.size(); i++)
        {
            ImGradientMark* m1 = (*it1);
            ImGradientMark* m2 = (*it2);
            if (m1->position != m2->position)
                return false;
            if (memcmp(m1->color, m2->color, sizeof(m1->color)) != 0)
                return false;
            it1++;
            it2++;
        }
        return true;
    }
    bool operator !=(const ImGradient& rhs)
    {
        return !((*this) == rhs);
    }
private:
    void computeColorAt(float position, float* color) const;
    std::list<ImGradientMark*> m_marks;
    float m_cachedValues[256 * 3];
};

namespace ImGui
{
    bool GradientButton(ImGradient* gradient, float dpr);

    bool GradientEditor(ImGradient* gradient,
        ImGradientMark*& draggingMark,
        ImGradientMark*& selectedMark,
        float mark_scale);
}
*/