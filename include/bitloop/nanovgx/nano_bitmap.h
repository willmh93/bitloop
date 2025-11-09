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

template<typename T>
class CanvasObjectBase
{
    CameraInfo* camera = nullptr;

    DVec2   toStage(Vec2<T> p)                  const { return camera->getTransform().toStage<T>(p); }
    DVec2   toStageOffset(const Vec2<T>& o)     const { return camera->getTransform().toStageOffset<T>(o); }
    Vec2<T> toWorld(double sx, double sy)       const { return camera->getTransform().toWorld<T>(sx, sy); }
    Vec2<T> toWorldOffset(double sx, double sy) const { return camera->getTransform().toWorldOffset<T>(sx, sy); }

    // local basis vectors after all transforms
    Vec2<T> u{ T{1}, T{0} };   // width direction
    Vec2<T> v{ T{0}, T{1} };   // height direction

public:

    union {
        Vec2<T> pos = { T{0}, T{0} };
        struct { T x, y; };
    };
    
    union {
        DVec2 align = { -1, -1 };
        struct { double align_x, align_y; };
    };

    T rotation = T{ 0 };

    CanvasObjectBase() {}
    ~CanvasObjectBase() {}

    void setCamera(const CameraInfo& cam) { camera = const_cast<CameraInfo*>(&cam); }
    const CameraInfo* getCamera() { return camera; }

    void setAlign(int ax, int ay)      { align_x = ax; align_y = ay; }
    void setAlign(const DVec2& _align) { align = _align; }

    // ======== Stage Methods ========

    [[nodiscard]] DVec2  stagePos()      const { return toStage(pos); }
    [[nodiscard]] double stageWidth()    const { return toStageOffset(u).mag(); }
    [[nodiscard]] double stageHeight()   const { return toStageOffset(v).mag(); }
    [[nodiscard]] DVec2  stageSize()     const { return {stageWidth(), stageHeight()}; }
    [[nodiscard]] double stageRotation() const {
        T cos_r = cos(rotation);
        T sin_r = sin(rotation);
        Vec2<T> local_u = Vec2<T>{ worldSize().x * cos_r, worldSize().x * sin_r };
        DVec2 stage_origin = toStage(pos);
        DVec2 stage_u_end = toStage(pos + local_u);
        DVec2 u_stage = stage_u_end - stage_origin;

        return std::atan2(u_stage.y, u_stage.x);
    }
    [[nodiscard]] DQuad stageQuad() const {
        return camera->getTransform().toStageQuad<T>(worldQuad());
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

    [[nodiscard]] T worldWidth()  const { return u.mag(); }
    [[nodiscard]] T worldHeight() const { return v.mag(); }
    [[nodiscard]] Vec2<T> worldSize() const { return { u.mag(), v.mag() }; }
    [[nodiscard]] Quad<T> worldQuad() const {
        Vec2<T> offset = T(0.5) * (T(-align_x - 1.0) * u + T(-align_y - 1.0) * v);
        Vec2<T> p = pos + offset;
        return { p, p + u, p + u + v, p + v };
    }
    [[nodiscard]] Vec2<T> topLeft() const {
        Vec2<T> offset = T(0.5) * (T(-align_x - 1.0) * u + T(-align_y - 1.0) * v);
        return pos + offset;
    }

    [[nodiscard]] Vec2<T> worldAlignOffset() const { return Vec2<T>{-(align + 1.0) * 0.5} * worldSize(); }
    [[nodiscard]] T worldAlignOffsetX() const { return T(-(align_x + 1) * 0.5) * worldWidth(); }
    [[nodiscard]] T worldAlignOffsetY() const { return T(-(align_y + 1) * 0.5) * worldHeight(); }

    [[nodiscard]] Vec2<T> worldToUVRatio(const Vec2<T>& p) const
    {
        // p = origin + [a] * u + [b] * v
        Vec2<T> origin = pos + T(0.5) * (T(-align_x - 1.0) * u + T(-align_y - 1.0) * v);
        Vec2<T> delta = p - origin;

        // Make sure uv vectors span 2D area
        T det = u.x * v.y - u.y * v.x;
        if (det == 0) return  Vec2<T>{ 0.0, 0.0 };
        //if (std::abs(det) < std::numeric_limits<double>::epsilon()) return { 0.0, 0.0 };

        // solve for a,b
        T inv_det = 1.0 / det;
        T a = (delta.x * v.y - delta.y * v.x) * inv_det;
        T b = (u.x * delta.y - u.y * delta.x) * inv_det;
        return { a, b };
    }

    void setWorldRect(T _x, T _y, T _w, T _h)
    {
        rotation = 0;
        x = _x - worldAlignOffsetX();
        y = _y - worldAlignOffsetY();
        u = { _w, 0 };
        v = { 0, _h };
    }
    void setWorldRect(Vec2<T> pos, Vec2<T> size)
    {
        rotation = 0;
        x = pos.x - worldAlignOffsetX();
        y = pos.y - worldAlignOffsetY();
        u = { size.x, 0 };
        v = { 0, size.y };
    }
};

typedef CanvasObjectBase<double> CanvasObject;
typedef CanvasObjectBase<f128> CanvasObject128;

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
        uint32_t u32 = c.rgba;
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
            //nano_img = nvgCreateImageRGBA(vg, bmp_width, bmp_height, NVG_IMAGE_NEAREST, pixels.data());
            nano_img = nvgCreateImageRGBA(vg, bmp_width, bmp_height, NVG_IMAGE_GENERATE_MIPMAPS, pixels.data());
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

#include <atomic>
#include <vector>
#include <chrono>
#include <algorithm>

struct Block {
    int tile_index;
    int x0, y0, x1, y1;
};

struct TileBlockProgress {
    // Build params (to detect changes)
    int bmp_w = 0, bmp_h = 0;
    int tile_w = 0, tile_h = 0;
    int block_w = 0, block_h = 0;
    int tiles_x = 0, tiles_y = 0;

    // Work decomposition (one micro-block = one job)
    std::vector<Block> blocks;         // size = sum over tiles of ceil(tile_w/block_w)*ceil(tile_h/block_h)
    std::atomic<int>  next_block{ 0 };   // global cursor (persists across frames)

    // Per-tile progress
    std::vector<uint32_t> blocks_total_per_tile;
    std::vector<uint32_t> blocks_done_per_tile; // incremented via atomic_ref

    int owner_count = 0;                // threads used when plan was initialized
    std::vector<int> owner_cursor;      // blocks consumed per owner (size = owner_count)

    // call this when blocks (or thread_count) may have changed
    void ensure_owner_slots(int threads) {
        if (owner_count != threads || owner_cursor.size() != (size_t)threads) {
            owner_count = threads;
            owner_cursor.assign(threads, 0); // reset resume state (mapping changed)
        }
    }

    // optional: reset after completion
    void reset_progress_only() {
        std::fill(owner_cursor.begin(), owner_cursor.end(), 0);
    }
};

inline double now_ms()
{
    #ifdef __EMSCRIPTEN__
    return emscripten_get_now(); // high-res ms
    #else
    using clock = std::chrono::steady_clock;
    return std::chrono::duration<double, std::milli>(clock::now().time_since_epoch()).count();
    #endif
}

// build/rebuild micro-blocks. Returns true if rebuilt.
inline bool ensure_blocks_built(TileBlockProgress& P,
    int bmp_w, int bmp_h,
    int tile_w, int tile_h,
    int block_w, int block_h)
{
    if (block_w <= 0) block_w = 64;
    if (block_h <= 0) block_h = 8;

    const bool changed =
        (P.bmp_w != bmp_w) || (P.bmp_h != bmp_h) ||
        (P.tile_w != tile_w) || (P.tile_h != tile_h) ||
        (P.block_w != block_w) || (P.block_h != block_h);

    if (!changed) return false;

    P.bmp_w = bmp_w;  P.bmp_h = bmp_h;
    P.tile_w = tile_w; P.tile_h = tile_h;
    P.block_w = block_w; P.block_h = block_h;

    P.tiles_x = (bmp_w + tile_w - 1) / tile_w;
    P.tiles_y = (bmp_h + tile_h - 1) / tile_h;
    const int tile_count = P.tiles_x * P.tiles_y;

    std::vector<Block> blocks;
    blocks.reserve((bmp_w / block_w + 1) * (bmp_h / block_h + 1));

    std::vector<uint32_t> total(tile_count, 0), done(tile_count, 0);

    for (int ty = 0; ty < P.tiles_y; ++ty) {
        for (int tx = 0; tx < P.tiles_x; ++tx) {
            const int tile_index = ty * P.tiles_x + tx;

            const int x0 = tx * tile_w;
            const int y0 = ty * tile_h;
            const int x1 = std::min(x0 + tile_w, bmp_w);
            const int y1 = std::min(y0 + tile_h, bmp_h);

            for (int by = y0; by < y1; by += block_h) {
                const int yy1 = std::min(by + block_h, y1);
                for (int bx = x0; bx < x1; bx += block_w) {
                    const int xx1 = std::min(bx + block_w, x1);
                    blocks.push_back(Block{ tile_index, bx, by, xx1, yy1 });
                    ++total[tile_index];
                }
            }
        }
    }

    P.blocks = std::move(blocks);
    P.blocks_total_per_tile = std::move(total);
    P.blocks_done_per_tile = std::move(done);
    P.next_block.store(0, std::memory_order_relaxed);
    return true;
}


template<typename T>
class CanvasImageBase : public Image, public CanvasObjectBase<T>
{
protected:
    double bmp_fw = 0;
    double bmp_fh = 0;

    bool needs_reshading = false;
    Quad<T> prev_world_quad;

    friend class PaintContext;

public:

    void setNeedsReshading(bool b = true)
    {
        needs_reshading = b;
    }

    [[nodiscard]] bool needsReshading()
    {
        Quad<T> world_quad = CanvasObjectBase<T>::worldQuad();
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

    // todo: p could be a lower-precision world coord
    [[nodiscard]] IVec2 pixelPosFromWorld(Vec2<T> p)
    {
        return static_cast<IVec2>(CanvasObjectBase<T>::worldToUVRatio(p) * bmp_size);
    }

    template<typename WorldT>
    void worldPos(int px, int py, WorldT& wx, WorldT& wy)
    {
        Quad<WorldT> world_quad = static_cast<Quad<WorldT>>(CanvasObjectBase<T>::worldQuad());
        WorldT ax = world_quad.a.x, ay = world_quad.a.y;
        WorldT bx = world_quad.b.x, by = world_quad.b.y;
        WorldT cx = world_quad.c.x, cy = world_quad.c.y;
        WorldT dx = world_quad.d.x, dy = world_quad.d.y;

        WorldT t_bmp_w = static_cast<WorldT>(bmp_fw);
        WorldT t_bmp_h = static_cast<WorldT>(bmp_fh);

        WorldT bmp_fx = static_cast<WorldT>(px) + WorldT{ 0.5 };
        WorldT bmp_fy = static_cast<WorldT>(py) + WorldT{ 0.5 };

        WorldT _u = bmp_fx / t_bmp_w;
        WorldT _v = bmp_fy / t_bmp_h;

        WorldT scan_left_x  = ax + (dx - ax) * _v;
        WorldT scan_left_y  = ay + (dy - ay) * _v;
        WorldT scan_right_x = bx + (cx - bx) * _v;
        WorldT scan_right_y = by + (cy - by) * _v;

        wx = scan_left_x + (scan_right_x - scan_left_x) * _u;
        wy = scan_left_y + (scan_right_y - scan_left_y) * _u;
    }
    

    template<typename Callback>
    bool forEachPixel(
        int& current_row,
        Callback&& callback,
        int thread_count = Thread::threadCount(),
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


    template<typename WorldT, typename Callback>
    bool forEachWorldPixel(
        int& current_row,
        Callback&& callback,
        int thread_count = Thread::threadCount(),
        int timeout_ms = 0,
        std::atomic<bool>*busy = nullptr
    )
    {
        auto timeout = timeout_ms ?
            std::chrono::milliseconds{ timeout_ms } :
            std::chrono::steady_clock::duration::max();

        // World quad might be higher precision than is requested for the current zoom level, downgrade to requested WorldT
        Quad<WorldT> world_quad = static_cast<Quad<WorldT>>( CanvasObjectBase<T>::worldQuad() );

        WorldT ax = world_quad.a.x, ay = world_quad.a.y;
        WorldT bx = world_quad.b.x, by = world_quad.b.y;
        WorldT cx = world_quad.c.x, cy = world_quad.c.y;
        WorldT dx = world_quad.d.x, dy = world_quad.d.y;

        WorldT t_bmp_w = static_cast<WorldT>(bmp_fw);
        WorldT t_bmp_h = static_cast<WorldT>(bmp_fh);

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
                        WorldT bmp_fx, bmp_fy = static_cast<WorldT>(row) + WorldT{ 0.5 };
                        WorldT _v = bmp_fy / t_bmp_h;
                        WorldT scan_left_x = ax + (dx - ax) * _v;
                        WorldT scan_left_y = ay + (dy - ay) * _v;
                        WorldT scan_right_x = bx + (cx - bx) * _v;
                        WorldT scan_right_y = by + (cy - by) * _v;
                        for (int bmp_x = 0; bmp_x < bmp_width; ++bmp_x)
                        {
                            bmp_fx = static_cast<WorldT>(bmp_x) + WorldT{ 0.5 };
                            WorldT _u = bmp_fx / t_bmp_w;
                            WorldT wx = scan_left_x + (scan_right_x - scan_left_x) * _u;
                            WorldT wy = scan_left_y + (scan_right_y - scan_left_y) * _u;

                            if constexpr (std::is_invocable_r_v<void, Callback, int, int, WorldT, WorldT, int>)
                                std::forward<Callback>(callback)(bmp_x, row, wx, wy, thread_index); 
                            else if constexpr (std::is_invocable_r_v<void, Callback, int, int, WorldT, WorldT>)
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
            WorldT bmp_fx, bmp_fy;
            for (int bmp_y = 0; bmp_y < bmp_height; ++bmp_y)
            {
                // Interpolate left and right edges of the scanline
                bmp_fy = static_cast<WorldT>(bmp_y) + WorldT{ 0.5 };
                WorldT _v = bmp_fy / t_bmp_h;
                WorldT scan_left_x = ax + (dx - ax) * _v;
                WorldT scan_left_y = ay + (dy - ay) * _v;
                WorldT scan_right_x = bx + (cx - bx) * _v;
                WorldT scan_right_y = by + (cy - by) * _v;
                for (int bmp_x = 0; bmp_x < bmp_width; ++bmp_x)
                {
                    bmp_fx = static_cast<WorldT>(bmp_x) + WorldT{ 0.5 };
                    WorldT _u = bmp_fx / t_bmp_w;
                    WorldT wx = scan_left_x + (scan_right_x - scan_left_x) * _u;
                    WorldT wy = scan_left_y + (scan_right_y - scan_left_y) * _u;

                    if constexpr (std::is_invocable_r_v<void, Callback, int, int, WorldT, WorldT, int>)
                        std::forward<Callback>(callback)(bmp_x, bmp_y, wx, wy, 0);
                    else if constexpr (std::is_invocable_r_v<void, Callback, int, int, WorldT, WorldT>)
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
        int thread_count = Thread::threadCount())
    {
        int row = 0;
        forEachPixel(
            row,
            callback,
            thread_count,
            0
        );
    }

    template<typename WorldT, typename Callback>
    bool forEachWorldTile(
        int tile_w, int tile_h,
        Callback&& callback,
        int thread_count = Thread::threadCount()
    )
    {
        int current_tile = 0;

        // World quad might be higher precision than is requested for the current zoom level, downgrade to requested WorldT
        Quad<WorldT> world_quad = static_cast<Quad<WorldT>>(CanvasObjectBase<T>::worldQuad());

        WorldT ax = world_quad.a.x, ay = world_quad.a.y;
        WorldT bx = world_quad.b.x, by = world_quad.b.y;
        WorldT cx = world_quad.c.x, cy = world_quad.c.y;
        WorldT dx = world_quad.d.x, dy = world_quad.d.y;

        WorldT t_bmp_w = static_cast<WorldT>(bmp_fw);
        WorldT t_bmp_h = static_cast<WorldT>(bmp_fh);

        const int tiles_x = (bmp_width + tile_w - 1) / tile_w;
        const int tiles_y = (bmp_height + tile_h - 1) / tile_h;
        const int tile_count = tiles_x * tiles_y;

        auto start_time = std::chrono::steady_clock::now();

        std::vector<std::future<void>> futures(thread_count);
        std::vector<std::atomic<bool>> active_threads(thread_count);
        for (int ti = 0; ti < thread_count; ++ti)
            active_threads[ti].store(false, std::memory_order_relaxed);

        std::atomic<bool> timed_out{ false };

        // Continuously spin checking for idle threads and scheduling new tiles...
        while (!timed_out.load(std::memory_order_relaxed))
        {
            for (int ti = 0; ti < thread_count; ++ti)
            {
                if (timed_out.load(std::memory_order_relaxed))
                    break;

                if (active_threads[ti].load(std::memory_order_relaxed))
                    continue;

                // Found an idle thread...
                const int thread_index = ti;
                const int tile_index = current_tile++;

                if (tile_index >= tile_count)
                {
                    timed_out.store(true, std::memory_order_relaxed);
                    break;
                }

                const int tx = tile_index % tiles_x;
                const int ty = tile_index / tiles_x;

                const int x0 = tx * tile_w;
                const int y0 = ty * tile_h;
                const int x1 = std::min(x0 + tile_w, bmp_width);
                const int y1 = std::min(y0 + tile_h, bmp_height);

                active_threads[thread_index].store(true, std::memory_order_relaxed);

                futures[ti] = Thread::pool().submit_task([&, x0, y0, x1, y1, thread_index, tile_index]()
                {
                    const int px = x0 + (x1 - x0 - 1) / 2;
                    const int py = y0 + (y1 - y0 - 1) / 2;

                    // Pixel-center sampling
                    const WorldT bmp_fx = static_cast<WorldT>(px) + WorldT{ 0.5 };
                    const WorldT bmp_fy = static_cast<WorldT>(py) + WorldT{ 0.5 };

                    // Interpolate world coords along the two vertical edges at this scanline
                    const WorldT v = bmp_fy / t_bmp_h;
                    const WorldT scan_left_x  = ax + (dx - ax) * v;
                    const WorldT scan_left_y  = ay + (dy - ay) * v;
                    const WorldT scan_right_x = bx + (cx - bx) * v;
                    const WorldT scan_right_y = by + (cy - by) * v;

                    // Interpolate across the scanline
                    const WorldT u = bmp_fx / t_bmp_w;
                    const WorldT wx = scan_left_x + (scan_right_x - scan_left_x) * u;
                    const WorldT wy = scan_left_y + (scan_right_y - scan_left_y) * u;

                    if constexpr (std::is_invocable_r_v<void, Callback, int, int, WorldT, WorldT, int, int, int, int, int>)
                    {
                        std::forward<Callback>(callback)(px, py, wx, wy, tile_index, x0, y0, x1, y1);
                    }
                    else
                    {
                        std::forward<Callback>(callback)(px, py, wx, wy, tile_index);
                    }

                    // Allow this worker to grab another tile
                    active_threads[thread_index].store(false, std::memory_order_relaxed);
                });
            }

            std::this_thread::yield();
        }

        // After timing out (or finishing), wait for remaining tasks
        for (int ti = 0; ti < thread_count; ++ti)
        {
            if (futures[ti].valid())
                futures[ti].get();
        }

        if (current_tile >= tile_count)
        {
            current_tile = 0;
            return true;
        }
        return false;
    }

    template<typename WorldT, typename Callback>
    bool forEachWorldTilePixel(
        int tile_w, int tile_h,
        TileBlockProgress& P, // progress tracker
        Callback&& callback,
        int thread_count = Thread::threadCount(),
        int budget_ms = 16, // 0 = no timeout (finish in this call)
        int block_w = 64,
        int block_h = 8
    )
    {
        ensure_blocks_built(P, bmp_width, bmp_height, tile_w, tile_h, block_w, block_h);

        // initialize cursors if thread_count changed
        P.ensure_owner_slots(thread_count);

        const int N = static_cast<int>(P.blocks.size());
        if (N == 0) return true;

        const bool no_timeout = (budget_ms == 0);
        const double t_end = no_timeout ? std::numeric_limits<double>::max() : (now_ms() + double(budget_ms));

        // world quad
        Quad<WorldT> world_quad = static_cast<Quad<WorldT>>(CanvasObjectBase<T>::worldQuad());
        const WorldT ax = world_quad.a.x, ay = world_quad.a.y;
        const WorldT bx = world_quad.b.x, by = world_quad.b.y;
        const WorldT cx = world_quad.c.x, cy = world_quad.c.y;
        const WorldT dx = world_quad.d.x, dy = world_quad.d.y;

        const WorldT t_bmp_w = static_cast<WorldT>(bmp_fw);
        const WorldT t_bmp_h = static_cast<WorldT>(bmp_fh);

        // one persistent task per worker
        std::vector<std::future<void>> futs;
        futs.reserve(thread_count);

        for (int k = 0; k < thread_count; ++k)
        {
            futs.push_back(Thread::pool().submit_task([&, owner_id = k]()
            {
                int M = P.owner_count;              // threads used for this plan
                int cur = P.owner_cursor[owner_id]; // blocks already consumed by this owner
                auto idx_from_cur = [&](int c) {
                    return owner_id + c * M;
                };

                while (true)
                {
                    // stop between blocks only
                    if (!no_timeout && now_ms() >= t_end)
                        break;

                    int bi = idx_from_cur(cur);
                    if (bi >= N)
                        break; // this owner finished its sequence

                    const Block b = P.blocks[bi];
                    const int x0 = b.x0, x1 = b.x1, y0 = b.y0, y1 = b.y1;

                    // render micro-block
                    for (int row = y0; row < y1; ++row)
                    {
                        const WorldT bmp_fy = static_cast<WorldT>(row) + WorldT{ 0.5 };
                        const WorldT v = bmp_fy / t_bmp_h;

                        const WorldT scan_left_x = ax + (dx - ax) * v;
                        const WorldT scan_left_y = ay + (dy - ay) * v;
                        const WorldT scan_right_x = bx + (cx - bx) * v;
                        const WorldT scan_right_y = by + (cy - by) * v;

                        for (int bmp_x = x0; bmp_x < x1; ++bmp_x)
                        {
                            const WorldT bmp_fx = static_cast<WorldT>(bmp_x) + WorldT{ 0.5 };
                            const WorldT u = bmp_fx / t_bmp_w; // TODO: Not efficient for f128 WorldT. Either cache or calculate offset in double precision

                            const WorldT wx = scan_left_x + (scan_right_x - scan_left_x) * u;
                            const WorldT wy = scan_left_y + (scan_right_y - scan_left_y) * u;

                            if constexpr (std::is_invocable_r_v<void, Callback, int, int, WorldT, WorldT, int>)
                                std::forward<Callback>(callback)(bmp_x, row, wx, wy, b.tile_index);
                            else if constexpr (std::is_invocable_r_v<void, Callback, int, int, WorldT, WorldT>)
                                std::forward<Callback>(callback)(bmp_x, row, wx, wy);
                        }
                    }
                    ++cur; // advance owner's local cursor
                }

                // write back owner's progress
                P.owner_cursor[owner_id] = cur;
            }));
        }

        for (auto& f : futs) f.get();

        bool finished = true;
        for (int k = 0; k < P.owner_count; ++k) {
            const int bi = k + P.owner_cursor[k] * P.owner_count;
            if (bi < N) { finished = false; break; }
        }

        if (finished) {
            P.reset_progress_only();
            return true;
        }
        return false;
    }

};

typedef CanvasImageBase<double> CanvasImage;
typedef CanvasImageBase<f128> CanvasImage128;

BL_END_NS
