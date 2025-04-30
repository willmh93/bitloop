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

#include "nano_bitmap.h"

#include "camera.h"
#include "debug.h"

class Viewport;

enum PathWinding
{
    WINDING_CCW = NVGwinding::NVG_CCW,
    WINDING_CW = NVGwinding::NVG_CW
};
enum LineCap
{
    CAP_BUTT = NVGlineCap::NVG_BUTT,
    CAP_ROUND = NVGlineCap::NVG_ROUND,
    CAP_SQUARE = NVGlineCap::NVG_SQUARE,
};
enum LineJoin
{
    JOIN_BEVEL = NVGlineCap::NVG_BEVEL,
    JOIN_MITER = NVGlineCap::NVG_MITER
};
enum TextAlign
{
    ALIGN_LEFT = NVGalign::NVG_ALIGN_LEFT,
    ALIGN_CENTER = NVGalign::NVG_ALIGN_CENTER,
    ALIGN_RIGHT = NVGalign::NVG_ALIGN_RIGHT
};
enum TextBaseline
{
    BASELINE_TOP = NVGalign::NVG_ALIGN_TOP,
    BASELINE_MIDDLE = NVGalign::NVG_ALIGN_MIDDLE,
    BASELINE_BOTTOM = NVGalign::NVG_ALIGN_BOTTOM
};
enum CompositeOperation
{
    COMPOSITE_SOURCE_OVER = NVGcompositeOperation::NVG_SOURCE_OVER,
    COMPOSITE_SOURCE_IN = NVGcompositeOperation::NVG_SOURCE_IN,
    COMPOSITE_SOURCE_OUT = NVGcompositeOperation::NVG_SOURCE_OUT,
    COMPOSITE_ATOP = NVGcompositeOperation::NVG_ATOP,
    COMPOSITE_DESTINATION_OVER = NVGcompositeOperation::NVG_DESTINATION_OVER,
    COMPOSITE_DESTINATION_IN = NVGcompositeOperation::NVG_DESTINATION_IN,
    COMPOSITE_DESTINATION_OUT = NVGcompositeOperation::NVG_DESTINATION_OUT,
    COMPOSITE_DESTINATION_ATOP = NVGcompositeOperation::NVG_DESTINATION_ATOP,
    COMPOSITE_LIGHTER = NVGcompositeOperation::NVG_LIGHTER,
    COMPOSITE_COPY = NVGcompositeOperation::NVG_COPY,
    COMPOSITE_XOR = NVGcompositeOperation::NVG_XOR
};
enum BlendFactor
{
    BLEND_ZERO = NVGblendFactor::NVG_ZERO,
    BLEND_ONE = NVGblendFactor::NVG_ONE,
    BLEND_SRC_COLOR = NVGblendFactor::NVG_SRC_COLOR,
    BLEND_ONE_MINUS_SRC_COLOR = NVGblendFactor::NVG_ONE_MINUS_SRC_COLOR,
    BLEND_DST_COLOR = NVGblendFactor::NVG_DST_COLOR,
    BLEND_ONE_MINUS_DST_COLOR = NVGblendFactor::NVG_ONE_MINUS_DST_COLOR,
    BLEND_SRC_ALPHA = NVGblendFactor::NVG_SRC_ALPHA,
    BLEND_ONE_MINUS_SRC_ALPHA = NVGblendFactor::NVG_ONE_MINUS_SRC_ALPHA,
    BLEND_DST_ALPHA = NVGblendFactor::NVG_DST_ALPHA,
    BLEND_ONE_MINUS_DST_ALPHA = NVGblendFactor::NVG_ONE_MINUS_DST_ALPHA,
    BLEND_SRC_ALPHA_SATURATE = NVGblendFactor::NVG_SRC_ALPHA_SATURATE
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
        DebugMessage("NanoFont() called");
        path = Platform::get()->path(virtual_path);
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

    // --- Transforms ---

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

    void translate(const Vec2& p)
    {
        nvgTranslate(vg, static_cast<float>(p.x), static_cast<float>(p.y));
    }

    void rotate(double angle)
    {
        nvgRotate(vg, static_cast<float>(angle));
    }

    void scale(double scale)
    {
        float fScale = static_cast<float>(scale);
        nvgScale(vg, fScale, fScale);
    }

    void scale(double scale_x, double scale_y)
    {
        nvgScale(vg, static_cast<float>(scale_x), static_cast<float>(scale_y));
    }

    void skewX(double angle)
    {
        nvgSkewX(vg, static_cast<float>(angle));
    }

    void skewY(double angle)
    {
        nvgSkewY(vg, static_cast<float>(angle));
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

    // --- Styles ---

    void setFillStyle(const Color& color)
    {
        nvgFillColor(vg, nvgRGBA(color.r, color.g, color.b, color.a));
    }
    void setStrokeStyle(const Color& color)
    {
        nvgStrokeColor(vg, nvgRGBA(color.r, color.g, color.b, color.a));
    }

    void setFillStyle(int r, int g, int b, int a = 255)
    {
        nvgFillColor(vg, nvgRGBA(r, g, b, a));
    }
    void setStrokeStyle(int r, int g, int b, int a = 255)
    {
        nvgStrokeColor(vg, nvgRGBA(r, g, b, a));
    }

    void setLineWidth(double w)
    {
        nvgStrokeWidth(vg, static_cast<float>(w));
    }

    void setLineCap(LineCap cap)
    {
        nvgLineCap(vg, cap);
    }

    void setLineJoin(LineJoin join)
    {
        nvgLineJoin(vg, join);
    }

    // --- Paths ---

    void beginPath()
    {
        nvgBeginPath(vg);
    }

    void moveTo(double x, double y)
    {
        nvgMoveTo(vg, static_cast<float>(x), static_cast<float>(y));
    }

    void lineTo(double x, double y)
    {
        nvgLineTo(vg, static_cast<float>(x), static_cast<float>(y));
    }

    void stroke()
    {
        nvgStroke(vg);
    }

    template<typename PointT> void drawPath(const std::vector<PointT>& path)
    {
        if (path.size() < 2) return;
        size_t len = path.size();
        moveTo(path[0]);
        for (size_t i = 1; i < len; i++)
            lineTo(path[i]);
    }

    void fill()
    {
        nvgFill(vg);
    }

    void circle(double x, double y, double radius)
    {
        nvgCircle(vg, 
            static_cast<float>(x), 
            static_cast<float>(y),
            static_cast<float>(radius)
        );
    }

    void ellipse(double x, double y, double radius_x, double radius_y)
    {
        nvgEllipse(vg, 
            static_cast<float>(x), 
            static_cast<float>(y), 
            static_cast<float>(radius_x), 
            static_cast<float>(radius_y)
        );
    }

    void fillRect(double x, double y, double w, double h)
    {
        nvgBeginPath(vg);
        nvgRect(vg, 
            static_cast<float>(x), 
            static_cast<float>(y),
            static_cast<float>(w),
            static_cast<float>(h)
        );
        nvgFill(vg);
    }

    void strokeRect(double x, double y, double w, double h)
    {
        nvgBeginPath(vg);
        nvgRect(vg, 
            static_cast<float>(x),
            static_cast<float>(y),
            static_cast<float>(w),
            static_cast<float>(h)
        );
        nvgStroke(vg);
    }

    // --- Image ---

    void drawImage(Image& bmp, double x, double y, double w = 0, double h = 0)
    {
        if (w <= 0.0) w = bmp.bmp_width;
        if (h <= 0.0) h = bmp.bmp_height;
        bmp.draw(vg, x, y, w, h);
    }

    // --- Text ---

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

    void setFont(std::shared_ptr<NanoFont> font)
    {
        if (!font->created)
        {
            font->id = nvgCreateFont(vg, font->path.c_str(), font->path.c_str());
            font->created = true;
        }

        nvgFontFaceId(vg, font->id);
    }

    FRect boundingBox(const std::string& txt)
    {
        float bounds[4];
        nvgTextBounds(vg, 0, 0, txt.c_str(), txt.c_str() + txt.size(), bounds);
        return FRect(
            static_cast<double>(bounds[0]),
            static_cast<double>(bounds[1]),
            static_cast<double>(bounds[2] - bounds[0]),
            static_cast<double>(bounds[3] - bounds[1])
        );
    }

    void fillText(const char* txt, double x, double y)
    {
        if (!active_font)
            setFont(default_font);

        nvgText(vg, static_cast<float>(x), static_cast<float>(y), txt, nullptr);
    }

    void fillText(const char* txt, const Vec2& pos)
    {
        fillText(txt, pos.x, pos.y);
    }

    void fillText(const std::string& txt, double x, double y)
    {
        if (!active_font)
            setFont(default_font);

        const char* ptr = txt.c_str();
        nvgText(vg,
            static_cast<float>(x),
            static_cast<float>(y),
            ptr, 
            ptr + txt.size()
        );
    }
};


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
        // todo: Alter point reference instead of returning new point
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

    // --- Styles ---

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

    /*void setFillStyle(const Color& color)
    {
        SimplePainter::setFillStyle(color);
    }
    void setStrokeStyle(int r, int g, int b, int a = 255)
    {
        SimplePainter::setStrokeStyle(r, g, b, a);
    }

    void setStrokeStyle(const Color& color)
    {
        SimplePainter::setStrokeStyle(color);
    }
    void setFillStyle(int r, int g, int b, int a = 255)
    {
        SimplePainter::setFillStyle(r, g, b, a);
    }*/

    // --- Paths (overrides) ---

    void circle(double cx, double cy, double radius)
    {
        Vec2 pt = PT(cx, cy);
        if (camera.scale_sizes)
        {
            SimplePainter::circle(pt.x, pt.y, radius);
        }
        else
        {
            SimplePainter::circle(pt.x, pt.y, radius / _avgZoom());
        }
    }

    void circle(Vec2 cen, double radius)
    {
        circle(cen.x, cen.y, radius);
    }

    void ellipse(double x, double y, double radius_x, double radius_y)
    {
        Vec2 pt = PT(x, y);
        if (camera.scale_sizes)
        {
            SimplePainter::ellipse(pt.x, pt.y, 
                radius_x,
                radius_y
            );
        }
        else
        {
            SimplePainter::ellipse(pt.x, pt.y,
                radius_x * camera.zoom_x,
                radius_y * camera.zoom_y
            );
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

    template<typename PointT> void drawPath(const std::vector<PointT>& path)
    {
        if (path.size() < 2) return;
        size_t len = path.size();
        moveTo(path[0]);
        for (size_t i = 1; i < len; i++)
            lineTo(path[i]);
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

    void fillRect(double x, double y, double w, double h)
    {
        if (camera.transform_coordinates)
        {
            SimplePainter::fillRect(x, y, w, h);
        }
        else
        {
            auto cur_transform = SimplePainter::currentTransform();
            SimplePainter::resetTransform();
            SimplePainter::transform(default_viewport_transform);

            SimplePainter::fillRect(x, y, w, h);

            SimplePainter::resetTransform();
            SimplePainter::transform(cur_transform);
        }
    }

    void fillRect(const FRect& r)
    {
        fillRect(
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

    void drawArrow(Vec2 a, Vec2 b, Color color)
    {
        double dx = b.x - a.x;
        double dy = b.y - a.y;
        double len = sqrt(dx * dx + dy * dy);
        double angle = atan2(dy, dx);
        constexpr double tip_sharp_angle = 145.0 * M_PI / 180.0;
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
        //closePath();
        fill();
    }

    double arrow_x0 = 0, arrow_y0 = 0;
    void arrowMoveTo(double px, double py)
    {
        arrow_x0 = px;
        arrow_y0 = py;
    }
    void arrowDrawTo(double px, double py, Color color = Color(255, 255, 255))
    {
        drawArrow({ arrow_x0, arrow_y0 }, { px, py }, color);
    }

    // --- Image ---

    void drawImage(Image& bmp, double x, double y, double w = 0, double h = 0)
    {
        if (w <= 0) w = bmp.bmp_width;
        if (h <= 0) h = bmp.bmp_height;
        SimplePainter::drawImage(bmp, x, y, w, h);
    }

    void drawImage(CanvasImage& bmp)
    {
        camera.saveCameraTransform();
        if (bmp.coordinate_type == CoordinateType::STAGE)
        {
            camera.stageTransform();
        }
        else
        {
            camera.worldTransform();
        }

        if (camera.transform_coordinates)
        {
            save();

            translate(bmp.x, bmp.y);
            rotate(bmp.rotation);
            translate(bmp.localAlignOffsetX(), bmp.localAlignOffsetY());
            scale(bmp.w / bmp.bmp_width, bmp.h / bmp.bmp_height);

            SimplePainter::drawImage(bmp, 0, 0);

            restore();
        }
        else
        {
            glm::mat3 cur_transform = currentTransform();
            resetTransform();
            transform(default_viewport_transform);

            translate(bmp.x, bmp.y);
            rotate(bmp.rotation);
            translate(bmp.localAlignOffsetX(), bmp.localAlignOffsetY());
            scale(bmp.w / bmp.bmp_width, bmp.h / bmp.bmp_height);

            SimplePainter::drawImage(bmp, 0, 0);

            resetTransform();
            transform(cur_transform);
        }

        camera.restoreCameraTransform();
    }

    // --- Text ---

    void fillText(const std::string& txt, double px, double py)
    {
        if (camera.transform_coordinates)
        {
            if (camera.scale_lines_text)
            {
                if (camera.rotate_text)
                {
                    // No change
                    SimplePainter::fillText(txt, px, py);
                }
                else
                {
                    glm::mat3 cur_transform = currentTransform();
                    resetTransform();
                    transform(default_viewport_transform);

                    translate(camera.toStage(px, py));
                    //rotate(camera.rotation);
                    scale(camera.zoom_x, camera.zoom_y);
                    SimplePainter::fillText(txt, 0, 0);

                    resetTransform();
                    transform(cur_transform);
                }
            }
            else
            {
                glm::mat3 cur_transform = currentTransform();
                resetTransform();
                transform(default_viewport_transform);

                //translate(camera.toWorldOffset(px, py));
                translate(camera.toStage(px, py));
                if (camera.rotate_text) 
                    rotate(camera.rotation);
                SimplePainter::fillText(txt, 0, 0);

                resetTransform();
                transform(cur_transform);
            }
        }
        else
        {
            glm::mat3 cur_transform = currentTransform();
            resetTransform();
            transform(default_viewport_transform);

            if (camera.scale_lines_text)
            {
                SimplePainter::fillText(txt, px, py);
            }
            else
            {
                SimplePainter::fillText(txt, px, py);
            }

            resetTransform();
            transform(cur_transform);
        }
    }

    void fillText(const std::string& txt, const Vec2& p)
    {
        fillText(txt, p.x, p.y);
    }

    std::string formatScientificMinimal(double v)
    {
        std::ostringstream oss;
        oss << std::scientific << v;
        std::string s = oss.str();

        // Example: "4.000000e-01"
        std::size_t ePos = s.find('e');
        if (ePos == std::string::npos)
            return s; // should not happen

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
    }

    void fillNumberScientific(double v, Vec2 pos, float fontSize = 12)
    {
        std::string txt = formatScientificMinimal(v);

        size_t ePos = txt.find("e");
        if (ePos != std::string::npos)
        {
            const int  exponent = std::stoi(txt.substr(ePos + 1));
            std::string mantissa_txt = txt.substr(0, ePos) + "x10";
            std::string exponent_txt = std::to_string(exponent);

            //setFont(active_font);
            //active_font.setPixelSize(fontSize);

            pos.x = floor(pos.x);
            pos.y = floor(pos.y);

            double mantissaWidth = boundingBox(mantissa_txt).x2 + 1;
            fillTextSharp(mantissa_txt.c_str(), pos);

            pos.x += mantissaWidth;
            pos.y -= (int)(fontSize * 0.7 + 1);

            //font.setPixelSize((int)(fontSize * 0.85));
            //painter->setFont(font);
            fillTextSharp(exponent_txt.c_str(), pos);

            //font.setPixelSize(fontSize);
            //painter->setFont(font);
        }
        else
        {
            //font.setPixelSize(fontSize);
            //painter->setFont(font);
            fillTextSharp(txt.c_str(), pos);
        }
    }

    FRect boundingBoxNumberScientific(double v, float fontSize = 12)
    {
        std::string txt = formatScientificMinimal(v);

        size_t ePos = txt.find("e");
        if (ePos != std::string::npos)
        {
            const int  exponent = std::stoi(txt.substr(ePos + 1));
            std::string mantissa_txt = txt.substr(0, ePos) + "x10";
            std::string exponent_txt = std::to_string(exponent);

            //font.setPixelSize(fontSize);
            //painter->setFont(font);

            FRect mantissaRect = boundingBox(mantissa_txt);

            //font.setPixelSize((int)(fontSize * 0.85));
            //painter->setFont(font);

            FRect exponentRect = boundingBox(exponent_txt);
            FRect ret = mantissaRect;

            ret.y1 -= (int)(fontSize * 0.7 + 1);
            ret.x2 += exponentRect.width();

            //font.setPixelSize(fontSize);
            //painter->setFont(font);

            return ret;
        }
        else
        {
            return boundingBox(txt);
        }
    }

    // --- Sharp variants ---

    void moveToSharp(double px, double py)
    {
        moveTo(px, py);
        //Vec2 pt = sharp(PT(px, py));
        //painter->moveTo(pt.x, pt.y);
    }

    void moveToSharp(Vec2 p)
    {
        moveTo(p.x, p.y);
        //Vec2 pt = sharp(PT(p.x, p.y));
        //painter->moveTo(pt.x, pt.y);
    }

    void lineToSharp(double px, double py)
    {
        lineTo(px, py);
        //Vec2 pt = sharp(PT(px, py));
        //painter->lineTo(pt.x, pt.y);
    }

    void lineToSharp(Vec2 p)
    {
        lineTo(p.x, p.y);
        //Vec2 pt = sharp(PT(p.x, p.y));
        //painter->lineTo(pt.x, pt.y);
    }

    void fillTextSharp(const char* txt, const Vec2& pos)
    {
        //painter->setPixelAlignText(QNanoPainter::PixelAlign::PIXEL_ALIGN_FULL);
        fillText(txt, /*sharpX*/(pos.x), /*sharpY*/(pos.y));
    }

    // --- Expose unchanged methods ---

    using SimplePainter::setRenderTarget;
    using SimplePainter::getRenderTarget;
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
    using SimplePainter::setTextAlign;
    using SimplePainter::setTextBaseline;
    //
    using SimplePainter::setStrokeStyle;
    using SimplePainter::setFillStyle;
    using SimplePainter::beginPath;
    using SimplePainter::stroke;
    using SimplePainter::fill;
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