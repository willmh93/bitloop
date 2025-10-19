#include <bitloop/core/layout.h>
#include <bitloop/core/viewport.h>
#include <bitloop/core/project.h>
#include <bitloop/core/scene.h>

BL_BEGIN_NS

void SimSceneList::mountTo(Layout& viewports)
{
    for (size_t i = 0; i < size(); i++)
        at(i)->mountTo(viewports[i]);
}

void Layout::expandCheck(size_t count)
{
    if (count > viewports.size())
        resize(count);
}

void Layout::add(int _viewport_index, int _grid_x, int _grid_y)
{
    Viewport* viewport = new Viewport(this, _viewport_index, _grid_x, _grid_y);
    viewports.push_back(viewport);
}

void Layout::resize(size_t viewport_count)
{
    if (targ_viewports_x <= 0 || targ_viewports_y <= 0)
    {
        // Spread proportionally
        rows = static_cast<int>(std::sqrt(viewport_count));
        cols = static_cast<int>(viewport_count) / rows;
    }
    else if (targ_viewports_y <= 0)
    {
        // Expand down
        cols = targ_viewports_x;
        rows = (int)std::ceil((float)viewport_count / (float)cols);
    }
    else if (targ_viewports_x <= 0)
    {
        // Expand right
        rows = targ_viewports_y;
        cols = (int)std::ceil((float)viewport_count / (float)rows);
    }

    // Expand rows down by default if not perfect fit
    if (viewport_count > rows * cols)
        rows++;

    for (int y = 0; y < rows; y++)
    {
        for (int x = 0; x < cols; x++)
        {
            int i = (y * cols) + x;
            if (i >= viewport_count)
            {
                goto break_nested;
            }

            if (i < viewports.size())
            {
                viewports[i]->viewport_grid_x = x;
                viewports[i]->viewport_grid_y = y;
            }
            else
            {
                Viewport* viewport = new Viewport(this, i, x, y);
                viewports.push_back(viewport);
            }
        }
    }
    break_nested:;

    // todo: Unmount remaining viewport sims
}

void Layout::clear()
{
    // layout freed each time you call setLayout
    for (Viewport* p : viewports) delete p;
    viewports.clear();
}

Layout& Layout::operator<<(SceneBase* scene) { scene->mountTo(*this); return *this; }
Layout& Layout::operator<<(std::shared_ptr<SimSceneList> scenes) {
    for (SceneBase* scene : *scenes)
        scene->mountTo(*this);
    return *this;
}

BL_END_NS
