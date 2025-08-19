#pragma once
#include "types.h"
#include "debug.h"
#include "bitloop/utility/math_helpers.h"

BL_BEGIN_NS

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

inline glm::ddmat3 glm_dtranslate(flt128 tx, flt128 ty) {
    glm::ddmat3 m(1.0);
    m[2][0] = tx; // column 2, row 0
    m[2][1] = ty; // column 2, row 1
    return m;
}

inline glm::ddmat3 glm_dscale(flt128 sx, flt128 sy) {
    glm::ddmat3 m(1.0);
    m[0][0] = sx;
    m[1][1] = sy;
    return m;
}

inline glm::ddmat3 glm_drotate(flt128 r) {
    flt128 c = cos(r);
    flt128 s = sin(r);
    return glm::ddmat3(
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

    flt128 cam_x = 0;
    flt128 cam_y = 0;
    double cam_rotation = 0;
    flt128 zoom_x = 1;
    flt128 zoom_y = 1;
    double pan_x = 0;
    double pan_y = 0;

    double focal_anchor_x = 0.0;
    double focal_anchor_y = 0.0;

    // ======== Zoom/focus attributes ========
    flt128 ref_zoom_x = 1;
    flt128 ref_zoom_y = 1;
    flt128 min_zoom = -1e+300; // Relative to "focused" rect
    flt128 max_zoom = 1e+300; // Relative to "focused" rect

    // ======== Pan attributes ========
    int    pan_down_touch_x = 0;
    int    pan_down_touch_y = 0;
    double pan_down_touch_dist = 0;
    double pan_down_touch_angle = 0;

    flt128 pan_beg_cam_x = 0;
    flt128 pan_beg_cam_y = 0;
    flt128 pan_beg_cam_zoom_x = 0;
    flt128 pan_beg_cam_zoom_y = 0;
    double pan_beg_cam_angle = 0;

    bool panning = false;
    bool direct_cam_panning = true;

protected:

    Viewport* viewport = nullptr;
    double viewport_w = 0;
    double viewport_h = 0;

    bool transform_coordinates = true;
    bool scale_lines = true;
    bool scale_sizes = true;
    bool rotate_text = true;

private:

    bool saved_transform_coordinates = transform_coordinates;
    bool saved_scale_lines = scale_lines;
    bool saved_scale_sizes = scale_sizes;
    bool saved_rotate_text = rotate_text;

    std::vector<FingerInfo> fingers;

public:

    static inline Camera* active = nullptr;

public:

    void worldTransform();
    void stageTransform();
    void worldHudTransform();

    void worldCoordinates(bool b) { transform_coordinates = b; }
    void scalingLines(bool b) { scale_lines = b; }
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

    void setX(flt128 _x)               { cam_x = _x;                updateCameraMatrix(); }
    void setY(flt128 _y)               { cam_y = _y;                updateCameraMatrix(); }
    void setPos(flt128 _x, flt128 _y)  { cam_x = _x; cam_y = _y;    updateCameraMatrix(); }
    void setZoom(flt128 zoom)          { zoom_x = zoom_y = zoom;    updateCameraMatrix(); }
    void setZoom(flt128 zx, flt128 zy) { zoom_x = zx; zoom_y = zy;  updateCameraMatrix(); }
    void setZoomX(flt128 _zoom_x)      { zoom_x = _zoom_x;          updateCameraMatrix(); }
    void setZoomY(flt128 _zoom_y)      { zoom_y = _zoom_y;          updateCameraMatrix(); }

    void setX(double _x)               { cam_x = _x;                updateCameraMatrix(); }
    void setY(double _y)               { cam_y = _y;                updateCameraMatrix(); }
    void setPos(double _x, double _y)  { cam_x = _x; cam_y = _y;    updateCameraMatrix(); }
    void setZoom(double zoom)          { zoom_x = zoom_y = zoom;    updateCameraMatrix(); }
    void setZoom(double zx, double zy) { zoom_x = zx; zoom_y = zy;  updateCameraMatrix(); }
    void setZoomX(double _zoom_x)      { zoom_x = _zoom_x;          updateCameraMatrix(); }
    void setZoomY(double _zoom_y)      { zoom_y = _zoom_y;          updateCameraMatrix(); }

    void setPan(double px, double py) { pan_x = px; pan_y = py;     updateCameraMatrix(); }
    void setRotation(double angle)    { cam_rotation = angle;       updateCameraMatrix(); }

    [[nodiscard]] double x()          { return (double)cam_x; }
    [[nodiscard]] double y()          { return (double)cam_y; }
    [[nodiscard]] DVec2  pos()        { return {(double)cam_x, (double)cam_y }; }
    [[nodiscard]] double panX()       { return (double)pan_x; }
    [[nodiscard]] double panY()       { return (double)pan_y; }
    [[nodiscard]] DVec2 zoom()        { return {(double)zoom_x, (double)zoom_y }; }
    [[nodiscard]] double zoomX()      { return (double)zoom_x; }
    [[nodiscard]] double zoomY()      { return (double)zoom_y; }
    [[nodiscard]] double rotation()   { return (double)cam_rotation; }

    // ======== World rect focusing ========
    void focusWorldRect(flt128 left, flt128 top, flt128 right, flt128 bottom, bool stretch = false);
    void focusWorldRect(const DRect& r, bool stretch = false) { focusWorldRect(r.x1, r.y1, r.x2, r.y2, stretch); }
    //void focusWorldRect();  // todo: Use current view as the focus rect

      /// Once world rect focused, use it as a "reference" for scaling relative to that reference zoom, e.g.
      /// > setRelativeZoom(2)  will zoom 2x relative to that focused rect

    [[nodiscard]] DDVec2 getReferenceZoom() { return { ref_zoom_x, ref_zoom_y }; }               // Cached zoom set by focusWorldRect()
    //[[nodiscard]] DDVec2 getRelativeZoom_f128() { return { zoom_x / ref_zoom_x, zoom_y / ref_zoom_y }; } // Current relative scale factor
    //[[nodiscard]] DVec2 getRelativeZoom() { return { zoom_x / ref_zoom_x, zoom_y / ref_zoom_y }; } // Current relative scale factor

    template<typename T=double> [[nodiscard]]
    Vec2<T> getRelativeZoom() const noexcept
    {
        return {
            static_cast<T>(zoom_x) / static_cast<T>(ref_zoom_x),
            static_cast<T>(zoom_y) / static_cast<T>(ref_zoom_y)
        };
    }

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

    void setRelativeZoom(flt128 relative_zoom)
    {
        zoom_x = ref_zoom_x * relative_zoom;
        zoom_y = ref_zoom_y * relative_zoom;
        updateCameraMatrix();
    }
    void setRelativeZoomX(flt128 relative_zoom_x) {
        zoom_x = ref_zoom_x * relative_zoom_x;
        updateCameraMatrix();
    }
    void setRelativeZoomY(flt128 relative_zoom_y) {
        zoom_y = ref_zoom_y * relative_zoom_y;
        updateCameraMatrix();
    }

    // Clamp the relative zoom range (relative to the reference)
    void restrictRelativeZoomRange(double min, double max);

    DVec2 getViewportFocusedWorldSize();

    // ======== Camera controls ========

    void cameraToViewport(double left, double top, double right, double bottom);

    void originToCenterViewport();

    [[nodiscard]] DDVec2 worldToStageOffset(const DDVec2& o)
    {
        return {
            (o.x * cos_128 - o.y * sin_128) * zoom_x,
            (o.y * cos_128 + o.x * sin_128) * zoom_y
        };
    }
    [[nodiscard]] DDVec2 stageToWorldOffset(const DDVec2& stage_offset)
    {
        return {
            (stage_offset.x * cos_128 + stage_offset.y * sin_128) / zoom_x,
            (stage_offset.y * cos_128 - stage_offset.x * sin_128) / zoom_y
        };
    }

    [[nodiscard]] DVec2 worldToStageOffset(const DVec2& o)
    {
        return {
            (o.x * cos_64 - o.y * sin_64) * (double)zoom_x,
            (o.y * cos_64 + o.x * sin_64) * (double)zoom_y
        };
    }
    [[nodiscard]] DVec2 stageToWorldOffset(const DVec2& stage_offset)
    {
        return {
            (stage_offset.x * cos_64 + stage_offset.y * sin_64) / (double)zoom_x,
            (stage_offset.y * cos_64 - stage_offset.x * sin_64) / (double)zoom_y
        };
    }

    [[nodiscard]] DVec2 worldAxisDirStage(bool isX) const
    {
        double c = std::cos(cam_rotation);
        double s = std::sin(cam_rotation);
        return isX ? DVec2{ c, s } : DVec2{ -s, c };
    }
    [[nodiscard]] DVec2 worldAxisPerpStage(bool isX) const
    {
        DVec2 d = worldAxisDirStage(isX).normalized();
        return { -d.y, d.x };
    }

    [[nodiscard]] DVec2 worldToStageOffset(double ox, double oy) { return worldToStageOffset(DVec2{ ox, oy }); }
    [[nodiscard]] DVec2 stageToWorldOffset(double ox, double oy) { return stageToWorldOffset(DVec2{ ox, oy }); }
    [[nodiscard]] DDVec2 worldToStageOffset(flt128 ox, flt128 oy) { return worldToStageOffset(DDVec2{ ox, oy }); }
    [[nodiscard]] DDVec2 stageToWorldOffset(flt128 ox, flt128 oy) { return stageToWorldOffset(DDVec2{ ox, oy }); }

    [[nodiscard]] DVec2 originPixelOffset();
    [[nodiscard]] DVec2 originWorldOffset();
    [[nodiscard]] DVec2 panPixelOffset();

    // world --> stage matrix
    glm::dmat3 m64     = glm::dmat3(1.0);
    glm::dmat3 inv_m64 = glm::dmat3(1.0);

    glm::ddmat3 m128     = glm::ddmat3(1.0);
    glm::ddmat3 inv_m128 = glm::ddmat3(1.0);

    double cos_64  = 1.0, sin_64  = 0.0;
    flt128 cos_128 = 1.0, sin_128 = 0.0;

    void ddResetTransform() { m64 = glm::ddmat3(1.0);  m128 = glm::ddmat3(1.0); }
    void ddTranslate(flt128 tx, flt128 ty) { m128 = m128 * glm_dtranslate(tx, ty); m64 = static_cast<glm::ddmat3>(m128); }
    void ddRotate(flt128 r)                { m128 = m128 * glm_drotate(r);         m64 = static_cast<glm::ddmat3>(m128); }
    void ddScale(flt128 sx, flt128 sy)     { m128 = m128 * glm_dscale(sx, sy);     m64 = static_cast<glm::ddmat3>(m128); }

    void updateCameraMatrix()
    {
        DVec2 origin_offset = originPixelOffset();

        ddResetTransform();

        ddTranslate(origin_offset.x + pan_x, origin_offset.y + pan_y);
        ddRotate(cam_rotation);
        ddTranslate(-cam_x * zoom_x, -cam_y * zoom_y);
        ddScale(zoom_x, zoom_y);

        cos_128 =  m128[0][0] / zoom_x;
        sin_128 = -m128[1][0] / zoom_x;

        cos_64 =  m64[0][0] / (double)zoom_x;
        sin_64 = -m64[1][0] / (double)zoom_x;

        inv_m64 = glm::inverse(m64);
        inv_m128 = glm::inverse(m128);
    }

    [[nodiscard]] DVec2 toStage(double sx, double sy) { return static_cast<DVec2>(m64 * glm::dvec3(sx, sy, 1)); }
    [[nodiscard]] DVec2 toWorld(double wx, double wy) { return static_cast<DVec2>(inv_m64 * glm::dvec3(wx, wy, 1.0)); }
    [[nodiscard]] DVec2 toWorld(DVec2 p) { return static_cast<DVec2>(m64 * glm::dvec3(p.x, p.y, 1)); }
    [[nodiscard]] DVec2 toStage(DVec2 p) { return static_cast<DVec2>(inv_m64 * glm::dvec3(p.x, p.y, 1.0)); }

    [[nodiscard]] DDVec2 toStage(flt128 sx, flt128 sy) { return static_cast<DDVec2>(m128 * glm::ddvec3(sx, sy, 1)); }
    [[nodiscard]] DDVec2 toWorld(flt128 wx, flt128 wy) { return static_cast<DDVec2>(inv_m128 * glm::ddvec3(wx, wy, 1.0)); }
    [[nodiscard]] DDVec2 toWorld(DDVec2 p) { return static_cast<DDVec2>(m128 * glm::ddvec3(p.x, p.y, 1)); }
    [[nodiscard]] DDVec2 toStage(DDVec2 p) { return static_cast<DDVec2>(inv_m128 * glm::ddvec3(p.x, p.y, 1.0)); }

    [[nodiscard]] DRect toWorldRect(const DRect& r);
    [[nodiscard]] DRect toWorldRect(double x1, double y1, double x2, double y2);

    [[nodiscard]] DQuad toWorldQuad(const DVec2& a, const DVec2& b, const DVec2& c, const DVec2& d);
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
            return sqrt(dx * dx + dy * dy);
        }
        return 0.0;
    }

    double panDownTouchDist() { return pan_down_touch_dist; }
    double panDownTouchAngle() { return pan_down_touch_angle; }
    double panDownTouchX() { return pan_down_touch_x; }
    double panDownTouchY() { return pan_down_touch_y; }
    flt128 panBegCamX() { return pan_beg_cam_x; }
    flt128 panBegCamY() { return pan_beg_cam_y; }
    flt128 panBegCamZoomX() { return pan_beg_cam_zoom_x; }
    flt128 panBegCamZoomY() { return pan_beg_cam_zoom_x; }
    double panBegCamAngle() { return pan_beg_cam_angle; }

    void handleWorldNavigation(Event e, bool single_touch_pan);
};

struct CameraViewController
{
    double cam_x = -0.5;
    double cam_y = 0.0;
    double cam_radians = 0.0;
    double cam_zoom = 1.0;
    DVec2  cam_zoom_xy = DVec2(1, 1);

    double old_cam_x = -0.5;
    double old_cam_y = 0.0;
    double old_cam_radians = 0.0;
    double old_cam_zoom = 1.0;
    DVec2  old_cam_zoom_xy = DVec2(1, 1);

    double cam_degrees = 0.0;
    double cam_avg_zoom = 1.0;

    void populateUI(DRect cam_area = DRect::infinite());

    void read(Camera* cam)
    {
        cam_x = cam->x();
        cam_y = cam->y();
        cam_radians = cam->rotation();
        cam_zoom = (cam->getRelativeZoom().x / cam_zoom_xy.x);
        cam_avg_zoom = cam->zoom().magnitude();
    }

    void apply(Camera* cam)
    {
        cam_degrees = Math::toDegrees(cam_radians);

        if (cam_radians != old_cam_radians) cam->setRotation(cam_radians);
        if (cam_x != old_cam_x) cam->setX(cam_x);
        if (cam_y != old_cam_y) cam->setY(cam_y);
        if (cam_zoom_xy != old_cam_zoom_xy ||
            cam_zoom != old_cam_zoom)
        {
            cam->setRelativeZoomX(cam_zoom * cam_zoom_xy.x);
            cam->setRelativeZoomY(cam_zoom * cam_zoom_xy.y);
        }
        else
        {
            cam_zoom = (cam->getRelativeZoom().x / cam_zoom_xy.x);
        }

        old_cam_radians = cam_radians;
        old_cam_x = cam_x;
        old_cam_y = cam_y;
        old_cam_zoom = cam_zoom;
        old_cam_zoom_xy = cam_zoom_xy;
    }

    CameraViewController& operator=(const CameraViewController& rhs)
    {
        cam_x = rhs.cam_x;
        cam_y = rhs.cam_y;
        cam_radians = rhs.cam_radians;
        cam_zoom = rhs.cam_zoom;
        cam_zoom_xy = rhs.cam_zoom_xy;

        cam_degrees = rhs.cam_degrees;
        cam_avg_zoom = rhs.cam_avg_zoom;
        return *this;
    }

    bool operator==(const CameraViewController& rhs) const
    {
        return cam_x == rhs.cam_x &&
            cam_y == rhs.cam_y &&
            cam_radians == rhs.cam_radians &&
            cam_zoom == rhs.cam_zoom &&
            cam_zoom_xy == rhs.cam_zoom_xy;
    }

    static CameraViewController lerp(const CameraViewController& a, const CameraViewController& b, double f)
    {
        CameraViewController ret;
        ret.cam_x = Math::lerp(a.cam_x, b.cam_x, f);
        ret.cam_y = Math::lerp(a.cam_y, b.cam_y, f);
        ret.cam_radians = Math::lerp(a.cam_radians, b.cam_radians, f);
        ret.cam_zoom = Math::lerp(a.cam_zoom, b.cam_zoom, f);
        ret.cam_zoom_xy = Math::lerp(a.cam_zoom_xy, b.cam_zoom_xy, f);
        ret.cam_degrees = Math::toDegrees(ret.cam_radians);
        return ret;
    }
};

//inline void Camera::setStageOffset(double ox, double oy)
//{
//    stage_ox = ox;
//    stage_oy = oy;
//}

inline DVec2 Camera::originWorldOffset()
{
    // zoom_x/zoom_y refer to world zoom, not stage zoom? bug if camera rotated?
    return originPixelOffset() / DDVec2(zoom_x, zoom_y);
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
    return DDVec2{ size.x * zoom_x, size.y * zoom_y };
}

inline DVec2 Camera::toStageSize(double w, double h)
{
    return DDVec2{ w * zoom_x, h * zoom_y };
}

inline DRect Camera::toStageRect(double x0, double y0, double x1, double y1)
{
    DVec2 p1 = toStage(DVec2{ x0, y0 });
    DVec2 p2 = toStage(DVec2{ x1, y1 });
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

BL_END_NS
