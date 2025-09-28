#pragma once
#include "types.h"
#include "debug.h"
#include <bitloop/util/math_util.h>

BL_BEGIN_NS

inline glm::ddmat3 glm_ddtranslate(flt128 tx, flt128 ty) {
    glm::ddmat3 m(f128(1.0));
    m[2][0] = tx; // column 2, row 0
    m[2][1] = ty; // column 2, row 1
    return m;
}

inline glm::ddmat3 glm_ddscale(flt128 sx, flt128 sy) {
    glm::ddmat3 m(f128(1.0));
    m[0][0] = sx;
    m[1][1] = sy;
    return m;
}

inline glm::ddmat3 glm_ddrotate(flt128 r) {
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

template<class T> concept is_f32 = std::is_same_v<T, float>;
template<class T> concept is_f64 = std::is_same_v<T, double>;
template<class T> concept is_f128 = std::is_same_v<T, flt128>;

class Camera
{

    friend class Painter;
    friend class ProjectBase;
    friend class Viewport;

    // ────── world <-> stage (128 bit versions - single souce of truth) ──────
    union { struct { flt128 cam_x;  flt128 cam_y; };  Vec2<flt128> cam_pos{}; };
    union { struct { flt128 zoom_x; flt128 zoom_y; }; Vec2<flt128> cam_zoom{ f128(1), f128(1) }; };
    glm::ddmat3 m128 = glm::ddmat3(f128(1.0));
    glm::ddmat3 inv_m128 = glm::ddmat3(f128(1.0));
    flt128 cos_128 = f128(1.0);
    flt128 sin_128 = f128(0.0);

    // ────── world <-> stage (cached 64-bit versions for performance) ──────
    union { struct { double cam_x_64;  double cam_y_64;  }; DVec2 cam_pos_64{}; };
    union { struct { double zoom_x_64; double zoom_y_64; }; DVec2 cam_zoom_64{ 1.0, 1.0 }; };
    glm::dmat3 m64 = glm::dmat3(1.0);
    glm::dmat3 inv_m64 = glm::dmat3(1.0);
    double cos_64 = 1.0;
    double sin_64 = 0.0;

    double cam_rotation = 0;
    double pan_x = 0;
    double pan_y = 0;
    double focal_anchor_x = 0.0;
    double focal_anchor_y = 0.0;

    // ────── zoom/focus attributes ──────
    flt128 ref_zoom_x = f128(1);
    flt128 ref_zoom_y = f128(1);
    flt128 min_zoom = f128(-1e+300); // Relative to "focused" rect
    flt128 max_zoom = f128(1e+300); // Relative to "focused" rect

    // ────── Pan attributes ──────
    int    pan_down_touch_x = 0;
    int    pan_down_touch_y = 0;
    double pan_down_touch_dist = 0;
    double pan_down_touch_angle = 0;

    flt128 pan_beg_cam_x = f128(0);
    flt128 pan_beg_cam_y = f128(0);
    flt128 pan_beg_cam_zoom_x = f128(0);
    flt128 pan_beg_cam_zoom_y = f128(0);
    double pan_beg_cam_angle = 0;

    int cam_pos_stage_snap_size = 1;
    DDVec2 last_pan_snapped_cam_grid_pos;

    bool panning = false;
    bool direct_cam_panning = true;

//protected:

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

    void ddResetTransform()                 { m128 = glm::ddmat3(f128(1.0)); }
    void ddTranslate(flt128 tx, flt128 ty)  { m128 = m128 * glm_ddtranslate(tx, ty); }
    void ddRotate(flt128 r)                 { m128 = m128 * glm_ddrotate(r);         }
    void ddScale(flt128 sx, flt128 sy)      { m128 = m128 * glm_ddscale(sx, sy);     }

    void updateCameraMatrix()
    {
        DDVec2 origin_offset = originPixelOffset();

        //flt128 cam_stage_x = cam_x * zoom_x;
        //flt128 cam_stage_y = cam_y * zoom_y;
        //
        //flt128 snapped_stage_cam_x = round(cam_stage_x / cam_pos_stage_snap_size) * cam_pos_stage_snap_size;
        //flt128 snapped_stage_cam_y = round(cam_stage_y / cam_pos_stage_snap_size) * cam_pos_stage_snap_size;
        //
        //flt128 snapped_cam_x = snapped_stage_cam_x / zoom_x;
        //flt128 snapped_cam_y = snapped_stage_cam_y / zoom_y;

        ddResetTransform();
        ddTranslate(origin_offset.x + flt128(pan_x), origin_offset.y + flt128(pan_y));
        ddRotate(f128(cam_rotation));
        ddTranslate(-cam_x * zoom_x, -cam_y * zoom_y);
        //ddTranslate(-snapped_cam_x * zoom_x, -snapped_cam_y * zoom_y);
        ddScale(zoom_x, zoom_y);

        // m128 = single source of truth
        m64 = static_cast<glm::dmat3>(m128);

        inv_m128 = glm::inverse(m128);
        inv_m64  = glm::inverse(m64);

        // Cached values
        cam_pos_64 = static_cast<DVec2>(cam_pos);
        cam_zoom_64 = static_cast<DVec2>(cam_zoom);

        cos_128 =  m128[0][0] / zoom_x;
        sin_128 = -m128[1][0] / zoom_x;

        cos_64 =  m64[0][0] / zoom_x_64;
        sin_64 = -m64[1][0] / zoom_x_64;
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

    void setOriginViewportAnchor(double ax, double ay);
    void setOriginViewportAnchor(Anchor anchor);

    constexpr double viewportWidth() const { return viewport_w; }
    constexpr double viewportHeight() const { return viewport_h; }

    [[nodiscard]] constexpr bool   isPanning()  const   { return panning; }
    [[nodiscard]] constexpr double panX()       const   { return (double)pan_x; }
    [[nodiscard]] constexpr double panY()       const   { return (double)pan_y; }
    [[nodiscard]] constexpr double rotation()   const   { return cam_rotation; }

    template<class T = double> requires is_f64<T>  [[nodiscard]] constexpr double x() const { return cam_x_64; }
    template<class T>          requires is_f128<T> [[nodiscard]] constexpr flt128 x() const { return cam_x; }
    template<class T = double> requires is_f64<T>  [[nodiscard]] constexpr double y() const { return cam_y_64; }
    template<class T>          requires is_f128<T> [[nodiscard]] constexpr flt128 y() const { return cam_y; }
    template<class T = double> requires is_f64<T>  [[nodiscard]] constexpr DVec2  pos() const { return cam_pos_64; }
    template<class T>          requires is_f128<T> [[nodiscard]] constexpr DDVec2 pos() const { return cam_pos; }
    template<class T = double> requires is_f64<T>  [[nodiscard]] constexpr DVec2  zoom() const { return cam_zoom_64; }
    template<class T>          requires is_f128<T> [[nodiscard]] constexpr DDVec2 zoom() const { return cam_zoom; }
    template<class T = double> requires is_f64<T>  [[nodiscard]] constexpr double zoomX() const { return zoom_x_64; }
    template<class T>          requires is_f128<T> [[nodiscard]] constexpr flt128 zoomX() const { return zoom_x; }
    template<class T = double> requires is_f64<T>  [[nodiscard]] constexpr double zoomY() const { return zoom_y_64; }
    template<class T>          requires is_f128<T> [[nodiscard]] constexpr flt128 zoomY() const { return zoom_y; }

    //void setCameraStageSnappingSize(int stage_pixels) { cam_pos_stage_snap_size = stage_pixels; } // todo: Unstable at deep flt128 zooms while panning

    // ────── f128 setters ──────
    bool setX(flt128 _x)               { if (cam_x  != _x)  { cam_x = _x;    updateCameraMatrix(); return true; } return false; }
    bool setY(flt128 _y)               { if (cam_y  != _y)  { cam_y = _y;    updateCameraMatrix(); return true; } return false; }
    bool setZoomX(flt128 zx)           { if (zoom_x != zx)  { zoom_x = zx;   updateCameraMatrix(); return true; } return false; }
    bool setZoomY(flt128 zy)           { if (zoom_y != zy)  { zoom_y = zy;   updateCameraMatrix(); return true; } return false; }
                                                                                                             
    bool setPos(flt128 _x, flt128 _y)  { if (cam_x  != _x   || cam_y  != _y)    { cam_x = _x; cam_y = _y;    updateCameraMatrix(); return true; } return false; }
    bool setZoom(flt128 zoom)          { if (zoom_x != zoom || zoom_y != zoom)  { zoom_x = zoom_y = zoom;    updateCameraMatrix(); return true; } return false; }
    bool setZoom(flt128 zx, flt128 zy) { if (zoom_x != zx   || zoom_y != zy)    { zoom_x = zx; zoom_y = zy;  updateCameraMatrix(); return true; } return false; }
    bool setZoom(DDVec2 zoom)          { if (cam_zoom != zoom)                  { cam_zoom = zoom;           updateCameraMatrix(); return true; } return false; }
                                                                                                                        
    // ────── f64 setters (always updates f128 versions so we have a single source of truth) ──────
    bool setX(double _x)               { if (cam_x_64  != _x) { cam_x  = f128(_x);  updateCameraMatrix(); return true; } return false; }
    bool setY(double _y)               { if (cam_y_64  != _y) { cam_y  = f128(_y);  updateCameraMatrix(); return true; } return false; }
    bool setZoomX(double zx)           { if (zoom_x_64 != zx) { zoom_x = f128(zx);  updateCameraMatrix(); return true; } return false; }
    bool setZoomY(double zy)           { if (zoom_y_64 != zy) { zoom_y = f128(zy);  updateCameraMatrix(); return true; } return false; }
                                                                                                                        
    bool setPos(double _x, double _y)  { if (cam_x_64  != _x     || cam_y_64  != _y)     { cam_pos = {f128(_x), f128(_y)};   updateCameraMatrix(); return true; } return false; }
    bool setPos(DVec2 pos)             { if (cam_x_64  != pos.x  || cam_y_64  != pos.y)  { cam_pos = pos;                    updateCameraMatrix(); return true; } return false; }
    bool setZoom(double zoom)          { if (zoom_x_64 != zoom   || zoom_y_64 != zoom)   { zoom_x = zoom_y = f128(zoom);     updateCameraMatrix(); return true; } return false; }
    bool setZoom(DVec2 zoom)           { if (zoom_x_64 != zoom.x || zoom_y_64 != zoom.y) { cam_zoom = zoom;                  updateCameraMatrix(); return true; } return false; }
    bool setZoom(double zx, double zy) { if (zoom_x_64 != zx     || zoom_y_64 != zy)     { cam_zoom = {f128(zx), f128(zy)};  updateCameraMatrix(); return true; } return false; }
                                                                                                                        
    bool setPan(double px, double py)  { if (pan_x != px || pan_y != py)  { pan_x = px; pan_y = py;  updateCameraMatrix(); return true; } return false; }
    bool setRotation(double angle)     { if (cam_rotation != angle)       { cam_rotation = angle;    updateCameraMatrix(); return true; } return false; }

    // ======== World rect focusing ========
    void focusWorldRect(flt128 x0, flt128 y0, flt128 x1, flt128 y1, bool stretch = false);
    void focusWorldRect(double x0, double y0, double x1, double y1, bool stretch = false) { focusWorldRect(f128(x0),   f128(y0),   f128(x1),   f128(y1),   stretch); }
    void focusWorldRect(const DRect& r, bool stretch = false)                             { focusWorldRect(f128(r.x1), f128(r.y1), f128(r.x2), f128(r.y2), stretch); }
    //void focusWorldRect();  // todo: Use current view as the focus rect

    /// Once focusWorldRect called, use it as a "reference" for scaling relative to that reference zoom, e.g.
    /// > setRelativeZoom(2)  will zoom 2x relative to that focused rect

    // Uses cached zoom ref, set by focusWorldRect()
    template<typename T> [[nodiscard]] Vec2<T> getReferenceZoom() const { return { T{ref_zoom_x}, T{ref_zoom_y} }; }
    template<typename T> [[nodiscard]] Vec2<T> getRelativeZoom() const noexcept { return {T{zoom_x / ref_zoom_x}, T{zoom_y / ref_zoom_y} }; }

    // Set zoom (relative to the reference)
    void setRelativeZoom(double rel_zoom)  { zoom_x = ref_zoom_x * rel_zoom; zoom_y = ref_zoom_y * rel_zoom; updateCameraMatrix(); }
    void setRelativeZoom(flt128 rel_zoom)  { zoom_x = ref_zoom_x * rel_zoom; zoom_y = ref_zoom_y * rel_zoom; updateCameraMatrix(); }
    void setRelativeZoomX(double rel_zx)   { zoom_x = ref_zoom_x * rel_zx; updateCameraMatrix(); }
    void setRelativeZoomX(flt128 rel_zx)   { zoom_x = ref_zoom_x * rel_zx; updateCameraMatrix(); }
    void setRelativeZoomY(double rel_zy)   { zoom_y = ref_zoom_y * rel_zy; updateCameraMatrix(); }
    void setRelativeZoomY(flt128 rel_zy)   { zoom_y = ref_zoom_y * rel_zy; updateCameraMatrix(); }

    // Clamp the relative zoom range (relative to the reference)
    void restrictRelativeZoomRange(double min, double max);

    // ======== Camera controls ========

    [[nodiscard]] DVec2 axisStageDirection(bool isX) const { return isX ? DVec2{ cos_64, sin_64 } : DVec2{ -sin_64, cos_64 }; }
    [[nodiscard]] DVec2 axisStagePerpDirection(bool isX) const { DVec2 d = axisStageDirection(isX).normalized(); return { -d.y, d.x }; }

    // ----- viewport origin / pan -----

    void cameraToViewport(double left, double top, double right, double bottom);
    void originToCenterViewport();

    [[nodiscard]] DVec2 originPixelOffset() const;
    [[nodiscard]] DVec2 originWorldOffset() const;
    [[nodiscard]] DVec2 panPixelOffset() const;

    // ----- toStage -----

    template<class T = double> requires is_f64<T>  [[nodiscard]] DVec2 toStage(double wx, double wy) const { return DVec2{ m64  * glm::dvec3(wx, wy, 1.0) }; }
    template<class T>          requires is_f128<T> [[nodiscard]] DVec2 toStage(flt128 wx, flt128 wy) const { return DVec2{ m128 * glm::ddvec3(wx, wy, f128(1)) }; }
    template<class T = double> requires is_f64<T>  [[nodiscard]] DVec2 toStage(DVec2 p)  const { return DVec2(m64  * glm::dvec3(p.x, p.y, 1.0)); }
    template<class T>          requires is_f128<T> [[nodiscard]] DVec2 toStage(DDVec2 p) const { return DVec2(m128 * glm::ddvec3(p.x, p.y, f128(1))); }

    // ----- toWorld -----

    template<class T = double> requires is_f64<T>  [[nodiscard]] Vec2<T> toWorld(double sx, double sy) const { return static_cast<DVec2>(  inv_m64  * glm::dvec3(sx, sy, 1.0) ); }
    template<class T>          requires is_f128<T> [[nodiscard]] Vec2<T> toWorld(double sx, double sy) const { return static_cast<DDVec2>( inv_m128 * glm::ddvec3(f128(sx), f128(sy), f128(1)) ); }
    template<class T = double> requires is_f64<T>  [[nodiscard]] Vec2<T> toWorld(DVec2 p)              const { return static_cast<DVec2>(  inv_m64  * glm::dvec3(p.x, p.y, 1.0) ); }
    template<class T>          requires is_f128<T> [[nodiscard]] Vec2<T> toWorld(DVec2 p)              const { return static_cast<DDVec2>( inv_m128 * glm::ddvec3(f128(p.x), f128(p.y), f128(1)) ); }

    // ----- world (f64) => stage (f64) -----
    template<class T = double> requires is_f64<T>
    [[nodiscard]] DVec2 worldToStageOffset(DVec2 o) const {
        return {
            (o.x * cos_64 - o.y * sin_64) * zoom_x_64,
            (o.y * cos_64 + o.x * sin_64) * zoom_y_64
        };
    }

    // ----- world (f128) => stage (f64) -----
    template<class T> requires is_f128<T>
    [[nodiscard]] DVec2 worldToStageOffset(const DDVec2& o) const {
        return DVec2{
            (double)((o.x * cos_128 - o.y * sin_128) * zoom_x),
            (double)((o.y * cos_128 + o.x * sin_128) * zoom_y)
        };
    }

    // ----- stageToWorldOffset -----

        /// --- stage (f64) => world (f32) ---
    template<class T> requires is_f32<T>
    [[nodiscard]] DVec2 stageToWorldOffset(DVec2 o) const {
        return Vec2<T>{
            (float)((o.x * cos_64 + o.y * sin_64) / zoom_x_64),
            (float)((o.y * cos_64 - o.x * sin_64) / zoom_y_64)
        };
    }

    /// --- stage (f64) => world (f64) ---
    template<class T = double> requires is_f64<T>
    [[nodiscard]] DVec2 stageToWorldOffset(DVec2 o) const {
        return Vec2<T>{
            (o.x * cos_64 + o.y * sin_64) / zoom_x_64,
            (o.y * cos_64 - o.x * sin_64) / zoom_y_64
        };
    }

    /// --- stage (f64) => world (f126) ---
    template<class T> requires is_f128<T>
    [[nodiscard]] Vec2<T> stageToWorldOffset(DVec2 o) const {
        return {
            (o.x * cos_128 + o.y * sin_128) / zoom_x,
            (o.y * cos_128 - o.x * sin_128) / zoom_y
        };
    }

    template<typename T=double>
    [[nodiscard]] Vec2<T> stageToWorldOffset(double sx, double sy) const {
        return stageToWorldOffset<T>({ sx, sy });
    }

    // ----- toWorldRect -----

    [[nodiscard]] DRect toWorldRect(const DRect& r) const {
        DVec2 tl = toWorld(r.x1, r.y1);
        DVec2 br = toWorld(r.x2, r.y2);
        return { tl.x, tl.y, br.x, br.y };
    }

    template<typename T = double>
    [[nodiscard]] Rect<T> toWorldRect(double x1, double y1, double x2, double y2) const {
        Vec2<T> tl = toWorld<T>(x1, y1);
        Vec2<T> br = toWorld<T>(x2, y2);
        return { tl.x, tl.y, br.x, br.y };
    }

    // ----- toWorldQuad -----

    template<typename T = double> Quad<T> toWorldQuad(DVec2 a, DVec2 b, DVec2 c, DVec2 d) const { return Quad<T>(toWorld<T>(a), toWorld<T>(b), toWorld<T>(c), toWorld<T>(d)); }
    template<typename T = double> Quad<T> toWorldQuad(const DQuad& quad) const { return toWorldQuad<T>(quad.a, quad.b, quad.c, quad.d); }
    template<typename T = double> Quad<T> toWorldQuad(double x1, double y1, double x2, double y2) const { return toWorldQuad<T>(DQuad(x1, y1, x2, y2)); }
    template<typename T = double> Quad<T> toWorldQuad(const DRect& r) const { return toWorldQuad<T>(static_cast<DQuad>(r)); }

    // ----- toStageSize -----

    [[nodiscard]] DVec2 toStageSize(const DVec2& size) const { return DDVec2{ size.x * zoom_x, size.y * zoom_y }; }
    [[nodiscard]] DVec2 toStageSize(double w, double h) const { return DDVec2{ w * zoom_x, h * zoom_y }; }

    // ----- toStageRect -----

    [[nodiscard]] DRect toStageRect(double x0, double y0, double x1, double y1) const { return { toStage(x0, y0), toStage(x1, y1) }; }
    [[nodiscard]] DRect toStageRect(DVec2 pt1, DVec2 pt2)                       const { return { toStage(pt1), toStage(pt2) }; }

    // ----- toStageQuad -----

    [[nodiscard]] DQuad toStageQuad(DVec2 a, DVec2 b, DVec2 c, DVec2 d) const { return { toStage(a), toStage(b), toStage(c), toStage(d) }; }

    void setDirectCameraPanning(bool b) { direct_cam_panning = b; }
    void panBegin(int _x, int _y, double touch_dist, double touch_angle);
    bool panDrag(int _x, int _y, double touch_dist, double touch_angle);
    void panEnd();
    bool panZoomProcess();

    [[nodiscard]] std::vector<FingerInfo> pressedFingers() {
        return fingers;
    }

    [[nodiscard]] double touchAngle() const {
        if (fingers.size() >= 2) return std::atan2(fingers[1].y - fingers[0].y, fingers[1].x - fingers[0].x);
        return 0.0;
    }
    [[nodiscard]] double touchDist() const {
        if (fingers.size() >= 2) {
            double dx = fingers[1].x - fingers[0].x;
            double dy = fingers[1].y - fingers[0].y;
            return std::sqrt(dx * dx + dy * dy);
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
};

class CameraViewController
{
public:
    union {
        struct { flt128 x; flt128 y; };
        Vec2<flt128> pos{ flt128{0}, flt128{0} };
    };

private:

    double angle_degrees = 0.0;
    double avg_real_zoom = 1.0;

    flt128 init_cam_x = x;
    flt128 init_cam_y = y;
    double init_degrees = angle_degrees;
    flt128 init_cam_zoom = f128(1.0);
    DVec2  init_cam_zoom_xy = zoom_xy;

    flt128 old_x = f128(0);
    flt128 old_y = f128(0.0);
    double old_angle = 0.0;
    flt128 old_zoom = f128(1.0);
    DVec2  old_zoom_xy = DVec2(1, 1);

public:

    double angle = 0.0;
    flt128 zoom = f128(1.0);
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

    flt128 getPositionPrecision() const
    {
        return Math::precisionFromDecimalPlaces<flt128>(getPositionDecimalPlaces());
    }

    void read(const Camera* camera)
    {
        x = camera->x<flt128>();
        y = camera->y<flt128>();
        angle = camera->rotation();
        zoom = (camera->getRelativeZoom<flt128>().x / f128(zoom_xy.x));
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
            zoom = (camera->getRelativeZoom<flt128>().x / f128(zoom_xy.x));
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

inline DVec2 Camera::originWorldOffset() const
{
    // zoom_x/zoom_y refer to world zoom, not stage zoom? bug if camera rotated?
    return originPixelOffset() / DDVec2(zoom_x, zoom_y);
}

inline DVec2 Camera::panPixelOffset() const
{
    return DVec2(pan_x, pan_y);
}


BL_END_NS
