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
        Vec2 pos = { 0, 0 };
        struct { double x, y; };
    };

    union {
        Vec2 size = { 0, 0 };
        struct { double w, h; };
    };

    union {
        Vec2 align = { -1, -1 };
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

    void setAlign(const Vec2& _align)
    {
        align = _align;
    }

    void setStageRect(double _x, double _y, double _w, double _h)
    {
        coordinate_type = CoordinateType::STAGE;
        x = _x;
        y = _y;
        w = _w;
        h = _h;
    }

    void setWorldRect(double _x, double _y, double _w, double _h)
    {
        coordinate_type = CoordinateType::STAGE;
        x = _x;
        y = _y;
        w = _w;
        h = _h;
    }

    Vec2 topLeft()
    {
        Vec2 offset = { -(align_x + 1) * 0.5 * w , -(align_y + 1) * 0.5 * h };
        return Math::rotateOffset(offset, rotation) + pos;
    }

    FQuad getQuad()
    {
        Vec2 pivot = { (align_x + 1) * 0.5 * w , (align_y + 1) * 0.5 * h };
        FQuad quad = { { 0.0, 0.0 }, { w, 0.0 }, { w, h }, { 0.0, h } };

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

    double localAlignOffsetX()
    {
        return -(align_x + 1) * 0.5 * w;
    }

    double localAlignOffsetY()
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
    Image(NVGcontext* ctx, int w, int h)
    {
        create(w, h); 
    }

    int width() const { return bmp_width; }
    int height() const { return bmp_height; }
    int imageId() const { return nano_img; }

    void create(int w, int h) 
    {
        bmp_width = w; bmp_height = h;
        pixels.assign(size_t(w) * h * 4, 0);
        pending_resize = true;
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

    uint32_t getPixel(int x, int y) const 
    {
        size_t i = (size_t(y) * bmp_width + x) * 4;
        return 
            pixels[i] |
            pixels[i + 1] << 8 | 
            pixels[i + 2] << 16 | 
            pixels[i + 3] << 24;
    }

    uint32_t getPixelSafe(int x, int y) const
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
            nano_img = nvgCreateImageRGBA(vg, bmp_width, bmp_height, 0, pixels.data());
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
    FQuad prev_world_quad;

    friend class PaintContext;

public:

    void setNeedsReshading(bool b = true)
    {
        needs_reshading = b;
    }

    bool needsReshading(Camera* camera)
    {
        FQuad world_quad = getWorldQuad(camera);
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

    FQuad getWorldQuad(Camera* camera);


    // Callback format: void(int x, int y, double wx, double wy)
    template<typename Callback>
    void forEachWorldPixel(
        Camera* camera,
        Callback&& callback,
        int thread_count = Thread::idealThreadCount())
    {
        static_assert(std::is_invocable_r_v<void, Callback, int, int, double, double>,
            "Callback must be: void(int x, int y, double wx, double wy)");

        FQuad world_quad = getWorldQuad(camera);

        double step_wx = (world_quad.b.x - world_quad.a.x) / bmp_fw;
        double step_wy = (world_quad.b.y - world_quad.a.y) / bmp_fw;

        double scanline_origin_dx = (world_quad.d.x - world_quad.a.x) / bmp_fh;
        double scanline_origin_dy = (world_quad.d.y - world_quad.a.y) / bmp_fh;

        double scanline_origin_dist_x = (world_quad.d.x - world_quad.a.x);
        double scanline_origin_dist_y = (world_quad.d.y - world_quad.a.y);

        double scanline_origin_ax = world_quad.a.x;
        double scanline_origin_ay = world_quad.a.y;

        auto row_ranges = Thread::splitRanges(bmp_height, thread_count);
        std::vector<std::future<void>> futures(thread_count);

        if (thread_count > 0)
        {
            for (int ti = 0; ti < thread_count; ++ti)
            {
                auto row_range = row_ranges[ti];
                futures[ti] = Thread::pool().submit_task([&, row_range]()
                {
                    double ax = world_quad.a.x;
                    double ay = world_quad.a.y;
                    double bx = world_quad.b.x;
                    double by = world_quad.b.y;
                    double cx = world_quad.c.x;
                    double cy = world_quad.c.y;
                    double dx = world_quad.d.x;
                    double dy = world_quad.d.y;

                    for (int bmp_y = row_range.first; bmp_y < row_range.second; ++bmp_y)
                    {
                        double v = static_cast<double>(bmp_y) / static_cast<double>(bmp_fh);

                        // Interpolate left and right edges of the scanline
                        double scan_left_x = world_quad.a.x + (world_quad.d.x - world_quad.a.x) * v;
                        double scan_left_y = world_quad.a.y + (world_quad.d.y - world_quad.a.y) * v;

                        double scan_right_x = world_quad.b.x + (world_quad.c.x - world_quad.b.x) * v;
                        double scan_right_y = world_quad.b.y + (world_quad.c.y - world_quad.b.y) * v;

                        for (int bmp_x = 0; bmp_x < bmp_width; ++bmp_x)
                        {
                            double u = static_cast<double>(bmp_x) / static_cast<double>(bmp_fw);

                            double wx = scan_left_x + (scan_right_x - scan_left_x) * u;
                            double wy = scan_left_y + (scan_right_y - scan_left_y) * u;

                            std::forward<Callback>(callback)(bmp_x, bmp_y, wx, wy);
                        }
                    }
                });
            }

            for (auto& future : futures)
                future.get();
        }
        else
        {
            double origin_x = world_quad.a.x;
            double origin_y = world_quad.a.y;

            for (int bmp_y = 0; bmp_y < bmp_height; bmp_y++)
            {
                double wx = origin_x;
                double wy = origin_y;
                for (int bmp_x = 0; bmp_x < bmp_width; bmp_x++)
                {
                    std::forward<Callback>(callback)(bmp_x, bmp_y, wx, wy);
                    wx += step_wx;
                    wy += step_wy;
                }
                origin_x += scanline_origin_dx;
                origin_y += scanline_origin_dy;
            }
        }
    }
};

/*#include <vector>
#include <shared_mutex>
#include <atomic>
#include <cassert>
#include <cstdint>
#include <cstring>

class GLBitmap {
public:
    GLBitmap() = default;
    GLBitmap(int w, int h) { resize(w, h); }
    ~GLBitmap() { destroyGL(); }

    void resize(int w, int h) {
        std::unique_lock lock(mutex_);
        bmp_width = w;
        bmp_height = h;
        pixels.assign(static_cast<size_t>(w) * h * 4, 0);
        dirty_.store(true, std::memory_order_relaxed);
        needRealloc_ = true;
    }

    void setPixel(int x, int y, uint32_t rgba) {
        if ((unsigned)x >= (unsigned)bmp_width || (unsigned)y >= (unsigned)bmp_height) return;
        std::unique_lock lock(mutex_);
        size_t idx = (static_cast<size_t>(y) * bmp_width + x) * 4;
        pixels[idx + 0] = rgba & 0xFF;
        pixels[idx + 1] = (rgba >> 8) & 0xFF;
        pixels[idx + 2] = (rgba >> 16) & 0xFF;
        pixels[idx + 3] = (rgba >> 24) & 0xFF;
        dirty_.store(true, std::memory_order_release);
    }

    uint32_t getPixel(int x, int y) const {
        if ((unsigned)x >= (unsigned)bmp_width || (unsigned)y >= (unsigned)bmp_height) return 0;
        std::shared_lock lock(mutex_);
        size_t idx = (static_cast<size_t>(y) * bmp_width + x) * 4;
        return  pixels[idx + 0] |
            (pixels[idx + 1] << 8) |
            (pixels[idx + 2] << 16) |
            (pixels[idx + 3] << 24);
    }

    int  width()  const { return bmp_width; }
    int  height() const { return bmp_height; }
    bool empty()  const { return pixels.empty(); }

    void draw(float x, float y, float scale = 1.f) {
        if (bmp_width == 0 || bmp_height == 0) return;

        bool wasCull = glIsEnabled(GL_CULL_FACE);
        if (wasCull) glDisable(GL_CULL_FACE);

        ensureGL();
        ensureTexture();

        if (dirty_.exchange(false, std::memory_order_acq_rel)) {
            std::shared_lock lock(mutex_);
            glBindTexture(GL_TEXTURE_2D, tex_);
            glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
            glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0,
                bmp_width, bmp_height,
                GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
        }

        GLint vp[4];
        glGetIntegerv(GL_VIEWPORT, vp);
        float W = static_cast<float>(vp[2]);
        float H = static_cast<float>(vp[3]);
        float w = bmp_width * scale;
        float h = bmp_height * scale;
        float x0 = 2.f * x / W - 1.f;
        float x1 = 2.f * (x + w) / W - 1.f;
        float y0 = 1.f - 2.f * (y + h) / H;
        float y1 = 1.f - 2.f * y / H;

        float verts[6][4] = {
            {x0,y1, 0,1}, {x1,y1, 1,1}, {x1,y0, 1,0},
            {x0,y1, 0,1}, {x1,y0, 1,0}, {x0,y0, 0,0}
        };

        glBindVertexArray(vao_);
        glBindBuffer(GL_ARRAY_BUFFER, vbo_);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(verts), verts);

        glUseProgram(prog_);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, tex_);
        glUniform1i(uniformTex_, 0);
        glUniformMatrix4fv(uniformMVP_, 1, GL_FALSE, identity_);

        glDrawArrays(GL_TRIANGLES, 0, 6);

        if (wasCull) glEnable(GL_CULL_FACE);
    }

    GLuint texture() const { return tex_; }

private:
    void ensureGL() {
        if (glReady_) return;
        compileProgram();
        setupQuad();
        glReady_ = true;
    }

    void compileProgram() {
        const char* vsSrc = (GLSL_SRC_VERSION
            "in  vec2 aPos;\n"
            "in  vec2 aUV;\n"
            "uniform mat4 uMVP;\n"
            "out vec2 vUV;\n"
            "void main(){ vUV=aUV; gl_Position=uMVP*vec4(aPos,0,1);}\n");

        const char* fsSrc = (GLSL_SRC_VERSION GLSL_OUT_PREC
            "in  vec2 vUV;\n"
            "uniform sampler2D uTex;\n"
            "out vec4 FragColor;\n"
            "void main(){ FragColor = texture(uTex, vUV);}\n");

        GLuint vs = compile(GL_VERTEX_SHADER, vsSrc);
        GLuint fs = compile(GL_FRAGMENT_SHADER, fsSrc);
        prog_ = glCreateProgram();
        glAttachShader(prog_, vs);
        glAttachShader(prog_, fs);

        #ifndef __EMSCRIPTEN__
        glBindAttribLocation(prog_, 0, "aPos");
        glBindAttribLocation(prog_, 1, "aUV");
        glBindFragDataLocation(prog_, 0, "FragColor");
        #endif

        glLinkProgram(prog_);
        glDeleteShader(vs);
        glDeleteShader(fs);

        uniformMVP_ = glGetUniformLocation(prog_, "uMVP");
        uniformTex_ = glGetUniformLocation(prog_, "uTex");
    }

    void setupQuad() {
        glGenVertexArrays(1, &vao_);
        glBindVertexArray(vao_);
        glGenBuffers(1, &vbo_);
        glBindBuffer(GL_ARRAY_BUFFER, vbo_);
        glBufferData(GL_ARRAY_BUFFER, 6 * 4 * sizeof(float), nullptr, GL_DYNAMIC_DRAW);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    }

    GLuint compile(GLenum type, const char* src) {
        GLuint s = glCreateShader(type);
        glShaderSource(s, 1, &src, nullptr);
        glCompileShader(s);
        #ifndef NDEBUG
        GLint ok; glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
        if (!ok) {
            char log[512];
            glGetShaderInfoLog(s, 512, nullptr, log);
            fprintf(stderr, "Shader compile error: %s\n", log);
        }
        #endif
        return s;
    }

    void ensureTexture() {
        if (tex_ && !needRealloc_) return;
        destroyGL();
        glGenTextures(1, &tex_);
        glBindTexture(GL_TEXTURE_2D, tex_);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, bmp_width, bmp_height, 0,
            GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
        needRealloc_ = false;
    }

    void destroyGL() {
        if (tex_) glDeleteTextures(1, &tex_);
        tex_ = 0;
    }

    std::vector<unsigned char> pixels;
    int  bmp_width = 0, bmp_height = 0;
    mutable std::shared_mutex mutex_;
    std::atomic<bool> dirty_{ true };

    bool   glReady_ = false;
    bool   needRealloc_ = true;
    GLuint tex_ = 0;
    GLuint vao_ = 0, vbo_ = 0;
    GLuint prog_ = 0;
    GLint  uniformMVP_ = -1;
    GLint  uniformTex_ = -1;
    const float identity_[16] = { 1,0,0,0,  0,1,0,0,  0,0,1,0,  0,0,0,1 };
};
*/

/*
#include <cassert>
#include <memory>

struct FBOHolder
{
    GLuint fbo = 0;
    GLuint colorTex = 0;
    GLuint depthStencil = 0;
    int    width = 0;
    int    height = 0;

    FBOHolder() = default;
    FBOHolder(int w, int h) { recreate(w, h); }
    ~FBOHolder() { destroy(); }

    void recreate(int w, int h)
    {
        destroy();
        width = w;
        height = h;

        //--- FBO -----------------------------------------------------------
        glGenFramebuffers(1, &fbo);
        glBindFramebuffer(GL_FRAMEBUFFER, fbo);

        //--- Color texture --------------------------------------------------
        glGenTextures(1, &colorTex);
        glBindTexture(GL_TEXTURE_2D, colorTex);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height,
            0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glFramebufferTexture2D(GL_FRAMEBUFFER,
            GL_COLOR_ATTACHMENT0,
            GL_TEXTURE_2D,
            colorTex, 0);

        //--- Depth-stencil (DEPTH24_STENCIL8 ~ Qt’s CombinedDepthStencil) ---
        glGenRenderbuffers(1, &depthStencil);
        glBindRenderbuffer(GL_RENDERBUFFER, depthStencil);
        glRenderbufferStorage(GL_RENDERBUFFER,
            GL_DEPTH24_STENCIL8,
            width, height);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER,
            GL_DEPTH_STENCIL_ATTACHMENT,
            GL_RENDERBUFFER,
            depthStencil);

        assert(glCheckFramebufferStatus(GL_FRAMEBUFFER)
            == GL_FRAMEBUFFER_COMPLETE && "FBO not complete");

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void destroy()
    {
        if (depthStencil)  glDeleteRenderbuffers(1, &depthStencil);
        if (colorTex)      glDeleteTextures(1, &colorTex);
        if (fbo)           glDeleteFramebuffers(1, &fbo);
        depthStencil = colorTex = fbo = 0;
    }

    // Convenience ------------------------------------------------------------------
    void   bind()    const { glBindFramebuffer(GL_FRAMEBUFFER, fbo); }
    void   release() const { glBindFramebuffer(GL_FRAMEBUFFER, 0); }
    GLuint texture() const { return colorTex; }
};

struct GLSurface
{
    std::shared_ptr<FBOHolder> holder;

    GLint prevFBO = 0;
    GLint prevViewport[4] = { 0 };
    int   width = 0;
    int   height = 0;

    // ––––– PUBLIC API (mirrors your Qt version) –––––––––––––––––––––––––––
    void prepare(int w, int h)
    {
        if (!holder || w != width || h != height)
        {
            width = w;
            height = h;
            if (!holder)
                holder = std::make_shared<FBOHolder>(w, h);
            else
                holder->recreate(w, h);
        }
    }

    void bind()
    {
        glGetIntegerv(GL_FRAMEBUFFER_BINDING, &prevFBO);
        glGetIntegerv(GL_VIEWPORT, prevViewport);
        holder->bind();
        glViewport(0, 0, width, height);
    }

    void release()
    {
        holder->release();
        glBindFramebuffer(GL_FRAMEBUFFER, prevFBO);
        glViewport(prevViewport[0], prevViewport[1],
            prevViewport[2], prevViewport[3]);
    }

    void clear(float r = 0, float g = 0, float b = 0, float a = 0)
    {
        glClearColor(r, g, b, a);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }

    bool  prepared()    const { return holder != nullptr; }
    float aspectRatio() const { return height ? float(width) / float(height) : 0.f; }

    GLuint texture()    const { return holder ? holder->texture() : 0u; }
};
*/