#pragma once
#include "types.h"
#include "debug.h"
#include "math_helpers.h"

inline glm::dmat3 glm_dtranslate(double tx, double ty) {
    glm::dmat3 m(1.0);
    m[2][0] = tx; // column 2, row 0
    m[2][1] = ty; // column 2, row 1
    return m;
}

inline glm::dmat3 glm_dscale(double sx, double sy) {
    glm::dmat3 m(1.0);
    m[0][0] = sx;
    m[1][1] = sy;
    return m;
}

inline glm::dmat3 glm_drotate(double r) {
    double c = std::cos(r);
    double s = std::sin(r);
    return glm::dmat3(
        c, s, 0.0,    // column 0
        -s, c, 0.0,   // column 1
        0.0, 0.0, 1.0 // column 2 (homogeneous)
    );
}


class Viewport;
class ProjectBase;
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
    friend class Painter;
    friend class ProjectBase;
    friend class Viewport;

    double cam_x = 0;
    double cam_y = 0;
    double cam_rotation = 0;
    double zoom_x = 1;
    double zoom_y = 1;
    double pan_x = 0;
    double pan_y = 0;

    double focal_anchor_x = 0.0;
    double focal_anchor_y = 0.0;

    // ======== Zoom/focus attributes ========
    double ref_zoom_x = 1;
    double ref_zoom_y = 1;
    double min_zoom = -1e+300; // Relative to "focused" rect
    double max_zoom = 1e+300; // Relative to "focused" rect

    // ======== Pan attributes ========
    int    pan_down_touch_x = 0;
    int    pan_down_touch_y = 0;
    double pan_down_touch_dist = 0;
    double pan_down_touch_angle = 0;

    double pan_beg_cam_x = 0;
    double pan_beg_cam_y = 0;
    double pan_beg_cam_zoom_x = 0;
    double pan_beg_cam_zoom_y = 0;
    double pan_beg_cam_angle = 0;

    bool panning = false;
    bool direct_cam_panning = true;

protected:

    Viewport* viewport = nullptr;
    double viewport_w = 0;
    double viewport_h = 0;

    bool transform_coordinates = true;
    bool scale_lines_text = true;
    bool scale_sizes = true;
    bool rotate_text = true;

private:

    bool saved_transform_coordinates = transform_coordinates;
    bool saved_scale_lines_text = scale_lines_text;
    bool saved_scale_sizes = scale_sizes;
    bool saved_rotate_text = rotate_text;

    std::vector<FingerInfo> fingers;

public:

    static inline Camera* active = nullptr;

public:

    //void setTransformFilters(
    //    bool _transform_coordinates,
    //    bool _scale_line_txt, 
    //    bool _scale_sizes,
    //    bool _rotate_text);
    //
    //void setTransformFilters(bool all);

    void worldTransform();
    void stageTransform();
    void worldHudTransform();

    void worldCoordinates(bool b) { transform_coordinates = b; }
    void scalingLines(bool b) { scale_lines_text = b; }
    void scalingSizes(bool b) { scale_sizes = b; }
    void rotatingText(bool b) { rotate_text = b; }

    void saveCameraTransform();
    void restoreCameraTransform();

    //double stage_ox = 0;
    //double stage_oy = 0;
    //void setStageOffset(double ox, double oy);

    void setOriginViewportAnchor(double ax, double ay);
    void setOriginViewportAnchor(Anchor anchor);

    double viewportWidth() { return viewport_w; }
    double viewportHeight() { return viewport_h; }

    void setX(double _x)              { cam_x = _x;              updateCameraMatrix(); }
    void setY(double _y)              { cam_y = _y;              updateCameraMatrix(); }
    void setPos(double _x, double _y) { cam_x = _x; cam_y = _y;  updateCameraMatrix(); }
    void setPan(double px, double py) { pan_x = px; pan_y = py;  updateCameraMatrix(); }
    void setZoom(double zoom)         { zoom_x = zoom_y = zoom;  updateCameraMatrix(); }
    void setZoomX(double _zoom_x)     { zoom_x = _zoom_x;        updateCameraMatrix(); }
    void setZoomY(double _zoom_y)     { zoom_y = _zoom_y;        updateCameraMatrix(); }
    void setRotation(double angle)    { cam_rotation = angle;    updateCameraMatrix(); }

    [[nodiscard]] double x()        { return cam_x; }
    [[nodiscard]] double y()        { return cam_y; }
    [[nodiscard]] DVec2  pos()      { return {cam_x, cam_y}; }
    [[nodiscard]] double panX()     { return pan_x; }
    [[nodiscard]] double panY()     { return pan_y; }
    [[nodiscard]] double zoomX()    { return zoom_x; }
    [[nodiscard]] double zoomY()    { return zoom_y; }
    [[nodiscard]] double rotation() { return cam_rotation; }

    // ======== World rect focusing ========
    void focusWorldRect(double left, double top, double right, double bottom, bool stretch=false);
    void focusWorldRect(const DRect &r, bool stretch = false) { focusWorldRect(r.x1, r.y1, r.x2, r.y2, stretch); }
  //void focusWorldRect();  // todo: Use current view as the focus rect

    /// Once world rect focused, use it as a "reference" for scaling relative to that reference zoom, e.g.
    /// > setRelativeZoom(2)  will zoom 2x relative to that focused rect

    [[nodiscard]] DVec2 getReferenceZoom() { return { ref_zoom_x, ref_zoom_y }; }               // Cached zoom set by focusWorldRect()
    [[nodiscard]] DVec2 getRelativeZoom()  { return { zoom_x/ref_zoom_x, zoom_y/ref_zoom_y }; } // Current relative scale factor

    // Set zoom (relative to the reference)
    void setRelativeZoom(double relative_zoom)
    {
        zoom_x = ref_zoom_x * relative_zoom;
        zoom_y = ref_zoom_y * relative_zoom;
        updateCameraMatrix();
    }
    void setRelativeZoomX(double relative_zoom_x) {
        zoom_x = ref_zoom_x * relative_zoom_x;
        updateCameraMatrix();
    }
    void setRelativeZoomY(double relative_zoom_y) {
        zoom_y = ref_zoom_y * relative_zoom_y;
        updateCameraMatrix();
    }

    // Clamp the relative zoom range (relative to the reference)
    void restrictRelativeZoomRange(double min, double max);

    DVec2 getViewportFocusedWorldSize();

    // ======== Camera controls ========

    void cameraToViewport(double left, double top, double right, double bottom);

    void originToCenterViewport();

    [[nodiscard]] DVec2 worldToStageOffset(const DVec2& o)
    {
        //double cos_r = cos(cam_rotation);
        //double sin_r = sin(cam_rotation);
        return {
            (o.x * cos_r - o.y * sin_r) * zoom_x,
            (o.y * cos_r + o.x * sin_r) * zoom_y
        };
    }
    [[nodiscard]] DVec2 stageToWorldOffset(const DVec2& stage_offset)
    {
        //double cos_r = cos(cam_rotation);
        //double sin_r = sin(cam_rotation);

        return {
            (stage_offset.x * cos_r + stage_offset.y * sin_r) / zoom_x,
            (stage_offset.y * cos_r - stage_offset.x * sin_r) / zoom_y
        };
    }

    [[nodiscard]] DVec2 worldAxisDirStage(bool isX) const
    {
        double c = std::cos(cam_rotation);
        double s = std::sin(cam_rotation);
        return isX ? DVec2{c, s} : DVec2{-s, c};
    }
    [[nodiscard]] DVec2 worldAxisPerpStage(bool isX) const
    {
        DVec2 d = worldAxisDirStage(isX).normalized();
        return {-d.y, d.x};
    }

    [[nodiscard]] DVec2 worldToStageOffset(double ox, double oy) { return worldToStageOffset({ ox, oy }); }
    [[nodiscard]] DVec2 stageToWorldOffset(double ox, double oy) { return stageToWorldOffset({ ox, oy }); }

    [[nodiscard]] DVec2 originPixelOffset();
    [[nodiscard]] DVec2 originWorldOffset();
    [[nodiscard]] DVec2 panPixelOffset();

    // world --> stage matrix
    glm::dmat3 m     = glm::dmat3(1.0);
    glm::dmat3 inv_m = glm::dmat3(1.0);
    double cos_r = 1.0, sin_r = 0.0;

    void dResetTransform()                { m = glm::dmat3(1.0); }
    void dTranslate(double tx, double ty) { m = m * glm_dtranslate(tx, ty); }
    void dRotate(double r)                { m = m * glm_drotate(r); }
    void dScale(double sx, double sy)     { m = m * glm_dscale(sx, sy); }

    void updateCameraMatrix()
    {
        DVec2 origin_offset = originPixelOffset();
        
        dResetTransform();
        dTranslate(origin_offset.x + pan_x, origin_offset.y + pan_y);
        dRotate(cam_rotation);
        dTranslate(-cam_x * zoom_x, -cam_y * zoom_y);
        dScale(zoom_x, zoom_y);

        cos_r =  m[0][0] / zoom_x;
        sin_r = -m[1][0] / zoom_x;
        inv_m = glm::inverse(m);
    }

    [[nodiscard]] DVec2 toStage(double sx, double sy) { return static_cast<DVec2>(m * glm::dvec3(sx, sy, 1)); }
    [[nodiscard]] DVec2 toWorld(double wx, double wy) { return static_cast<DVec2>(inv_m * glm::dvec3(wx, wy, 1.0)); }
    [[nodiscard]] DVec2 toWorld(DVec2 p) { return static_cast<DVec2>(m * glm::dvec3(p.x, p.y, 1)); }
    [[nodiscard]] DVec2 toStage(DVec2 p) { return static_cast<DVec2>(inv_m * glm::dvec3(p.x, p.y, 1.0)); }

    [[nodiscard]] DRect toWorldRect(const DRect& r);
    [[nodiscard]] DRect toWorldRect(double x1, double y1, double x2, double y2);

    [[nodiscard]] DQuad toWorldQuad(const DVec2 &a, const DVec2& b, const DVec2& c, const DVec2& d);
    [[nodiscard]] DQuad toWorldQuad(const DQuad& quad);
    [[nodiscard]] DQuad toWorldQuad(const DRect& quad);
    [[nodiscard]] DQuad toWorldQuad(double x1, double y1, double x2, double y2);

    [[nodiscard]] DVec2 toStageSize(const DVec2& size);
    [[nodiscard]] DVec2 toStageSize(double w, double h);
    [[nodiscard]] DRect toStageRect(double x0, double y0, double x1, double y1);
    [[nodiscard]] DRect toStageRect(const DVec2& pt1, const DVec2& pt2);
    [[nodiscard]] DQuad toStageQuad(const DVec2& a, const DVec2& b, const DVec2& c, const DVec2& d);

    //[[nodiscard]] DAngledRect toWorldRect(const DAngledRect& r);
    //[[nodiscard]] DAngledRect toStageRect(const DAngledRect& r);

    void setDirectCameraPanning(bool b) { direct_cam_panning = b; }
    void panBegin(int _x, int _y, double touch_dist, double touch_angle);
    void panDrag(int _x, int _y, double touch_dist, double touch_angle);
    void panEnd();
    void panZoomProcess();

    std::vector<FingerInfo> pressedFingers() {
        return fingers;
    }

    double touchAngle()
    {
        if (fingers.size() >= 2)
            return atan2(fingers[1].y - fingers[0].y, fingers[1].x - fingers[0].x);
        return 0.0;
    }
    double touchDist()
    {
        if (fingers.size() >= 2)
        {
            double dx = fingers[1].x - fingers[0].x;
            double dy = fingers[1].y - fingers[0].y;
            return sqrt(dx*dx + dy*dy);
        }
        return 0.0;
    }

    double panDownTouchDist()  { return pan_down_touch_dist; }
    double panDownTouchAngle() { return pan_down_touch_angle; }
    double panDownTouchX()     { return pan_down_touch_x; }
    double panDownTouchY()     { return pan_down_touch_y; }
    double panBegCamX()        { return pan_beg_cam_x; }
    double panBegCamY()        { return pan_beg_cam_y; }
    double panBegCamZoomX()    { return pan_beg_cam_zoom_x; }
    double panBegCamZoomY()    { return pan_beg_cam_zoom_x; }
    double panBegCamAngle()    { return pan_beg_cam_angle; }

    void handleWorldNavigation(Event e, bool single_touch_pan);
};

//inline void Camera::setStageOffset(double ox, double oy)
//{
//    stage_ox = ox;
//    stage_oy = oy;
//}


inline DVec2 Camera::originWorldOffset()
{
    // zoom_x/zoom_y refer to world zoom, not stage zoom? bug if camera rotated?
    return originPixelOffset() / DVec2(zoom_x, zoom_y);
}

inline DVec2 Camera::panPixelOffset()
{
    return DVec2(pan_x, pan_y);
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

//inline DAngledRect Camera::toWorldRect(const DAngledRect& r)
//{
//    DAngledRect ret;
//    ret.cen = toWorld(r.cen);
//    ret.size = toWorldOffset(r.size);
//    ret.angle = r.angle; // todo: toWorldAngle
//    return ret;
//}

inline DQuad Camera::toWorldQuad(const DVec2& a, const DVec2& b, const DVec2& c, const DVec2& d)
{
    DVec2 qA = toWorld(a);
    DVec2 qB = toWorld(b);
    DVec2 qC = toWorld(c);
    DVec2 qD = toWorld(d);
    return DQuad(qA, qB, qC, qD);
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

//inline DAngledRect Camera::toStageRect(const DAngledRect& r)
//{
//    DAngledRect ret;
//    ret.cen = toStage(r.cen);
//    ret.size = toStageOffset(r.size);
//    ret.angle = r.angle; // todo: toStageAngle
//    return ret;
//}

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