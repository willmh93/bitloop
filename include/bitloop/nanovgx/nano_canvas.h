#pragma once

#include <memory>

#include <bitloop/platform/platform.h>

#include "nanovg.h"
#include "nanovg_gl.h"

#include <bitloop/core/debug.h>
#include <bitloop/core/camera.h>
#include <bitloop/nanovgx/nano_bitmap.h>

BL_BEGIN_NS

class Viewport;

enum struct PathWinding
{
    WINDING_CCW  = NVGwinding::NVG_CCW,
    WINDING_CW   = NVGwinding::NVG_CW
};
enum struct LineCap
{
    CAP_BUTT     = NVGlineCap::NVG_BUTT,
    CAP_ROUND    = NVGlineCap::NVG_ROUND,
    CAP_SQUARE   = NVGlineCap::NVG_SQUARE,
};
enum struct LineJoin
{
    JOIN_BEVEL   = NVGlineCap::NVG_BEVEL,
    JOIN_MITER   = NVGlineCap::NVG_MITER
};
enum struct TextAlign
{
    ALIGN_LEFT    = NVGalign::NVG_ALIGN_LEFT,
    ALIGN_CENTER  = NVGalign::NVG_ALIGN_CENTER,
    ALIGN_RIGHT   = NVGalign::NVG_ALIGN_RIGHT
};
enum struct TextBaseline
{
    BASELINE_TOP     = NVGalign::NVG_ALIGN_TOP,
    BASELINE_MIDDLE  = NVGalign::NVG_ALIGN_MIDDLE,
    BASELINE_BOTTOM  = NVGalign::NVG_ALIGN_BOTTOM
};
enum struct CompositeOperation
{
    SOURCE_OVER       = NVGcompositeOperation::NVG_SOURCE_OVER,
    SOURCE_IN         = NVGcompositeOperation::NVG_SOURCE_IN,
    SOURCE_OUT        = NVGcompositeOperation::NVG_SOURCE_OUT,
    ATOP              = NVGcompositeOperation::NVG_ATOP,
    DESTINATION_OVER  = NVGcompositeOperation::NVG_DESTINATION_OVER,
    DESTINATION_IN    = NVGcompositeOperation::NVG_DESTINATION_IN,
    DESTINATION_OUT   = NVGcompositeOperation::NVG_DESTINATION_OUT,
    DESTINATION_ATOP  = NVGcompositeOperation::NVG_DESTINATION_ATOP,
    LIGHTER           = NVGcompositeOperation::NVG_LIGHTER,
    COPY              = NVGcompositeOperation::NVG_COPY,
    XOR               = NVGcompositeOperation::NVG_XOR
};
enum struct BlendFactor
{
    ZERO                  = NVGblendFactor::NVG_ZERO,
    ONE                   = NVGblendFactor::NVG_ONE,
    SRC_COLOR             = NVGblendFactor::NVG_SRC_COLOR,
    ONE_MINUS_SRC_COLOR   = NVGblendFactor::NVG_ONE_MINUS_SRC_COLOR,
    DST_COLOR             = NVGblendFactor::NVG_DST_COLOR,
    ONE_MINUS_DST_COLOR   = NVGblendFactor::NVG_ONE_MINUS_DST_COLOR,
    SRC_ALPHA             = NVGblendFactor::NVG_SRC_ALPHA,
    ONE_MINUS_SRC_ALPHA   = NVGblendFactor::NVG_ONE_MINUS_SRC_ALPHA,
    DST_ALPHA             = NVGblendFactor::NVG_DST_ALPHA,
    ONE_MINUS_DST_ALPHA   = NVGblendFactor::NVG_ONE_MINUS_DST_ALPHA,
    SRC_ALPHA_SATURATE    = NVGblendFactor::NVG_SRC_ALPHA_SATURATE
};

struct CompositeScope
{
    NVGcontext* vg;
    CompositeScope(NVGcontext* vg, CompositeOperation op) : vg(vg) { nvgSave(vg); nvgGlobalCompositeOperation(vg, (int)op); }
    CompositeScope(NVGcontext* vg, BlendFactor src, BlendFactor dst) : vg(vg) { nvgSave(vg); nvgGlobalCompositeBlendFunc(vg, (int)src, (int)dst); }
    CompositeScope(NVGcontext* vg, BlendFactor sRGB, BlendFactor dRGB, BlendFactor sA, BlendFactor dA) : vg(vg) { nvgSave(vg); nvgGlobalCompositeBlendFuncSeparate(vg, (int)sRGB, (int)dRGB, (int)sA, (int)dA); }
    ~CompositeScope() { nvgRestore(vg); }
};

class NanoFontInternal
{
    friend class SimplePainter;

protected:

    std::string path;
    int id = 0;
    bool created = false;
    float size = 16.0f;

public:

    //static std::shared_ptr<NanoFont> create(const char* virtual_path)
    //{
    //    return std::make_shared<NanoFont>(virtual_path);
    //}

    NanoFontInternal(const char* virtual_path)
    {
        path = platform()->path(virtual_path);
    }

    void setSize(double size_pts)
    {
        size = (float)size_pts;
    }
};

struct NanoFont : std::shared_ptr<NanoFontInternal>
{
    static NanoFont create(const char* virtual_path)
    {
        return std::make_shared<NanoFontInternal>(virtual_path);
    }

    NanoFont() = default;
    NanoFont(const NanoFont& rhs) = default;
    NanoFont(std::shared_ptr<NanoFontInternal> f)
        : std::shared_ptr<NanoFontInternal>(f)
    {}
};

struct PainterContext
{
    NVGcontext* vg = nullptr;
    double global_scale = 1.0;

    // Text
    NanoFont default_font;
    NanoFont active_font;

    TextAlign text_align = TextAlign::ALIGN_LEFT;
    TextBaseline text_baseline = TextBaseline::BASELINE_TOP;

    double font_size = 16.0;

    double adjust_scale_x = 1.0;
    double adjust_scale_y = 1.0;
};

class Painter;

// ===========================================
// ======== Simple nanovg C++ wrapper ========
// ===========================================

static inline NVGcolor toNVG(const Color& c) { return nvgRGBA(c.r, c.g, c.b, c.a); }
static inline NVGcolor toNVG(const float(&rgb)[3], float a) { return NVGcolor{ rgb[0], rgb[1], rgb[2], a }; }
static inline NVGcolor toNVG(const float(&rgba)[4]) { return NVGcolor{ rgba[0], rgba[1], rgba[2], rgba[3] }; }

class SimplePainter
{
    PainterContext* paint_ctx = nullptr;
    NVGcontext* vg = nullptr;

protected:

    friend class Viewport;
    friend class CameraInfo;
    friend class Painter;

public:


    void usePainter(PainterContext* target)
    {
        paint_ctx = target;
        vg = target->vg;

        // Necessary? Project should be resetting them each frame anyway
        ///setFontSize(target->font_size);
    }

    // ======== Context getter/setters ========

    [[nodiscard]] NanoFont getDefaultFont() { return paint_ctx->default_font; }
    [[nodiscard]] double getGlobalScale() { return paint_ctx->global_scale; }
    void setGlobalScale(double scale) { paint_ctx->global_scale = scale; }
    void setGlobalAlpha(double alpha) { nvgGlobalAlpha(vg, (float)alpha); }

    // ======== Transforms ========

    void save() const { nvgSave(vg); }
    void restore() { nvgRestore(vg); }
    
    void resetTransform()                                    { nvgResetTransform(vg); }
    void transform(const glm::mat3& m)                       { nvgTransform(vg, m[0][0], m[0][1], m[1][0], m[1][1], m[2][0], m[2][1]); }
    glm::mat3 currentTransform() const                       { float x[6]; nvgCurrentTransform(vg, x); return glm::mat3(x[0], x[1], 0, x[2], x[3], 0, x[4], x[5], 1); }

    void translate(double x, double y)                       { nvgTranslate(vg, (float)(x), (float)(y)); }
    void translate(DVec2 p)                                  { nvgTranslate(vg, (float)(p.x), (float)(p.y)); }
    void rotate(double angle)                                { nvgRotate(vg, (float)(angle)); }
    void scale(double scale)                                 { nvgScale(vg, (float)(scale), (float)(scale)); }
    void scale(double scale_x, double scale_y)               { nvgScale(vg, (float)(scale_x), (float)(scale_y)); }
    void skewX(double angle)                                 { nvgSkewX(vg, (float)(angle)); }
    void skewY(double angle)                                 { nvgSkewY(vg, (float)(angle)); }
    void setClipRect(double x, double y, double w, double h) { nvgScissor(vg, (float)(x), (float)(y), (float)(w), (float)(h)); }
    void resetClipping()                                     { nvgResetScissor(vg); }

    // ======== Styles ========

    void setFillStyle(const Color& color)                    { nvgFillColor(vg, nvgRGBA(color.r, color.g, color.b, color.a)); }
    void setFillStyle(const Color& color, int a)             { nvgFillColor(vg, nvgRGBA(color.r, color.g, color.b, a)); }
    void setFillStyle(const float(&color)[3])                { nvgFillColor(vg, {color[0], color[1], color[2], 1.0f}); }
    void setFillStyle(const float(&color)[4])                { nvgFillColor(vg, {color[0], color[1], color[2], color[3]}); }
    void setFillStyle(int r, int g, int b, int a = 255)      { nvgFillColor(vg, nvgRGBA(r, g, b, a)); }
    
    void setStrokeStyle(const Color& color)                  { nvgStrokeColor(vg, nvgRGBA(color.r, color.g, color.b, color.a)); }
    void setStrokeStyle(const float(&color)[3])              { nvgStrokeColor(vg, {color[0], color[1], color[2], 1.0f}); }
    void setStrokeStyle(const float(&color)[4])              { nvgStrokeColor(vg, {color[0], color[1], color[2], color[3]}); }
    void setStrokeStyle(int r, int g, int b, int a = 255)    { nvgStrokeColor(vg, nvgRGBA(r, g, b, a)); }
    
    void setLineWidth(double w)                              { nvgStrokeWidth(vg, (float)w); }
    void setLineCap(LineCap cap)                             { nvgLineCap(vg, (int)cap); }
    void setLineJoin(LineJoin join)                          { nvgLineJoin(vg, (int)join); }
    void setMiterLimit(double limit)                         { nvgMiterLimit(vg, (float)limit); }

    // ======== composite ========

    void setCompositeSourceOver()             { nvgGlobalCompositeOperation(vg, NVG_SOURCE_OVER); }
    void setCompositeSourceIn()               { nvgGlobalCompositeOperation(vg, NVG_SOURCE_IN); }
    void setCompositeSourceOut()              { nvgGlobalCompositeOperation(vg, NVG_SOURCE_OUT); }
    void setCompositeSourceAtop()             { nvgGlobalCompositeOperation(vg, NVG_ATOP); }
    void setCompositeDestOver()               { nvgGlobalCompositeOperation(vg, NVG_DESTINATION_OVER); }
    void setCompositeDestIn()                 { nvgGlobalCompositeOperation(vg, NVG_DESTINATION_IN); }
    void setCompositeDestOut()                { nvgGlobalCompositeOperation(vg, NVG_DESTINATION_OUT); }
    void setCompositeDestAtop()               { nvgGlobalCompositeOperation(vg, NVG_DESTINATION_ATOP); }
    void setCompositeLighter()                { nvgGlobalCompositeOperation(vg, NVG_LIGHTER); }
    void setCompositeCopy()                   { nvgGlobalCompositeOperation(vg, NVG_COPY); }
    void setCompositeXor()                    { nvgGlobalCompositeOperation(vg, NVG_XOR); }
    void setComposite(CompositeOperation op)  { nvgGlobalCompositeOperation(vg, (int)op); }

    // ======== blending ========

    void setBlendFunc(BlendFactor src, BlendFactor dst) {
        nvgGlobalCompositeBlendFunc(vg, (int)src, (int)dst);
    }
    void setBlendFuncSeparate(BlendFactor srcRGB, BlendFactor dstRGB, BlendFactor srcA, BlendFactor dstA) {
        nvgGlobalCompositeBlendFuncSeparate(vg, (int)srcRGB, (int)dstRGB, (int)srcA, (int)dstA);
    }

    // blend presets
    void setBlendAdditive() { nvgGlobalCompositeOperation(vg, NVG_LIGHTER); }
    void setBlendAlphaPremult() { nvgGlobalCompositeBlendFunc(vg, NVG_ONE, NVG_ONE_MINUS_SRC_ALPHA); }
    void setBlendAlphaStraight() { nvgGlobalCompositeBlendFunc(vg, NVG_SRC_ALPHA, NVG_ONE_MINUS_SRC_ALPHA); }
    void setBlendMultiply() { nvgGlobalCompositeBlendFuncSeparate(vg, NVG_DST_COLOR, NVG_ONE_MINUS_SRC_ALPHA, NVG_ONE, NVG_ONE_MINUS_SRC_ALPHA); }
    void setBlendScreen() { nvgGlobalCompositeBlendFuncSeparate(vg, NVG_ONE, NVG_ONE_MINUS_DST_COLOR, NVG_ONE, NVG_ONE_MINUS_DST_ALPHA); }

    CompositeScope scopedComposite(CompositeOperation op) { return CompositeScope(vg, op); }
    CompositeScope scopedBlend(BlendFactor src, BlendFactor dst) { return CompositeScope(vg, src, dst); }
    CompositeScope scopedBlendSeparate(BlendFactor sRGB, BlendFactor dRGB, BlendFactor sA, BlendFactor dA) { return CompositeScope(vg, sRGB, dRGB, sA, dA); }

    // ======== Linear Gradient ========

    void setFillLinearGradient(double x0, double y0, double x1, double y1, const Color& inner, const Color& outer) {
        nvgFillPaint(vg, nvgLinearGradient(vg, float(x0), float(y0), float(x1), float(y1), toNVG(inner), toNVG(outer)));
    }
    void setFillLinearGradient(double x0, double y0, double x1, double y1, const float(&inner)[3], const float(&outer)[3], float a = 1.0f) {
        nvgFillPaint(vg, nvgLinearGradient(vg, float(x0), float(y0), float(x1), float(y1), toNVG(inner, a), toNVG(outer, a)));
    }
    void setFillLinearGradient(double x0, double y0, double x1, double y1, const float(&inner)[4], const float(&outer)[4]) {
        nvgFillPaint(vg, nvgLinearGradient(vg, float(x0), float(y0), float(x1), float(y1), toNVG(inner), toNVG(outer)));
    }

    // ======== Radial Gradient ========

    void setFillRadialGradient(double cx, double cy, double r0, double r1, const Color& inner, const Color& outer) {
        nvgFillPaint(vg, nvgRadialGradient(vg, float(cx), float(cy), float(r0), float(r1), toNVG(inner), toNVG(outer)));
    }
    void setFillRadialGradient(double cx, double cy, double r0, double r1, const float(&inner)[3], const float(&outer)[3], float a = 1.0f) {
        nvgFillPaint(vg, nvgRadialGradient(vg, float(cx), float(cy), float(r0), float(r1), toNVG(inner, a), toNVG(outer, a)));
    }
    void setFillRadialGradient(double cx, double cy, double r0, double r1, const float(&inner)[4], const float(&outer)[4]) {
        nvgFillPaint(vg, nvgRadialGradient(vg, float(cx), float(cy), float(r0), float(r1), toNVG(inner), toNVG(outer)));
    }

    // ======== Box Gradient ========

    void setFillBoxGradient(double x, double y, double w, double h, double r, double f, const Color& inner, const Color& outer) {
        nvgFillPaint(vg, nvgBoxGradient(vg, float(x), float(y), float(w), float(h), float(r), float(f), toNVG(inner), toNVG(outer)));
    }
    void setFillBoxGradient(double x, double y, double w, double h, double r, double f, const float(&inner)[3], const float(&outer)[3], float a = 1.0f) {
        nvgFillPaint(vg, nvgBoxGradient(vg, float(x), float(y), float(w), float(h), float(r), float(f), toNVG(inner, a), toNVG(outer, a)));
    }
    void setFillBoxGradient(double x, double y, double w, double h, double r, double f, const float(&inner)[4], const float(&outer)[4]) {
        nvgFillPaint(vg, nvgBoxGradient(vg, float(x), float(y), float(w), float(h), float(r), float(f), toNVG(inner), toNVG(outer)));
    }

    // ======== Linear Gradient (Stroke) ========

    void setStrokeLinearGradient(double x0, double y0, double x1, double y1, const Color& inner, const Color& outer) {
        nvgStrokePaint(vg, nvgLinearGradient(vg, float(x0), float(y0), float(x1), float(y1), toNVG(inner), toNVG(outer)));
    }
    void setStrokeLinearGradient(double x0, double y0, double x1, double y1, const float(&inner)[3], const float(&outer)[3], float a = 1.0f) {
        nvgStrokePaint(vg, nvgLinearGradient(vg, float(x0), float(y0), float(x1), float(y1), toNVG(inner, a), toNVG(outer, a)));
    }
    void setStrokeLinearGradient(double x0, double y0, double x1, double y1, const float(&inner)[4], const float(&outer)[4]) {
        nvgStrokePaint(vg, nvgLinearGradient(vg, float(x0), float(y0), float(x1), float(y1), toNVG(inner), toNVG(outer)));
    }

    // ======== Radial Gradient (Stroke) ========

    void setStrokeRadialGradient(double cx, double cy, double r0, double r1, const Color& inner, const Color& outer) {
        nvgStrokePaint(vg, nvgRadialGradient(vg, float(cx), float(cy), float(r0), float(r1), toNVG(inner), toNVG(outer)));
    }
    void setStrokeRadialGradient(double cx, double cy, double r0, double r1, const float(&inner)[3], const float(&outer)[3], float a = 1.0f) {
        nvgStrokePaint(vg, nvgRadialGradient(vg, float(cx), float(cy), float(r0), float(r1), toNVG(inner, a), toNVG(outer, a)));
    }
    void setStrokeRadialGradient(double cx, double cy, double r0, double r1, const float(&inner)[4], const float(&outer)[4]) {
        nvgStrokePaint(vg, nvgRadialGradient(vg, float(cx), float(cy), float(r0), float(r1), toNVG(inner), toNVG(outer)));
    }

    // ======== Box Gradient (Stroke) ========

    void setStrokeBoxGradient(double x, double y, double w, double h, double r, double f, const Color& inner, const Color& outer) {
        nvgStrokePaint(vg, nvgBoxGradient(vg, float(x), float(y), float(w), float(h), float(r), float(f), toNVG(inner), toNVG(outer)));
    }
    void setStrokeBoxGradient(double x, double y, double w, double h, double r, double f, const float(&inner)[3], const float(&outer)[3], float a = 1.0f) {
        nvgStrokePaint(vg, nvgBoxGradient(vg, float(x), float(y), float(w), float(h), float(r), float(f), toNVG(inner, a), toNVG(outer, a)));
    }
    void setStrokeBoxGradient(double x, double y, double w, double h, double r, double f, const float(&inner)[4], const float(&outer)[4]) {
        nvgStrokePaint(vg, nvgBoxGradient(vg, float(x), float(y), float(w), float(h), float(r), float(f), toNVG(inner), toNVG(outer)));
    }

    // Linear (Fill)
    void setFillLinearGradient(DVec2 p0, DVec2 p1, const Color& inner, const Color& outer) { setFillLinearGradient(p0.x, p0.y, p1.x, p1.y, inner, outer); }
    void setFillLinearGradient(DVec2 p0, DVec2 p1, const float(&inner)[3], const float(&outer)[3], float a = 1.0f) { setFillLinearGradient(p0.x, p0.y, p1.x, p1.y, inner, outer, a); }
    void setFillLinearGradient(DVec2 p0, DVec2 p1, const float(&inner)[4], const float(&outer)[4]) { setFillLinearGradient(p0.x, p0.y, p1.x, p1.y, inner, outer); }

    // Linear (Stroke)
    void setStrokeLinearGradient(DVec2 p0, DVec2 p1, const Color& inner, const Color& outer) { setStrokeLinearGradient(p0.x, p0.y, p1.x, p1.y, inner, outer); }
    void setStrokeLinearGradient(DVec2 p0, DVec2 p1, const float(&inner)[3], const float(&outer)[3], float a = 1.0f) { setStrokeLinearGradient(p0.x, p0.y, p1.x, p1.y, inner, outer, a); }
    void setStrokeLinearGradient(DVec2 p0, DVec2 p1, const float(&inner)[4], const float(&outer)[4]) { setStrokeLinearGradient(p0.x, p0.y, p1.x, p1.y, inner, outer); }

    // Radial (Fill)
    void setFillRadialGradient(DVec2 c, double r0, double r1, const Color& inner, const Color& outer) { setFillRadialGradient(c.x, c.y, r0, r1, inner, outer); }
    void setFillRadialGradient(DVec2 c, double r0, double r1, const float(&inner)[3], const float(&outer)[3], float a = 1.0f) { setFillRadialGradient(c.x, c.y, r0, r1, inner, outer, a); }
    void setFillRadialGradient(DVec2 c, double r0, double r1, const float(&inner)[4], const float(&outer)[4]) { setFillRadialGradient(c.x, c.y, r0, r1, inner, outer); }

    // Radial (Stroke)
    void setStrokeRadialGradient(DVec2 c, double r0, double r1, const Color& inner, const Color& outer) { setStrokeRadialGradient(c.x, c.y, r0, r1, inner, outer); }
    void setStrokeRadialGradient(DVec2 c, double r0, double r1, const float(&inner)[3], const float(&outer)[3], float a = 1.0f) { setStrokeRadialGradient(c.x, c.y, r0, r1, inner, outer, a); }
    void setStrokeRadialGradient(DVec2 c, double r0, double r1, const float(&inner)[4], const float(&outer)[4]) { setStrokeRadialGradient(c.x, c.y, r0, r1, inner, outer); }

    // Box (Fill)
    void setFillBoxGradient(DVec2 pos, DVec2 size, double r, double f, const Color& inner, const Color& outer) { setFillBoxGradient(pos.x, pos.y, size.x, size.y, r, f, inner, outer); }
    void setFillBoxGradient(DVec2 pos, DVec2 size, double r, double f, const float(&inner)[3], const float(&outer)[3], float a = 1.0f) { setFillBoxGradient(pos.x, pos.y, size.x, size.y, r, f, inner, outer, a); }
    void setFillBoxGradient(DVec2 pos, DVec2 size, double r, double f, const float(&inner)[4], const float(&outer)[4]) { setFillBoxGradient(pos.x, pos.y, size.x, size.y, r, f, inner, outer); }

    // Box (Stroke)
    void setStrokeBoxGradient(DVec2 pos, DVec2 size, double r, double f, const Color& inner, const Color& outer) { setStrokeBoxGradient(pos.x, pos.y, size.x, size.y, r, f, inner, outer); }
    void setStrokeBoxGradient(DVec2 pos, DVec2 size, double r, double f, const float(&inner)[3], const float(&outer)[3], float a = 1.0f) { setStrokeBoxGradient(pos.x, pos.y, size.x, size.y, r, f, inner, outer, a); }
    void setStrokeBoxGradient(DVec2 pos, DVec2 size, double r, double f, const float(&inner)[4], const float(&outer)[4]) { setStrokeBoxGradient(pos.x, pos.y, size.x, size.y, r, f, inner, outer); }

    // ======== Paths ========                               
                                                             
    void beginPath()                                         { nvgBeginPath(vg); }
    void moveTo(double x, double y)                          { nvgMoveTo(vg, (float)(x), (float)(y)); }
    void lineTo(double x, double y)                          { nvgLineTo(vg, (float)(x), (float)(y)); }
    void moveTo(DVec2 p)                                     { nvgMoveTo(vg, (float)(p.x), (float)(p.y)); }
    void lineTo(DVec2 p)                                     { nvgLineTo(vg, (float)(p.x), (float)(p.y)); }
    void stroke()                                            { nvgStroke(vg); }
    void fill()                                              { nvgFill(vg); }
    void closePath()                                         { nvgClosePath(vg); }

    void bezierTo(double x1, double y1, double x2, double y2, double x, double y) {
        nvgBezierTo(vg, (float)(x1), (float)(y1), (float)(x2), (float)(y2), (float)(x), (float)(y));
    }
    void bezierTo(DVec2 p1, DVec2 p2, DVec2 p) {
        nvgBezierTo(vg, (float)p1.x, (float)p1.y, (float)p2.x, (float)p2.y, (float)p.x, (float)p.y);
    }
    void quadraticTo(double cx, double cy, double x, double y) {
        nvgQuadTo(vg, (float)(cx), (float)(cy), (float)(x), (float)(y));
    }
    void quadraticTo(DVec2 c, DVec2 p) {
        nvgQuadTo(vg, (float)c.x, (float)c.y, (float)p.x, (float)p.y);
    }

    void arc(double cx, double cy, double r, double a0, double a1, PathWinding winding = PathWinding::WINDING_CCW) {
        nvgArc(vg, (float)(cx), (float)(cy), (float)(r), (float)(a0), (float)(a1), (int)winding);
    }
    void arc(DVec2 cen, double r, double a0, double a1, PathWinding winding = PathWinding::WINDING_CCW) {
        nvgArc(vg, (float)(cen.x), (float)(cen.y), (float)(r), (float)(a0), (float)(a1), (int)winding);
    }
    void arcTo(double x0, double y0, double x1, double y1, double r) {
        nvgArcTo(vg, (float)(x0), (float)(y0), (float)(x1), (float)(y1), (float)(r));
    }
    void arcTo(DVec2 p0, DVec2 p1, double r) {
        nvgArcTo(vg, (float)(p0.x), (float)(p0.y), (float)(p1.x), (float)(p1.y), (float)(r));
    }

    template<typename PointT> void drawPath(const std::vector<PointT>& path) {
        if (path.size() >= 2) {
            moveTo(path[0]); 
            for (size_t i = 1; i < path.size(); i++) 
                lineTo(path[i]); 
        }
    }

    // ======== Shapes ========

    void circle(double cx, double cy, double r) { nvgCircle(vg, (float)(cx), (float)(cy), (float)(r)); }
    void circle(DVec2 p, double r) { nvgCircle(vg, (float)(p.x), (float)(p.y), (float)(r)); }
    void ellipse(double cx, double cy, double rx, double ry) { nvgEllipse(vg, (float)(cx), (float)(cy), (float)(rx), (float)(ry)); }
    void ellipse(DVec2 cen, DVec2 size) { nvgEllipse(vg, (float)(cen.x), (float)(cen.y), (float)(size.x), (float)(size.y)); }
    void fillRect(double x, double y, double w, double h) { nvgBeginPath(vg); nvgRect(vg, (float)(x), (float)(y), (float)(w), (float)(h)); nvgFill(vg); }
    void strokeRect(double x, double y, double w, double h) { nvgBeginPath(vg); nvgRect(vg, (float)(x), (float)(y), (float)(w), (float)(h)); nvgStroke(vg); }

    void strokeRoundedRect(double x, double y, double w, double h, double r)
    {
        nvgBeginPath(vg);
        nvgRoundedRect(vg, (float)(x), (float)(y), (float)(w), (float)(h), (float)(r));
        nvgStroke(vg);
    }
    void fillRoundedRect(double x, double y, double w, double h, double r)
    {
        nvgBeginPath(vg);
        nvgRoundedRect(vg, (float)(x), (float)(y), (float)(w), (float)(h), (float)(r));
        nvgFill(vg);
    }

    // ======== Image ========

    //void drawImage(Image& bmp, double x, double y, double w = 0, double h = 0) { 
    //    bmp.draw(vg, x, y, w <= 0 ? bmp.bmp_width : w, h <= 0 ? bmp.bmp_height : h); 
    //}

    //void drawImage(Image& bmp, DQuad quad) {
    //    bmp.drawSheared(vg, quad);
    //}

    // ======== Text ========

    void setTextAlign(TextAlign align)          { paint_ctx->text_align = align;       nvgTextAlign(vg, (int)(paint_ctx->text_align) | (int)(paint_ctx->text_baseline)); }
    void setTextBaseline(TextBaseline baseline) { paint_ctx->text_baseline = baseline; nvgTextAlign(vg, (int)(paint_ctx->text_align) | (int)(paint_ctx->text_baseline)); }
    virtual void setFontSize(double size_pts)   { nvgFontSize(vg, (float)(paint_ctx->global_scale * size_pts)); }
    void setFontSizePx(double size_px)          { nvgFontSize(vg, (float)size_px); }
    void setFont(NanoFont font)
    {
        if (font == paint_ctx->active_font)
            return;

        if (!font->created)
        {
            font->id = nvgCreateFont(vg, font->path.c_str(), font->path.c_str());
            font->created = true;

            // Todo: Check if font changed and update even if already created
            nvgFontSize(vg, (float)paint_ctx->global_scale * font->size);

        }

        nvgFontFaceId(vg, font->id);
        paint_ctx->active_font = font;
    }


    [[nodiscard]] DRect boundingBox(std::string_view txt) const
    {
        float bounds[4];
        nvgTextBounds(vg, 0, 0, txt.data(), txt.data()+txt.size(), bounds);
        return DRect((double)(bounds[0]), (double)(bounds[1]), (double)(bounds[2] - bounds[0]), (double)(bounds[3] - bounds[1]));
    }

    void fillText(std::string_view txt, DVec2 pos) { fillText(txt, pos.x, pos.y); }
    void fillText(std::string_view txt, double x, double y)
    {
        if (!paint_ctx->active_font) setFont(paint_ctx->default_font);
        nvgText(vg, (float)(x), (float)(y), txt.data(), txt.data() + txt.size());
    }
};

class SurfaceInfo
{
    // ----- "local" rect info (not necessarily client-space) -----
    double x = 0, y = 0;
    double w = 0, h = 0;

    double start_w = 0;
    double start_h = 0;
    double old_w = 0;
    double old_h = 0;

    // If surface resized, keep track of the scale factor from the "start" size
    double scale_adjust = 1;

    // ----- "client" rect info (for SDL event coordinate conversions) -----
    double client_x = 0, client_y = 0;
    double client_w = 0, client_h = 0;

public:

    void setInitialSize()
    {
        start_w = w;
        start_h = h;
        scale_adjust = 1;
    }

    void setOldSize()
    {
        old_w = w;
        old_h = h;
    }

    void setSurfacePos(double _x, double _y)
    {
        x = _x;
        y = _y;
    }

    void setSurfaceSize(double _w, double _h)
    {
        w = _w;
        h = _h;

        DVec2 start_size = { start_w, start_h };
        DVec2 new_size = { w, h };

        scale_adjust = std::min(new_size.x / start_size.x, new_size.y / start_size.y);
    }

    void setClientRect(double _x, double _y, double _w, double _h)
    {
        client_x = _x;
        client_y = _y;
        client_w = _w;
        client_h = _h;
    }

    [[nodiscard]] constexpr double left() const { return x; }
    [[nodiscard]] constexpr double top() const { return y; }
    [[nodiscard]] constexpr double right() const { return x + w; }
    [[nodiscard]] constexpr double bottom() const { return y + h; }

    [[nodiscard]] constexpr double width() const { return w; }
    [[nodiscard]] constexpr double height() const { return h; }
    [[nodiscard]] constexpr DVec2  size() const { return DVec2(w, h); }
    [[nodiscard]] constexpr DRect  surfaceRect() const { return DRect(x, y, x + w, y + h); }
    [[nodiscard]] constexpr DRect  clientRect() const { return DRect(client_x, client_y, client_x + client_w, client_y + client_h); }

    [[nodiscard]] constexpr double initialSizeScale() { return scale_adjust; }
    [[nodiscard]] constexpr bool   resized() { return (w != old_w) || (h != old_h); }

    [[nodiscard]] double toSurfaceX(double client_posX) { return ((client_posX - client_x) / client_w) * w; }
    [[nodiscard]] double toSurfaceY(double client_posY) { return ((client_posY - client_y) / client_h) * h; }
};

// ==================================
// ======== Advanced Painter ========
// ==================================
//
// > Pretransforms world coordinates (double precision) before rendering in screen-space with nanovg 
// > Supports toggling between screen-space / world-space
//

class Painter : private SimplePainter
{
    friend class CameraInfo;
    friend class Viewport;

    DVec2 align_full(DVec2 p)              { return DVec2{ std::floor(p.x), std::floor(p.y) }; }
    DVec2 align_full(double px, double py) { return DVec2{ std::floor(px),  std::floor(py) }; }
    DVec2 align_half(DVec2 p)              { return DVec2{ std::floor(p.x) + 0.5, std::floor(p.y) + 0.5 }; }
    DVec2 align_half(double px, double py) { return DVec2{ std::floor(px)  + 0.5, std::floor(py)  + 0.5 }; }

    glm::mat3 default_viewport_transform;
    double line_width = 1;

    bool transform_coordinates = true;
    bool scale_lines = true;
    bool scale_sizes = true;
    bool scale_text = true;
    bool rotate_text = true;

    bool saved_transform_coordinates = transform_coordinates;
    bool saved_scale_lines = scale_lines;
    bool saved_scale_sizes = scale_sizes;
    bool saved_scale_text = scale_text;
    bool saved_rotate_text = rotate_text;

    SurfaceInfo* surface = nullptr;
    WorldStageTransform m;

public:

    Painter(SurfaceInfo* s) : surface(s)
    {}

    double _avgAdjustedZoom() const {
        return m.avgZoomScaleFactor();
    }

    void worldCoordinates(bool b) { transform_coordinates = b; }
    void scalingLines(bool b)     { scale_lines = b; }
    void scalingSizes(bool b)     { scale_sizes = b; }
    void scalingText(bool b)      { scale_text = b; }
    void rotatingText(bool b)     { rotate_text = b; }

    void worldMode()
    {
        transform_coordinates = true;
        scale_lines = true;
        scale_sizes = true;
        scale_text = true;
        rotate_text = true;

        setLineWidth(line_width);
    }
    void stageMode()
    {
        transform_coordinates = false;
        scale_lines = false;
        scale_sizes = false;
        scale_text = false;
        rotate_text = false;

        // Don't preserve NanoVG transforms, start fresh
        SimplePainter::resetTransform();
        SimplePainter::transform(default_viewport_transform);
        setLineWidth(line_width);
    }
    void worldHudMode()
    {
        transform_coordinates = true;
        scale_lines = false;
        scale_sizes = false;
        scale_text = false;
        rotate_text = false;

        setLineWidth(line_width);
    }

    // todo: integrate with save()/load() stack?
    void saveCameraTransform()
    {
        saved_transform_coordinates = transform_coordinates;
        saved_scale_lines = scale_lines;
        saved_scale_sizes = scale_sizes;
        saved_scale_text = scale_text;
        saved_rotate_text = rotate_text;
    }
    void restoreCameraTransform()
    {
        transform_coordinates = saved_transform_coordinates;
        scale_lines = saved_scale_lines;
        scale_sizes = saved_scale_sizes;
        scale_text = saved_scale_text;
        rotate_text = saved_rotate_text;

        setLineWidth(line_width);
    }

    void resetTransform()
    {
        m.reset();
        SimplePainter::resetTransform();
    }

    template<typename T = f128> GlmMat3<T> currentTransform() const { return m.stageTransform<T>(); }
    template<typename T = f128> GlmMat3<T> inverseTransform() const { return m.worldTransform<T>(); }

    void transform(const glm::ddmat3& _m) {
        if (transform_coordinates)
            m.transform(_m);
        else
            SimplePainter::transform(_m);
    }

    void setTransform(const glm::ddmat3& _m) {
        if (transform_coordinates) {
            m.reset();
            m.transform(_m);
        }
        else {
            SimplePainter::resetTransform();
            SimplePainter::transform(static_cast<glm::mat3>(_m));
        }
    }

    template<typename T> void translate(T x, T y) { if (transform_coordinates) m.translate(x, y); else SimplePainter::translate(x, y); }
    template<typename T> void scale(T s)          { if (transform_coordinates) m.scale(s);        else SimplePainter::scale(s); }
    void rotate(double r)                         { if (transform_coordinates) m.rotate(r);       else SimplePainter::rotate(r); }

    [[nodiscard]] DVec2 Offset(double stage_offX, double stage_offY) const
    {
        return m.toWorldOffset<double>(stage_offX, stage_offY);
    }

    double lineScale() {
        return scale_lines ?
            1.0                         // Scaling already handled by world projection matrix
            : paint_ctx->global_scale;  // Stage-transform, fall back to DPR scaling
    }
    double sizeScale() {
        return scale_sizes ?
            1.0                         // Scaling already handled by world projection matrix
            : paint_ctx->global_scale;  // Stage-transform, fall back to DPR scaling
    }


    struct ScopedTransform
    {
        Painter* painter;
        ScopedTransform(Painter* p) : painter(p) {
            p->SimplePainter::save();
            p->SimplePainter::resetTransform();
            p->SimplePainter::transform(painter->default_viewport_transform);
        }
        ~ScopedTransform() { painter->SimplePainter::restore(); }
    };

    // ======== Position/Size wrappers (applies only enabled camera transforms) ========

    // IF (transform_coordinates)    Input = World (double/f128),   Output = Stage (double)
    // IF (!transform_coordinates)   Input = Stage (double/f128),   Output = Stage (double)

    template<typename T> DVec2  PT(T x, T y)    const { return transform_coordinates ? m.toStage<T>(x, y) : Vec2{ (double)x, (double)y }; }
    template<typename T> DVec2  PT(Vec2<T> p)   const { return transform_coordinates ? m.toStage<T>(p) : static_cast<DVec2>(p); }
    //template<typename T> DVec2  PT(T x, T y)    const { return m.toStage<T>(x, y); }
    //template<typename T> DVec2  PT(Vec2<T> p)   const { return m.toStage<T>(p); }

    template<typename T> DVec2  SIZE(T w, T h)  const { return scale_sizes ? m.toStageSideLengths<T>({w, h}) : DVec2{(double)w, (double)h}; }
    template<typename T> DVec2  SIZE(Vec2<T> s) const { return scale_sizes ? m.toStageSideLengths<T>(s) : s; }
    template<typename T> double SIZE(T radius)  const { return scale_sizes ? (_avgAdjustedZoom() * radius) : radius; }

    template<typename T> DQuad  QUAD(const Quad<T>& q) const { return { PT(q.a), PT(q.b), PT(q.c), PT(q.d) }; }

    template<typename T> void ROTATE(T r)           { if (r != 0.0) SimplePainter::rotate(r); }
    template<typename T> void TRANSLATE(T x, T y)   { SimplePainter::translate(PT(x, y)); }
    template<typename T> DVec2 TRANSFORMED(T x, T y, T w, T h, double rotation) {
        SimplePainter::translate(PT(x, y));
        if (rotation != 0.0)
            SimplePainter::rotate(rotation);
        return SIZE(w, h);
    }


    // ======== Styles ========

    void setLineWidth(double w)
    {
        this->line_width = w;
        if (scale_lines)
            SimplePainter::setLineWidth(w * _avgAdjustedZoom());
        else
            SimplePainter::setLineWidth(w * lineScale());
    }

    // ======== Paths (overrides) ========

    template<typename T> void moveTo(T px, T py)    { SimplePainter::moveTo(PT(px, py)); }
    template<typename T> void lineTo(T px, T py)    { SimplePainter::lineTo(PT(px, py)); }
    template<typename T> void moveTo(Vec2<T> p)     { SimplePainter::moveTo(PT(p)); }
    template<typename T> void lineTo(Vec2<T> p)     { SimplePainter::lineTo(PT(p)); }

    template<typename T> void circle(T cx, T cy, T r)            { SimplePainter::circle(PT(cx, cy), SIZE(r)); }
    template<typename T> void circle(Vec2<T> cen, T r)           { SimplePainter::circle(PT(cen),    SIZE(r)); }
    template<typename T> void ellipse(T x, T y, T rx, T ry)      { SimplePainter::ellipse(PT(x, y),  SIZE(rx, ry)); }
    template<typename T> void ellipse(Vec2<T> cen, Vec2<T> size) { SimplePainter::ellipse(PT(cen),   SIZE(size)); }

    template<typename T> void arc(T cx, T cy, T r, T a0, T a1, PathWinding winding = PathWinding::WINDING_CCW) {
        SimplePainter::arc(PT(cx, cy), SIZE(r), a0, a1, winding);
    }
    template<typename T> void arc(DVec2 cen,  T r, T a0, T a1, PathWinding winding = PathWinding::WINDING_CCW) {
        SimplePainter::arc(PT(cen), SIZE(r), a0, a1, winding);
    }
    template<typename T> void arcTo(T x0, T y0, T x1, T y1, T r) {
        SimplePainter::arcTo(PT(x0, y0), PT(x1, y1), SIZE(r));
    }
    template<typename T> void arcTo(Vec2<T> p0, Vec2<T> p1, T r) {
        SimplePainter::arcTo(PT(p0), PT(p1), SIZE(r));
    }

    template<typename T> void bezierTo(T x1, T y1, T x2, T y2, T x3, T y3) {
        SimplePainter::bezierTo(PT(x1, y1), PT(x2, y2), PT(x3, y3));
    }
    template<typename T> void quadraticTo(T cx, T cy, T x, T y) {
        SimplePainter::quadraticTo(PT(cx, cy), PT(x, y));
    }

    template<typename PointT> void drawPath(const std::vector<PointT>& path)
    {
        if (path.size() < 2) return;
        size_t len = path.size();
        moveTo(path[0]);
        for (size_t i = 1; i < len; i++)
            lineTo(path[i]);
    }
    template<typename PointT, size_t N> void drawPath(const PointT(&path)[N])
    {
        if constexpr (N < 2) return;
        moveTo(path[0]);
        for (size_t i = 1; i < N; ++i)
            lineTo(path[i]);
    }
    template<typename PointT, size_t N> void drawClosedPath(const PointT(&path)[N])
    {
        if constexpr (N < 2) return;
        moveTo(path[0]);
        for (size_t i = 1; i < N; ++i)
            lineTo(path[i]);
        lineTo(path[0]);
    }

    template<typename PointT> void strokePath(const std::vector<PointT>& path)
    {
        if (path.size() < 2) return;
        beginPath();
        size_t len = path.size();
        moveTo(path[0]);
        for (size_t i = 1; i < len; i++)
            lineTo(path[i]);
        stroke();
    }
    template<typename PointT> void strokePath(const std::vector<PointT>& path, size_t i0, size_t i1)
    {
        size_t len = i1 - i0;
        if (len < 2) return;
        beginPath();
        moveTo(path[i0]);
        for (size_t i = i0+1; i < i1; i++)
            lineTo(path[i]);
        stroke();
    }

    // Linear (Fill)
    void setFillLinearGradient(double x0, double y0, double x1, double y1, const float(&inner)[3], const float(&outer)[3], float a = 1.0f) { SimplePainter::setFillLinearGradient(PT(x0, y0), PT(x1, y1), inner, outer, a); }
    void setFillLinearGradient(double x0, double y0, double x1, double y1, const float(&inner)[4], const float(&outer)[4])                 { SimplePainter::setFillLinearGradient(PT(x0, y0), PT(x1, y1), inner, outer); }
    void setFillLinearGradient(DVec2 p0, DVec2 p1, const Color& inner, const Color& outer)                                                 { SimplePainter::setFillLinearGradient(PT(p0), PT(p1), inner, outer); }
    void setFillLinearGradient(DVec2 p0, DVec2 p1, const float(&inner)[3], const float(&outer)[3], float a = 1.0f)                         { SimplePainter::setFillLinearGradient(PT(p0), PT(p1), inner, outer, a); }
    void setFillLinearGradient(DVec2 p0, DVec2 p1, const float(&inner)[4], const float(&outer)[4])                                         { SimplePainter::setFillLinearGradient(PT(p0), PT(p1), inner, outer); }

    // Linear (Stroke)
    void setStrokeLinearGradient(double x0, double y0, double x1, double y1, const Color& inner, const Color& outer)                       { SimplePainter::setStrokeLinearGradient(PT(x0, y0), PT(x1, y1), inner, outer); }
    void setStrokeLinearGradient(double x0, double y0, double x1, double y1, const float(&inner)[3], const float(&outer)[3], float a=1.0f) { SimplePainter::setStrokeLinearGradient(PT(x0, y0), PT(x1, y1), inner, outer, a); }
    void setStrokeLinearGradient(double x0, double y0, double x1, double y1, const float(&inner)[4], const float(&outer)[4])               { SimplePainter::setStrokeLinearGradient(PT(x0, y0), PT(x1, y1), inner, outer); }
    void setStrokeLinearGradient(DVec2 p0, DVec2 p1, const Color& inner, const Color& outer)                                               { SimplePainter::setStrokeLinearGradient(PT(p0), PT(p1), inner, outer); }
    void setStrokeLinearGradient(DVec2 p0, DVec2 p1, const float(&inner)[3], const float(&outer)[3], float a = 1.0f)                       { SimplePainter::setStrokeLinearGradient(PT(p0), PT(p1), inner, outer, a); }
    void setStrokeLinearGradient(DVec2 p0, DVec2 p1, const float(&inner)[4], const float(&outer)[4])                                       { SimplePainter::setStrokeLinearGradient(PT(p0), PT(p1), inner, outer); }

    // Radial (Fill)
    void setFillRadialGradient(double cx, double cy, double r0, double r1, const float(&inner)[3], const float(&outer)[3], float a=1.0f)   { SimplePainter::setFillRadialGradient(PT(cx, cy), SIZE(r0), SIZE(r1), inner, outer, a); }
    void setFillRadialGradient(double cx, double cy, double r0, double r1, const float(&inner)[4], const float(&outer)[4])                 { SimplePainter::setFillRadialGradient(PT(cx, cy), SIZE(r0), SIZE(r1), inner, outer); }
    void setFillRadialGradient(DVec2 c, double r0, double r1, const Color& inner, const Color& outer)                                      { SimplePainter::setFillRadialGradient(PT(c), SIZE(r0), SIZE(r1), inner, outer); }
    void setFillRadialGradient(DVec2 c, double r0, double r1, const float(&inner)[3], const float(&outer)[3], float a = 1.0f)              { SimplePainter::setFillRadialGradient(PT(c), SIZE(r0), SIZE(r1), inner, outer, a); }
    void setFillRadialGradient(DVec2 c, double r0, double r1, const float(&inner)[4], const float(&outer)[4])                              { SimplePainter::setFillRadialGradient(PT(c), SIZE(r0), SIZE(r1), inner, outer); }

    // Radial (Stroke)
    void setStrokeRadialGradient(double cx, double cy, double r0, double r1, const Color& inner, const Color& outer)                       { SimplePainter::setStrokeRadialGradient(PT(cx, cy), SIZE(r0), SIZE(r1), inner, outer); }
    void setStrokeRadialGradient(double cx, double cy, double r0, double r1, const float(&inner)[3], const float(&outer)[3], float a=1.0f) { SimplePainter::setStrokeRadialGradient(PT(cx, cy), SIZE(r0), SIZE(r1), inner, outer, a); }
    void setStrokeRadialGradient(double cx, double cy, double r0, double r1, const float(&inner)[4], const float(&outer)[4])               { SimplePainter::setStrokeRadialGradient(PT(cx, cy), SIZE(r0), SIZE(r1), inner, outer); }
    void setStrokeRadialGradient(DVec2 c, double r0, double r1, const Color& inner, const Color& outer)                                    { SimplePainter::setStrokeRadialGradient(PT(c), SIZE(r0), SIZE(r1), inner, outer); }
    void setStrokeRadialGradient(DVec2 c, double r0, double r1, const float(&inner)[3], const float(&outer)[3], float a = 1.0f)            { SimplePainter::setStrokeRadialGradient(PT(c), SIZE(r0), SIZE(r1), inner, outer, a); }
    void setStrokeRadialGradient(DVec2 c, double r0, double r1, const float(&inner)[4], const float(&outer)[4])                            { SimplePainter::setStrokeRadialGradient(PT(c), SIZE(r0), SIZE(r1), inner, outer); }

    // Box (Fill)
    void setFillBoxGradient(double x, double y, double w, double h, double r, double f, const Color& inner, const Color& outer)            { SimplePainter::setFillBoxGradient(PT(x, y), SIZE(w, h), SIZE(r), SIZE(f), inner, outer); }
    void setFillBoxGradient(double x, double y, double w, double h, double r, double f, const float(&inner)[3], const float(&outer)[3], float a=1.0f) { SimplePainter::setFillBoxGradient(PT(x, y), SIZE(w, h), SIZE(r), SIZE(f), inner, outer, a); }
    void setFillBoxGradient(double x, double y, double w, double h, double r, double f, const float(&inner)[4], const float(&outer)[4])    { SimplePainter::setFillBoxGradient(PT(x, y), SIZE(w, h), SIZE(r), SIZE(f), inner, outer); }
    void setFillBoxGradient(DVec2 pos, DVec2 size, double r, double f, const Color& inner, const Color& outer)                             { SimplePainter::setFillBoxGradient(PT(pos), SIZE(size), SIZE(r), SIZE(f), inner, outer); }
    void setFillBoxGradient(DVec2 pos, DVec2 size, double r, double f, const float(&inner)[3], const float(&outer)[3], float a=1.0f)       { SimplePainter::setFillBoxGradient(PT(pos), SIZE(size), SIZE(r), SIZE(f), inner, outer, a); }
    void setFillBoxGradient(DVec2 pos, DVec2 size, double r, double f, const float(&inner)[4], const float(&outer)[4])                     { SimplePainter::setFillBoxGradient(PT(pos), SIZE(size), SIZE(r), SIZE(f), inner, outer); }

    // Box (Stroke)
    void setStrokeBoxGradient(double x, double y, double w, double h, double r, double f, const Color& inner, const Color& outer)          { SimplePainter::setStrokeBoxGradient(PT(x, y), SIZE(w, h), SIZE(r), SIZE(f), inner, outer); }
    void setStrokeBoxGradient(double x, double y, double w, double h, double r, double f, const float(&inner)[3], const float(&outer)[3], float a=1.0f) { SimplePainter::setStrokeBoxGradient(PT(x, y), SIZE(w, h), SIZE(r), SIZE(f), inner, outer, a); }
    void setStrokeBoxGradient(double x, double y, double w, double h, double r, double f, const float(&inner)[4], const float(&outer)[4])  { SimplePainter::setStrokeBoxGradient(PT(x, y), SIZE(w, h), SIZE(r), SIZE(f), inner, outer); }
    void setStrokeBoxGradient(DVec2 pos, DVec2 size, double r, double f, const Color& inner, const Color& outer)                           { SimplePainter::setStrokeBoxGradient(PT(pos), SIZE(size), SIZE(r), SIZE(f), inner, outer); }
    void setStrokeBoxGradient(DVec2 pos, DVec2 size, double r, double f, const float(&inner)[3], const float(&outer)[3], float a=1.0f)     { SimplePainter::setStrokeBoxGradient(PT(pos), SIZE(size), SIZE(r), SIZE(f), inner, outer, a); }
    void setStrokeBoxGradient(DVec2 pos, DVec2 size, double r, double f, const float(&inner)[4], const float(&outer)[4])                   { SimplePainter::setStrokeBoxGradient(PT(pos), SIZE(size), SIZE(r), SIZE(f), inner, outer); }


    // ======== Shapes ========

    template<typename T> void strokeRect(T x, T y, T w, T h)
    {
        ScopedTransform t(this);
        DVec2 s = TRANSFORMED(x, y, w, h, m._angle());
        SimplePainter::strokeRect(0, 0, s.x, s.y);
    }
    template<typename T> void fillRect(T x, T y, T w, T h)
    {
        ScopedTransform t(this);
        DVec2 s = TRANSFORMED(x, y, w, h, m._angle());
        SimplePainter::fillRect(0, 0, s.x, s.y);
    }
    template<typename T> void strokeRoundedRect(T x, T y, T w, T h, T r) {
        ScopedTransform t(this);
        DVec2 s = TRANSFORMED(x, y, w, h, m._angle());
        SimplePainter::strokeRoundedRect(0, 0, s.x, s.y, SIZE(r));
    }
    template<typename T> void fillRoundedRect(T x, T y, T w, T h, T r)
    {
        ScopedTransform t(this);
        DVec2 s = TRANSFORMED(x, y, w, h, m._angle());
        SimplePainter::fillRoundedRect(0, 0, s.x, s.y, SIZE(r));
    }
    template<typename T> void strokeEllipse(T cx, T cy, T rx, T ry)
    {
        ScopedTransform t(this);
        DVec2 s = TRANSFORMED(cx, cy, rx, ry, m._angle());
        SimplePainter::beginPath();
        SimplePainter::ellipse(0, 0, s.x, s.y);
        SimplePainter::stroke();
    }
    template<typename T> void fillEllipse(T cx, T cy, T rx, T ry)
    {
        ScopedTransform t(this);
        DVec2 s = TRANSFORMED(cx, cy, rx, ry, m._angle());
        SimplePainter::beginPath();
        SimplePainter::ellipse(0, 0, s.x, s.y);
        SimplePainter::fill();
    }

    // Overloads
    template<typename T> void strokeQuad(const Quad<T>& q)                             { beginPath(); drawClosedPath<Vec2<T>>({q.a, q.b, q.c, q.d}); stroke(); }
    template<typename T> void strokeQuad(const Quad<T>& q, Color col)                  { setStrokeStyle(col); beginPath(); drawClosedPath<Vec2<T>>({ q.a, q.b, q.c, q.d }); stroke(); }
    template<typename T> void fillQuad(const Quad<T>& q)                               { beginPath(); drawClosedPath<Vec2<T>>({ q.a, q.b, q.c, q.d }); fill(); }
    template<typename T> void fillQuad(const Quad<T>& q, Color col)                    { setFillStyle(col); beginPath(); drawClosedPath<Vec2<T>>({ q.a, q.b, q.c, q.d }); fill(); }
    template<typename T> void strokeRect(const Rect<T>& r)                             { strokeRect<T>(r.x1, r.y1, r.x2 - r.x1, r.y2 - r.y1); }
    template<typename T> void strokeRect(const Rect<T>& r, Color col)                  { setStrokeStyle(col); strokeRect<T>(r.x1, r.y1, r.x2 - r.x1, r.y2 - r.y1); }
    template<typename T> void fillRect(const Rect<T>& r)                               { fillRect<T>(r.x1, r.y1, r.x2 - r.x1, r.y2 - r.y1); }
    template<typename T> void fillRect(const Rect<T>& r, Color col)                    { setFillStyle(col); fillRect<T>(r.x1, r.y1, r.x2 - r.x1, r.y2 - r.y1); }
    template<typename T> void strokeRoundedRect(const Rect<T>& r, T radius)            { strokeRoundedRect<T>(r.x1, r.y1, r.x2 - r.x1, r.y2 - r.y1, radius); }
    template<typename T> void strokeRoundedRect(const Rect<T>& r, T radius, Color col) { setStrokeStyle(col); strokeRoundedRect<T>(r.x1, r.y1, r.x2 - r.x1, r.y2 - r.y1, radius); }
    template<typename T> void fillRoundedRect(const Rect<T>& r, T radius)              { fillRoundedRect<T>(r.x1, r.y1, r.x2 - r.x1, r.y2 - r.y1, radius); }
    template<typename T> void fillRoundedRect(const Rect<T>& r, T radius, Color col)   { setFillStyle(col); fillRoundedRect<T>(r.x1, r.y1, r.x2 - r.x1, r.y2 - r.y1, radius); }
    template<typename T> void strokeEllipse(T cx, T cy, T r)                           { strokeEllipse<T>(cx, cy, r, r); }
    template<typename T> void strokeEllipse(T cx, T cy, T r, Color col)                { setStrokeStyle(col); strokeEllipse<T>(cx, cy, r, r); }
    template<typename T> void fillEllipse(T cx, T cy, T r)                             { fillEllipse<T>(cx, cy, r, r); }
    template<typename T> void fillEllipse(T cx, T cy, T r, Color col)                  { setFillStyle(col); fillEllipse<T>(cx, cy, r, r); }
    template<typename T> void strokeEllipse(Vec2<T> cen, T r)                          { strokeEllipse<T>(cen.x, cen.y, r, r); }
    template<typename T> void strokeEllipse(Vec2<T> cen, T r, Color col)               { setStrokeStyle(col); strokeEllipse<T>(cen.x, cen.y, r, r); }
    template<typename T> void fillEllipse(Vec2<T> cen, T r)                            { fillEllipse<T>(cen.x, cen.y, r, r); }
    template<typename T> void fillEllipse(Vec2<T> cen, T r, Color col)                 { setFillStyle(col); fillEllipse<T>(cen.x, cen.y, r, r); }

    template<typename T> void drawArrow(Vec2<T> a, Vec2<T> b, Color color=Color(255,255,255), double tip_angle=35, double tip_scale=1.0, bool fill_tip=true)
    {
        double dx = b.x - a.x;
        double dy = b.y - a.y;
        double angle = std::atan2(dy, dx);
        const double tip_sharp_angle = Math::toRadians(180.0 - tip_angle);// 145.0 * Math::PI / 180.0;
        double arrow_size;// = (line_width * 4) / (camera->scale_lines ? _avgZoom() : 1);

        if (transform_coordinates)
        {
            arrow_size = (line_width * 4 * tip_scale) / _avgAdjustedZoom();
        }
        else
        {
            arrow_size = (line_width * 4 * tip_scale) / (scale_lines ? _avgAdjustedZoom() : 1);
        }

        DVec2 c = b - (b - a).normalized() * arrow_size * 0.7;

        double rx1 = b.x + std::cos(angle + tip_sharp_angle) * arrow_size;
        double ry1 = b.y + std::sin(angle + tip_sharp_angle) * arrow_size;
        double rx2 = b.x + std::cos(angle - tip_sharp_angle) * arrow_size;
        double ry2 = b.y + std::sin(angle - tip_sharp_angle) * arrow_size;

        if (fill_tip)
        {
            setLineCap(LineCap::CAP_ROUND);
            setFillStyle(color);
            setStrokeStyle(color);
            beginPath();
            moveTo(a);
            lineTo(c);
            stroke();

            beginPath();
            moveTo(b);
            lineTo(rx1, ry1);
            lineTo(rx2, ry2);
            fill();
        }
        else // Stroke tip
        {
            setLineCap(LineCap::CAP_ROUND);
            setFillStyle(color);
            setStrokeStyle(color);
            beginPath();
            moveTo(a);
            lineTo(b);
            stroke();

            strokeLine(b, {rx1, ry1});
            strokeLine(b, {rx2, ry2});
        }
    }

    // ======== Image ========

    //void drawImage(Image& bmp, double x, double y, double w = 0, double h = 0) {
    //    SimplePainter::drawImage(bmp, x, y, w <= 0 ? bmp.bmp_width : w, h <= 0 ? bmp.bmp_height : h);
    //}

    template<typename T> void drawImage(const Image& bmp, Quad<T> q)
    {
        DQuad quad = { PT(q.a), PT(q.b), PT(q.c), PT(q.d) };

        DVec2 a = quad.a;
        DVec2 u = quad.b - quad.a;
        DVec2 v = quad.d - quad.a;

        bmp.refreshData(vg);

        nvgSave(vg);
        nvgTransform(vg, (float)u.x, (float)u.y, (float)v.x, (float)v.y, (float)a.x, (float)a.y);
        NVGpaint paint = nvgImagePattern(vg, 0, 0, 1, 1, 0.0f, bmp.imageId(), 1.0f);
        nvgBeginPath(vg);
        nvgRect(vg, 0, 0, 1, 1);
        nvgFillPaint(vg, paint);
        nvgFill(vg);

        nvgRestore(vg);
    }
    template<typename T> void drawImage(const CanvasImageBase<T>& bmp)
    {
        drawImage(bmp, bmp.worldQuad());
    }


    // ======== Text ========

    void setFontSize(double size_pts) { nvgFontSize(vg, (float)(sizeScale() * size_pts)); }

    template<typename T> void fillText(std::string_view txt, T px, T py)
    {
        //DVec2 p = PT(px, py);

        //if (camera->scale_text)
        //    SimplePainter::setFontSize(SIZE(font_size));
        //else
        //    SimplePainter::setFontSize(font_size);

        ScopedTransform t(this);
        TRANSLATE(px, py);

        if (rotate_text)
            ROTATE(m._angle());

        if (scale_text)
        {
            DVec2 z = m._zoom();
            SimplePainter::scale(z.x, z.y);
        }

        SimplePainter::fillText(txt, 0, 0);
    }
    template<typename T> void fillText(std::string_view txt, const Vec2<T>& p) {
        fillText(txt, p.x, p.y);
    }

    template<typename T=double> [[nodiscard]] Rect<T> boundingBox(std::string_view txt)
    {
        SimplePainter::save();
        SimplePainter::resetTransform();
        SimplePainter::transform(default_viewport_transform);
        auto r = SimplePainter::boundingBox(txt);
        SimplePainter::restore();
        return r;
    }

private:
    [[nodiscard]] std::string format_number(double v, int decimals, double fixed_min = 0.001, double fixed_max = 100000) {
        char buffer[32];
        double abs_v = std::abs(v);

        char fmt[8];

        if ((abs_v != 0.0 && abs_v < fixed_min) || abs_v >= fixed_max) {
            // Use scientific notation
            std::snprintf(fmt, sizeof(fmt), "%%.%de", decimals);
            std::snprintf(buffer, sizeof(buffer), fmt, v);
        }
        else {
            // Use fixed-point
            std::snprintf(fmt, sizeof(fmt), "%%.%df", decimals);
            std::snprintf(buffer, sizeof(buffer), fmt, v);
        }

        std::string s(buffer);

        // Trim trailing zeros (both fixed and scientific cases)
        size_t dot_pos = s.find('.');
        if (dot_pos != std::string::npos) {
            size_t end = s.find_first_of("eE", dot_pos); // handle scientific part separately
            size_t trim_end = (end == std::string::npos) ? s.size() : end;

            // Trim zeros in the fractional part
            size_t last_nonzero = s.find_last_not_of('0', trim_end - 1);
            if (last_nonzero != std::string::npos && s[last_nonzero] == '.') {
                last_nonzero--; // also remove the decimal point
            }

            s.erase(last_nonzero + 1, trim_end - last_nonzero - 1);
        }

        return s;
    }
    [[nodiscard]] std::string format_number(f128 v, int decimals, double fixed_min = 0.001, double fixed_max = 100000) {
        f128 abs_v = abs(v);

        std::string s;
        if ((abs_v != 0 && abs_v < fixed_min) || abs_v >= fixed_max)
        {
            // Use scientific notation (strip zeros)
            s = to_string(abs_v, decimals, false, true, true);
        }
        else {
            // Use fixed-point (keep trailing zeros for consistency)
            s = to_string(abs_v, decimals, true, false, false);
        }

        return s;
    }

    const double exponent_font_scale = 0.85;
    const double exponent_spacing_x = 0.06;
    const double exponent_spacing_y = -0.3;

public:
    template<typename PosT=double, typename ValT> void fillNumberScientific(ValT v, Vec2<PosT> pos, int decimals, double fontSize = 12)
    {
        std::string txt = format_number(v, decimals);

        size_t ePos = txt.find("e");
        if (ePos != std::string::npos)
        {
            const int exponent = std::stoi(txt.substr(ePos + 1));
            std::string mantissa_txt = txt.substr(0, ePos) + "e";
            std::string exponent_txt = std::to_string(exponent);

            double mantissaWidth = boundingBox<PosT>(mantissa_txt).x2 + exponent_spacing_x;

            /// todo: Take whatever alignment you're given and adjust right bound
            setTextAlign(TextAlign::ALIGN_CENTER);
            setFontSize(fontSize);

            pos = pos.floored();
            fillTextSharp(mantissa_txt.c_str(), pos);

            pos.x += PosT{ mantissaWidth } / PosT{ 2 } + PosT{ fontSize * exponent_spacing_x };
            pos.y -= PosT{ fontSize * (exponent_font_scale + exponent_spacing_y) };

            setTextAlign(TextAlign::ALIGN_LEFT);
            setFontSize(fontSize * exponent_font_scale);

            fillTextSharp(exponent_txt.c_str(), pos);

            setFontSize(fontSize);
            setTextAlign(TextAlign::ALIGN_CENTER);
        }
        else
        {
            setFontSize(fontSize);
            fillTextSharp(txt.c_str(), pos);
        }
    }
    template<typename PosT=double, typename ValT> [[nodiscard]] Rect<PosT> boundingBoxScientific(ValT v, int decimals, double fontSize = 12)
    {
        std::string txt = format_number(v, decimals);

        size_t ePos = txt.find("e");
        if (ePos != std::string::npos)
        {
            const int  exponent = std::stoi(txt.substr(ePos + 1));
            std::string mantissa_txt = txt.substr(0, ePos) + "e";
            std::string exponent_txt = std::to_string(exponent);

            Rect<PosT> mantissaRect = boundingBox<PosT>(mantissa_txt);
            Rect<PosT> exponentRect = boundingBox<PosT>(exponent_txt);
            Rect<PosT> ret = mantissaRect;

            ret.x2 += exponentRect.width() + PosT{ fontSize * exponent_spacing_x };
            ret.y1 -= PosT{ fontSize * (exponent_font_scale + exponent_spacing_y) };

            return ret;
        }
        else
        {
            return boundingBox(txt);
        }
    }

    // ======== Sharp variants ========

    template<typename T> void moveToSharp(T px, T py)  { SimplePainter::moveTo(align_half(PT(px, py))); }
    template<typename T> void moveToSharp(Vec2<T> p)   { SimplePainter::moveTo(align_half(PT(p))); }
    template<typename T> void lineToSharp(T px, T py)  { SimplePainter::lineTo(align_half(PT(px, py))); }
    template<typename T> void lineToSharp(Vec2<T> p)   { SimplePainter::lineTo(align_half(PT(p.x, p.y))); }

    template<typename T> void strokeLine(Vec2<T> p1, Vec2<T> p2)
    {
        beginPath();
        moveTo(p1);
        lineTo(p2);
        stroke();
    }
    template<typename T> void strokeLineSharp(Vec2<T> p1, Vec2<T> p2)
    {
        beginPath();
        moveToSharp(p1);
        lineToSharp(p2);
        stroke();
    }

    template<typename T> void fillTextSharp(std::string_view txt, Vec2<T> pos)
    {
        SimplePainter::fillText(txt, align_full(PT(pos)));
    }

    // ======== Axis ========

    void drawWorldAxis(
        double axis_opacity = 0.3,
        double grid_opacity = 0.075,
        double text_opacity = 0.45
    );

    void drawCursor(double x, double y, double size = 24.0)
    {
        const double s = size;
        const double lw = std::max(1.0, s * 0.075);

        save();

        // Fill
        auto drawPath = [&]()
        {
            beginPath();
            moveTo(x + 0.0, y + 0.0);
            lineTo(x + 0.0, y + 0.95 * s); // bottom
            lineTo(x + 0.30 * s, y + 0.57 * s); // << dip
            lineTo(x + 0.8 * s, y + 0.50 * s); // right
            closePath();
        };

        drawPath();
        setFillStyle(255, 255, 255);
        fill();

        // Outline
        setLineWidth(lw);
        setStrokeStyle(0, 0, 0);
        setLineJoin(LineJoin::JOIN_MITER);
        setMiterLimit(6.0);

        drawPath();
        stroke();

        restore();
    }


    // ======== Expose unchanged methods ========

    using SimplePainter::usePainter;
    using SimplePainter::setGlobalScale;
    using SimplePainter::getGlobalScale;
    using SimplePainter::setGlobalAlpha;
    //
    using SimplePainter::getDefaultFont;
    //
    using SimplePainter::save;
    using SimplePainter::restore;
    using SimplePainter::skewX; // todo: untested
    using SimplePainter::skewY; // todo: untested
    //
    using SimplePainter::setClipRect;   // todo: untested
    using SimplePainter::resetClipping; // todo: untested
    //
    using SimplePainter::setFont;
    using SimplePainter::setTextAlign;
    using SimplePainter::setTextBaseline;
    //
    // ======== composite ========

    using SimplePainter::setCompositeSourceOver;
    using SimplePainter::setCompositeSourceIn;
    using SimplePainter::setCompositeSourceOut;
    using SimplePainter::setCompositeSourceAtop;
    using SimplePainter::setCompositeDestOver;
    using SimplePainter::setCompositeDestIn;
    using SimplePainter::setCompositeDestOut;
    using SimplePainter::setCompositeDestAtop;
    using SimplePainter::setCompositeLighter;
    using SimplePainter::setCompositeCopy;
    using SimplePainter::setCompositeXor;
    using SimplePainter::setComposite;

    // ======== blending ========

    using SimplePainter::setBlendFunc;
    using SimplePainter::setBlendFuncSeparate;
    using SimplePainter::setBlendAdditive;
    using SimplePainter::setBlendAlphaPremult;
    using SimplePainter::setBlendAlphaStraight;
    using SimplePainter::setBlendMultiply;
    using SimplePainter::setBlendScreen;

    using SimplePainter::scopedComposite;
    using SimplePainter::scopedBlend;
    using SimplePainter::scopedBlendSeparate;

    // ======== line cap/join/miter ========

    using SimplePainter::setLineCap;
    using SimplePainter::setLineJoin;
    using SimplePainter::setMiterLimit; // todo: Use world size?

    //
    using SimplePainter::setStrokeStyle;
    using SimplePainter::setFillStyle;
    using SimplePainter::beginPath;
    using SimplePainter::stroke;
    using SimplePainter::fill;
    using SimplePainter::closePath;
};

class Canvas : public SimplePainter
{
    GLuint fbo = 0, tex = 0, rbo = 0;
    bool has_fbo = false;
    int fbo_width = 0, fbo_height = 0;  // Local Canvas dimensions (FBO size)

    //int render_width = 0, render_height = 0;  // Actual size Canvas gets drawn on to screen
    IRect client_rect{};

    // The default painter which draws to this canvas
    PainterContext context;

public:

    void create(double global_scale);
    bool resize(int w, int h);

    IRect clientRect() const { return client_rect; }
    void setClientRect(IRect r) { client_rect = r; }

    void begin(float r, float g, float b, float a = 1.0);
    void end();

    PainterContext* getPainterContext() { return &context; }
    GLuint texture() { return tex; }
    [[nodiscard]] int fboWidth() { return fbo_width; }
    [[nodiscard]] int fboHeight() { return fbo_height; }
    [[nodiscard]] IVec2 fboSize() { return { fbo_width, fbo_height }; }
    [[nodiscard]] int fboExists() { return has_fbo; }

    bool readPixels(std::vector<uint8_t>& out_rgba);
};

GLuint loadSVG(const char* path, int outputWidth, int outputHeight);

BL_END_NS
