#pragma once
#include <bitloop/util/thread_queue.h>
#include "nanovgx.h"

BL_BEGIN_NS;

struct GLStateGuard
{
    GLint prevFbo = 0;
    GLint prevViewport[4] = { 0,0,0,0 };
    GLint prevProg = 0;
    GLint prevVAO = 0;
    GLint prevActiveTex = 0;
    GLint prevTex2D = 0;
    GLint prevPackAlign = 4;
    GLint prevUnpackAlign = 4;

    GLStateGuard()
    {
        glGetIntegerv(GL_FRAMEBUFFER_BINDING, &prevFbo);
        glGetIntegerv(GL_VIEWPORT, prevViewport);
        glGetIntegerv(GL_CURRENT_PROGRAM, &prevProg);
        glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &prevVAO);
        glGetIntegerv(GL_ACTIVE_TEXTURE, &prevActiveTex);

        glActiveTexture(GL_TEXTURE0);
        glGetIntegerv(GL_TEXTURE_BINDING_2D, &prevTex2D);

        glGetIntegerv(GL_PACK_ALIGNMENT, &prevPackAlign);
        glGetIntegerv(GL_UNPACK_ALIGNMENT, &prevUnpackAlign);
    }

    ~GLStateGuard()
    {
        glPixelStorei(GL_PACK_ALIGNMENT, prevPackAlign);
        glPixelStorei(GL_UNPACK_ALIGNMENT, prevUnpackAlign);

        glBindTexture(GL_TEXTURE_2D, (GLuint)prevTex2D);
        glActiveTexture((GLenum)prevActiveTex);

        glBindVertexArray((GLuint)prevVAO);
        glUseProgram((GLuint)prevProg);

        glBindFramebuffer(GL_FRAMEBUFFER, (GLuint)prevFbo);
        glViewport(prevViewport[0], prevViewport[1], prevViewport[2], prevViewport[3]);
    }
};

class ShaderSurface
{
    std::string vertex_source =
        "#version 300 es\n"
        "precision highp float;\n"
        "out vec2 v_uv;\n"
        "void main(){\n"
        "  vec2 pos;\n"
        "  if (gl_VertexID==0) pos=vec2(-1.0,-1.0);\n"
        "  else if (gl_VertexID==1) pos=vec2(3.0,-1.0);\n"
        "  else pos=vec2(-1.0, 3.0);\n"
        "  v_uv=0.5*(pos+1.0);\n"
        "  gl_Position=vec4(pos,0.0,1.0);\n"
        "}\n";

    std::string fragment_source;

    mutable bool sources_dirty = false;

    mutable GLuint program = 0;
    mutable GLuint vertex_array = 0;

    mutable GLuint output_texture = 0;
    mutable GLuint framebuffer = 0;
    mutable int out_w = 0;
    mutable int out_h = 0;

    mutable NVGcontext* nvg_ctx = nullptr;
    mutable int nvg_image = 0;

    mutable std::unordered_map<std::string, GLint> uniform_locations;

    mutable bool vertex_errors = false;
    mutable bool fragment_errors = false;
    mutable std::string vertex_log;
    mutable std::string fragment_log;

    mutable int tex_unit = 0;

    void destroyProgramOnly() const
    {
        if (program) glDeleteProgram(program);
        program = 0;
        uniform_locations.clear();
    }

    void invalidateNvgImage() const
    {
        if (nvg_image && nvg_ctx)
            nvgDeleteImage(nvg_ctx, nvg_image);

        nvg_image = 0;
        nvg_ctx = nullptr;
    }

    GLint uniformLocation(std::string_view name) const
    {
        auto it = uniform_locations.find(std::string(name));
        if (it != uniform_locations.end())
            return it->second;

        GLint loc = glGetUniformLocation(program, std::string(name).c_str());
        uniform_locations.emplace(std::string(name), loc);
        return loc;
    }

    static GLuint compileShader(GLenum type, const char* src, std::string* log=nullptr)
    {
        GLuint s = glCreateShader(type);
        glShaderSource(s, 1, &src, nullptr);
        glCompileShader(s);

        GLint ok = 0;
        glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
        if (!ok)
        {
            GLint logLen = 0;
            glGetShaderiv(s, GL_INFO_LOG_LENGTH, &logLen);

            if (log)
            {
                log->resize((logLen > 1) ? size_t(logLen) : 1);
                glGetShaderInfoLog(s, logLen, nullptr, log->data());

                blPrint("shader compile failed (%s):\n%s\n",
                    (type == GL_VERTEX_SHADER ? "VS" : "FS"),
                    log->c_str());
            }

            glDeleteShader(s);
            return 0;
        }

        return s;
    }

    static GLuint linkProgram(GLuint vs, GLuint fs)
    {
        GLuint p = glCreateProgram();
        glAttachShader(p, vs);
        glAttachShader(p, fs);
        glLinkProgram(p);

        GLint ok = 0;
        glGetProgramiv(p, GL_LINK_STATUS, &ok);
        if (!ok)
        {
            GLint logLen = 0;
            glGetProgramiv(p, GL_INFO_LOG_LENGTH, &logLen);

            std::string log;
            log.resize((logLen > 1) ? size_t(logLen) : 1);
            glGetProgramInfoLog(p, logLen, nullptr, log.data());

            blPrint("program link failed:\n%s\n", log.c_str());

            glDeleteProgram(p);
            return 0;
        }

        return p;
    }

    static int createNvgImageFromTextureHandle(NVGcontext* vg, GLuint tex, int w, int h)
    {
        return nvglCreateImageFromHandle(vg, (int)tex, w, h, NVG_IMAGE_NEAREST);
    }

public:

    ShaderSurface() = default;
    ~ShaderSurface() { destroy(); }

    void destroy()
    {
        if (nvg_image && nvg_ctx)
            nvgDeleteImage(nvg_ctx, nvg_image);

        if (framebuffer) glDeleteFramebuffers(1, &framebuffer);
        if (output_texture) glDeleteTextures(1, &output_texture);
        if (vertex_array) glDeleteVertexArrays(1, &vertex_array);
        if (program) glDeleteProgram(program);

        nvg_image = 0;
        nvg_ctx = nullptr;

        framebuffer = 0;
        output_texture = 0;
        vertex_array = 0;
        program = 0;

        out_w = out_h = 0;
    }

    void setShaderSources(std::string_view vertex_src, std::string_view fragment_src)
    {
        vertex_source = vertex_src;
        fragment_source = fragment_src;
        sources_dirty = !vertex_source.empty() && !fragment_source.empty();
    }

    void setVertexSource(std::string_view vertex_src)
    {
        vertex_source = vertex_src;
        sources_dirty = !vertex_source.empty() && !fragment_source.empty();
    }

    void setFragmentSource(std::string_view fragment_src)
    {
        fragment_source = fragment_src;
        sources_dirty = !vertex_source.empty() && !fragment_source.empty();
    }

    void setScale(float s)
    {
	    // todo:
        (void)s;
    }

    void setFilter(std::string_view f)
    {
	    // todo:
        (void)f;
    }

    bool errors() const
    {
        return vertex_errors || fragment_errors;
    }

    bool testCompile(
        bool& vert_errors,
        bool& frag_errors,
        std::string& vert_log,
        std::string& frag_log)
    {
        GLuint vs = compileShader(GL_VERTEX_SHADER, vertex_source.c_str(), &vert_log);
        GLuint fs = compileShader(GL_FRAGMENT_SHADER, fragment_source.c_str(), &frag_log);
        vert_errors = (vs == 0);
        frag_errors = (fs == 0);
        if (vs) glDeleteShader(vs);
        if (fs) glDeleteShader(fs);
        return !(vert_errors || frag_errors);
    }

    bool updateProgramSources() const
    {
        if (!sources_dirty)
            return false;

        GLuint vs = compileShader(GL_VERTEX_SHADER, vertex_source.c_str(), &vertex_log);
        GLuint fs = compileShader(GL_FRAGMENT_SHADER, fragment_source.c_str(), &fragment_log);
        vertex_errors = (vs == 0);
        fragment_errors = (fs == 0);

        if (vertex_errors || fragment_errors)
        {
            if (vs) glDeleteShader(vs);
            if (fs) glDeleteShader(fs);
            sources_dirty = false;
            return false;
        }

        GLuint new_program = linkProgram(vs, fs);

        glDeleteShader(vs);
        glDeleteShader(fs);

        if (new_program == 0)
        {
            // avoid recompiling the same broken sources repeatedly
            sources_dirty = false;
            return false;
        }

        // only drop the old program after the new one is valid
        destroyProgramOnly();
        program = new_program;

        if (vertex_array == 0)
            glGenVertexArrays(1, &vertex_array);

        uniform_locations.clear();

        sources_dirty = false;
        return true;
    }


    void ensureOutput(int w, int h) const
    {
        if (w <= 0 || h <= 0)
            return;

        if (output_texture != 0 && out_w == w && out_h == h)
            return;

        // resizing requires texture reallocation, which also invalidates the NVG image wrapper
        invalidateNvgImage();

        if (output_texture)
            glDeleteTextures(1, &output_texture);

        glGenTextures(1, &output_texture);
        glBindTexture(GL_TEXTURE_2D, output_texture);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

        glBindTexture(GL_TEXTURE_2D, 0);

        if (framebuffer == 0)
            glGenFramebuffers(1, &framebuffer);

        out_w = w;
        out_h = h;
    }

    GLuint texture() const { return output_texture; }
    int width()  const { return out_w; }
    int height() const { return out_h; }

    // render pass into the output texture
    template<typename BindFn>
    void render(int w, int h, BindFn&& bind_inputs_and_uniforms) const
    {
        updateProgramSources();

        if (!program)
            return;

        ensureOutput(w, h);
        if (!output_texture || !framebuffer)
            return;

        GLStateGuard guard;

        glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, output_texture, 0);

        glViewport(0, 0, w, h);

        glUseProgram(program);
        glBindVertexArray(vertex_array);

        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        tex_unit = 0;
        bind_inputs_and_uniforms(*this);

        glDrawArrays(GL_TRIANGLES, 0, 3);
    }

    // create (or reuse) a NanoVG image that wraps the GL output texture
    int nvgImageId(NVGcontext* vg) const
    {
        if (!vg || output_texture == 0 || out_w <= 0 || out_h <= 0)
            return 0;

        if (nvg_image && nvg_ctx == vg)
            return nvg_image;

        nvg_ctx = vg;
        nvg_image = createNvgImageFromTextureHandle(vg, output_texture, out_w, out_h);

        return nvg_image;
    }

    // convenience binders
    void bindTexture2D(std::string_view sampler_uniform, GLuint tex) const
    {
        glActiveTexture(GL_TEXTURE0 + tex_unit);
        glBindTexture(GL_TEXTURE_2D, tex);

        GLint loc = uniformLocation(sampler_uniform);
        if (loc >= 0)
            glUniform1i(loc, tex_unit);

        tex_unit++;
    }

    void setUniform1f(std::string_view name, float v) const
    {
        GLint loc = uniformLocation(name);
        if (loc >= 0)
            glUniform1f(loc, v);
    }

    void setUniform2f(std::string_view name, float x, float y) const
    {
        GLint loc = uniformLocation(name);
        if (loc >= 0)
            glUniform2f(loc, x, y);
    }
};

typedef deferred_unique_ptr<ShaderSurface> ShaderSurfacePtr;

inline ShaderSurfacePtr makeShaderSurface(ThreadQueue& queue)
{
    return make_deferred_unique<ShaderSurface>(queue);
}


BL_END_NS;
