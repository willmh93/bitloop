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

#include <list>
#include <algorithm>
#include <cstring>
#include <utility>

struct ImGradientMark
{
    int uid{};
    float color[4]{};
    float position{};
};

class ImGradient
{
public:
    ImGradient();
    ImGradient(const ImGradient& rhs);            // deep copy
    ImGradient(ImGradient&& rhs) noexcept;        // move
    ImGradient& operator=(const ImGradient& rhs); // copy-assign
    ImGradient& operator=(ImGradient&& rhs) noexcept; // move-assign
    ~ImGradient();                                // frees marks

    /* API used by the editor ------------------------------------------------ */
    void   getColorAt(float position, float* color) const;
    void   addMark(float position, ImColor color);
    void   removeMark(ImGradientMark* mark);
    void   refreshCache();
    std::list<ImGradientMark*>& getMarks() { return m_marks; }
    const std::list<ImGradientMark*>& getMarks() const { return m_marks; }

    /* Comparison ------------------------------------------------------------ */
    bool operator==(const ImGradient& rhs) const;
    bool operator!=(const ImGradient& rhs) const { return !(*this == rhs); }


    /* Threading helpers ----------------------------------------------------- */
    static int uid_counter;

    void copyFrom(const ImGradient& rhs)
    {
        operator=(rhs);
    }

    /*int markIndex(ImGradientMark* mark) const
    {
        if (mark == nullptr)
            return -1;

        int index = 0;
        for (ImGradientMark* item : m_marks) {
            if (item == mark) return index;
            ++index;
        }
        return -1;
    }
    ImGradientMark* markFromIndex(int index) const
    {
        if (index < 0 || index >= m_marks.size())
            return nullptr;

        auto it = m_marks.begin();
        std::advance(it, index);
        return *it;
    }*/

    ImGradientMark* markFromUID(int uid) const
    {
        if (uid < 0)
            return nullptr;

        for (ImGradientMark* item : m_marks) {
            if (item->uid == uid) return item;
        }

        return nullptr;
    }

private:
    static constexpr float kEps = 1e-6f;

    void   clear() noexcept;                 // delete all marks + clear list
    void   swap(ImGradient& other) noexcept; // member-wise swap
    void   computeColorAt(float position, float* color) const;

    std::list<ImGradientMark*> m_marks;
    float                      m_cachedValues[256 * 3]{}; // 256 rgb triples
};

/* ---------------------------- editor helpers ------------------------------ */
namespace ImGui
{
    bool GradientButton(ImGradient* gradient, float dpr);
    bool GradientEditor(ImGradient* gradient,
        ImGradientMark*& draggingMark,
        ImGradientMark*& selectedMark,
        float dpr);
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