#pragma once

#ifdef __EMSCRIPTEN__
#include <GLES3/gl3.h>
#define NANOVG_GLES3
#else
#include "glad/glad.h"
#define NANOVG_GL3
#endif

#include "nanovg.h"
#include "nanovg_gl.h"

NVGcontext* nvgCreate(int flags);
void nvgDelete(NVGcontext* ctx);

int nvglCreateImageFromHandle(NVGcontext* ctx, GLuint textureId, int w, int h, int flags);
GLuint nvglImageHandle(NVGcontext* ctx, int image);
