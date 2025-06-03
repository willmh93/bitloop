#pragma once
#include "types.h"
#include "debug.h"

class Viewport;
class Event;

struct FingerInfo
{
    Viewport* ctx_owner = nullptr;
    int64_t fingerId;
    double x;
    double y;
};

class Camera
{
public:

    Viewport* viewport = nullptr;

    bool panning_enabled = true;
    bool zooming_enabled = true;

    finite_double x = 0;
    finite_double y = 0;
    finite_double zoom_x = 1;
    finite_double zoom_y = 1;
    finite_double rotation = 0;

    double targ_zoom_x = 1;
    double targ_zoom_y = 1;
    double reference_zoom_x = 1;
    double reference_zoom_y = 1;
    double min_zoom = -1e+300; // Relative to "focused" rect
    double max_zoom = 1e+300; // Relative to "focused" rect

    double pan_x = 0;
    double pan_y = 0;
    double targ_pan_x = 0;
    double targ_pan_y = 0;

    bool transform_coordinates = true;
    bool scale_lines_text = true;
    bool scale_sizes = true;
    bool rotate_text = true;

    double viewport_w = 0;
    double viewport_h = 0;

    double focal_anchor_x = 0.0;
    double focal_anchor_y = 0.0;

    double pan_mult = 1.0;

    int    pan_down_touch_x      = 0;
    int    pan_down_touch_y      = 0;
    double pan_down_touch_dist   = 0;
    double pan_down_touch_angle  = 0;
                                 
    double pan_beg_cam_x         = 0;
    double pan_beg_cam_y         = 0;
    double pan_beg_cam_zoom_x    = 0;
    double pan_beg_cam_zoom_y    = 0;
    double pan_beg_cam_angle     = 0;

    bool panning = false;
    bool use_panning_offset = true;

    bool saved_transform_coordinates = transform_coordinates;
    bool saved_scale_lines_text = scale_lines_text;
    bool saved_scale_sizes = scale_sizes;
    bool saved_rotate_text = rotate_text;

    void setTransformFilters(
        bool _transform_coordinates,
        bool _scale_line_txt, 
        bool _scale_sizes,
        bool _rotate_text);

    void setTransformFilters(bool all);

    void worldTransform();
    void stageTransform();
    void labelTransform();

    void worldCoordinates(bool b) { transform_coordinates = b; }
    void scalingLines(bool b) { scale_lines_text = b; }
    void scalingSizes(bool b) { scale_sizes = b; }

    void saveCameraTransform();
    void restoreCameraTransform();

    double stage_ox = 0;
    double stage_oy = 0;
    void setStageOffset(double ox, double oy);

    void textTransform(bool world_position, bool scale_size, bool rotate);

    void setOriginViewportAnchor(double ax, double ay);
    void setOriginViewportAnchor(Anchor anchor);

    void setStagePanX(int px);
    void setStagePanY(int py);
    [[nodiscard]] double getStagePanX();
    [[nodiscard]] double getStagePanY();

    void setCameraPos(double _x, double _y);
    void setPan(double pan_x, double pan_y, bool immediate=true);
    void setZoom(double zoom, bool immediate= true);
    void setZoomX(double zoom_x, bool immediate= true);
    void setZoomY(double zoom_y, bool immediate= true);

    [[nodiscard]] DVec2 getRelativeZoomFactor() {
        return { 
            zoom_x / reference_zoom_x, 
            zoom_y / reference_zoom_x 
        };
    }
    void setRelativeZoomRange(double min, double max);
    void setRelativeZoomFactor(double relative_zoom)
    {
        zoom_x = reference_zoom_x * relative_zoom;
        zoom_y = reference_zoom_y * relative_zoom;
    }
    void setRelativeZoomFactorX(double relative_zoom_x)
    {
        zoom_x = reference_zoom_x * relative_zoom_x;
    }
    void setRelativeZoomFactorY(double relative_zoom_y)
    {
        zoom_y = reference_zoom_y * relative_zoom_y;
    }


    void cameraToViewport(double left, double top, double right, double bottom);
    void focusWorldRect(double left, double top, double right, double bottom, bool stretch=false);
    void focusWorldRect(const DRect &r, bool stretch = false);

    void originToCenterViewport();

    [[nodiscard]] DVec2 toStageOffset(const DVec2& world_offset)
    {
        if (this->transform_coordinates)
        {
            return world_offset;
        }
        else
        {
            double cos_r = cos(rotation);
            double sin_r = sin(rotation);

            return {
                (world_offset.x * cos_r - world_offset.y * sin_r) * zoom_x,
                (world_offset.y * cos_r + world_offset.x * sin_r) * zoom_y
            };
        }
    }
    [[nodiscard]] DVec2 toWorldOffset(const DVec2& stage_offset)
    {
        if (this->transform_coordinates)
        {
            double cos_r = cos(rotation);
            double sin_r = sin(rotation);

            return {
                (stage_offset.x * cos_r + stage_offset.y * sin_r) / zoom_x,
                (stage_offset.y * cos_r - stage_offset.x * sin_r) / zoom_y
            };
        }
        else
        {
            return stage_offset;
        }
    }

    [[nodiscard]] DVec2 toStageOffset(double world_ox, double world_oy)
    {
        return toStageOffset({ world_ox, world_oy });
    }
    [[nodiscard]] DVec2 toWorldOffset(double stage_ox, double stage_oy)
    {
        return toWorldOffset({ stage_ox, stage_oy });
    }

    [[nodiscard]] DVec2 originPixelOffset();
    [[nodiscard]] DVec2 originWorldOffset();
    [[nodiscard]] DVec2 panPixelOffset();

    [[nodiscard]] DVec2 toWorld(const DVec2& pt);
    [[nodiscard]] DVec2 toWorld(double x, double y);
    [[nodiscard]] DRect toWorldRect(const DRect& r);
    [[nodiscard]] DRect toWorldRect(double x1, double y1, double x2, double y2);

    [[nodiscard]] DQuad toWorldQuad(const DVec2 &a, const DVec2& b, const DVec2& c, const DVec2& d);
    [[nodiscard]] DQuad toWorldQuad(const DQuad& quad);
    [[nodiscard]] DQuad toWorldQuad(const DRect& quad);
    [[nodiscard]] DQuad toWorldQuad(double x1, double y1, double x2, double y2);

    [[nodiscard]] DVec2 toStage(const DVec2& pt);
    [[nodiscard]] DVec2 toStage(double x, double y);
    [[nodiscard]] DVec2 toStageSize(const DVec2& size);
    [[nodiscard]] DVec2 toStageSize(double w, double h);
    [[nodiscard]] DRect toStageRect(double x0, double y0, double x1, double y1);
    [[nodiscard]] DRect toStageRect(const DVec2& pt1, const DVec2& pt2);
    [[nodiscard]] DQuad toStageQuad(const DVec2& a, const DVec2& b, const DVec2& c, const DVec2& d);

    void setPanningUsesOffset(bool b) { use_panning_offset = b; }
    void panBegin(int _x, int _y, double touch_dist, double touch_angle);
    void panDrag(int _x, int _y, double touch_dist, double touch_angle);
    void panEnd(int _x, int _y);
    void panZoomProcess();

    std::vector<FingerInfo> pressed_fingers;

    double touchAngle()
    {
        if (pressed_fingers.size() >= 2)
        {
            return atan2(
                pressed_fingers[1].y - pressed_fingers[0].y,
                pressed_fingers[1].x - pressed_fingers[0].x
            );
        }
        return 0.0;
    }
    double touchDist()
    {
        if (pressed_fingers.size() >= 2)
        {
            double dx = pressed_fingers[1].x - pressed_fingers[0].x;
            double dy = pressed_fingers[1].y - pressed_fingers[0].y;
            return sqrt(dx*dx + dy*dy);
        }
        return 0.0;
    }


    void handleWorldNavigation(Event e, bool single_touch_pan);
};

inline void Camera::setStageOffset(double ox, double oy)
{
    stage_ox = ox;
    stage_oy = oy;
}

inline void Camera::setStagePanX(int px)
{
    pan_x = px / zoom_x;
}

inline void Camera::setStagePanY(int py)
{
    pan_y = py / zoom_x;
}

inline double Camera::getStagePanX()
{
    return pan_x * zoom_x;
}

inline double Camera::getStagePanY()
{
    return pan_x * zoom_x;
}

inline void Camera::setCameraPos(double _x, double _y)
{
    x = _x;
    y = _y;
}

inline void Camera::setPan(double pan_x, double pan_y, bool immediate)
{
    targ_pan_x = pan_x;
    targ_pan_y = pan_y;
    if (immediate)
    {
        pan_x = pan_x;
        pan_y = pan_y;
    }
}

inline void Camera::setZoom(double zoom, bool immediate)
{
    targ_zoom_x = zoom;
    targ_zoom_y = zoom;
    if (immediate)
    {
        zoom_x = zoom;
        zoom_y = zoom;
    }
}

inline void Camera::setZoomX(double _zoom_x, bool immediate)
{
    targ_zoom_x = _zoom_x;
    if (immediate)
        zoom_x = _zoom_x;
}

inline void Camera::setZoomY(double _zoom_y, bool immediate)
{
    targ_zoom_y = _zoom_y;
    if (immediate)
        zoom_y = _zoom_y;
}

inline void Camera::focusWorldRect(const DRect& r, bool stretch)
{
    focusWorldRect(r.x1, r.y1, r.x2, r.y2, stretch);
}

inline DVec2 Camera::originWorldOffset()
{
    return originPixelOffset() / DVec2(zoom_x, zoom_y);
}

inline DVec2 Camera::panPixelOffset()
{
    return DVec2(
        pan_x * zoom_x,
        pan_y * zoom_y
    );
}

inline DVec2 Camera::toWorld(const DVec2& pt)
{
    // World coordinate
    double px = pt.x;
    double py = pt.y;

    double cos_r = cos(rotation);
    double sin_r = sin(rotation);

    DVec2 _originPixelOffset = originPixelOffset();
    DVec2 _panPixelOffset = panPixelOffset();

    double origin_ox = _originPixelOffset.x;
    double origin_oy = _originPixelOffset.y;

    px -= origin_ox;
    py -= origin_oy;

    px -= _panPixelOffset.x;
    py -= _panPixelOffset.y;

    double world_x = (px * cos_r + py * sin_r);
    double world_y = (py * cos_r - px * sin_r);

    world_x -= -this->x * zoom_x;
    world_y -= -this->y * zoom_y;

    world_x /= zoom_x;
    world_y /= zoom_y;

    return { world_x, world_y };
}

inline DVec2 Camera::toWorld(double x, double y)
{
    return toWorld({ x, y });
}


inline DRect Camera::toWorldRect(const DRect& r)
{
    DVec2 tl = toWorld(r.x1, r.y1);
    DVec2 br = toWorld(r.x2, r.y2);
    return { tl.x, tl.y, br.x, br.y };
}

inline DRect Camera::toWorldRect(double x1, double y1, double x2, double y2)
{
    DVec2 tl = toWorld(x1, y1);
    DVec2 br = toWorld(x2, y2);
    return { tl.x, tl.y, br.x, br.y };
}


inline DQuad Camera::toWorldQuad(const DVec2& a, const DVec2& b, const DVec2& c, const DVec2& d)
{
    DVec2 qA = toWorld(a);
    DVec2 qB = toWorld(b);
    DVec2 qC = toWorld(c);
    DVec2 qD = toWorld(d);
    return { qA, qB, qC, qD };
}

inline DQuad Camera::toWorldQuad(const DQuad& quad)
{
    return toWorldQuad(quad.a, quad.b, quad.c, quad.d);
}

inline DQuad Camera::toWorldQuad(double x1, double y1, double x2, double y2)
{
    return toWorldQuad(
        Vec2(x1, y1),
        Vec2(x2, y1),
        Vec2(x2, y2),
        Vec2(x1, y2)
    );
}

inline DQuad Camera::toWorldQuad(const DRect& r)
{
    return toWorldQuad(
        Vec2(r.x1, r.y1),
        Vec2(r.x2, r.y1),
        Vec2(r.x2, r.y2),
        Vec2(r.x1, r.y2)
    );
}


inline DVec2 Camera::toStage(const DVec2& pt)
{
    // World coordinate
    double px = pt.x;
    double py = pt.y;

    double cos_r = cos(rotation);
    double sin_r = sin(rotation);

    DVec2 _originPixelOffset = originPixelOffset();
    DVec2 _panPixelOffset = panPixelOffset();

    double origin_ox = _originPixelOffset.x;
    double origin_oy = _originPixelOffset.y;

    /// Do transform

    px *= zoom_x;
    py *= zoom_y;

    px += -this->x * zoom_x;
    py += -this->y * zoom_y;

    double ret_x = (px * cos_r - py * sin_r);
    double ret_y = (py * cos_r + px * sin_r);

    ret_x += _panPixelOffset.x;
    ret_y += _panPixelOffset.y;

    ret_x += origin_ox;
    ret_y += origin_oy;

    return { ret_x, ret_y };
}

inline DVec2 Camera::toStage(double x, double y)
{
    return toStage({ x, y });
}

inline DVec2 Camera::toStageSize(const DVec2& size)
{
    return { size.x * zoom_x, size.y * zoom_y };
}

inline DVec2 Camera::toStageSize(double w, double h)
{
    return { w * zoom_x, h * zoom_y };
}

inline DRect Camera::toStageRect(double x0, double y0, double x1, double y1)
{
    DVec2 p1 = toStage({ x0, y0 });
    DVec2 p2 = toStage({ x1, y1 });
    return { p1.x, p1.y, p2.x, p2.y };
}

inline DRect Camera::toStageRect(const DVec2& pt1, const DVec2& pt2)
{
    return toStageRect(pt1.x, pt1.y, pt2.x, pt2.y);
}

inline DQuad Camera::toStageQuad(const DVec2& a, const DVec2& b, const DVec2& c, const DVec2& d)
{
    DVec2 qA = toStage(a);
    DVec2 qB = toStage(b);
    DVec2 qC = toStage(c);
    DVec2 qD = toStage(d);
    return { qA, qB, qC, qD };
}