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
    float size = 16.0f;

public:

    static std::shared_ptr<NanoFont> create(const char* virtual_path)
    {
        return std::make_shared<NanoFont>(virtual_path);
    }

    NanoFont(const char* virtual_path)
    {
        DebugPrint("NanoFont() called");
        path = Platform()->path(virtual_path);
    }

    void setSize(double size_pts)
    {
        size = (float)size_pts;
    }
};

class Painter;

// Simple nanovg abstract layer
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

public:

    void setGlobalScale(double _global_scale) {
        global_scale = _global_scale;
    }
    double getGlobalScale() {
        return global_scale; 
    }

    void setRenderTarget(NVGcontext* nvg_ctx) { vg = nvg_ctx; }
    NVGcontext* getRenderTarget() { return vg; }
    std::shared_ptr<NanoFont> getDefaultFont() { return default_font; }

    // --- Transforms ---

    void save() const { nvgSave(vg); }
    void restore() { nvgRestore(vg); }

    void resetTransform()                                    { nvgResetTransform(vg); }
    void transform(const glm::mat3& m)                       { nvgTransform(vg, m[0][0], m[0][1], m[1][0], m[1][1], m[2][0], m[2][1]); }
    glm::mat3 currentTransform() const                       { float x[6]; nvgCurrentTransform(vg, x); return glm::mat3(x[0], x[1], 0, x[2], x[3], 0, x[4], x[5], 1); }

    void translate(double x, double y)                       { nvgTranslate(vg, (float)(x), (float)(y)); }
    void translate(const DVec2& p)                           { nvgTranslate(vg, (float)(p.x), (float)(p.y)); }
    void rotate(double angle)                                { nvgRotate(vg, (float)(angle)); }
    void scale(double scale)                                 { nvgScale(vg, (float)(scale), (float)(scale)); }
    void scale(double scale_x, double scale_y)               { nvgScale(vg, (float)(scale_x), (float)(scale_y)); }
    void skewX(double angle)                                 { nvgSkewX(vg, (float)(angle)); }
    void skewY(double angle)                                 { nvgSkewY(vg, (float)(angle)); }
    void setClipRect(double x, double y, double w, double h) { nvgScissor(vg, (float)(x), (float)(y), (float)(w), (float)(h)); }
    void resetClipping() { nvgResetScissor(vg); }

    // --- Styles ---

    void setStrokeStyle(const Color& color)                  { nvgStrokeColor(vg, nvgRGBA(color.r, color.g, color.b, color.a)); }
    void setFillStyle(const Color& color)                    { nvgFillColor(vg, nvgRGBA(color.r, color.g, color.b, color.a)); }
    void setFillStyle(int r, int g, int b, int a = 255)      { nvgFillColor(vg, nvgRGBA(r, g, b, a)); }
    void setStrokeStyle(int r, int g, int b, int a = 255)    { nvgStrokeColor(vg, nvgRGBA(r, g, b, a)); }
    void setLineWidth(double w)                              { nvgStrokeWidth(vg, (float)(w)); }
    void setLineCap(LineCap cap)                             { nvgLineCap(vg, cap); }
    void setLineJoin(LineJoin join)                          { nvgLineJoin(vg, join); }

    // --- Paths ---

    void beginPath()                                         { nvgBeginPath(vg); }
    void moveTo(double x, double y)                          { nvgMoveTo(vg, (float)(x), (float)(y)); }
    void lineTo(double x, double y)                          { nvgLineTo(vg, (float)(x), (float)(y)); }
    void stroke()                                            { nvgStroke(vg); }
    void fill()                                              { nvgFill(vg); }

    void circle(double x, double y, double r)                { nvgCircle(vg, (float)(x), (float)(y), (float)(r)); }
    void ellipse(double x, double y, double rx, double ry)   { nvgEllipse(vg, (float)(x), (float)(y), (float)(rx), (float)(ry)); }
    void fillRect(double x, double y, double w, double h)    { nvgBeginPath(vg); nvgRect(vg, (float)(x), (float)(y), (float)(w), (float)(h)); nvgFill(vg); }
    void strokeRect(double x, double y, double w, double h)  { nvgBeginPath(vg); nvgRect(vg, (float)(x), (float)(y), (float)(w), (float)(h)); nvgStroke(vg); }
   
    template<typename PointT> void drawPath(const std::vector<PointT>& path) {
        if (path.size() >= 2) {
            moveTo(path[0]); 
            for (size_t i = 1; i < path.size(); i++) 
                lineTo(path[i]); 
        }
    }

    // --- Image ---

    void drawImage(Image& bmp, double x, double y, double w = 0, double h = 0) { 
        bmp.draw(vg, x, y, w <= 0 ? bmp.bmp_width : w, h <= 0 ? bmp.bmp_height : h); 
    }

    // --- Text ---

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


    DRect boundingBox(const std::string& txt) const
    {
        float bounds[4];
        nvgTextBounds(vg, 0, 0, txt.c_str(), txt.c_str() + txt.size(), bounds);
        return DRect((double)(bounds[0]), (double)(bounds[1]), (double)(bounds[2] - bounds[0]), (double)(bounds[3] - bounds[1]));
    }

    void fillText(const char* txt, double x, double y)
    {
        if (!active_font) setFont(default_font);
        nvgText(vg, (float)(x), (float)(y), txt, nullptr);
    }

    void fillText(const char* txt, const DVec2& pos) { fillText(txt, pos.x, pos.y); }
    void fillText(const std::string& txt, double x, double y)
    {
        if (!active_font) setFont(default_font);
        const char* ptr = txt.c_str();
        nvgText(vg, (float)(x), (float)(y), ptr, ptr + txt.size());
    }
};


class Painter : private SimplePainter
{
    double _avgZoom() { return (abs(camera.zoom_x) + abs(camera.zoom_y)) * 0.5; }

public:

    mutable Camera camera;
    glm::mat3 default_viewport_transform;
    double line_width = 1;


    DVec2 PT(double x, double y)
    {
        // todo: Alter point reference instead of returning new point
        if (camera.transform_coordinates)
        {
            DVec2 o = camera.toWorldOffset({ camera.stage_ox, camera.stage_oy });
            return { x + o.x, y + o.y };
        }
        else
        {
            // (x,y) represents stage coordinate, but transform is active
            DVec2 ret = camera.toWorld(x + camera.stage_ox, y + camera.stage_oy);
            return ret;
        }
    }

    // --- Styles ---

    void setLineWidth(double w)
    {
        this->line_width = w;
        if (camera.scale_lines_text)
            SimplePainter::setLineWidth(w);
        else
            SimplePainter::setLineWidth(w / _avgZoom());
    }

    // --- Paths (overrides) ---

    void circle(double cx, double cy, double radius)
    {
        DVec2 pt = PT(cx, cy);
        if (camera.scale_sizes)
            SimplePainter::circle(pt.x, pt.y, radius);
        else
            SimplePainter::circle(pt.x, pt.y, radius / _avgZoom());
    }

    void circle(DVec2 cen, double r) { circle(cen.x, cen.y, r); }
    void ellipse(double x, double y, double rx, double ry)
    {
        DVec2 pt = PT(x, y);
        if (camera.scale_sizes)
            SimplePainter::ellipse(pt.x, pt.y, rx, ry);
        else
            SimplePainter::ellipse(pt.x, pt.y, rx * camera.zoom_x, ry * camera.zoom_y);
    }

    void moveTo(double px, double py) { DVec2 pt = PT(px, py);   SimplePainter::moveTo(pt.x, pt.y); }
    void lineTo(double px, double py) { DVec2 pt = PT(px, py);   SimplePainter::lineTo(pt.x, pt.y); }
    void moveTo(DVec2 p)              { DVec2 pt = PT(p.x, p.y); SimplePainter::moveTo(pt.x, pt.y); }
    void lineTo(DVec2 p)              { DVec2 pt = PT(p.x, p.y); SimplePainter::lineTo(pt.x, pt.y); }

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
                SimplePainter::strokeRect(x, y, w, h);
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

    void strokeRect(const FRect& r) { strokeRect(r.x1, r.y1, r.x2 - r.x1, r.y2 - r.y1); }
    void fillRect(const FRect& r) { fillRect(r.x1, r.y1, r.x2 - r.x1, r.y2 - r.y1); }

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
        DVec2 pt = PT(cx, cy);
        SimplePainter::beginPath();
        if (camera.scale_sizes)
            SimplePainter::ellipse(cx, cy, r, r);
        else
            SimplePainter::ellipse(cx, cy, r / camera.zoom_x, r / camera.zoom_y);
        SimplePainter::fill();
    }

    void fillEllipse(double cx, double cy, double rx, double ry)
    {
        SimplePainter::beginPath();
        if (camera.scale_sizes)
            SimplePainter::ellipse(cx, cy, rx, ry);
        else
            SimplePainter::ellipse(cx, cy, rx / camera.zoom_x, ry / camera.zoom_y);
        SimplePainter::fill();
    }

    void drawArrow(DVec2 a, DVec2 b, Color color)
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
        fill();
    }

    double arrow_x0 = 0, arrow_y0 = 0;
    void arrowMoveTo(double px, double py) { arrow_x0 = px; arrow_y0 = py; }
    void arrowDrawTo(double px, double py, Color color = Color(255, 255, 255)) { drawArrow({ arrow_x0, arrow_y0 }, { px, py }, color); }

    // --- Image ---

    void drawImage(Image& bmp, double x, double y, double w = 0, double h = 0) {
        SimplePainter::drawImage(bmp, x, y, w <= 0 ? bmp.bmp_width : w, h <= 0 ? bmp.bmp_height : h);
    }
    void drawImage(CanvasImage& bmp)
    {
        camera.saveCameraTransform();
        if (bmp.coordinate_type == CoordinateType::STAGE)
            camera.stageTransform();
        else
            camera.worldTransform();

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

    void fillText(const std::string& txt, const DVec2& p) {
        fillText(txt, p.x, p.y);
    }

    DRect boundingBox(const std::string& txt)
    {
        save();
        resetTransform();
        transform(default_viewport_transform);
        auto r = SimplePainter::boundingBox(txt);
        restore();
        return r;
    }

    std::string format_number(double v) {
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

    void fillNumberScientific(double v, DVec2 pos, float fontSize = 12)
    {
        std::string txt = format_number(v);

        double scale = 1.0;// fontSize;// ScaleSize(1.0);
        double exponent_spacing = 3.0 * scale;

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

            double mantissaWidth = boundingBox(mantissa_txt).x2 + (1.0 * scale);

            /// todo: Take whatever alignment you're given and adjust right bound
            setTextAlign(TextAlign::ALIGN_CENTER);
            setFontSize(fontSize);
            fillTextSharp(mantissa_txt.c_str(), pos);

            pos.x += mantissaWidth/2.0f + exponent_spacing;
            pos.y -= (int)(fontSize * 0.7 + (1.0 * scale));

            //font.setPixelSize((int)(fontSize * 0.85));
            //painter->setFont(font);
            setTextAlign(TextAlign::ALIGN_LEFT);
            setFontSize(fontSize * 0.85);

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

    DRect boundingBoxNumberScientific(double v, float fontSize = 12)
    {
        std::string txt = format_number(v);

        double scale = 1.0;// fontSize;// ScaleSize(1.0);
        double exponent_spacing = 3.0 * scale;

        size_t ePos = txt.find("e");
        if (ePos != std::string::npos)
        {
            const int  exponent = std::stoi(txt.substr(ePos + 1));
            std::string mantissa_txt = txt.substr(0, ePos) + "x10";
            std::string exponent_txt = std::to_string(exponent);

            //font.setPixelSize(fontSize);
            //painter->setFont(font);

            DRect mantissaRect = boundingBox(mantissa_txt);

            //font.setPixelSize((int)(fontSize * 0.85));
            //painter->setFont(font);

            DRect exponentRect = boundingBox(exponent_txt);
            DRect ret = mantissaRect;

            ret.y1 -= (int)(fontSize * 0.7 + (1.0 * scale));
            ret.x2 += exponentRect.width() + exponent_spacing;

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
        //DVec2 pt = sharp(PT(px, py));
        //painter->moveTo(pt.x, pt.y);
    }

    void moveToSharp(DVec2 p)
    {
        moveTo(p.x, p.y);
        //DVec2 pt = sharp(PT(p.x, p.y));
        //painter->moveTo(pt.x, pt.y);
    }

    void lineToSharp(double px, double py)
    {
        lineTo(px, py);
        //DVec2 pt = sharp(PT(px, py));
        //painter->lineTo(pt.x, pt.y);
    }

    void lineToSharp(DVec2 p)
    {
        lineTo(p.x, p.y);
        //DVec2 pt = sharp(PT(p.x, p.y));
        //painter->lineTo(pt.x, pt.y);
    }

    void fillTextSharp(const char* txt, const DVec2& pos)
    {
        //painter->setPixelAlignText(QNanoPainter::PixelAlign::PIXEL_ALIGN_FULL);
        fillText(txt, /*sharpX*/(pos.x), /*sharpY*/(pos.y));
    }

    // --- Expose unchanged methods ---

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
    //int logicalWidth() { return (int)(fbo_width/global_scale); }   // Logical pixel width
    //int logicalHeight() { return (int)(fbo_height/global_scale); } // Logical pixel height
    int fboWidth() { return fbo_width; }
    int fboHeight() { return fbo_height; }
};