#pragma once
#include "types.h"
#include "debug.h"
#include <bitloop/util/math_util.h>

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

    int cam_pos_stage_snap_size = 1;
    DDVec2 last_pan_snapped_cam_grid_pos;

    bool panning = false;
    bool direct_cam_panning = true;

protected:

    Viewport* viewport = nullptr;
    double viewport_w = 0;
    double viewport_h = 0;

    bool transform_coordinates = true;
    bool scale_lines = true;
    bool scale_sizes = true;
    bool scale_text  = true;
    bool rotate_text = true;

private:

    bool saved_transform_coordinates = transform_coordinates;
    bool saved_scale_lines = scale_lines;
    bool saved_scale_sizes = scale_sizes;
    bool saved_scale_text  = scale_text;
    bool saved_rotate_text = rotate_text;

    std::vector<FingerInfo> fingers;

public:

    static inline Camera* active = nullptr;

public:


    // world --> stage matrix
    glm::dmat3 m64 = glm::dmat3(1.0);
    glm::dmat3 inv_m64 = glm::dmat3(1.0);

    glm::ddmat3 m128 = glm::ddmat3(1.0);
    glm::ddmat3 inv_m128 = glm::ddmat3(1.0);

    double cos_64 = 1.0, sin_64 = 0.0;
    flt128 cos_128 = 1.0, sin_128 = 0.0;

    void ddResetTransform()                 { m64 = glm::ddmat3(1.0);  m128 = glm::ddmat3(1.0); }
    void ddTranslate(flt128 tx, flt128 ty)  { m128 = m128 * glm_dtranslate(tx, ty); m64 = static_cast<glm::ddmat3>(m128); }
    void ddRotate(flt128 r)                 { m128 = m128 * glm_drotate(r);         m64 = static_cast<glm::ddmat3>(m128); }
    void ddScale(flt128 sx, flt128 sy)      { m128 = m128 * glm_dscale(sx, sy);     m64 = static_cast<glm::ddmat3>(m128); }

    void updateCameraMatrix()
    {
        DVec2 origin_offset = originPixelOffset();

        flt128 cam_stage_x = cam_x * zoom_x;
        flt128 cam_stage_y = cam_y * zoom_y;

        flt128 snapped_stage_cam_x = round(cam_stage_x / cam_pos_stage_snap_size) * cam_pos_stage_snap_size;
        flt128 snapped_stage_cam_y = round(cam_stage_y / cam_pos_stage_snap_size) * cam_pos_stage_snap_size;

        flt128 snapped_cam_x = snapped_stage_cam_x / zoom_x;
        flt128 snapped_cam_y = snapped_stage_cam_y / zoom_y;

        ddResetTransform();

        ddTranslate(origin_offset.x + pan_x, origin_offset.y + pan_y);
        ddRotate(cam_rotation);
        ddTranslate(-snapped_cam_x * zoom_x, -snapped_cam_y * zoom_y);
        ddScale(zoom_x, zoom_y);

        cos_128 = m128[0][0] / zoom_x;
        sin_128 = -m128[1][0] / zoom_x;

        cos_64 = m64[0][0] / (double)zoom_x;
        sin_64 = -m64[1][0] / (double)zoom_x;

        inv_m64 = glm::inverse(m64);
        inv_m128 = glm::inverse(m128);
    }

    void worldTransform();
    void stageTransform();
    void worldHudTransform();

    void worldCoordinates(bool b) { transform_coordinates = b; }
    void scalingLines(bool b)     { scale_lines = b; }
    void scalingSizes(bool b)     { scale_sizes = b; }
    void scalingText(bool b)      { scale_text  = b; }
    void rotatingText(bool b)     { rotate_text = b; }

    void saveCameraTransform();
    void restoreCameraTransform();

    //double stage_ox = 0;
    //double stage_oy = 0;
    //void setStageOffset(double ox, double oy);

    void setCameraStageSnappingSize(int stage_pixels)
    {
        cam_pos_stage_snap_size = stage_pixels;
    }

    void setOriginViewportAnchor(double ax, double ay);
    void setOriginViewportAnchor(Anchor anchor);

    constexpr double viewportWidth() const { return viewport_w; }
    constexpr double viewportHeight() const { return viewport_h; }

    // flt128
    bool setX(flt128 _x)               { if (cam_x  != _x)                          { cam_x = _x;                       updateCameraMatrix(); return true; } return false; }
    bool setY(flt128 _y)               { if (cam_y  != _y)                          { cam_y = _y;                       updateCameraMatrix(); return true; } return false; }
    bool setZoomX(flt128 zx)           { if (zoom_x != zx)                          { zoom_x = zx;                      updateCameraMatrix(); return true; } return false; }
    bool setZoomY(flt128 zy)           { if (zoom_y != zy)                          { zoom_y = zy;                      updateCameraMatrix(); return true; } return false; }
                                                                                                                        
    bool setPos(flt128 _x, flt128 _y)  { if (cam_x  != _x   || cam_y  != _y)        { cam_x = _x; cam_y = _y;           updateCameraMatrix(); return true; } return false; }
    bool setZoom(flt128 zoom)          { if (zoom_x != zoom || zoom_y != zoom)      { zoom_x = zoom_y = zoom;           updateCameraMatrix(); return true; } return false; }
    bool setZoom(flt128 zx, flt128 zy) { if (zoom_x != zx   || zoom_y != zy)        { zoom_x = zx; zoom_y = zy;         updateCameraMatrix(); return true; } return false; }
                                                                                                                        
    // double                                                                                                           
    bool setX(double _x)               { if (cam_x  != _x)                          { cam_x = _x;                       updateCameraMatrix(); return true; } return false; }
    bool setY(double _y)               { if (cam_y  != _y)                          { cam_y = _y;                       updateCameraMatrix(); return true; } return false; }
    bool setZoomX(double zx)           { if (zoom_x != zx)                          { zoom_x = zx;                      updateCameraMatrix(); return true; } return false; }
    bool setZoomY(double zy)           { if (zoom_y != zy)                          { zoom_y = zy;                      updateCameraMatrix(); return true; } return false; }
                                                                                                                        
    bool setPos(double _x, double _y)  { if (cam_x  != _x     || cam_y  != _y)      { cam_x = _x; cam_y = _y;           updateCameraMatrix(); return true; } return false; }
    bool setPos(DVec2 pos)             { if (cam_x  != pos.x  || cam_y  != pos.y)   { cam_x = pos.x; cam_y = pos.y;     updateCameraMatrix(); return true; } return false; }
    bool setZoom(double zoom)          { if (zoom_x != zoom   || zoom_y != zoom)    { zoom_x = zoom_y = zoom;           updateCameraMatrix(); return true; } return false; }
    bool setZoom(DVec2 zoom)           { if (zoom_x != zoom.x || zoom_y != zoom.y)  { zoom_x = zoom.x; zoom_y = zoom.y; updateCameraMatrix(); return true; } return false; }
    bool setZoom(double zx, double zy) { if (zoom_x != zx     || zoom_y != zy)      { zoom_x = zx; zoom_y = zy;         updateCameraMatrix(); return true; } return false; }
                                                                                                                        
    bool setPan(double px, double py)  { if (pan_x != px || pan_y != py)            { pan_x = px; pan_y = py;           updateCameraMatrix(); return true; } return false; }
    bool setRotation(double angle)     { if (cam_rotation != angle)                 { cam_rotation = angle;             updateCameraMatrix(); return true; } return false; }

    [[nodiscard]] constexpr bool isPanning()  const   { return panning; }

    [[nodiscard]] constexpr double x()        const   { return (double)cam_x; }
    [[nodiscard]] constexpr double y()        const   { return (double)cam_y; }
    [[nodiscard]] constexpr DVec2  pos()      const   { return {(double)cam_x, (double)cam_y }; }
    [[nodiscard]] constexpr double panX()     const   { return (double)pan_x; }
    [[nodiscard]] constexpr double panY()     const   { return (double)pan_y; }
    [[nodiscard]] constexpr DVec2  zoom()      const   { return {(double)zoom_x, (double)zoom_y }; }
    [[nodiscard]] constexpr double zoomX()    const   { return (double)zoom_x; }
    [[nodiscard]] constexpr double zoomY()    const   { return (double)zoom_y; }
    [[nodiscard]] constexpr double rotation() const   { return (double)cam_rotation; }

    // ======== World rect focusing ========
    void focusWorldRect(flt128 left, flt128 top, flt128 right, flt128 bottom, bool stretch = false);
    void focusWorldRect(const DRect& r, bool stretch = false) { focusWorldRect(r.x1, r.y1, r.x2, r.y2, stretch); }
    //void focusWorldRect();  // todo: Use current view as the focus rect

      /// Once world rect focused, use it as a "reference" for scaling relative to that reference zoom, e.g.
      /// > setRelativeZoom(2)  will zoom 2x relative to that focused rect

    [[nodiscard]] DVec2 getReferenceZoom() const  { return { (double)ref_zoom_x, (double)ref_zoom_y }; }               // Cached zoom set by focusWorldRect()
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

    DVec2 getViewportFocusedWorldSize() const;

    // ======== Camera controls ========

    void cameraToViewport(double left, double top, double right, double bottom);

    void originToCenterViewport();

    [[nodiscard]] DDVec2 worldToStageOffset(const DDVec2& o) const
    {
        return {
            (o.x * cos_128 - o.y * sin_128) * zoom_x,
            (o.y * cos_128 + o.x * sin_128) * zoom_y
        };
    }
    [[nodiscard]] DDVec2 stageToWorldOffset(const DDVec2& stage_offset) const
    {
        return {
            (stage_offset.x * cos_128 + stage_offset.y * sin_128) / zoom_x,
            (stage_offset.y * cos_128 - stage_offset.x * sin_128) / zoom_y
        };
    }

    [[nodiscard]] DVec2 worldToStageOffset(const DVec2& o) const
    {
        return {
            (o.x * cos_64 - o.y * sin_64) * (double)zoom_x,
            (o.y * cos_64 + o.x * sin_64) * (double)zoom_y
        };
    }
    [[nodiscard]] DVec2 stageToWorldOffset(const DVec2& stage_offset) const
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

    [[nodiscard]] DVec2 worldToStageOffset(double ox, double oy)  const { return worldToStageOffset(DVec2{ ox, oy }); }
    [[nodiscard]] DVec2 stageToWorldOffset(double ox, double oy)  const { return stageToWorldOffset(DVec2{ ox, oy }); }
    [[nodiscard]] DDVec2 worldToStageOffset(flt128 ox, flt128 oy) const { return worldToStageOffset(DDVec2{ ox, oy }); }
    [[nodiscard]] DDVec2 stageToWorldOffset(flt128 ox, flt128 oy) const { return stageToWorldOffset(DDVec2{ ox, oy }); }

    [[nodiscard]] DVec2 originPixelOffset() const;
    [[nodiscard]] DVec2 originWorldOffset() const;
    [[nodiscard]] DVec2 panPixelOffset() const;

    [[nodiscard]] DVec2 toStage(double wx, double wy)  const { return static_cast<DVec2>(    m64 * glm::dvec3(wx, wy, 1)); }
    [[nodiscard]] DVec2 toWorld(double sx, double sy)  const { return static_cast<DVec2>(inv_m64 * glm::dvec3(sx, sy, 1.0)); }
    [[nodiscard]] DVec2 toStage(DVec2 p)               const { return static_cast<DVec2>(    m64 * glm::dvec3(p.x, p.y, 1.0)); }
    [[nodiscard]] DVec2 toWorld(DVec2 p)               const { return static_cast<DVec2>(inv_m64 * glm::dvec3(p.x, p.y, 1)); }

    [[nodiscard]] DDVec2 toStage(flt128 wx, flt128 wy) const { return static_cast<DDVec2>(    m128 * glm::ddvec3(wx, wy, 1)); }
    [[nodiscard]] DDVec2 toWorld(flt128 sx, flt128 sy) const { return static_cast<DDVec2>(inv_m128 * glm::ddvec3(sx, sy, 1.0)); }
    [[nodiscard]] DDVec2 toStage(DDVec2 p)             const { return static_cast<DDVec2>(    m128 * glm::ddvec3(p.x, p.y, 1.0)); }
    [[nodiscard]] DDVec2 toWorld(DDVec2 p)             const { return static_cast<DDVec2>(inv_m128 * glm::ddvec3(p.x, p.y, 1)); }

    [[nodiscard]] DRect toWorldRect(const DRect& r) const;
    [[nodiscard]] DRect toWorldRect(double x1, double y1, double x2, double y2) const;

    [[nodiscard]] DQuad toWorldQuad(const DVec2& a, const DVec2& b, const DVec2& c, const DVec2& d) const;
    [[nodiscard]] DQuad toWorldQuad(const DQuad& quad) const;
    [[nodiscard]] DQuad toWorldQuad(const DRect& quad) const;
    [[nodiscard]] DQuad toWorldQuad(double x1, double y1, double x2, double y2) const;

    [[nodiscard]] DVec2 toStageSize(const DVec2& size) const;
    [[nodiscard]] DVec2 toStageSize(double w, double h) const;
    [[nodiscard]] DRect toStageRect(double x0, double y0, double x1, double y1) const;
    [[nodiscard]] DRect toStageRect(const DVec2& pt1, const DVec2& pt2) const;
    [[nodiscard]] DQuad toStageQuad(const DVec2& a, const DVec2& b, const DVec2& c, const DVec2& d) const;

    //[[nodiscard]] DAngledRect toWorldRect(const DAngledRect& r);
    //[[nodiscard]] DAngledRect toStageRect(const DAngledRect& r);

    void setDirectCameraPanning(bool b) { direct_cam_panning = b; }
    void panBegin(int _x, int _y, double touch_dist, double touch_angle);
    bool panDrag(int _x, int _y, double touch_dist, double touch_angle);
    void panEnd();
    bool panZoomProcess();

    [[nodiscard]] std::vector<FingerInfo> pressedFingers() {
        return fingers;
    }

    [[nodiscard]] double touchAngle() const
    {
        if (fingers.size() >= 2)
            return atan2(fingers[1].y - fingers[0].y, fingers[1].x - fingers[0].x);
        return 0.0;
    }
    [[nodiscard]] double touchDist() const
    {
        if (fingers.size() >= 2)
        {
            double dx = fingers[1].x - fingers[0].x;
            double dy = fingers[1].y - fingers[0].y;
            return sqrt(dx * dx + dy * dy);
        }
        return 0.0;
    }

    [[nodiscard]] double panDownTouchDist()  const { return pan_down_touch_dist; }
    [[nodiscard]] double panDownTouchAngle() const { return pan_down_touch_angle; }
    [[nodiscard]] double panDownTouchX()     const { return pan_down_touch_x; }
    [[nodiscard]] double panDownTouchY()     const { return pan_down_touch_y; }
    [[nodiscard]] flt128 panBegCamX()        const { return pan_beg_cam_x; }
    [[nodiscard]] flt128 panBegCamY()        const { return pan_beg_cam_y; }
    [[nodiscard]] flt128 panBegCamZoomX()    const { return pan_beg_cam_zoom_x; }
    [[nodiscard]] flt128 panBegCamZoomY()    const { return pan_beg_cam_zoom_x; }
    [[nodiscard]] double panBegCamAngle()    const { return pan_beg_cam_angle; }

    bool handleWorldNavigation(Event e, bool single_touch_pan, bool zoom_anchor_mouse=false);
	//void handleWorldNavigation(Event e, bool single_touch_pan);
};

class CameraViewController
{
    double angle_degrees = 0.0;
    double avg_real_zoom = 1.0;

    double init_cam_x = x;
    double init_cam_y = y;
    double init_degrees = angle_degrees;
    double init_cam_zoom = 1.0;
    DVec2 init_cam_zoom_xy = zoom_xy;

    double old_x = 0;
    double old_y = 0.0;
    double old_angle = 0.0;
    double old_zoom = 1.0;
    DVec2  old_zoom_xy = DVec2(1, 1);

public:

    union
    {
        struct {
            double x;
            double y;
        };
        DVec2 pos{ 0, 0 };
    };

    double angle = 0.0;
    double zoom = 1.0;
    DVec2  zoom_xy = DVec2(1, 1);

    void populateUI(DRect cam_area = DRect::infinite());

    void setCurrentAsDefault()
    {
        init_cam_x = x;
        init_cam_y = y;
        init_degrees = angle_degrees;
    }

    int getPositionDecimalPlaces() const
    {
        return 1 + Math::countWholeDigits(zoom * 5);
    }

    double getPositionPrecision() const
    {
        return Math::precisionFromDecimalPlaces<double>(getPositionDecimalPlaces());
    }

    void read(const Camera* camera)
    {
        x = camera->x();
        y = camera->y();
        angle = camera->rotation();
        zoom = (camera->getRelativeZoom().x / zoom_xy.x);
        avg_real_zoom = camera->zoom().magnitude();
    }
    void read(const Camera& camera)
    {
        read(&camera);
    }

    void apply(Camera* camera)
    {
        angle_degrees = Math::toDegrees(angle);

        if (angle != old_angle) camera->setRotation(angle);
        if (x != old_x) camera->setX(x);
        if (y != old_y) camera->setY(y);
        if (zoom_xy != old_zoom_xy ||
            zoom != old_zoom)
        {
            camera->setRelativeZoomX(zoom * zoom_xy.x);
            camera->setRelativeZoomY(zoom * zoom_xy.y);
        }
        else
        {
            zoom = (camera->getRelativeZoom().x / zoom_xy.x);
        }

        avg_real_zoom = camera->zoom().magnitude();

        old_angle = angle;
        old_x = x;
        old_y = y;
        old_zoom = zoom;
        old_zoom_xy = zoom_xy;
    }

    bool operator==(const CameraViewController& rhs) const
    {
        return x == rhs.x &&
            y == rhs.y &&
            angle == rhs.angle &&
            zoom == rhs.zoom &&
            zoom_xy == rhs.zoom_xy &&
            avg_real_zoom == rhs.avg_real_zoom;
    }

    static CameraViewController lerp(const CameraViewController& a, const CameraViewController& b, double f)
    {
        CameraViewController ret;
        ret.x = Math::lerp(a.x, b.x, f);
        ret.y = Math::lerp(a.y, b.y, f);
        ret.angle = Math::lerpAngle(a.angle, b.angle, f);
        ret.zoom = Math::lerp(a.zoom, b.zoom, f);
        ret.zoom_xy = Math::lerp(a.zoom_xy, b.zoom_xy, f);
        ret.angle_degrees = Math::toDegrees(ret.angle);
        return ret;
    }
};

//inline void Camera::setStageOffset(double ox, double oy)
//{
//    stage_ox = ox;
//    stage_oy = oy;
//}

inline DVec2 Camera::originWorldOffset() const
{
    // zoom_x/zoom_y refer to world zoom, not stage zoom? bug if camera rotated?
    return originPixelOffset() / DDVec2(zoom_x, zoom_y);
}

inline DVec2 Camera::panPixelOffset() const
{
    return DVec2(pan_x, pan_y);
}


inline DRect Camera::toWorldRect(const DRect& r) const
{
    DVec2 tl = toWorld(r.x1, r.y1);
    DVec2 br = toWorld(r.x2, r.y2);
    return { tl.x, tl.y, br.x, br.y };
}

inline DRect Camera::toWorldRect(double x1, double y1, double x2, double y2) const
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

inline DQuad Camera::toWorldQuad(const DVec2& a, const DVec2& b, const DVec2& c, const DVec2& d) const
{
    DVec2 qA = toWorld(a);
    DVec2 qB = toWorld(b);
    DVec2 qC = toWorld(c);
    DVec2 qD = toWorld(d);
    return DQuad(qA, qB, qC, qD);
}

inline DQuad Camera::toWorldQuad(const DQuad& quad) const
{
    return toWorldQuad(quad.a, quad.b, quad.c, quad.d);
}

inline DQuad Camera::toWorldQuad(double x1, double y1, double x2, double y2) const
{
    return toWorldQuad(
        Vec2(x1, y1),
        Vec2(x2, y1),
        Vec2(x2, y2),
        Vec2(x1, y2)
    );
}

inline DQuad Camera::toWorldQuad(const DRect& r) const
{
    return toWorldQuad(
        Vec2(r.x1, r.y1),
        Vec2(r.x2, r.y1),
        Vec2(r.x2, r.y2),
        Vec2(r.x1, r.y2)
    );
}

inline DVec2 Camera::toStageSize(const DVec2& size) const
{
    return DDVec2{ size.x * zoom_x, size.y * zoom_y };
}

inline DVec2 Camera::toStageSize(double w, double h) const
{
    return DDVec2{ w * zoom_x, h * zoom_y };
}

inline DRect Camera::toStageRect(double x0, double y0, double x1, double y1) const
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

inline DRect Camera::toStageRect(const DVec2& pt1, const DVec2& pt2) const
{
    return toStageRect(pt1.x, pt1.y, pt2.x, pt2.y);
}

inline DQuad Camera::toStageQuad(const DVec2& a, const DVec2& b, const DVec2& c, const DVec2& d) const
{
    DVec2 qA = toStage(a);
    DVec2 qB = toStage(b);
    DVec2 qC = toStage(c);
    DVec2 qD = toStage(d);
    return { qA, qB, qC, qD };
}

BL_END_NS
