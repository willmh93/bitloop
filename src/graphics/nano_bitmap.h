#pragma once

/// OpenGL
#ifdef __EMSCRIPTEN__
#include <GLES3/gl3.h>
#else
#include "glad.h"
#endif

#include "nanovg.h"
#include <vector>

#include "math_helpers.h"
#include "threads.h"
#include "camera.h"

struct CanvasObject
{
    union {
        DVec2 pos = { 0, 0 };
        struct { double x, y; };
    };

    union {
        DVec2 size = { 0, 0 };
        struct { double w, h; };
    };

    union {
        DVec2 align = { -1, -1 };
        struct { double align_x, align_y; };
    };

    double rotation = 0.0;
    CoordinateType coordinate_type = CoordinateType::WORLD;

    ~CanvasObject() {}

    void setCoordinateType(CoordinateType type)
    {
        coordinate_type = type;
    }

    void setAlign(int ax, int ay)
    {
        align_x = ax;
        align_y = ay;
    }

    void setAlign(const DVec2& _align)
    {
        align = _align;
    }

    void setStageRect(double _x, double _y, double _w, double _h)
    {
        coordinate_type = CoordinateType::STAGE;
        rotation = 0.0;
        x = _x;
        y = _y;
        w = _w;
        h = _h;
    }

    void setWorldRect(double _x, double _y, double _w, double _h)
    {
        coordinate_type = CoordinateType::WORLD;
        rotation = 0.0;
        x = _x;
        y = _y;
        w = _w;
        h = _h;
    }

    [[nodiscard]] DVec2 topLeft()
    {
        DVec2 offset = { -(align_x + 1) / 2 * w , -(align_y + 1) / 2 * h };
        return Math::rotateOffset(offset, rotation) + pos;
    }

    [[nodiscard]] DQuad getQuad()
    {
        DVec2 pivot = { (align_x + 1) / 2 * w , (align_y + 1) / 2 * h };
        DQuad quad = { { 0, 0 }, { w, 0 }, { w, h }, { 0, h } };

        // Precompute cos and sin of rotation
        double _cos = cos(rotation);
        double _sin = sin(rotation);

        // Shift corner by negative pivot & Rotate around (0,0)
        quad.a = Math::rotateOffset(quad.a - pivot, _cos, _sin) + pos;
        quad.b = Math::rotateOffset(quad.b - pivot, _cos, _sin) + pos;
        quad.c = Math::rotateOffset(quad.c - pivot, _cos, _sin) + pos;
        quad.d = Math::rotateOffset(quad.d - pivot, _cos, _sin) + pos;

        return quad;
    }

    [[nodiscard]] double localAlignOffsetX()
    {
        return -(align_x + 1) * 0.5 * w;
    }

    [[nodiscard]] double localAlignOffsetY()
    {
        return -(align_y + 1) * 0.5 * h;
    }
};

class Image
{
    friend class SimplePainter;
    friend class Painter;

protected:

    int bmp_width = 0;
    int bmp_height = 0;
    int nano_img = 0;
    bool pending_resize = false;

    std::vector<uint8_t> pixels;

public:

    explicit Image(NVGcontext* ctx = nullptr) {}
    Image(NVGcontext* ctx, int w, int h) {
        create(w, h); 
    }

    [[nodiscard]] int width() const { return bmp_width; }
    [[nodiscard]] int height() const { return bmp_height; }
    [[nodiscard]] int imageId() const { return nano_img; }

    void create(int w, int h) 
    {
        bmp_width = w; bmp_height = h;
        pixels.assign(size_t(w) * h * 4, 0);
        pending_resize = true;
    }

    void clear(Color c)
    {
        if (pixels.size() == 0)
            return;
        uint32_t u32 = c.u32;
        uint32_t* pixel = reinterpret_cast<uint32_t*>(&pixels.front());
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
        size_t i = (size_t(y) * bmp_width + x) * 4;
        pixels[i + 0] = rgba & 0xFF;
        pixels[i + 1] = (rgba >> 8) & 0xFF;
        pixels[i + 2] = (rgba >> 16) & 0xFF;
        pixels[i + 3] = (rgba >> 24) & 0xFF;
    }

    void setPixel(int x, int y, int r, int g, int b, int a=255)
    {
        size_t i = (size_t(y) * bmp_width + x) * 4;
        pixels[i + 0] = r;
        pixels[i + 1] = g;
        pixels[i + 2] = b;
        pixels[i + 3] = a;
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

protected:

    void draw(NVGcontext* vg, double x, double y, double w, double h)
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

    [[nodiscard]] bool needsReshading(Camera* camera)
    {
        DQuad world_quad = getWorldQuad(camera);
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

    [[nodiscard]] DQuad getWorldQuad(Camera* camera);

    template<typename Callback>
    void forEachPixel(Callback&& callback)
    {
        static_assert(std::is_invocable_r_v<void, Callback, int, int>,
            "Callback must be: void(int x, int y)");

        for (int bmp_y = 0; bmp_y < bmp_height; bmp_y++)
        {
            for (int bmp_x = 0; bmp_x < bmp_width; bmp_x++)
            {
                std::forward<Callback>(callback)(bmp_x, bmp_y);
            }
        }
    }

    template<typename Callback>
    bool forEachWorldPixel(
        Camera* camera,
        int& current_row,
        Callback&& callback,
        int thread_count = Thread::idealThreadCount(),
        int timeout_ms = 0)
    {
        static_assert(std::is_invocable_r_v<void, Callback, int, int, double, double>,
            "Callback must be: void(int x, int y, double wx, double wy)");

        auto timeout = timeout_ms ?
            std::chrono::milliseconds{ timeout_ms } :
            std::chrono::steady_clock::duration::max();

        DQuad world_quad = getWorldQuad(camera);
        double ax = world_quad.a.x, ay = world_quad.a.y;
        double bx = world_quad.b.x, by = world_quad.b.y;
        double cx = world_quad.c.x, cy = world_quad.c.y;
        double dx = world_quad.d.x, dy = world_quad.d.y;

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
                        // Interpolate row pixel coordinate and invoke callback
                        double bmp_fx, bmp_fy = static_cast<double>(row) + 0.5;
                        double v = bmp_fy / bmp_fh;
                        double scan_left_x = ax + (dx - ax) * v;
                        double scan_left_y = ay + (dy - ay) * v;
                        double scan_right_x = bx + (cx - bx) * v;
                        double scan_right_y = by + (cy - by) * v;
                        for (int bmp_x = 0; bmp_x < bmp_width; ++bmp_x)
                        {
                            bmp_fx = static_cast<double>(bmp_x) + 0.5;
                            double u = bmp_fx / bmp_fw;
                            double wx = scan_left_x + (scan_right_x - scan_left_x) * u;
                            double wy = scan_left_y + (scan_right_y - scan_left_y) * u;
                            std::forward<Callback>(callback)(bmp_x, row, wx, wy);
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
            double scanline_origin_dx = (dx - ax) / bmp_fh;
            double scanline_origin_dy = (dy - ay) / bmp_fh;
            double bmp_fx, bmp_fy;
            for (int bmp_y = 0; bmp_y < bmp_height; ++bmp_y)
            {
                // Interpolate left and right edges of the scanline
                bmp_fy = static_cast<double>(bmp_y) + 0.5;
                double v = bmp_fy / bmp_fh;
                double scan_left_x = ax + (dx - ax) * v;
                double scan_left_y = ay + (dy - ay) * v;
                double scan_right_x = bx + (cx - bx) * v;
                double scan_right_y = by + (cy - by) * v;
                for (int bmp_x = 0; bmp_x < bmp_width; ++bmp_x)
                {
                    bmp_fx = static_cast<double>(bmp_x) + 0.5;
                    double u = bmp_fx / bmp_fw;
                    double wx = scan_left_x + (scan_right_x - scan_left_x) * u;
                    double wy = scan_left_y + (scan_right_y - scan_left_y) * u;
                    std::forward<Callback>(callback)(bmp_x, bmp_y, wx, wy);
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
    void forEachWorldPixel(
        Camera* camera,
        Callback&& callback,
        int thread_count = Thread::idealThreadCount())
    {
        int row = 0;
        forEachWorldPixel(
            camera, 
            row,
            callback,
            thread_count,
            0
        );
    }
};