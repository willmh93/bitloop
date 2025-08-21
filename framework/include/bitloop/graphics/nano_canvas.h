#pragma once

#include <memory>

#include "platform.h"

#include "nanovg/nanovg.h"
#include "nanovg/nanovg_gl.h"

#include "nano_bitmap.h"
#include "camera.h"
#include "debug.h"

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
    COMPOSITE_SOURCE_OVER       = NVGcompositeOperation::NVG_SOURCE_OVER,
    COMPOSITE_SOURCE_IN         = NVGcompositeOperation::NVG_SOURCE_IN,
    COMPOSITE_SOURCE_OUT        = NVGcompositeOperation::NVG_SOURCE_OUT,
    COMPOSITE_ATOP              = NVGcompositeOperation::NVG_ATOP,
    COMPOSITE_DESTINATION_OVER  = NVGcompositeOperation::NVG_DESTINATION_OVER,
    COMPOSITE_DESTINATION_IN    = NVGcompositeOperation::NVG_DESTINATION_IN,
    COMPOSITE_DESTINATION_OUT   = NVGcompositeOperation::NVG_DESTINATION_OUT,
    COMPOSITE_DESTINATION_ATOP  = NVGcompositeOperation::NVG_DESTINATION_ATOP,
    COMPOSITE_LIGHTER           = NVGcompositeOperation::NVG_LIGHTER,
    COMPOSITE_COPY              = NVGcompositeOperation::NVG_COPY,
    COMPOSITE_XOR               = NVGcompositeOperation::NVG_XOR
};
enum struct BlendFactor
{
    BLEND_ZERO                  = NVGblendFactor::NVG_ZERO,
    BLEND_ONE                   = NVGblendFactor::NVG_ONE,
    BLEND_SRC_COLOR             = NVGblendFactor::NVG_SRC_COLOR,
    BLEND_ONE_MINUS_SRC_COLOR   = NVGblendFactor::NVG_ONE_MINUS_SRC_COLOR,
    BLEND_DST_COLOR             = NVGblendFactor::NVG_DST_COLOR,
    BLEND_ONE_MINUS_DST_COLOR   = NVGblendFactor::NVG_ONE_MINUS_DST_COLOR,
    BLEND_SRC_ALPHA             = NVGblendFactor::NVG_SRC_ALPHA,
    BLEND_ONE_MINUS_SRC_ALPHA   = NVGblendFactor::NVG_ONE_MINUS_SRC_ALPHA,
    BLEND_DST_ALPHA             = NVGblendFactor::NVG_DST_ALPHA,
    BLEND_ONE_MINUS_DST_ALPHA   = NVGblendFactor::NVG_ONE_MINUS_DST_ALPHA,
    BLEND_SRC_ALPHA_SATURATE    = NVGblendFactor::NVG_SRC_ALPHA_SATURATE
};

class NanoFont
{
    friend class SimplePainter;

protected:

    std::string path;
    int id = 0;
    bool created = false;
    float size = 16.0f;

public:

    static std::shared_ptr<NanoFont> create(const char* virtual_path)
    {
        return std::make_shared<NanoFont>(virtual_path);
    }

    NanoFont(const char* virtual_path)
    {
        //BL::print("NanoFont() called");
        path = Platform()->path(virtual_path);
    }

    void setSize(double size_pts)
    {
        size = (float)size_pts;
    }
};

class Painter;

// ===========================================
// ======== Simple nanovg C++ wrapper ========
// ===========================================

class SimplePainter
{
protected:

    friend class Viewport;
    friend class Camera;
    friend class Painter;

    NVGcontext* vg = nullptr;

    TextAlign text_align = TextAlign::ALIGN_LEFT;
    TextBaseline text_baseline = TextBaseline::BASELINE_TOP;

    static std::shared_ptr<NanoFont> default_font;
    std::shared_ptr<NanoFont> active_font;
    
    double global_scale = 1.0;
    double font_size = 16.0;

public:

    void setGlobalScale(double _global_scale) {
        global_scale = _global_scale;
    }
    double getGlobalScale() {
        return global_scale; 
    }

    void setRenderTarget(NVGcontext* nvg_ctx) { vg = nvg_ctx; }
    [[nodiscard]] NVGcontext* getRenderTarget() { return vg; }
    [[nodiscard]] std::shared_ptr<NanoFont> getDefaultFont() { return default_font; }

    // ======== Transforms ========

    void save() const { nvgSave(vg); }
    void restore() { nvgRestore(vg); }
    struct LocalTransform
    {
        SimplePainter* painter;
        LocalTransform(SimplePainter* p) : painter(p) { p->save(); p->resetTransform(); }
        ~LocalTransform() { painter->restore(); }
    };

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
    void setFillStyle(const float(&color)[3])                { nvgFillColor(vg, {color[0], color[1], color[2], 1.0f}); }
    void setFillStyle(const float(&color)[4])                { nvgFillColor(vg, {color[0], color[1], color[2], color[3]}); }
    void setFillStyle(int r, int g, int b, int a = 255)      { nvgFillColor(vg, nvgRGBA(r, g, b, a)); }
    
    void setStrokeStyle(const Color& color)                  { nvgStrokeColor(vg, nvgRGBA(color.r, color.g, color.b, color.a)); }
    void setStrokeStyle(const float(&color)[3])              { nvgStrokeColor(vg, {color[0], color[1], color[2], 1.0f}); }
    void setStrokeStyle(const float(&color)[4])              { nvgStrokeColor(vg, {color[0], color[1], color[2], color[3]}); }
    void setStrokeStyle(int r, int g, int b, int a = 255)    { nvgStrokeColor(vg, nvgRGBA(r, g, b, a)); }
    
    void setLineWidth(double w)                              { nvgStrokeWidth(vg, (float)(w)); }
    void setLineCap(LineCap cap)                             { nvgLineCap(vg, (int)cap); }
    void setLineJoin(LineJoin join)                          { nvgLineJoin(vg, (int)join); }
                                                             
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

    void setTextAlign(TextAlign align)          { text_align = align;       nvgTextAlign(vg, (int)(text_align) | (int)(text_baseline)); }
    void setTextBaseline(TextBaseline baseline) { text_baseline = baseline; nvgTextAlign(vg, (int)(text_align) | (int)(text_baseline)); }
    void setFontSize(double size_pts)           { nvgFontSize(vg, (float)(global_scale * size_pts)); }
    void setFont(std::shared_ptr<NanoFont> font)
    {
        if (font == active_font)
            return;

        if (!font->created)
        {
            font->id = nvgCreateFont(vg, font->path.c_str(), font->path.c_str());
            font->created = true;

            // Todo: Check if font changed and update even if already created
            nvgFontSize(vg, (float)global_scale * font->size);

        }

        nvgFontFaceId(vg, font->id);
        active_font = font;
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
        if (!active_font) setFont(default_font);
        nvgText(vg, (float)(x), (float)(y), txt.data(), txt.data() + txt.size());
    }
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
    friend class Camera;
    friend class Viewport;

    double _avgZoom() const {
        return (fabs(camera.zoomX()) + fabs(camera.zoomY())) * 0.5;
    }

    DVec2 align_full(DVec2 p)              { return DVec2{ floor(p.x), floor(p.y) }; }
    DVec2 align_full(double px, double py) { return DVec2{ floor(px),  floor(py) }; }
    DVec2 align_half(DVec2 p)              { return DVec2{ floor(p.x) + 0.5, floor(p.y) + 0.5 }; }
    DVec2 align_half(double px, double py) { return DVec2{ floor(px)  + 0.5, floor(py)  + 0.5 }; }

    glm::mat3 default_viewport_transform;
    double line_width = 1;

public:

    mutable Camera camera;

    // ======== Position/Size wrappers (applies only enabled camera transforms) ========

    DVec2 PT(double x, double y)      const { return camera.transform_coordinates ? camera.toStage(x, y) : DVec2{ x, y }; }
    DVec2 PT(DVec2 p)                 const { return camera.transform_coordinates ? camera.toStage(p.x, p.y) : p; }
    DQuad QUAD(DQuad q)               const { return { PT(q.a), PT(q.b), PT(q.c), PT(q.d) }; }
    DVec2 SIZE(double w, double h)    const { return camera.scale_sizes ? DVec2{w*camera.zoomX(), h* camera.zoomY()} : DVec2{w, h}; }
    DVec2 SIZE(DVec2 s)               const { return camera.scale_sizes ? DVec2{s.x*camera.zoomX(), s.y*camera.zoomY()} : s; }
    double SIZE(double radius)        const { return camera.scale_sizes ? (radius*_avgZoom()) : radius; }

    void ROTATE(double r)              { if (/*camera.transform_coordinates &&*/ r != 0.0) rotate(r); }
    void TRANSLATE(double x, double y) { translate(PT(x, y)); }


    // ======== Styles ========

    void setLineWidth(double w)
    {
        this->line_width = w;
        if (camera.scale_lines)
            SimplePainter::setLineWidth(w * _avgZoom());
        else
            SimplePainter::setLineWidth(w);
    }

    // ======== Paths (overrides) ========

    void moveTo(double px, double py)                      { SimplePainter::moveTo(PT(px, py)); }
    void lineTo(double px, double py)                      { SimplePainter::lineTo(PT(px, py)); }
    void moveTo(DVec2 p)                                   { SimplePainter::moveTo(PT(p)); }
    void lineTo(DVec2 p)                                   { SimplePainter::lineTo(PT(p)); }

    void circle(double cx, double cy, double r)            { SimplePainter::circle(PT(cx, cy), SIZE(r)); }
    void circle(DVec2 cen, double r)                       { SimplePainter::circle(PT(cen), SIZE(r)); }
    void ellipse(double x, double y, double rx, double ry) { SimplePainter::ellipse(PT(x, y), SIZE(rx, ry)); }
    void ellipse(DVec2 cen, DVec2 size)                    { SimplePainter::ellipse(PT(cen), SIZE(size)); }

    void arc(double cx, double cy, double r, double a0, double a1, PathWinding winding = PathWinding::WINDING_CCW) {
        SimplePainter::arc(PT(cx, cy), SIZE(r), a0, a1, winding);
    }
    void arc(DVec2 cen, double r, double a0, double a1, PathWinding winding = PathWinding::WINDING_CCW) {
        SimplePainter::arc(PT(cen), SIZE(r), a0, a1, winding);
    }
    void arcTo(double x0, double y0, double x1, double y1, double r) {
        SimplePainter::arcTo(PT(x0, y0), PT(x1, y1), SIZE(r));
    }
    void arcTo(DVec2 p0, DVec2 p1, double r) {
        SimplePainter::arcTo(PT(p0), PT(p1), SIZE(r));
    }

    void bezierTo(double x1, double y1, double x2, double y2, double x3, double y3) {
        SimplePainter::bezierTo(PT(x1, y1), PT(x2, y2), PT(x3, y3));
    }
    void quadraticTo(double cx, double cy, double x, double y) {
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

    template<typename PointT, size_t N>
    void drawPath(const PointT(&path)[N])
    {
        if constexpr (N < 2) return;
        moveTo(path[0]);
        for (size_t i = 1; i < N; ++i)
            lineTo(path[i]);
    }

    template<typename PointT, size_t N>
    void drawClosedPath(const PointT(&path)[N])
    {
        if constexpr (N < 2) return;
        moveTo(path[0]);
        for (size_t i = 1; i < N; ++i)
            lineTo(path[i]);
        lineTo(path[0]);
    }

    // ======== Shapes ========

    template<typename T> void strokeQuad(const Quad<T>& q) 
    {
        beginPath();
        drawClosedPath(q._data);
        stroke();
    }

    void strokeRect(double x, double y, double w, double h)
    {
        LocalTransform t(this);
        TRANSLATE(x, y);
        ROTATE(camera.rotation());
        DVec2 s = SIZE(w, h);
        SimplePainter::strokeRect(0, 0, s.x, s.y);
    }
    void fillRect(double x, double y, double w, double h)
    {
        LocalTransform t(this);
        TRANSLATE(x, y);
        ROTATE(camera.rotation());
        DVec2 s = SIZE(w, h);
        SimplePainter::fillRect(0, 0, s.x, s.y);
    }
    void strokeRoundedRect(double x, double y, double w, double h, double r)
    {
        LocalTransform t(this);
        TRANSLATE(x, y);
        ROTATE(camera.rotation());
        DVec2 s = SIZE(w, h);
        SimplePainter::strokeRoundedRect(0, 0, s.x, s.y, SIZE(r));
    }
    void fillRoundedRect(double x, double y, double w, double h, double r)
    {
        LocalTransform t(this);
        TRANSLATE(x, y);
        ROTATE(camera.rotation());
        DVec2 s = SIZE(w, h);
        SimplePainter::fillRoundedRect(0, 0, s.x, s.y, SIZE(r));
    }
    void strokeEllipse(double cx, double cy, double rx, double ry)
    {
        LocalTransform t(this);
        TRANSLATE(cx, cy);
        ROTATE(camera.rotation());
        DVec2 s = SIZE(rx, ry);

        SimplePainter::beginPath();
        SimplePainter::ellipse(0, 0, s.x, s.y);
        SimplePainter::stroke();
    }
    void fillEllipse(double cx, double cy, double rx, double ry)
    {
        LocalTransform t(this);
        TRANSLATE(cx, cy);
        ROTATE(camera.rotation());
        DVec2 s = SIZE(rx, ry);

        SimplePainter::beginPath();
        SimplePainter::ellipse(0, 0, s.x, s.y);
        SimplePainter::fill();
    }

    // Overloads
    void strokeRect(const FRect& r)                    { strokeRect(r.x1, r.y1, r.x2 - r.x1, r.y2 - r.y1); }
    void fillRect(const FRect& r)                      { fillRect(r.x1, r.y1, r.x2 - r.x1, r.y2 - r.y1); }
    void strokeEllipse(double cx, double cy, double r) { strokeEllipse(cx, cy, r, r); }
    void fillEllipse(double cx, double cy, double r)   { fillEllipse(cx, cy, r, r); }

    void drawArrow(DVec2 a, DVec2 b, Color color)
    {
        double dx = b.x - a.x;
        double dy = b.y - a.y;
        double angle = atan2(dy, dx);
        constexpr double tip_sharp_angle = 145.0 * Math::PI / 180.0;
        double arrow_size = line_width * 4 / _avgZoom();

        setLineCap(LineCap::CAP_ROUND);
        setFillStyle(color);
        setStrokeStyle(color);
        beginPath();
        moveTo(a);
        lineTo(b);
        stroke();

        double rx1 = b.x + cos(angle + tip_sharp_angle) * arrow_size;
        double ry1 = b.y + sin(angle + tip_sharp_angle) * arrow_size;
        double rx2 = b.x + cos(angle - tip_sharp_angle) * arrow_size;
        double ry2 = b.y + sin(angle - tip_sharp_angle) * arrow_size;

        beginPath();
        moveTo(b);
        lineTo(rx1, ry1);
        lineTo(rx2, ry2);
        fill();
    }

    double arrow_x0 = 0, arrow_y0 = 0;
    void arrowMoveTo(double px, double py) { arrow_x0 = px; arrow_y0 = py; }
    void arrowDrawTo(double px, double py, Color color = Color(255, 255, 255)) { drawArrow({ arrow_x0, arrow_y0 }, { px, py }, color); }

    // ======== Image ========

    //void drawImage(Image& bmp, double x, double y, double w = 0, double h = 0) {
    //    SimplePainter::drawImage(bmp, x, y, w <= 0 ? bmp.bmp_width : w, h <= 0 ? bmp.bmp_height : h);
    //}

    void drawImage(Image& bmp, DQuad quad) {

        quad = QUAD(quad);

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
    void drawImage(CanvasImage& bmp)
    {
        drawImage(bmp, bmp.worldQuad());
    }


    // ======== Text ========

    void fillText(std::string_view txt, double px, double py)
    {
        //DVec2 p = PT(px, py);

        //if (camera.scale_text)
        //    SimplePainter::setFontSize(SIZE(font_size));
        //else
        //    SimplePainter::setFontSize(font_size);

        LocalTransform t(this);
        TRANSLATE(px, py);

        if (camera.rotate_text)
            ROTATE(camera.rotation());

        if (camera.scale_text)
            scale(camera.zoomX(), camera.zoomY());

        SimplePainter::fillText(txt, px, py);
    }

    void fillText(std::string_view txt, const DVec2& p) {
        fillText(txt, p.x, p.y);
    }

    [[nodiscard]] DRect boundingBox(std::string_view txt)
    {
        save();
        resetTransform();
        transform(default_viewport_transform);
        auto r = SimplePainter::boundingBox(txt);
        restore();
        return r;
    }

    [[nodiscard]] std::string format_number(double v) {
        char buffer[32];
        double abs_v = std::abs(v);

        if ((abs_v != 0.0 && abs_v < 1e-3) || abs_v >= 1e4) {
            // Use scientific notation
            std::snprintf(buffer, sizeof(buffer), "%.5e", v);
        }
        else {
            // Use fixed-point
            std::snprintf(buffer, sizeof(buffer), "%.5f", v);
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

    /*std::string formatScientificMinimal(double v)
    {
        //std::ostringstream oss;
        //oss << std::scientific << v;
        //std::string s = oss.str();
        std::string s = format_number(v);

        // Example: "4.000000e-01"
        std::size_t ePos = s.find('e');
        //if (ePos == std::string::npos)
        //    return s; // should not happen

        std::string mantissa = s.substr(0, ePos);
        std::string exponent = s.substr(ePos); // includes 'e'

        // Remove trailing zeros in mantissa
        std::size_t dotPos = mantissa.find('.');
        if (dotPos != std::string::npos)
        {
            std::size_t lastNonZero = mantissa.find_last_not_of('0');
            if (lastNonZero != std::string::npos)
            {
                if (mantissa[lastNonZero] == '.')
                    lastNonZero--; // keep no trailing dot
                mantissa.erase(lastNonZero + 1);
            }
        }

        return mantissa + exponent;
    }*/

    const float exponent_font_scale = 0.85f;
    const float exponent_spacing_x = 0.06f;
    const float exponent_spacing_y = -0.3f;

    void fillNumberScientific(double v, DVec2 pos, float fontSize = 12)
    {
        std::string txt = format_number(v);

        size_t ePos = txt.find("e");
        if (ePos != std::string::npos)
        {
            const int  exponent = std::stoi(txt.substr(ePos + 1));
            std::string mantissa_txt = txt.substr(0, ePos) + "e";// "x10";
            std::string exponent_txt = std::to_string(exponent);

            //setFont(active_font);
            //active_font.setPixelSize(fontSize);

            pos.x = floor(pos.x);
            pos.y = floor(pos.y);

            double mantissaWidth = boundingBox(mantissa_txt).x2 + exponent_spacing_x;

            /// todo: Take whatever alignment you're given and adjust right bound
            setTextAlign(TextAlign::ALIGN_CENTER);
            setFontSize(fontSize);
            fillTextSharp(mantissa_txt.c_str(), pos);

            pos.x += mantissaWidth/2.0f + (fontSize * exponent_spacing_x);
            pos.y -= (int)(fontSize * (exponent_font_scale + exponent_spacing_y));

            //font.setPixelSize((int)(fontSize * 0.85));
            //painter->setFont(font);
            setTextAlign(TextAlign::ALIGN_LEFT);
            setFontSize(fontSize * exponent_font_scale);

            fillTextSharp(exponent_txt.c_str(), pos);

            setFontSize(fontSize);
            setTextAlign(TextAlign::ALIGN_CENTER);

            //font.setPixelSize(fontSize);
            //painter->setFont(font);
        }
        else
        {
            //font.setPixelSize(fontSize);
            //painter->setFont(font);
            setFontSize(fontSize);
            fillTextSharp(txt.c_str(), pos);
        }
    }

    [[nodiscard]] DRect boundingBoxScientific(double v, float fontSize = 12)
    {
        std::string txt = format_number(v);


        size_t ePos = txt.find("e");
        if (ePos != std::string::npos)
        {
            const int  exponent = std::stoi(txt.substr(ePos + 1));
            std::string mantissa_txt = txt.substr(0, ePos) + "e";// "x10";
            std::string exponent_txt = std::to_string(exponent);

            //font.setPixelSize(fontSize);
            //painter->setFont(font);

            DRect mantissaRect = boundingBox(mantissa_txt);

            //font.setPixelSize((int)(fontSize * 0.85));
            //painter->setFont(font);

            DRect exponentRect = boundingBox(exponent_txt);
            DRect ret = mantissaRect;

            ret.x2 += exponentRect.width() + (fontSize * exponent_spacing_x);
            ret.y1 -= (int)(fontSize * (exponent_font_scale + exponent_spacing_y));

            //font.setPixelSize(fontSize);
            //painter->setFont(font);

            return ret;
        }
        else
        {
            return boundingBox(txt);
        }
    }

   

    // ======== Sharp variants ========

    void moveToSharp(double px, double py)  { SimplePainter::moveTo(align_half(PT(px, py))); }
    void moveToSharp(DVec2 p)               { SimplePainter::moveTo(align_half(PT(p))); }
    void lineToSharp(double px, double py)  { SimplePainter::lineTo(align_half(PT(px, py))); }
    void lineToSharp(DVec2 p)               { SimplePainter::lineTo(align_half(PT(p.x, p.y))); }

    void strokeLine(DVec2 p1, DVec2 p2)
    {
        beginPath();
        moveTo(p1);
        lineTo(p2);
        stroke();
    }

    void strokeLineSharp(DVec2 p1, DVec2 p2)
    {
        beginPath();
        moveToSharp(p1);
        lineToSharp(p2);
        stroke();
    }

    void fillTextSharp(std::string_view txt, const DVec2& pos)
    {
        SimplePainter::fillText(txt, align_full(PT(pos)));
    }

    // ======== Axis Drawer ========
    void drawWorldAxis(
        double axis_opacity = 0.3,//0.3,
        double grid_opacity = 0.075,
        double text_opacity = 0.45// 0.4
    );

    // ======== Expose unchanged methods ========

    using SimplePainter::setRenderTarget;
    using SimplePainter::getRenderTarget;
    using SimplePainter::setGlobalScale;
    using SimplePainter::getGlobalScale;
    //
    using SimplePainter::getDefaultFont;
    //
    using SimplePainter::save;
    using SimplePainter::restore;
    using SimplePainter::resetTransform;
    using SimplePainter::currentTransform;
    using SimplePainter::transform;
    using SimplePainter::translate;
    using SimplePainter::rotate;
    using SimplePainter::scale;
    using SimplePainter::skewX;
    using SimplePainter::skewY;
    //
    using SimplePainter::setClipRect;
    using SimplePainter::resetClipping;
    //
    using SimplePainter::setFont;
    using SimplePainter::setFontSize;
    using SimplePainter::setTextAlign;
    using SimplePainter::setTextBaseline;
    //
    using SimplePainter::setStrokeStyle;
    using SimplePainter::setFillStyle;
    using SimplePainter::beginPath;
    using SimplePainter::stroke;
    using SimplePainter::fill;
    using SimplePainter::closePath;
    //using SimplePainter::bezierTo;
    //using SimplePainter::quadraticTo;
};

class Canvas : public SimplePainter
{
    GLuint fbo = 0, tex = 0, rbo = 0;
    int fbo_width = 0, fbo_height = 0;

public:

    void create(double global_scale);
    bool resize(int w, int h);

    void begin(float r, float g, float b, float a = 1.0);
    void end();

    GLuint texture() { return tex; }
    [[nodiscard]] int fboWidth() { return fbo_width; }
    [[nodiscard]] int fboHeight() { return fbo_height; }
};

GLuint loadSVG(const char* path, int outputWidth, int outputHeight);

BL_END_NS
