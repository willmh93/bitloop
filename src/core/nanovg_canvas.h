#pragma once

/// OpenGL
#ifdef __EMSCRIPTEN__
#include <GLES3/gl3.h>
#else
#include "glad.h"
#endif



#include "nanovg.h"
#include "nanovg_gl.h"

class Canvas
{
    NVGcontext* vg = nullptr;
    GLuint fbo = 0, tex = 0, rbo = 0;
    int fbo_width = 0, fbo_height = 0;

public:

    void create();
    bool resize(int w, int h);

    void begin(float r, float g, float b, float a = 1.0);
    void end();

    GLuint texture() { return tex; }
    int width() { return fbo_width; }
    int height() { return fbo_height; }

    void setFillStyle(int r, int g, int b, int a=255)
    {
        nvgFillColor(vg, nvgRGBA(r, g, b, a));
    }

    void beginPath()
    {
        nvgBeginPath(vg);
    }

    void fill()
    {
        nvgFill(vg);
    }

    void drawCircle(double x, double y, double radius)
    {
        nvgCircle(vg, x, y, radius);
    }
};