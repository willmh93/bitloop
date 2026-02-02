#pragma once

#include <sstream>

#include <bitloop/nanovgx/nano_canvas.h>

BL_BEGIN_NS

class ProjectBase;
class SceneBase;
class Layout;
class CameraInfo;

class Viewport : public Painter, public SurfaceInfo
{
    std::string print_text;
    std::stringstream print_stream;

protected:

    friend class ProjectBase;
    friend class SceneBase;
    friend class Layout;
    friend class CameraInfo;

    int viewport_index;
    int viewport_grid_x;
    int viewport_grid_y;

    Layout* layout = nullptr; // owner
    SceneBase* scene = nullptr; // mounted scene

    static inline float focus_flash_frames = 20.0f;
    float focused_dt = 0;

public:
    
    Viewport(
        Layout* layout,
        int viewport_index,
        int grid_x,
        int grid_y
    );

    ~Viewport();

    void draw();

    [[nodiscard]] int viewportIndex() const { return viewport_index; }
    [[nodiscard]] int viewportGridX() const { return viewport_grid_x; }
    [[nodiscard]] int viewportGridY() const { return viewport_grid_y; }

    //[[nodiscard]] double posX() const { return x; }
    //[[nodiscard]] double posY() const { return y; }

    [[nodiscard]] IVec2 outputSize() const;

    template<typename T=double> [[nodiscard]] Vec2<T> worldSize() const { return m.toWorldOffset<T>(width(), height()); }
    template<typename T=double> [[nodiscard]] Quad<T> worldQuad() const {
        Rect<T> r = Rect<T>(Vec2<T>(T{0},T{0}), Vec2<T>(size()));
        return m.toWorldQuad(r);
    }
    //[[nodiscard]] DAngledRect worldRect() { return camera.toWorldRect(DAngledRect(width/2, height/2, width, height, 0.0)); }



    template<typename T>
    T* mountScene(T* sim)
    {
        static_assert(std::is_base_of<SceneBase, T>::value, "T must derive from SceneBase");
        scene = sim;
        sim->registerMount(this);
        return sim;
    }

    [[nodiscard]] SceneBase* mountedScene()
    {
        return scene;
    }

    // Viewport-specific draw helpers (i.e. size of viewport needed)
    ///void printTouchInfo();

    std::stringstream& print() {
        return print_stream;
    }
};

BL_END_NS
