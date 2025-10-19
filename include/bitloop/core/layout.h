#pragma once

#include <vector>
#include <memory>

#include <bitloop/platform/platform_macros.h>

BL_BEGIN_NS

class ProjectBase;
class Viewport;
class SceneBase;
class Layout;

struct SimSceneList : public std::vector<SceneBase*>
{
    void mountTo(Layout& viewports);
};

class Layout
{
    std::vector<Viewport*> viewports;

protected:

    friend class ProjectBase;
    friend class SceneBase;

    // If 0, viewports expand in that direction. If both 0, expand whole grid.
    int targ_viewports_x = 0;
    int targ_viewports_y = 0;
    int cols = 0;
    int rows = 0;

    std::vector<SceneBase*> all_scenes;
    ProjectBase* project = nullptr;

public:

    using iterator = typename std::vector<Viewport*>::iterator;
    using const_iterator = typename std::vector<Viewport*>::const_iterator;

    ~Layout()
    {
        // Only invoked when you switch project
        clear();
    }

    const std::vector<SceneBase*>& scenes() {
        return all_scenes;
    }

    void expandCheck(size_t count);
    void add(int _viewport_index, int _grid_x, int _grid_y);
    void resize(size_t viewport_count);
    void clear();

    void setSize(int _targ_viewports_x, int _targ_viewports_y)
    {
        this->targ_viewports_x = _targ_viewports_x;
        this->targ_viewports_y = _targ_viewports_y;
    }


    [[nodiscard]] Viewport* operator[](size_t i) { expandCheck(i + 1); return viewports[i]; }
    Layout& operator<<(SceneBase* scene);
    Layout& operator<<(std::shared_ptr<SimSceneList> scenes);

    [[nodiscard]] iterator begin() { return viewports.begin(); }
    [[nodiscard]] iterator end() { return viewports.end(); }

    [[nodiscard]] const_iterator begin() const { return viewports.begin(); }
    [[nodiscard]] const_iterator end() const { return viewports.end(); }

    [[nodiscard]] int count() const {
        return static_cast<int>(viewports.size());
    }
};

BL_END_NS
