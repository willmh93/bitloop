#pragma once
#include <type_traits>
#include <cmath>

#if defined(__wasm_simd128__)
  #include <wasm_simd128.h>
#endif
#if defined(__ARM_NEON) || defined(__ARM_NEON__)
  #include <arm_neon.h>
#endif
#if defined(__SSE2__) || (defined(_M_IX86_FP) && _M_IX86_FP >= 2) || defined(__x86_64__)
  #include <immintrin.h>
#endif

BL_BEGIN_NS;

namespace simd2 {

// --------- feature tags (always declared)
struct wasm_tag {};
struct neon_tag {};
struct sse_tag {};
struct scalar_tag {};

// pick active tag
using tag_t =
#if defined(__wasm_simd128__)
  wasm_tag;
#elif defined(__ARM_NEON) || defined(__ARM_NEON__)
  neon_tag;
#elif defined(__SSE2__) || (defined(_M_IX86_FP) && _M_IX86_FP >= 2) || defined(__x86_64__)
  sse_tag;
#else
  scalar_tag;
#endif

// primary template
template<class T, class TAG = tag_t> struct v2;

// ========================= float specialization =========================

// ---- float / WASM (vector-only cmul/dot)
#if defined(__wasm_simd128__)
template<> struct v2<float, wasm_tag> {
  using V = v128_t;
  V v;

  static inline v2 set(float x,float y){ return { wasm_f32x4_make(x,y,0,0) }; }
  static inline v2 load2(const float* p){ return set(p[0],p[1]); }
  inline void store2(float* p) const { p[0]=wasm_f32x4_extract_lane(v,0); p[1]=wasm_f32x4_extract_lane(v,1); }

  static inline v2 add(v2 a,v2 b){ return { wasm_f32x4_add(a.v,b.v) }; }
  static inline v2 sub(v2 a,v2 b){ return { wasm_f32x4_sub(a.v,b.v) }; }
  static inline v2 mul(v2 a,v2 b){ return { wasm_f32x4_mul(a.v,b.v) }; }
  static inline v2 fma(v2 a,v2 b,v2 c){ return add(mul(a,b), c); } // no native fma

  // helpers
  static inline V swapxy(V x){ return wasm_i32x4_shuffle(x,x, 1,0, 3,2); } // (y,x,*,*)
  static inline float hsum2(V x){ V s = wasm_f32x4_add(x, swapxy(x)); return wasm_f32x4_extract_lane(s,0); }

  static inline float dot(v2 a){ return hsum2(wasm_f32x4_mul(a.v,a.v)); }

  // complex multiply without scalar extracts
  // real = ax*bx - ay*by   imag = ax*by + ay*bx
  static inline v2 cmul(v2 a, v2 b){
    V ab    = wasm_f32x4_mul(a.v, b.v);               // (ax*bx, ay*by, 0, 0)
    V bsw   = swapxy(b.v);                             // (by, bx,  *, *)
    V apb   = wasm_f32x4_mul(a.v, bsw);               // (ax*by, ay*bx, 0, 0)
    V realv = wasm_f32x4_sub(ab, swapxy(ab));         // (ax*bx - ay*by, ...)
    V imagv = wasm_f32x4_add(apb, swapxy(apb));       // (ax*by + ay*bx, ...)
    // pack lane0(real), lane0(imag) into lanes 0,1 (other lanes don't matter)
    V out   = wasm_i32x4_shuffle(realv, imagv, 0, 4, 2, 6);
    return { out };
  }

  inline float x() const { return wasm_f32x4_extract_lane(v,0); }
  inline float y() const { return wasm_f32x4_extract_lane(v,1); }
};
#endif

// ---- float / NEON
#if defined(__ARM_NEON) || defined(__ARM_NEON__)
template<> struct v2<float, neon_tag> {
  using V = float32x4_t;
  V v;

  static inline v2 set(float x,float y){ float32x4_t t={x,y,0,0}; return {t}; }
  static inline v2 load2(const float* p){ return set(p[0],p[1]); }
  inline void store2(float* p) const { float tmp[4]; vst1q_f32(tmp,v); p[0]=tmp[0]; p[1]=tmp[1]; }

  static inline v2 add(v2 a,v2 b){ return { vaddq_f32(a.v,b.v) }; }
  static inline v2 sub(v2 a,v2 b){ return { vsubq_f32(a.v,b.v) }; }
  static inline v2 mul(v2 a,v2 b){ return { vmulq_f32(a.v,b.v) }; }
  static inline v2 fma(v2 a,v2 b,v2 c){ return add(mul(a,b), c); }

  static inline float dot(v2 a){
    float32x4_t m = vmulq_f32(a.v,a.v);
    // horizontal add lanes 0+1
    return vgetq_lane_f32(m,0) + vgetq_lane_f32(m,1);
  }

  static inline v2 cmul(v2 a,v2 b){
    // fallback with minimal spills
    float ax=vgetq_lane_f32(a.v,0), ay=vgetq_lane_f32(a.v,1);
    float bx=vgetq_lane_f32(b.v,0), by=vgetq_lane_f32(b.v,1);
    return set(ax*bx - ay*by, ax*by + ay*bx);
  }

  inline float x() const { return vgetq_lane_f32(v,0); }
  inline float y() const { return vgetq_lane_f32(v,1); }
};
#endif

// ---- float / SSE2+
#if defined(__SSE2__) || (defined(_M_IX86_FP) && _M_IX86_FP >= 2) || defined(__x86_64__)
template<> struct v2<float, sse_tag> {
  using V = __m128;
  V v;

  static inline v2 set(float x,float y){ return { _mm_set_ps(0,0,y,x) }; }
  static inline v2 load2(const float* p){ return set(p[0],p[1]); }
  inline void store2(float* p) const { alignas(16) float t[4]; _mm_store_ps(t,v); p[0]=t[0]; p[1]=t[1]; }

  static inline v2 add(v2 a,v2 b){ return { _mm_add_ps(a.v,b.v) }; }
  static inline v2 sub(v2 a,v2 b){ return { _mm_sub_ps(a.v,b.v) }; }
  static inline v2 mul(v2 a,v2 b){ return { _mm_mul_ps(a.v,b.v) }; }
  static inline v2 fma(v2 a,v2 b,v2 c){
  #if defined(__FMA__) || defined(__AVX2__) || defined(__FMA4__)
    return { _mm_fmadd_ps(a.v,b.v,c.v) };
  #else
    return add(mul(a,b), c);
  #endif
  }

  static inline float dot(v2 a){
    __m128 m = _mm_mul_ps(a.v,a.v);
    // sum m0+m1
    alignas(16) float t[4]; _mm_store_ps(t,m); return t[0]+t[1];
  }

  static inline v2 cmul(v2 a,v2 b){
    // compute with shuffles, no stores
    const __m128 a_yx = _mm_shuffle_ps(a.v,a.v,_MM_SHUFFLE(3,2,0,1)); // (ay,ax,*,*)
    const __m128 b_yx = _mm_shuffle_ps(b.v,b.v,_MM_SHUFFLE(3,2,0,1)); // (by,bx,*,*)
    const __m128 ab    = _mm_mul_ps(a.v,b.v);   // (ax*bx, ay*by, 0,0)
    const __m128 apb   = _mm_mul_ps(a.v,b_yx);  // (ax*by, ay*bx, 0,0)
    const __m128 real  = _mm_sub_ps(ab, _mm_shuffle_ps(ab,ab,_MM_SHUFFLE(3,2,0,1)));
    const __m128 imag  = _mm_add_ps(apb,_mm_shuffle_ps(apb,apb,_MM_SHUFFLE(3,2,0,1)));
    // interleave lane0(real), lane0(imag)
    const __m128 out   = _mm_movelh_ps(real, imag);
    return { out };
  }

  inline float x() const { alignas(16) float t[4]; _mm_store_ps(t,v); return t[0]; }
  inline float y() const { alignas(16) float t[4]; _mm_store_ps(t,v); return t[1]; }
};
#endif

// ---- float / scalar
template<> struct v2<float, scalar_tag> {
  float _x,_y;
  static inline v2 set(float x,float y){ return {x,y}; }
  static inline v2 load2(const float* p){ return set(p[0],p[1]); }
  inline void store2(float* p) const { p[0]=_x; p[1]=_y; }
  static inline v2 add(v2 a,v2 b){ return { a._x+b._x, a._y+b._y }; }
  static inline v2 sub(v2 a,v2 b){ return { a._x-b._x, a._y-b._y }; }
  static inline v2 mul(v2 a,v2 b){ return { a._x*b._x, a._y*b._y }; }
  static inline v2 fma(v2 a,v2 b,v2 c){ return { std::fma(a._x,b._x,c._x), std::fma(a._y,b._y,c._y) }; }
  static inline float dot(v2 a){ return a._x*a._x + a._y*a._y; }
  static inline v2 cmul(v2 a,v2 b){ return { a._x*b._x - a._y*b._y, a._x*b._y + a._y*b._x }; }
  inline float x() const { return _x; }
  inline float y() const { return _y; }
};

// ========================= double specialization =========================

// ---- double / WASM (vector-only cmul/dot)
#if defined(__wasm_simd128__)
template<> struct v2<double, wasm_tag> {
  using V = v128_t;
  V v;

  static inline v2 set(double x,double y){ return { wasm_f64x2_make(x,y) }; }
  static inline v2 load2(const double* p){ return set(p[0],p[1]); }
  inline void store2(double* p) const { p[0]=wasm_f64x2_extract_lane(v,0); p[1]=wasm_f64x2_extract_lane(v,1); }

  static inline v2 add(v2 a,v2 b){ return { wasm_f64x2_add(a.v,b.v) }; }
  static inline v2 sub(v2 a,v2 b){ return { wasm_f64x2_sub(a.v,b.v) }; }
  static inline v2 mul(v2 a,v2 b){ return { wasm_f64x2_mul(a.v,b.v) }; }
  static inline v2 fma(v2 a,v2 b,v2 c){ return add(mul(a,b), c); } // no native fma

  static inline V swapxy(V x){ return wasm_i64x2_shuffle(x,x,1,0); }
  static inline double hsum2(V x){ V s = wasm_f64x2_add(x, swapxy(x)); return wasm_f64x2_extract_lane(s,0); }

  static inline double dot(v2 a){ return hsum2(wasm_f64x2_mul(a.v,a.v)); }

  //static inline v2 cmul(v2 a,v2 b){
  //  V ab    = wasm_f64x2_mul(a.v, b.v);           // (ax*bx, ay*by)
  //  V bsw   = swapxy(b.v);                         // (by, bx)
  //  V apb   = wasm_f64x2_mul(a.v, bsw);           // (ax*by, ay*bx)
  //  V realv = wasm_f64x2_sub(ab, swapxy(ab));     // (ax*bx - ay*by, ...)
  //  V imagv = wasm_f64x2_add(apb, swapxy(apb));   // (ax*by + ay*bx, ...)
  //  V out   = wasm_i64x2_shuffle(realv, imagv, 0, 2); // lane0(real), lane0(imag)
  //  return { out };
  //}
  static inline v2 cmul(v2 a, v2 b)
  {
      const double ax = a.x(), ay = a.y();
      const double bx = b.x(), by = b.y();
      return v2::set(ax * bx - ay * by, ax * by + ay * bx);
  }

  inline double x() const { return wasm_f64x2_extract_lane(v,0); }
  inline double y() const { return wasm_f64x2_extract_lane(v,1); }
};
#endif

// ---- double / NEON (AArch64 only for f64 lanes)
#if (defined(__ARM_NEON) || defined(__ARM_NEON__)) && defined(__aarch64__)
template<> struct v2<double, neon_tag> {
  using V = float64x2_t;
  V v;

  static inline v2 set(double x,double y){ return { vsetq_lane_f64(y, vsetq_lane_f64(vdupq_n_f64(0), x, 0), 1) }; }
  static inline v2 load2(const double* p){ return set(p[0],p[1]); }
  inline void store2(double* p) const { p[0]=vgetq_lane_f64(v,0); p[1]=vgetq_lane_f64(v,1); }

  static inline v2 add(v2 a,v2 b){ return { vaddq_f64(a.v,b.v) }; }
  static inline v2 sub(v2 a,v2 b){ return { vsubq_f64(a.v,b.v) }; }
  static inline v2 mul(v2 a,v2 b){ return { vmulq_f64(a.v,b.v) }; }
  static inline v2 fma(v2 a,v2 b,v2 c){ return add(mul(a,b), c); }

  static inline double dot(v2 a){
    float64x2_t m = vmulq_f64(a.v,a.v);
    return vgetq_lane_f64(m,0) + vgetq_lane_f64(m,1);
  }
  static inline v2 cmul(v2 a,v2 b){
    double ax=vgetq_lane_f64(a.v,0), ay=vgetq_lane_f64(a.v,1);
    double bx=vgetq_lane_f64(b.v,0), by=vgetq_lane_f64(b.v,1);
    return set(ax*bx - ay*by, ax*by + ay*bx);
  }

  inline double x() const { return vgetq_lane_f64(v,0); }
  inline double y() const { return vgetq_lane_f64(v,1); }
};
#endif

// ---- double / SSE2+
#if defined(__SSE2__) || defined(__x86_64__)
template<> struct v2<double, sse_tag> {
  using V = __m128d;
  V v;

  static inline v2 set(double x,double y){ return { _mm_set_pd(y,x) }; }
  static inline v2 load2(const double* p){ return set(p[0],p[1]); }
  inline void store2(double* p) const { alignas(16) double t[2]; _mm_store_pd(t,v); p[0]=t[0]; p[1]=t[1]; }

  static inline v2 add(v2 a,v2 b){ return { _mm_add_pd(a.v,b.v) }; }
  static inline v2 sub(v2 a,v2 b){ return { _mm_sub_pd(a.v,b.v) }; }
  static inline v2 mul(v2 a,v2 b){ return { _mm_mul_pd(a.v,b.v) }; }
  static inline v2 fma(v2 a,v2 b,v2 c){
  #if defined(__FMA__) || defined(__AVX2__)
    return { _mm_fmadd_pd(a.v,b.v,c.v) };
  #else
    return add(mul(a,b), c);
  #endif
  }

  static inline double dot(v2 a){
    __m128d m = _mm_mul_pd(a.v,a.v);
    alignas(16) double t[2]; _mm_store_pd(t,m); return t[0]+t[1];
  }

  static inline v2 cmul(v2 a,v2 b){
    // (ax,ay)*(bx,by) = (ax*bx - ay*by, ax*by + ay*bx)
    __m128d b_sw = _mm_shuffle_pd(b.v,b.v,0x1);     // (by,bx)
    __m128d ab   = _mm_mul_pd(a.v, b.v);            // (ax*bx, ay*by)
    __m128d apb  = _mm_mul_pd(a.v, b_sw);           // (ax*by, ay*bx)
    __m128d real = _mm_sub_pd(ab, _mm_shuffle_pd(ab,ab,0x1));
    __m128d imag = _mm_add_pd(apb,_mm_shuffle_pd(apb,apb,0x1));
    // pack lane0(real), lane0(imag)
    __m128d out  = _mm_unpacklo_pd(real, imag);
    return { out };
  }

  inline double x() const { alignas(16) double t[2]; _mm_store_pd(t,v); return t[0]; }
  inline double y() const { alignas(16) double t[2]; _mm_store_pd(t,v); return t[1]; }
};
#endif

// ---- double / scalar
template<> struct v2<double, scalar_tag> {
  double _x,_y;
  static inline v2 set(double x,double y){ return {x,y}; }
  static inline v2 load2(const double* p){ return set(p[0],p[1]); }
  inline void store2(double* p) const { p[0]=_x; p[1]=_y; }
  static inline v2 add(v2 a,v2 b){ return { a._x+b._x, a._y+b._y }; }
  static inline v2 sub(v2 a,v2 b){ return { a._x-b._x, a._y-b._y }; }
  static inline v2 mul(v2 a,v2 b){ return { a._x*b._x, a._y*b._y }; }
  static inline v2 fma(v2 a,v2 b,v2 c){ return { std::fma(a._x,b._x,c._x), std::fma(a._y,b._y,c._y) }; }
  static inline double dot(v2 a){ return a._x*a._x + a._y*a._y; }
  static inline v2 cmul(v2 a,v2 b){ return { a._x*b._x - a._y*b._y, a._x*b._y + a._y*b._x }; }
  inline double x() const { return _x; }
  inline double y() const { return _y; }
};

} // namespace simd2

BL_END_NS
