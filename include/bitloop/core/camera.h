#pragma once
#include "types.h"
#include "debug.h"
#include "input.h"
#include <bitloop/util/math_util.h>

BL_BEGIN_NS

inline glm::ddmat3 glm_ddtranslate(f128 tx, f128 ty) {
    glm::ddmat3 m(1.0);
    m[2][0] = tx; // column 2, row 0
    m[2][1] = ty; // column 2, row 1
    return m;
}

inline glm::ddmat3 glm_ddscale(f128 sx, f128 sy) {
    glm::ddmat3 m(1.0);
    m[0][0] = sx;
    m[1][1] = sy;
    return m;
}

inline glm::ddmat3 glm_ddrotate(f128 r) {
    f128 c = cos(r);
    f128 s = sin(r);
    return glm::ddmat3(
        c, s, 0.0,    // column 0
        -s, c, 0.0,   // column 1
        0.0, 0.0, 1.0 // column 2 (homogeneous)
    );
}

class SurfaceInfo;
class CameraInfo;
class Viewport;
class ProjectBase;
class Event;

template<class T> concept is_f32  = std::is_same_v<T, f32>;
template<class T> concept is_f64  = std::is_same_v<T, f64>;
template<class T> concept is_f128 = std::is_same_v<T, f128>;

class WorldStageTransform
{
    friend class CameraInfo;

    // ────── world <-> stage (128 bit single souce of truth) ──────
    glm::ddmat3 m128     = glm::ddmat3(1.0);
    glm::ddmat3 inv_m128 = glm::ddmat3(1.0);

    // ────── world <-> stage (cached 64-bit versions for performance) ──────
    glm::dmat3 m64     = glm::dmat3(1.0);
    glm::dmat3 inv_m64 = glm::dmat3(1.0);

    void updateCache()
    {
        m64 = static_cast<glm::dmat3>(m128);
        inv_m128 = glm::inverse(m128);
        inv_m64 = static_cast<glm::dmat3>(inv_m128);
    }

public:

    void reset()                         { m128 = glm::ddmat3(1);                           updateCache(); }
    void transform(const glm::dmat3& m)  { m128 *= static_cast<glm::ddmat3>(m);             updateCache(); }
    void transform(const glm::ddmat3& m) { m128 *= m;                                       updateCache(); }
    void translate(f64 x, f64 y)         { m128 *= glm_ddtranslate(x, y);                   updateCache(); }
    void translate(f128 x, f128 y)       { m128 *= glm_ddtranslate(x, y);                   updateCache(); }
    void scale(f64 s)                    { auto s128 = s; m128 *= glm_ddscale(s128, s128);  updateCache(); }
    void scale(f128 s)                   { m128 *= glm_ddscale(s, s);                       updateCache(); }
    void scale(f64 sx, f64 sy)           { m128 *= glm_ddscale(sx, sy);                     updateCache(); }
    void scale(f128 sx, f128 sy)         { m128 *= glm_ddscale(sx, sy);                     updateCache(); }
    void rotate(f64 r)                   { m128 *= glm_ddrotate(r);                         updateCache(); }

    template<class T = f64> requires is_f64<T>  [[nodiscard]] constexpr const glm::dmat3&  stageTransform() const { return m64; }
    template<class T>       requires is_f128<T> [[nodiscard]] constexpr const glm::ddmat3& stageTransform() const { return m128; }
    template<class T = f64> requires is_f64<T>  [[nodiscard]] constexpr const glm::dmat3&  worldTransform() const { return inv_m64; }
    template<class T>       requires is_f128<T> [[nodiscard]] constexpr const glm::ddmat3& worldTransform() const { return inv_m128; }

    constexpr operator const glm::ddmat3&() const { return m128; }

    f64 avgZoomScaleFactor() const
    {
        f64 a = m64[0][0], b = m64[1][0];
        f64 c = m64[0][1], d = m64[1][1];
        return std::sqrt(0.5 * (a * a + b * b + c * c + d * d));
    }

    template<class T = f128>
    Vec2<T> _zoom() const
    {
        const auto& m = stageTransform<T>();
        const T m00 = m[0][0], m01 = m[1][0], m10 = m[0][1], m11 = m[1][1];

        // Column lengths give the local (pre-rotation)
        const T sx = sqrt(m00 * m00 + m10 * m10);
        if (sx == T{ 0 }) return { T{0}, T{0} };

        // Preserve reflection sign on Y via determinant
        const T det = m00 * m11 - m01 * m10;
        const T sy = det / sx;

        return { sx, sy };
    }

    f64 _angle() const { return glm::atan(m64[0][1], m64[0][0]); }

    // ────── toStage ──────
    template<class T>       requires is_f32<T>  [[nodiscard]] DVec2 toStage(f32 wx, f32 wy)    const { return DVec2{m64  * glm::dvec3(wx, wy, 1.0) }; }
    template<class T = f64> requires is_f64<T>  [[nodiscard]] DVec2 toStage(f64 wx, f64 wy)    const { return DVec2{m64  * glm::dvec3(wx, wy, 1.0) }; }
    template<class T>       requires is_f128<T> [[nodiscard]] DVec2 toStage(f128 wx, f128 wy)  const { return DVec2{m128 * glm::ddvec3(wx, wy, 1.0) }; }

    template<class T>       requires is_f32<T>  [[nodiscard]] DVec2 toStage(FVec2 p)           const { return DVec2(m64  * glm::dvec3(p.x, p.y, 1.0)); }
    template<class T = f64> requires is_f64<T>  [[nodiscard]] DVec2 toStage(DVec2 p)           const { return DVec2(m64  * glm::dvec3(p.x, p.y, 1.0)); }
    template<class T>       requires is_f128<T> [[nodiscard]] DVec2 toStage(DDVec2 p)          const { return DVec2(m128 * glm::ddvec3(p.x, p.y, 1.0)); }
                                                                                                        
    // ────── toWorld ──────                                                                            
    template<class T>       requires is_f32<T>  [[nodiscard]] Vec2<T> toWorld(f64 sx, f64 sy)  const { return static_cast<FVec2>(  inv_m64  * glm::dvec3(sx, sy, 1.0)); }
    template<class T = f64> requires is_f64<T>  [[nodiscard]] Vec2<T> toWorld(f64 sx, f64 sy)  const { return static_cast<DVec2>(  inv_m64  * glm::dvec3(sx, sy, 1.0)); }
    template<class T>       requires is_f128<T> [[nodiscard]] Vec2<T> toWorld(f64 sx, f64 sy)  const { return static_cast<DDVec2>( inv_m128 * glm::ddvec3(sx, sy, 1.0)); }

    template<class T>       requires is_f32<T>  [[nodiscard]] Vec2<T> toWorld(DVec2 p)         const { return static_cast<FVec2>(  inv_m64  * glm::dvec3(p.x, p.y, 1.0) ); }
    template<class T = f64> requires is_f64<T>  [[nodiscard]] Vec2<T> toWorld(DVec2 p)         const { return static_cast<DVec2>(  inv_m64  * glm::dvec3(p.x, p.y, 1.0) ); }
    template<class T>       requires is_f128<T> [[nodiscard]] Vec2<T> toWorld(DVec2 p)         const { return static_cast<DDVec2>( inv_m128 * glm::ddvec3(p.x, p.y, 1.0)); }
                                                                                                        
    // ────── worldToStageOffset ──────                                                                 
    template<class T>       requires is_f32<T>  [[nodiscard]] DVec2 toStageOffset(FVec2 o)     const { return static_cast<DVec2>(  m64  * glm::dvec3(o.x, o.y, 0.0)); }
    template<class T = f64> requires is_f64<T>  [[nodiscard]] DVec2 toStageOffset(DVec2 o)     const { return static_cast<DVec2>(  m64  * glm::dvec3(o.x, o.y, 0.0)); }
    template<class T>       requires is_f128<T> [[nodiscard]] DVec2 toStageOffset(DDVec2 o)    const { return static_cast<DDVec2>( m128 * glm::ddvec3(o.x, o.y, 0.0)); };

    // ────── stageToWorldOffset ──────                                                                 
    template<class T>       requires is_f32<T>  [[nodiscard]] FVec2  toWorldOffset(DVec2 o)    const { return static_cast<FVec2>(  inv_m64  * glm::dvec3(o.x, o.y, 0.0)); }
    template<class T = f64> requires is_f64<T>  [[nodiscard]] DVec2  toWorldOffset(DVec2 o)    const { return static_cast<DVec2>(  inv_m64  * glm::dvec3(o.x, o.y, 0.0)); }
    template<class T>       requires is_f128<T> [[nodiscard]] DDVec2 toWorldOffset(DVec2 o)    const { return static_cast<DDVec2>( inv_m128 * glm::ddvec3(o.x, o.y, 0.0)); };

    template<typename T=f64>
    [[nodiscard]] Vec2<T> toWorldOffset(f64 sx, f64 sy) const {
        return toWorldOffset<T>({ sx, sy });
    }

    // ────── toWorldRect ──────
    [[nodiscard]] DRect toWorldRect(const DRect& r) const {
        DVec2 tl = toWorld(r.x1, r.y1);
        DVec2 br = toWorld(r.x2, r.y2);
        return { tl.x, tl.y, br.x, br.y };
    }

    template<typename T = f64>
    [[nodiscard]] Rect<T> toWorldRect(f64 x1, f64 y1, f64 x2, f64 y2) const {
        Vec2<T> tl = toWorld<T>(x1, y1);
        Vec2<T> br = toWorld<T>(x2, y2);
        return { tl.x, tl.y, br.x, br.y };
    }

    // ────── toWorldQuad ──────
    template<class T = f64> Quad<T> toWorldQuad(DVec2 a, DVec2 b, DVec2 c, DVec2 d)  const { return Quad<T>(toWorld<T>(a), toWorld<T>(b), toWorld<T>(c), toWorld<T>(d)); }
    template<class T = f64> Quad<T> toWorldQuad(const DQuad& quad)                   const { return toWorldQuad<T>(quad.a, quad.b, quad.c, quad.d); }
    template<class T = f64> Quad<T> toWorldQuad(f64 x1, f64 y1, f64 x2, f64 y2)      const { return toWorldQuad<T>(DQuad(x1, y1, x2, y2)); }
    template<class T = f64> Quad<T> toWorldQuad(const DRect& r)                      const { return toWorldQuad<T>(static_cast<DQuad>(r)); }

    // ────── DVec2 toStageSize<INPUT_WORLD_PRECISION>(...) ──────
    template<class T>       requires is_f128<T> [[nodiscard]] DVec2 toStageSizeAABB(Vec2<T> wh) const {
        auto a = m128[0][0], b = m128[1][0];
        auto c = m128[0][1], d = m128[1][1];
        auto nw = abs(a) * wh.x + abs(b) * wh.y;
        auto nh = abs(c) * wh.x + abs(d) * wh.y;
        return { (f64)nw, (f64)nh };
    }
    template<class T = f64> requires is_f64<T>  [[nodiscard]] DVec2 toStageSizeAABB(Vec2<T> wh) const {
        f64 a = m64[0][0], b = m64[1][0];
        f64 c = m64[0][1], d = m64[1][1];
        f64 nw = std::abs(a) * wh.x + std::abs(b) * wh.y;
        f64 nh = std::abs(c) * wh.x + std::abs(d) * wh.y;
        return { nw, nh };
    }
    template<class T>       requires is_f32<T>  [[nodiscard]] DVec2 toStageSizeAABB(Vec2<T> wh) const {
        return toStageSizeAABB((DVec2)wh);
    }

    template<class T>       requires is_f128<T> [[nodiscard]] DVec2 toStageSideLengths(Vec2<T> wh) const {
        auto a = m128[0][0], b = m128[1][0];
        auto c = m128[0][1], d = m128[1][1];
        auto sx = sqrt(a * a + b * b);
        auto sy = sqrt(c * c + d * d);
        return { (f64)(sx * wh.x), (f64)(sy * wh.y) };
    }
    template<class T = f64> requires is_f64<T>  [[nodiscard]] DVec2 toStageSideLengths(Vec2<T> wh) const {
        f64 a = m64[0][0], b = m64[1][0];
        f64 c = m64[0][1], d = m64[1][1];
        f64 sx = sqrt(a * a + b * b);
        f64 sy = sqrt(c * c + d * d);
        return { sx * wh.x, sy * wh.y };
    }
    template<class T>       requires is_f32<T>  [[nodiscard]] DVec2 toStageSideLengths(Vec2<T> wh) const {
        return toStageSideLengths((DVec2)wh);
    }

    // ────── DRect toStageRect<INPUT_WORLD_PRECISION>(...) ──────
    template<class T = f64> [[nodiscard]] DRect toStageRect(T x0, T y0, T x1, T y1)   const { return { toStage<T>(x0, y0), toStage<T>(x1, y1) }; }
    template<class T = f64> [[nodiscard]] DRect toStageRect(Vec2<T> pt1, Vec2<T> pt2) const { return { toStage<T>(pt1), toStage<T>(pt2) }; }

    // ────── toStageQuad ──────
    template<class T> [[nodiscard]] DQuad toStageQuad(Vec2<T> a, Vec2<T> b, Vec2<T> c, Vec2<T> d) const { return { toStage<T>(a),   toStage<T>(b),   toStage<T>(c),   toStage<T>(d)   }; }
    template<class T> [[nodiscard]] DQuad toStageQuad(const Quad<T> &q)                           const { return { toStage<T>(q.a), toStage<T>(q.b), toStage<T>(q.c), toStage<T>(q.d) }; }

    // ────── axis ──────
    [[nodiscard]] DVec2 axisStageDirection(bool isX) const
    {
        const f64 m00 = m64[0][0], m01 = m64[0][1];
        const f64 m10 = m64[1][0], m11 = m64[1][1];

        if (isX) {
            const f64 sx = std::hypot(m00, m10);
            if (sx == 0) return { 0, 0 };
            return { m00 / sx, -m10 / sx };
        }
        else {
            const f64 sy = std::hypot(m01, m11);
            if (sy == 0) return { 0, 0 };
            return { m01 / sy, -m11 / sy };
        }
    }
    [[nodiscard]] DVec2 axisStagePerpDirection(bool isX) const { DVec2 d = axisStageDirection(isX).normalized(); return { -d.y, d.x }; }
};


class CameraInfo
{
    // ─────── camera data ────────────────────────────────────────────────────────────────────────────────────────────────
    union { struct { f128 x_128;     f128 y_128; };     DDVec2 pos_128    { 0, 0 }; };
    union { struct { f64  x_64;      f64  y_64;  };     DVec2  pos_64     { 0, 0 }; };
    union { struct { f64  sx_64;     f64  sy_64; };     DVec2  stretch_64 { 1, 1 }; };
    union { struct { f64  pan_x;     f64  pan_y; };     DVec2  cam_pan    { 0, 0 }; };
    union { struct { f128 zoom_x;    f128 zoom_y; };    DDVec2 zoom_xy    { 1, 1 }; };
    union { struct { f64  zoom_x_64; f64  zoom_y_64; }; DVec2  zoom_xy_64 { 1, 1 }; };

    f128  zoom_128 = 1;
    f64   zoom_64 = 1;
    f64   rotation_64 = 0;

    SurfaceInfo* surface = nullptr;
    DVec2 viewport_anchor{ 0.5, 0.5 };
    f128  ref_zoom = 1;

    // UI
    DDVec2 init_pos{ 0, 0 };
    f128   init_zoom = zoom_128;
    DVec2  init_stretch = stretch_64;
    f64    init_rotation = rotation_64;
    bool   ui_using_relative_zoom = false;

    // rather than return a new transform with each const getTransform call, reset and rebuild when dirty
    mutable bool is_dirty = false;
    mutable WorldStageTransform* t = nullptr;
    void dirty() { is_dirty = true; }

    // helpers for updating cache variables and flagging transform as dirty
    void posDirty()  { pos_64 = (DVec2)pos_128; dirty(); }
    void zoomDirty() { zoom_xy = (stretch_64*zoom_128);  zoom_xy_64 = (DVec2)zoom_xy;  zoom_64 = (f64)zoom_128;  dirty(); }
    void panDirty()  { dirty(); }


public:

    CameraInfo() {}
    CameraInfo(const CameraInfo& rhs) { *this = rhs; }
    ~CameraInfo() { if (t) delete t; }

    bool operator ==(const CameraInfo& rhs) const;
    CameraInfo& operator =(const CameraInfo& rhs);

    void setSurface(SurfaceInfo* s) { surface = s; }
    const WorldStageTransform& getTransform() const;

    // ─────── f64 getters ────────────────────────────────────────────────────────────────────────────────────────────────
    [[nodiscard]] constexpr f64 panX()       const  { return pan_x; }
    [[nodiscard]] constexpr f64 panY()       const  { return pan_y; }
    [[nodiscard]] constexpr f64 rotation()   const  { return rotation_64; }
    [[nodiscard]] constexpr f64 stretchX()   const  { return sx_64; }
    [[nodiscard]] constexpr f64 stretchY()   const  { return sy_64; }
    [[nodiscard]] constexpr DVec2 stretch()  const  { return stretch_64; }

    // ─────── f32 / f64 / f128 getters ─────────────────────────────────────────────────────────────────────────────────────────
    template<class T>       requires is_f32<T>  [[nodiscard]] constexpr f32    x() const { return (f32)x_64; }
    template<class T = f64> requires is_f64<T>  [[nodiscard]] constexpr f64    x() const { return x_64; }
    template<class T>       requires is_f128<T> [[nodiscard]] constexpr f128   x() const { return x_128; }
    template<class T>       requires is_f32<T>  [[nodiscard]] constexpr f32    y() const { return (f32)y_64; }
    template<class T = f64> requires is_f64<T>  [[nodiscard]] constexpr f64    y() const { return y_64; }
    template<class T>       requires is_f128<T> [[nodiscard]] constexpr f128   y() const { return y_128; }
    template<class T>       requires is_f32<T>  [[nodiscard]] constexpr FVec2  pos() const { return (FVec2)pos_64; }
    template<class T = f64> requires is_f64<T>  [[nodiscard]] constexpr DVec2  pos() const { return pos_64; }
    template<class T>       requires is_f128<T> [[nodiscard]] constexpr DDVec2 pos() const { return pos_128; }
    template<class T>       requires is_f32<T>  [[nodiscard]] constexpr f32    zoom() const { return (f32)zoom_64; }
    template<class T = f64> requires is_f64<T>  [[nodiscard]] constexpr f64    zoom() const { return zoom_64; }
    template<class T>       requires is_f128<T> [[nodiscard]] constexpr f128   zoom() const { return zoom_128; }

    /// includes stretch
    template<class T = f64> requires is_f64<T>  [[nodiscard]] constexpr f64    zoomX() const { return zoom_x_64; } 
    template<class T>       requires is_f128<T> [[nodiscard]] constexpr f128   zoomX() const { return zoom_x; }   
    template<class T = f64> requires is_f64<T>  [[nodiscard]] constexpr f64    zoomY() const { return zoom_y_64; }
    template<class T>       requires is_f128<T> [[nodiscard]] constexpr f128   zoomY() const { return zoom_y; }   

    // ─────── f128 setters ───────────────────────────────────────────────────────────────────────────────────────────────
    bool setX(f128 x)           { if (x_128 != x)        { x_128 = x;         posDirty();  return true; }  return false; }
    bool setY(f128 y)           { if (y_128 != y)        { y_128 = y;         posDirty();  return true; }  return false; }                                                                                               
    bool setPos(f128 x, f128 y) { if (!pos_128.eq(x, y)) { pos_128 = {x, y};  posDirty();  return true; }  return false; }
    bool setZoom(f128 z)        { if (zoom_128 != z)     { zoom_128 = z;      zoomDirty(); return true; }  return false; }
                                                
    // ─────── f64 setters ────────────────────────────────────────────────────────────────────────────────────────────────
    bool setX(f64 x)            { if (x_64  != x)        { x_128  = x;        posDirty();  return true; }  return false; }
    bool setY(f64 y)            { if (y_64  != y)        { y_128  = y;        posDirty();  return true; }  return false; }
    bool setZoom(f64 z)         { if (zoom_64 != z)      { zoom_128 = z;      zoomDirty(); return true; }  return false; }
    bool setStretchX(f64 x)     { if (sx_64 != x)        { sx_64 = x;         zoomDirty(); return true; }  return false; }
    bool setStretchY(f64 y)     { if (sy_64 != y)        { sy_64 = y;         zoomDirty(); return true; }  return false; }
    bool setStretch(DVec2 s)    { if (stretch_64 != s)   { stretch_64 = s;    zoomDirty(); return true; }  return false; }                                                                                              
    bool setPos(f64 x, f64 y)   { if (!pos_64.eq(x, y))  { pos_128 = {x, y};  posDirty();  return true; }  return false; }
    bool setPos(DVec2 p)        { if (pos_64 != p)       { pos_128 = p;       posDirty();  return true; }  return false; }                                                                                                    
    bool setPan(f64 x, f64 y)   { if (!cam_pan.eq(x, y)) { cam_pan = {x, y};  panDirty();  return true; }  return false; }
    bool setRotation(f64 r)     { if (rotation_64 != r)  { rotation_64 = r;   panDirty();  return true; }  return false; }

    // ────── world rect focusing ─────────────────────────────────────────────────────────────────────────────────────────
    void focusWorldRect(f128 x0, f128 y0, f128 x1, f128 y1, bool stretch = false);
    void focusWorldRect(f64 x0, f64 y0, f64 x1, f64 y1, bool stretch = false)
       { focusWorldRect(f128(x0), f128(y0), f128(x1), f128(y1), stretch); }
    void focusWorldRect(const DDRect& r, bool stretch = false)
       { focusWorldRect(r.x1, r.y1, r.x2, r.y2, stretch); }
    void focusWorldRect(const DRect& r, bool stretch = false)
       { focusWorldRect(f128(r.x1), f128(r.y1), f128(r.x2), f128(r.y2), stretch); }
    //void focusWorldRect();  // todo: Use current view as the focus rect

    // ────── relative zoom ───────────────────────────────────────────────────────────────────────────────────────────────
    /// Once focusWorldRect called, use it as a "reference" for future scaling relative to that reference zoom
    /// e.g. setRelativeZoom(2)  will zoom 2x relative to focused rect
    template<typename T> void setReferenceZoom(T _ref_zoom) { ref_zoom = _ref_zoom; }
    template<typename T> [[nodiscard]] T getReferenceZoom() const { return T{ref_zoom}; }
    template<typename T> [[nodiscard]] T relativeZoom() const noexcept { return T{zoom_128 / ref_zoom}; }

    DVec2 viewportStageSize() const;
    template<typename T = f64> Vec2<T> viewportWorldSize() const { return Vec2<T>(viewportStageSize()) / getReferenceZoom<T>(); }
    template<typename T = f64> AngledRect<T> worldAngledRect() const {
        return AngledRect<T>(pos<T>(), viewportWorldSize<T>(), (T)rotation());
    }
    template<typename T = f64> Quad<T> worldQuad() const {
        return static_cast<Quad<T>>(AngledRect<T>(pos<T>(), viewportWorldSize<T>(), (T)rotation()));
    }

    // Set zoom (relative to the reference)
    void setRelativeZoom(f64 rel_zoom) { zoom_128 = ref_zoom * rel_zoom; zoomDirty(); }
    void setRelativeZoom(f128 rel_zoom) { zoom_128 = ref_zoom * rel_zoom; zoomDirty(); }
    
    // ────── viewport origin ─────────────────────────────────────────────────────────────────────────────────────────────
    void setOriginViewportAnchor(f64 ax, f64 ay);
    void setOriginViewportAnchor(Anchor anchor);
    void cameraToViewport(f64 left, f64 top, f64 right, f64 bottom);
    void originToCenterViewport();

    [[nodiscard]] DVec2 originPixelOffset() const;
    [[nodiscard]] DVec2 originWorldOffset() const;
    [[nodiscard]] DVec2 panPixelOffset() const { return DVec2(pan_x, pan_y); }


    // ────── UI ──────────────────────────────────────────────────────────────────────────────────────────────────────────

public:

    void populateUI(DRect restrict_world_rect = DRect::max_extent());
    int  getPositionDecimals()  const { return 1 + Math::countWholeDigits(zoom_128 * 5); }
    f128 getPositionPrecision() const { return Math::precisionFromDecimals<f128>(getPositionDecimals()); }

    void uiSetUsingActualZoom() { ui_using_relative_zoom = false; }
    void uiSetUsingRelativeZoom() { ui_using_relative_zoom = true; }
    void uiSetCurrentAsDefault()
    {
        init_pos.set(x_128, y_128);
        init_stretch = stretch_64;
        init_rotation = rotation_64;
        init_zoom = relativeZoom<f128>();
    }

    // ────── tween ───────────────────────────────────────────────────────────────────────────────────────────────────────
    static void lerp(CameraInfo& dst, const CameraInfo& a, const CameraInfo& b, f64 lerp_factor)
    {
        dst.setX(Math::lerp(a.x<f128>(), b.x<f128>(), lerp_factor));
        dst.setY(Math::lerp(a.y<f128>(), b.y<f128>(), lerp_factor));
        dst.setRotation(Math::lerpAngle(a.rotation(), b.rotation(), lerp_factor));
        dst.setZoom(Math::lerp(a.zoom<f128>(), b.zoom<f128>(), lerp_factor));
        dst.setStretch(Math::lerp(a.stretch(), b.stretch(), lerp_factor));
    }
};

struct CameraNavigator
{
    CameraInfo* camera = nullptr;

    void setTarget(CameraInfo& cam)
    {
        camera = &cam;
    }

    // ────── Pan attributes ──────
    int pan_down_touch_x = 0;
    int pan_down_touch_y = 0;
    f64 pan_down_touch_dist = 0;
    f64 pan_down_touch_angle = 0;

    f128 pan_beg_cam_x = 0;
    f128 pan_beg_cam_y = 0;
    f128 pan_beg_cam_zoom = 0;
    f64  pan_beg_cam_angle = 0;

    f128 min_zoom = -1e+300; // Relative to "focused" rect
    f128 max_zoom = 1e+300; // Relative to "focused" rect

    bool panning = false;
    bool direct_cam_panning = true;

    std::vector<FingerInfo> fingers;

    // Clamp the relative zoom range (relative to the reference)
    void restrictRelativeZoomRange(f64 min, f64 max);

    void setDirectCameraPanning(bool b) { direct_cam_panning = b; }
    void panBegin(int _x, int _y, f64 touch_dist, f64 touch_angle);
    bool panDrag(int _x, int _y, f64 touch_dist, f64 touch_angle);
    void panEnd();
    bool panZoomProcess();


    [[nodiscard]] std::vector<FingerInfo> pressedFingers() const {
        return fingers;
    }

    [[nodiscard]] f64 touchAngle() const {
        if (fingers.size() >= 2) return std::atan2(fingers[1].y - fingers[0].y, fingers[1].x - fingers[0].x);
        return 0.0;
    }
    [[nodiscard]] f64 touchDist() const {
        if (fingers.size() >= 2) {
            f64 dx = fingers[1].x - fingers[0].x;
            f64 dy = fingers[1].y - fingers[0].y;
            return std::sqrt(dx * dx + dy * dy);
        }
        return 0.0;
    }

    [[nodiscard]] constexpr bool isPanning()         const { return panning; }
    [[nodiscard]] constexpr f64  panDownTouchDist()  const { return pan_down_touch_dist; }
    [[nodiscard]] constexpr f64  panDownTouchAngle() const { return pan_down_touch_angle; }
    [[nodiscard]] constexpr f64  panDownTouchX()     const { return pan_down_touch_x; }
    [[nodiscard]] constexpr f64  panDownTouchY()     const { return pan_down_touch_y; }
    [[nodiscard]] constexpr f128 panBegCamX()        const { return pan_beg_cam_x; }
    [[nodiscard]] constexpr f128 panBegCamY()        const { return pan_beg_cam_y; }
    [[nodiscard]] constexpr f128 panBegCamZoom()     const { return pan_beg_cam_zoom; }
    [[nodiscard]] constexpr f64  panBegCamAngle()    const { return pan_beg_cam_angle; }

    bool handleWorldNavigation(Event e, bool single_touch_pan, bool zoom_anchor_mouse = false);

    void debugPrint(Viewport* ctx) const;
};


BL_END_NS
