#pragma once

/// OpenGL
#ifdef __EMSCRIPTEN__
#include <GLES3/gl3.h>
#else
#include "glad.h"
#endif
#include "glm/glm.hpp"
#include "platform.h"

#include <memory>

#include "nanovg.h"
#include "nanovg_gl.h"

#include "camera.h"

class Viewport;

enum PathWinding
{
    WINDING_CCW = NVGwinding::NVG_CCW,
    WINDING_CW  = NVGwinding::NVG_CW
};
enum LineCap 
{
    CAP_BUTT   = NVGlineCap::NVG_BUTT,
    CAP_ROUND  = NVGlineCap::NVG_ROUND,
    CAP_SQUARE = NVGlineCap::NVG_SQUARE,
};
enum LineJoin 
{
    JOIN_BEVEL  = NVGlineCap::NVG_BEVEL,
    JOIN_MITER  = NVGlineCap::NVG_MITER
};
enum TextAlign 
{
    ALIGN_LEFT   = NVGalign::NVG_ALIGN_LEFT,
    ALIGN_CENTER = NVGalign::NVG_ALIGN_CENTER,
    ALIGN_RIGHT  = NVGalign::NVG_ALIGN_RIGHT
};
enum TextBaseline 
{
    BASELINE_TOP    = NVGalign::NVG_ALIGN_TOP,
    BASELINE_MIDDLE = NVGalign::NVG_ALIGN_MIDDLE,
    BASELINE_BOTTOM = NVGalign::NVG_ALIGN_BOTTOM
};
enum CompositeOperation 
{
    COMPOSITE_SOURCE_OVER      = NVGcompositeOperation::NVG_SOURCE_OVER,
    COMPOSITE_SOURCE_IN        = NVGcompositeOperation::NVG_SOURCE_IN,
    COMPOSITE_SOURCE_OUT       = NVGcompositeOperation::NVG_SOURCE_OUT,
    COMPOSITE_ATOP             = NVGcompositeOperation::NVG_ATOP,
    COMPOSITE_DESTINATION_OVER = NVGcompositeOperation::NVG_DESTINATION_OVER,
    COMPOSITE_DESTINATION_IN   = NVGcompositeOperation::NVG_DESTINATION_IN,
    COMPOSITE_DESTINATION_OUT  = NVGcompositeOperation::NVG_DESTINATION_OUT,
    COMPOSITE_DESTINATION_ATOP = NVGcompositeOperation::NVG_DESTINATION_ATOP,
    COMPOSITE_LIGHTER          = NVGcompositeOperation::NVG_LIGHTER,
    COMPOSITE_COPY             = NVGcompositeOperation::NVG_COPY,
    COMPOSITE_XOR              = NVGcompositeOperation::NVG_XOR
};
enum BlendFactor 
{
    BLEND_ZERO                 = NVGblendFactor::NVG_ZERO,
    BLEND_ONE                  = NVGblendFactor::NVG_ONE,
    BLEND_SRC_COLOR            = NVGblendFactor::NVG_SRC_COLOR,
    BLEND_ONE_MINUS_SRC_COLOR  = NVGblendFactor::NVG_ONE_MINUS_SRC_COLOR,
    BLEND_DST_COLOR            = NVGblendFactor::NVG_DST_COLOR,
    BLEND_ONE_MINUS_DST_COLOR  = NVGblendFactor::NVG_ONE_MINUS_DST_COLOR,
    BLEND_SRC_ALPHA            = NVGblendFactor::NVG_SRC_ALPHA,
    BLEND_ONE_MINUS_SRC_ALPHA  = NVGblendFactor::NVG_ONE_MINUS_SRC_ALPHA,
    BLEND_DST_ALPHA            = NVGblendFactor::NVG_DST_ALPHA,
    BLEND_ONE_MINUS_DST_ALPHA  = NVGblendFactor::NVG_ONE_MINUS_DST_ALPHA,
    BLEND_SRC_ALPHA_SATURATE   = NVGblendFactor::NVG_SRC_ALPHA_SATURATE
};

class NanoFont
{
    friend class SimplePainter;

protected:

    std::string path;
    int id = 0;
    bool created = false;

public:

    static std::shared_ptr<NanoFont> create(const char* virtual_path)
    {
        return std::make_shared<NanoFont>(virtual_path);
    }

    NanoFont(const char* virtual_path)
    {
        path = Platform::get()->path(virtual_path);
        //#ifdef __EMSCRIPTEN__
        //path = virtual_path;
        //#else
        //path = virtual_path + 1; // Skip first '/'
        //#endif
    }
};

class Painter;

// Simple nanovg abstract layer
class SimplePainter
{
public:

    //enum PixelAlign 
    //{
    //    PIXEL_ALIGN_NONE = 0,
    //    PIXEL_ALIGN_HALF = 1,
    //    PIXEL_ALIGN_FULL = 2
    //};

protected:

    friend class Viewport;
    friend class Camera;
    friend class Painter;

    NVGcontext* vg = nullptr;

    TextAlign text_align = TextAlign::ALIGN_LEFT;
    TextBaseline text_baseline = TextBaseline::BASELINE_TOP;


    static std::shared_ptr<NanoFont> default_font;
    std::shared_ptr<NanoFont> active_font;

public:

    void setRenderTarget(NVGcontext* nvg_ctx)
    {
        vg = nvg_ctx;
    }

    NVGcontext* getRenderTarget()
    {
        return vg;
    }

    void save()
    {
        nvgSave(vg);
    }

    void restore()
    {
        nvgRestore(vg);
    }

    void resetTransform()
    {
        nvgResetTransform(vg);
    }

    glm::mat3 currentTransform() const
    {
        float xform[6];
        nvgCurrentTransform(vg, xform);

        // Construct a glm::mat3 using the 3x2 affine matrix data
        glm::mat3 t = glm::mat3(
            xform[0], xform[1], 0.0f,  // column 0
            xform[2], xform[3], 0.0f,  // column 1
            xform[4], xform[5], 1.0f   // column 2 (translation + homogeneous)
        );

        return t;
    }

    void transform(const glm::mat3& m)
    {
        nvgTransform(vg,
            m[0][0], // a
            m[0][1], // b
            m[1][0], // c
            m[1][1], // d
            m[2][0], // e (translate x)
            m[2][1]  // f (translate y)
        );
    }

    void translate(double x, double y)
    {
        nvgTranslate(vg, static_cast<float>(x), static_cast<float>(y));
    }

    void rotate(double angle)
    {
        nvgRotate(vg, static_cast<float>(angle));
    }

    void scale(double scale)
    {
        nvgScale(vg, scale, scale);
    }

    void scale(double scale_x, double scale_y)
    {
        nvgScale(vg, scale_x, scale_y);
    }

    void skewX(double angle)
    {
        nvgSkewX(vg, angle);
    }
    
    void skewY(double angle)
    {
        nvgSkewY(vg, angle);
    }

    void setClipRect(double x, double y, double w, double h)
    {
        nvgScissor(vg, 
            static_cast<float>(x), static_cast<float>(y),
            static_cast<float>(w), static_cast<float>(h)
        );
    }

    void resetClipping()
    {
        nvgResetScissor(vg);
    }

    void setTextAlign(TextAlign align)
    {
        text_align = align;
        nvgTextAlign(vg, static_cast<int>(text_align) | static_cast<int>(text_baseline));
    }

    void setTextBaseline(TextBaseline baseline)
    {
        text_baseline = baseline;
        nvgTextAlign(vg, static_cast<int>(text_align) | static_cast<int>(text_baseline));
    }

    void setStrokeStyle(int r, int g, int b, int a = 255)
    {
        nvgStrokeColor(vg, nvgRGBA(r, g, b, a));
    }

    void setFillStyle(int r, int g, int b, int a = 255)
    {
        nvgFillColor(vg, nvgRGBA(r, g, b, a));
    }

    void setLineWidth(double w)
    {
        nvgStrokeWidth(vg, static_cast<float>(w));
    }

    void beginPath()
    {
        nvgBeginPath(vg);
    }

    void moveTo(double x, double y)
    {
        nvgMoveTo(vg, x, y);
    }

    void lineTo(double x, double y)
    {
        nvgLineTo(vg, x, y);
    }

    void stroke()
    {
        nvgStroke(vg);
    }

    void fill()
    {
        nvgFill(vg);
    }

    void setFont(std::shared_ptr<NanoFont> font)
    {
        if (!font->created)
        {
            font->id = nvgCreateFont(vg, font->path.c_str(), font->path.c_str());
            font->created = true;
        }

        nvgFontFaceId(vg, font->id);
    }

    void fillText(const char* text, double x, double y)
    {
        if (!active_font)
            setFont(default_font);
        
        nvgText(vg, x, y, text, nullptr);
    }

    void fillText(const char* text, const Vec2& pos)
    {
        fillText(text, pos.x, pos.y);
    }

    void fillText(const std::string& txt, double x, double y)
    {
        if (!active_font)
            setFont(default_font);

        nvgText(vg, x, y, txt.c_str(), txt.c_str() + txt.size());
    }

    void circle(double x, double y, double radius)
    {
        nvgCircle(vg, x, y, radius);
    }

    void ellipse(double x, double y, double radius_x, double radius_y)
    {
        nvgEllipse(vg, x, y, radius_x, radius_y);
    }

    void fillRect(double x, double y, double w, double h)
    {
        nvgBeginPath(vg);
        nvgRect(vg, x, y, w, h);
        nvgFill(vg);
    }

    void strokeRect(double x, double y, double w, double h)
    {
        nvgBeginPath(vg);
        nvgRect(vg, x, y, w, h);
        nvgStroke(vg);
    }
};

//using PathWinding  = SimplePainter::PathWinding;
//using LineCap      = SimplePainter::LineCap;
//using LineJoin     = SimplePainter::LineJoin;
//using TextAlign    = SimplePainter::TextAlign;
//using TextBaseline = SimplePainter::TextBaseline;
//using PixelAlign   = SimplePainter::PixelAlign;

class Painter : private SimplePainter
{
    double _avgZoom()
    {
        return (abs(camera.zoom_x) + abs(camera.zoom_y)) * 0.5;
    }

public:

    Camera camera;
    glm::mat3 default_viewport_transform;
    double line_width = 1;

    Vec2 PT(double x, double y)
    {
        if (camera.transform_coordinates)
        {
            Vec2 o = camera.toWorldOffset({ camera.stage_ox, camera.stage_oy });
            return { x + o.x, y + o.y };
        }
        else
        {
            // (x,y) represents stage coordinate, but transform is active
            Vec2 ret = camera.toWorld(x + camera.stage_ox, y + camera.stage_oy);
            return ret;
        }
    }

    void setLineWidth(double w)
    {
        this->line_width = w;

        if (camera.scale_lines_text)
        {
            SimplePainter::setLineWidth(w);
        }
        else
        {
            SimplePainter::setLineWidth(w / _avgZoom());
        }
    }

    void circle(double cx, double cy, double r)
    {
        Vec2 pt = PT(cx, cy);
        if (camera.scale_sizes)
        {
            SimplePainter::circle(pt.x, pt.y, r);
        }
        else
        {
            SimplePainter::circle(pt.x, pt.y, r / _avgZoom());
        }
    }

    void circle(Vec2 cen, double r)
    {
        Vec2 pt = PT(cen.x, cen.y);
        if (camera.scale_sizes)
        {
            SimplePainter::circle(pt.x, pt.y, r);
        }
        else
        {
            SimplePainter::circle(pt.x, pt.y, r / _avgZoom());
        }
    }

    void moveTo(double px, double py)
    {
        Vec2 pt = PT(px, py);
        SimplePainter::moveTo(pt.x, pt.y);
    }

    void moveTo(Vec2 p)
    {
        Vec2 pt = PT(p.x, p.y);
        SimplePainter::moveTo(pt.x, pt.y);
    }

    void lineTo(double px, double py)
    {
        Vec2 pt = PT(px, py);
        SimplePainter::lineTo(pt.x, pt.y);
    }

    void lineTo(Vec2 p)
    {
        Vec2 pt = PT(p.x, p.y);
        SimplePainter::lineTo(pt.x, pt.y);
    }

    void strokeRect(double x, double y, double w, double h)
    {
        if (camera.transform_coordinates)
        {
            if (camera.scale_lines_text)
            {
                SimplePainter::strokeRect(x, y, w, h);
            }
            else
            {
                double old_linewidth = line_width;
                SimplePainter::setLineWidth(line_width / _avgZoom());
                SimplePainter::strokeRect(x, y, w, h);
                SimplePainter::setLineWidth(old_linewidth);
            }
        }
        else
        {
            auto cur_transform = SimplePainter::currentTransform();
            SimplePainter::resetTransform();
            SimplePainter::transform(default_viewport_transform);

            if (camera.scale_lines_text)
            {
                double old_linewidth = line_width;
                SimplePainter::setLineWidth(line_width * _avgZoom());
                SimplePainter::strokeRect(x, y, w, h);
                SimplePainter::setLineWidth(old_linewidth);
            }
            else
            {
                SimplePainter::setLineWidth(line_width); // Refresh cached line width
                SimplePainter::strokeRect(x, y, w, h);
            }

            SimplePainter::resetTransform();
            SimplePainter::transform(cur_transform);
        }
    }

    void strokeRect(const FRect& r)
    {
        strokeRect(
            r.x1,
            r.y1,
            r.x2 - r.x1,
            r.y2 - r.y1
        );
    }

    void strokeEllipse(double cx, double cy, double r)
    {
        SimplePainter::beginPath();
        if (camera.scale_sizes)
        {
            SimplePainter::ellipse(cx, cy, r, r);
        }
        else
        {
            SimplePainter::ellipse(cx, cy, r / camera.zoom_x, r / camera.zoom_y);
        }
        SimplePainter::stroke();
    }

    void strokeEllipse(double cx, double cy, double rx, double ry)
    {
        SimplePainter::beginPath();
        if (camera.scale_sizes)
        {
            SimplePainter::ellipse(cx, cy, rx, ry);
        }
        else
        {
            SimplePainter::ellipse(cx, cy, rx / camera.zoom_x, ry / camera.zoom_y);
        }
        SimplePainter::stroke();
    }

    void fillEllipse(double cx, double cy, double r)
    {
        //painter->beginPath();
        //painter->ellipse(cx, cy, r, r);
        //painter->fill();

        Vec2 pt = PT(cx, cy);
        SimplePainter::beginPath();
        if (camera.scale_sizes)
        {
            SimplePainter::ellipse(cx, cy, r, r);
        }
        else
        {
            SimplePainter::ellipse(cx, cy, r / camera.zoom_x, r / camera.zoom_y);
        }
        SimplePainter::fill();
    }

    void fillEllipse(double cx, double cy, double rx, double ry)
    {
        SimplePainter::beginPath();
        if (camera.scale_sizes)
        {
            SimplePainter::ellipse(cx, cy, rx, ry);
        }
        else
        {
            SimplePainter::ellipse(cx, cy, rx / camera.zoom_x, ry / camera.zoom_y);
        }
        SimplePainter::fill();
    }

    // expose unchanged methods/enums
    //using SimplePainter::PathWinding;
    //using SimplePainter::LineCap;
    //using SimplePainter::LineJoin;
    //using SimplePainter::TextAlign;
    //using SimplePainter::TextBaseline;

    using SimplePainter::setRenderTarget;
    using SimplePainter::getRenderTarget;

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
    using SimplePainter::setClipRect;
    using SimplePainter::resetClipping;
    using SimplePainter::setTextAlign;
    using SimplePainter::setTextBaseline;
    using SimplePainter::setStrokeStyle;
    using SimplePainter::setFillStyle;
    using SimplePainter::beginPath;
    using SimplePainter::stroke;
    using SimplePainter::fill;
    using SimplePainter::fillText;
};

class Canvas : public SimplePainter
{
    
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

    
};