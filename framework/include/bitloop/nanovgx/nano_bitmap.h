#pragma once

/// OpenGL
#ifdef __EMSCRIPTEN__
#include <GLES3/gl3.h>
#else
#include "glad/glad.h"
#endif

#include "nanovg.h"
#include <vector>

#include <bitloop/util/math_util.h>
#include <bitloop/util/color.h>
#include <bitloop/core/threads.h>
#include <bitloop/core/camera.h>

BL_BEGIN_NS

class CanvasObject
{
    DVec2 toStage(const DVec2& p)             const { return Camera::active->toStage(p); }
    DVec2 toWorld(double sx, double sy)       const { return Camera::active->toWorld(sx, sy); }
    DVec2 toWorldOffset(double sx, double sy) const { return Camera::active->stageToWorldOffset(sx, sy); }
    DVec2 toStageOffset(const DVec2& o)       const { return Camera::active->worldToStageOffset(o); }

    // local basis vectors after all transforms
    DVec2 u{ 1, 0 };   // width direction
    DVec2 v{ 0, 1 };   // height direction

public:

    union {
        DVec2 pos = { 0, 0 };
        struct { double x, y; };
    };
    
    union {
        DVec2 align = { -1, -1 };
        struct { double align_x, align_y; };
    };

    double rotation = 0.0;

    ~CanvasObject() {}

    void setAlign(int ax, int ay)      { align_x = ax; align_y = ay; }
    void setAlign(const DVec2& _align) { align = _align; }

    // ======== Stage Methods ========

    [[nodiscard]] DVec2  stagePos()      const { return toStage(pos); }
    [[nodiscard]] double stageWidth()    const { return toStageOffset(u).magnitude(); }
    [[nodiscard]] double stageHeight()   const { return toStageOffset(v).magnitude(); }
    [[nodiscard]] DVec2  stageSize()     const { return {stageWidth(), stageHeight()}; }
    [[nodiscard]] double stageRotation() const {
        double cos_r = std::cos(rotation);
        double sin_r = std::sin(rotation);
        DVec2 local_u = DVec2{ worldSize().x * cos_r, worldSize().x * sin_r };

        DVec2 stage_origin = Camera::active->toStage(pos);
        DVec2 stage_u_end = Camera::active->toStage(pos + local_u);
        DVec2 u_stage = stage_u_end - stage_origin;

        return std::atan2(u_stage.y, u_stage.x);
    }

    void setStagePos(double sx, double sy) { pos = toWorld(sx, sy); }
    void setStageRect(double sx, double sy, double sw, double sh)
    {
        pos = toWorld(sx, sy) - worldAlignOffset();
        u = toWorld(sx + sw, sy) - pos;
        v = toWorld(sx, sy + sh) - pos;
    }

    void setStageSize(double sw, double sh)
    {
        u = toWorldOffset(sw, 0);
        v = toWorldOffset(0, sh);
    }

    // ======== World Methods ========

    [[nodiscard]] double worldWidth()  const { return u.magnitude(); }
    [[nodiscard]] double worldHeight() const { return v.magnitude(); }
    [[nodiscard]] DVec2  worldSize() const { return { u.magnitude(), v.magnitude() }; }
    [[nodiscard]] DQuad worldQuad() {
        DVec2 offset = 0.5 * ((-align_x - 1.0) * u + (-align_y - 1.0) * v);
        DVec2 p = pos + offset;
        return { p, p + u, p + u + v, p + v };
    }
    [[nodiscard]] DVec2 topLeft() {
        DVec2 offset = 0.5 * ((-align_x - 1.0) * u + (-align_y - 1.0) * v);
        return pos + offset;
    }

    [[nodiscard]] DVec2 worldAlignOffset()   { return -(align + 1.0) * 0.5 * worldSize(); }
    [[nodiscard]] double worldAlignOffsetX() { return -(align_x + 1) * 0.5 * worldWidth(); }
    [[nodiscard]] double worldAlignOffsetY() { return -(align_y + 1) * 0.5 * worldHeight(); }

    [[nodiscard]] DVec2 worldToUVRatio(const DVec2& p) const
    {
        // p = origin + [a] * u + [b] * v
        DVec2 origin = pos + 0.5 * ((-align_x - 1.0) * u + (-align_y - 1.0) * v);
        DVec2 delta = p - origin;

        // Make sure uv vectors span 2D area
        double det = u.x * v.y - u.y * v.x;
        if (det == 0) return  { 0.0, 0.0 };
        //if (std::abs(det) < std::numeric_limits<double>::epsilon()) return { 0.0, 0.0 };

        // solve for a,b
        double inv_det = 1.0 / det;
        double a = (delta.x * v.y - delta.y * v.x) * inv_det;
        double b = (u.x * delta.y - u.y * delta.x) * inv_det;
        return { a, b };
    }

    void setWorldRect(double _x, double _y, double _w, double _h)
    {
        rotation = 0.0;
        x = _x - worldAlignOffsetX();
        y = _y - worldAlignOffsetY();
        u = { _w, 0 };
        v = { 0, _h };
    }
};

class Image
{
    friend class SimplePainter;
    friend class Painter;

protected:

    union
    {
        struct { int bmp_width; int bmp_height; };
        IVec2 bmp_size;
    };

    int nano_img = 0;
    bool pending_resize = false;

    std::vector<uint8_t> pixels;
    uint32_t* colors;

public:

    Image() : bmp_size{ 0, 0 }, colors(nullptr) {}

    [[nodiscard]] int width() const { return bmp_width; }
    [[nodiscard]] int height() const { return bmp_height; }
    [[nodiscard]] int imageId() const { return nano_img; }

    void create(int w, int h) 
    {
        bmp_width = w; bmp_height = h;
        pixels.assign(size_t(w) * h * 4, 0);
        colors = reinterpret_cast<uint32_t*>(&pixels.front());
        pending_resize = true;
    }

    void clear(Color c)
    {
        if (pixels.size() == 0)
            return;
        uint32_t u32 = c.u32;
        uint32_t* pixel = colors;
        uint32_t count = bmp_width * bmp_height;
        for (uint32_t i=0; i< count; i++)
            *pixel++ = u32;
    }

    void clear(int r, int g, int b, int a)
    {
        clear(Color(r, g, b, a));
    }

    void setPixel(int x, int y, uint32_t rgba)
    {
        size_t i = (size_t(y) * bmp_width + x);
        colors[i] = rgba;
    }

    void setPixel(int x, int y, int r, int g, int b, int a=255)
    {
        size_t i = (size_t(y) * bmp_width + x) * 4;
        pixels[i++] = r;
        pixels[i++] = g;
        pixels[i++] = b;
        pixels[i++] = a;
    }

    void setPixelSafe(int x, int y, uint32_t rgba)
    {
        if ((unsigned)x >= (unsigned)bmp_width ||
            (unsigned)y >= (unsigned)bmp_height)
        {
            return;
        }
        size_t i = (size_t(y) * bmp_width + x) * 4;
        pixels[i + 0] = rgba & 0xFF;
        pixels[i + 1] = (rgba >> 8) & 0xFF;
        pixels[i + 2] = (rgba >> 16) & 0xFF;
        pixels[i + 3] = (rgba >> 24) & 0xFF;
    }

    void setPixelSafe(int x, int y, int r, int g, int b, int a = 255)
    {
        if ((unsigned)x >= (unsigned)bmp_width ||
            (unsigned)y >= (unsigned)bmp_height)
        {
            return;
        }
        size_t i = (size_t(y) * bmp_width + x) * 4;
        pixels[i + 0] = r;
        pixels[i + 1] = g;
        pixels[i + 2] = b;
        pixels[i + 3] = a;
    }

    [[nodiscard]] Color getPixel(int x, int y) const
    {
        size_t i = (size_t(y) * bmp_width + x) * 4;
        return 
            pixels[i] |
            pixels[i + 1] << 8 | 
            pixels[i + 2] << 16 | 
            pixels[i + 3] << 24;
    }

    [[nodiscard]] Color getPixelSafe(int x, int y) const
    {
        if ((unsigned)x >= (unsigned)bmp_width ||
            (unsigned)y >= (unsigned)bmp_height)
        {
            return 0;
        }

        size_t i = (size_t(y) * bmp_width + x) * 4;
        return
            pixels[i] | 
            pixels[i + 1] << 8 | 
            pixels[i + 2] << 16 | 
            pixels[i + 3] << 24;
    }

    [[nodiscard]] uint32_t* getU32PtrSafe(int x, int y)
    {
        if ((unsigned)x >= (unsigned)bmp_width ||
            (unsigned)y >= (unsigned)bmp_height)
        {
            return nullptr;
        }
        return colors + (y * bmp_width + x);
    }

protected:

    void refreshData(NVGcontext* vg)
    {
        if (bmp_width <= 0 || bmp_height <= 0)
            return;

        if (pending_resize)
        {
            if (nano_img) nvgDeleteImage(vg, nano_img);
            nano_img = nvgCreateImageRGBA(vg, bmp_width, bmp_height, NVG_IMAGE_NEAREST, pixels.data());
            pending_resize = false;
        }
        else
        {
            nvgUpdateImage(vg, nano_img, pixels.data());
        }
    }

    /*void draw(NVGcontext* vg, double x, double y, double w, double h)
    {
        if (bmp_width <= 0 || bmp_height <= 0)
            return;

        if (pending_resize)
        {
            if (nano_img) nvgDeleteImage(vg, nano_img);
            nano_img = nvgCreateImageRGBA(vg, bmp_width, bmp_height, NVG_IMAGE_NEAREST, pixels.data());
            pending_resize = false;
        }
        else
        {
            nvgUpdateImage(vg, nano_img, pixels.data());
        }

        float _x = static_cast<float>(x);
        float _y = static_cast<float>(y);
        float _w = static_cast<float>(w);
        float _h = static_cast<float>(h);


        NVGpaint p = nvgImagePattern(vg, _x, _y, _w, _h, 0.0f, nano_img, 1.0f);
        nvgBeginPath(vg);
        nvgRect(vg, _x, _y, _w, _h);
        nvgFillPaint(vg, p);
        nvgFill(vg);
    }

    void drawSheared(NVGcontext* vg, DQuad quad)
    {
        if (bmp_width <= 0 || bmp_height <= 0)
            return;

        if (pending_resize)
        {
            if (nano_img) nvgDeleteImage(vg, nano_img);
            nano_img = nvgCreateImageRGBA(vg, bmp_width, bmp_height, NVG_IMAGE_NEAREST, pixels.data());
            pending_resize = false;
        }
        else
        {
            nvgUpdateImage(vg, nano_img, pixels.data());
        }

        DVec2 a = quad.a;
        DVec2 u = quad.b - quad.a;
        DVec2 v = quad.d - quad.a;

        nvgSave(vg);
        nvgTransform(vg,
            (float)u.x, (float)u.y,
            (float)v.x, (float)v.y,
            (float)a.x, (float)a.y);

        NVGpaint paint = nvgImagePattern(vg, 0, 0, 1, 1, 0.0f, nano_img, 1.0f);
        nvgBeginPath(vg);
        nvgRect(vg, 0, 0, 1, 1);
        nvgFillPaint(vg, paint);
        nvgFill(vg);

        nvgRestore(vg);
    }*/
};

class CanvasImage : public Image, public CanvasObject
{
protected:
    double bmp_fw = 0;
    double bmp_fh = 0;

    bool needs_reshading = false;
    DQuad prev_world_quad;

    friend class PaintContext;

public:

    void setNeedsReshading(bool b = true)
    {
        needs_reshading = b;
    }

    [[nodiscard]] bool needsReshading()
    {
        //DQuad world_quad = worldQuad(camera);
        DQuad world_quad = worldQuad();
        if (needs_reshading || (world_quad != prev_world_quad))
        {
            needs_reshading = false; // todo: Move to when drawn?
            prev_world_quad = world_quad;
            return true;
        }
        prev_world_quad = world_quad;
        return false;
    }

    void setBitmapSize(int bmp_w, int bmp_h)
    {
        if (bmp_w < 0) bmp_w = 0;
        if (bmp_h < 0) bmp_h = 0;

        if (bmp_width != bmp_w || bmp_height != bmp_h)
        {
            bmp_fw = static_cast<double>(bmp_w);
            bmp_fh = static_cast<double>(bmp_h);

            //qDebug() << "Resizing";
            create(bmp_w, bmp_h);
            needs_reshading = true;
        }
    }

    //[[nodiscard]] DQuad worldQuad(Camera* camera);

    [[nodiscard]] IVec2 pixelPosFromWorld(DVec2 p)
    {
        return static_cast<IVec2>(worldToUVRatio(p) * bmp_size);
    }

    template<typename T = double, typename Callback>
    bool forEachPixel(
        int& current_row,
        Callback&& callback,
        int thread_count = Thread::idealThreadCount(),
        int timeout_ms = 0)
    {
        static_assert(std::is_invocable_r_v<void, Callback, int, int>,
            "Callback must be: void(int x, int y)");

        auto timeout = timeout_ms ?
            std::chrono::milliseconds{ timeout_ms } :
            std::chrono::steady_clock::duration::max();

        if (thread_count > 0)
        {
            auto start_time = std::chrono::steady_clock::now();

            std::vector<std::future<void>> futures(thread_count);
            std::vector<std::atomic<bool>> active_threads(thread_count);

            for (int ti = 0; ti < thread_count; ti++)
                active_threads[ti].store(false);

            std::atomic<bool> timed_out{ false };

            // Continuously spin checking for idle threads and scheduling new rows...
            while (!timed_out.load(std::memory_order_relaxed))
            {
                for (int ti = 0; ti < thread_count; ti++)
                {
                    if (timed_out.load(std::memory_order_relaxed))
                        break;

                    if (active_threads[ti].load(std::memory_order_relaxed))
                        continue;

                    // Found an idle thread...

                    const int thread_index = ti;
                    const int row = current_row++;

                    if (row >= bmp_height)
                    {
                        timed_out.store(true);
                        break;
                    }

                    active_threads[ti].store(true);

                    futures[ti] = Thread::pool().submit_task([&, row, thread_index]()
                    {
                        for (int bmp_x = 0; bmp_x < bmp_width; ++bmp_x)
                            std::forward<Callback>(callback)(bmp_x, row);

                        // After each row solved, check if timeout exceeded
                        if (std::chrono::steady_clock::now() - start_time >= timeout)
                        {
                            // If so, signal that we shouldn't pick up any new tasks - break out the loop
                            timed_out.store(true);
                        }
                        else
                        {
                            // Otherwise, schedule the next available row
                            active_threads[thread_index].store(false);
                        }
                    });
                }

                std::this_thread::yield();
            }

            // After timing out, wait for remaining threads to finalize
            for (int ti = 0; ti < thread_count; ti++)
            {
                if (futures[ti].valid())
                    futures[ti].get();
            }
        }
        else
        {
            for (int bmp_y = 0; bmp_y < bmp_height; ++bmp_y)
            {
                for (int bmp_x = 0; bmp_x < bmp_width; ++bmp_x)
                    std::forward<Callback>(callback)(bmp_x, bmp_y);
            }
        }

        if (current_row >= bmp_height)
        {
            current_row = 0;
            return true;
        }
        return false;
    }

        //static_assert(std::is_invocable_r_v<void, Callback, int, int, T, T>,
        //    "Callback must be: void(int x, int y, float_t wx, float_y wy)");


    template<typename T = double, typename Callback>
    bool forEachWorldPixel(
        int& current_row,
        Callback&& callback,
        int thread_count = Thread::idealThreadCount(),
        int timeout_ms = 0,
        std::atomic<bool>*busy = nullptr
    )
    {
        auto timeout = timeout_ms ?
            std::chrono::milliseconds{ timeout_ms } :
            std::chrono::steady_clock::duration::max();

        Quad<T> world_quad = static_cast<Quad<T>>(worldQuad());
        T ax = world_quad.a.x, ay = world_quad.a.y;
        T bx = world_quad.b.x, by = world_quad.b.y;
        T cx = world_quad.c.x, cy = world_quad.c.y;
        T dx = world_quad.d.x, dy = world_quad.d.y;

        T t_bmp_w = static_cast<T>(bmp_fw);
        T t_bmp_h = static_cast<T>(bmp_fh);

        if (thread_count > 0)
        {
            if (busy)
            {
                for (int i = 0; i < thread_count; ++i)
                    busy[i].store(false, std::memory_order_relaxed);
            }

            auto start_time = std::chrono::steady_clock::now();

            std::vector<std::future<void>> futures(thread_count);
            std::vector<std::atomic<bool>> active_threads(thread_count);
            
            for (int ti = 0; ti < thread_count; ti++)
                active_threads[ti].store(false);
            
            std::atomic<bool> timed_out{ false };

            // Continuously spin checking for idle threads and scheduling new rows...
            while (!timed_out.load(std::memory_order_relaxed))
            {
                for (int ti = 0; ti < thread_count; ti++)
                {
                    if (timed_out.load(std::memory_order_relaxed))
                        break;

                    if (active_threads[ti].load(std::memory_order_relaxed))
                        continue;

                    // Found an idle thread...

                    const int thread_index = ti;
                    const int row = current_row++;

                    if (row >= bmp_height)
                    {
                        timed_out.store(true);
                        break;
                    }
                  
                    active_threads[thread_index].store(true, std::memory_order_relaxed);
                    if (busy) busy[thread_index].store(true, std::memory_order_relaxed);

                    futures[ti] = Thread::pool().submit_task([&, row, thread_index]()
                    {
                        // Interpolate row pixel coordinate and invoke callback
                        T bmp_fx, bmp_fy = static_cast<T>(row) + T{ 0.5 };
                        T _v = bmp_fy / t_bmp_h;
                        T scan_left_x = ax + (dx - ax) * _v;
                        T scan_left_y = ay + (dy - ay) * _v;
                        T scan_right_x = bx + (cx - bx) * _v;
                        T scan_right_y = by + (cy - by) * _v;
                        for (int bmp_x = 0; bmp_x < bmp_width; ++bmp_x)
                        {
                            bmp_fx = static_cast<T>(bmp_x) + T{ 0.5 };
                            T _u = bmp_fx / t_bmp_w;
                            T wx = scan_left_x + (scan_right_x - scan_left_x) * _u;
                            T wy = scan_left_y + (scan_right_y - scan_left_y) * _u;

                            if constexpr (std::is_invocable_r_v<void, Callback, int, int, T, T, int>)
                                std::forward<Callback>(callback)(bmp_x, row, wx, wy, thread_index); 
                            else if constexpr (std::is_invocable_r_v<void, Callback, int, int, T, T>)
                                std::forward<Callback>(callback)(bmp_x, row, wx, wy);
                            else 
                                static_assert(sizeof(Callback) == 0,
                                    "Callback must be: void( int x, int y, float_t wx, float_y wy, [[optional]] int thread_index)");
                        }

                        // After each row solved, check if timeout exceeded
                        if (std::chrono::steady_clock::now() - start_time >= timeout)
                        {
                            // If so, signal that we shouldn't pick up any new tasks - break out the loop
                            timed_out.store(true);
                        }
                        else
                        {
                            // Otherwise, schedule the next available row
                            active_threads[thread_index].store(false);
                            if (busy) busy[thread_index].store(false, std::memory_order_relaxed);
                        }
                    });
                }

                std::this_thread::yield();
            }

            // After timing out, wait for remaining threads to finalize
            for (int ti = 0; ti < thread_count; ti++)
            {
                if (futures[ti].valid())
                    futures[ti].get();
            }
        }
        else
        {
            T bmp_fx, bmp_fy;
            for (int bmp_y = 0; bmp_y < bmp_height; ++bmp_y)
            {
                // Interpolate left and right edges of the scanline
                bmp_fy = static_cast<T>(bmp_y) + T{ 0.5 };
                T _v = bmp_fy / t_bmp_h;
                T scan_left_x = ax + (dx - ax) * _v;
                T scan_left_y = ay + (dy - ay) * _v;
                T scan_right_x = bx + (cx - bx) * _v;
                T scan_right_y = by + (cy - by) * _v;
                for (int bmp_x = 0; bmp_x < bmp_width; ++bmp_x)
                {
                    bmp_fx = static_cast<T>(bmp_x) + T{ 0.5 };
                    T _u = bmp_fx / t_bmp_w;
                    T wx = scan_left_x + (scan_right_x - scan_left_x) * _u;
                    T wy = scan_left_y + (scan_right_y - scan_left_y) * _u;

                    if constexpr (std::is_invocable_r_v<void, Callback, int, int, T, T, int>)
                        std::forward<Callback>(callback)(bmp_x, bmp_y, wx, wy, 0);
                    else if constexpr (std::is_invocable_r_v<void, Callback, int, int, T, T>)
                        std::forward<Callback>(callback)(bmp_x, bmp_y, wx, wy);
                    else
                        static_assert(sizeof(Callback) == 0,
                            "Callback must be: void( int x, int y, float_t wx, float_y wy, [[optional]] int thread_index)");
                }
            }
        }

        if (current_row >= bmp_height)
        {
            current_row = 0;
            return true;
        }
        return false;
    }

    template<typename Callback>
    void forEachPixel(
        Callback&& callback,
        int thread_count = Thread::idealThreadCount())
    {
        int row = 0;
        forEachPixel(
            row,
            callback,
            thread_count,
            0
        );
    }

    //template<typename Callback>
    //void forEachWorldPixel(
    //    Camera* camera,
    //    Callback&& callback,
    //    int thread_count = Thread::idealThreadCount())
    //{
    //    int row = 0;
    //    forEachWorldPixel(
    //        camera, 
    //        row,
    //        callback,
    //        thread_count,
    //        0
    //    );
    //}
};

BL_END_NS
