#ifdef __EMSCRIPTEN__
#define NANOVG_GLES3_IMPLEMENTATION
#else
#define NANOVG_GL3_IMPLEMENTATION
#endif

#include <bitloop/nanovgx/nanovgx.h>

#if defined NANOVG_GLES3
NVGcontext* nvgCreate(int flags) {
    return nvgCreateGLES3(flags);
}
void nvgDelete(NVGcontext* ctx) {
    nvgDeleteGLES3(ctx);
}

int nvglCreateImageFromHandle(NVGcontext* ctx, GLuint textureId, int w, int h, int flags) {
    return nvglCreateImageFromHandleGLES3(ctx, textureId, w, h, flags);
}
GLuint nvglImageHandle(NVGcontext* ctx, int image) {
    return nvglImageHandleGLES3(ctx, image);
}
#elif defined NANOVG_GL3
NVGcontext* nvgCreate(int flags) {
    return nvgCreateGL3(flags);
}
void nvgDelete(NVGcontext* ctx) {
    nvgDeleteGL3(ctx);
}

int nvglCreateImageFromHandle(NVGcontext* ctx, GLuint textureId, int w, int h, int flags) {
    return nvglCreateImageFromHandleGL3(ctx, textureId, w, h, flags);
}
GLuint nvglImageHandle(NVGcontext* ctx, int image) {
    return nvglImageHandleGL3(ctx, image);
}
#endif
