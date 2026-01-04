#pragma once
#ifndef BL_F256_H
#define BL_F256_H

// Standalone quad-double (approx. 256-bit) floating-point utility.
// Printing / string conversion intentionally omitted.

#include <cmath>
#include <cstdint>
#include <limits>
#include <type_traits>

#ifndef FORCE_INLINE
    #if defined(_MSC_VER)
        #define FORCE_INLINE __forceinline
    #elif defined(__GNUC__) || defined(__clang__)
        #define FORCE_INLINE inline __attribute__((always_inline))
    #else
        #define FORCE_INLINE inline
    #endif
#endif

#ifndef FAST_INLINE
    #define FAST_INLINE FORCE_INLINE
#endif

#ifndef BL_PUSH_PRECISE
    #if defined(_MSC_VER)
        #define BL_PUSH_PRECISE __pragma(float_control(precise, on, push))
        #define BL_POP_PRECISE  __pragma(float_control(pop))
    #elif defined(__clang__)
        #define BL_PUSH_PRECISE _Pragma("clang fp contract(off)") _Pragma("clang fp reassociate(off)")
        #define BL_POP_PRECISE
    #elif defined(__GNUC__)
        #define BL_PUSH_PRECISE _Pragma("GCC optimize (\"no-fast-math\")")
        #define BL_POP_PRECISE
    #else
        #define BL_PUSH_PRECISE
        #define BL_POP_PRECISE
    #endif
#endif

#ifndef BL_FAST_MATH
    #if defined(__FAST_MATH__)
        #define BL_FAST_MATH 1
    #else
        #define BL_FAST_MATH 0
    #endif
#endif

// For now, disabling std::fma seems to offer a large performance boost on web
#ifndef FMA_AVAILABLE
    #ifndef __EMSCRIPTEN__
        #if defined(__FMA__) || defined(__FMA4__) || defined(_MSC_VER) || defined(__clang__) || defined(__GNUC__)
            #define FMA_AVAILABLE
        #endif
    #endif
#endif

namespace bl
{
    struct f256;

    namespace f256_detail
    {
        static constexpr double kSplitter = 134217729.0; // 2^27 + 1

        FORCE_INLINE double absd(double x) { return x < 0.0 ? -x : x; }

        FORCE_INLINE void two_sum(double a, double b, double& s, double& e)
        {
            s = a + b;
            const double bb = s - a;
            e = (a - (s - bb)) + (b - bb);
        }

        FORCE_INLINE void quick_two_sum(double a, double b, double& s, double& e)
        {
            s = a + b;
            e = b - (s - a);
        }

        FORCE_INLINE void two_diff(double a, double b, double& s, double& e)
        {
            s = a - b;
            const double bb = a - s;
            e = (a - (s + bb)) + (bb - b);
        }

        FORCE_INLINE void split(double a, double& hi, double& lo)
        {
            const double t = kSplitter * a;
            hi = t - (t - a);
            lo = a - hi;
        }

        FORCE_INLINE void two_prod_dekker(double a, double b, double& p, double& e)
        {
            p = a * b;
            double a_hi, a_lo, b_hi, b_lo;
            split(a, a_hi, a_lo);
            split(b, b_hi, b_lo);
            e = ((a_hi * b_hi - p) + a_hi * b_lo + a_lo * b_hi) + a_lo * b_lo;
        }

        FORCE_INLINE void two_prod_fma(double a, double b, double& p, double& e)
        {
            p = a * b;
            e = std::fma(a, b, -p);
        }

        FORCE_INLINE void two_prod(double a, double b, double& p, double& e)
        {
#ifdef FMA_AVAILABLE
            two_prod_fma(a, b, p, e);
#else
            two_prod_dekker(a, b, p, e);
#endif
        }

        FORCE_INLINE void renorm4(double& a0, double& a1, double& a2, double& a3)
        {
            // Assumes |a0| >= |a1| >= |a2| >= |a3| approximately.
            double s, e;

            quick_two_sum(a2, a3, s, e); a2 = s; a3 = e;
            quick_two_sum(a1, a2, s, e); a1 = s; a2 = e;
            quick_two_sum(a0, a1, s, e); a0 = s; a1 = e;

            quick_two_sum(a2, a3, s, e); a2 = s; a3 = e;
            quick_two_sum(a1, a2, s, e); a1 = s; a2 = e;
            quick_two_sum(a0, a1, s, e); a0 = s; a1 = e;
        }

        FORCE_INLINE void acc4(double& a0, double& a1, double& a2, double& a3, double x)
        {
            // Add x into (a0,a1,a2,a3) with error propagation; leaves a0..a3 roughly normalized.
            double s, e;

            two_sum(a0, x,  s, e); a0 = s;
            two_sum(a1, e,  s, e); a1 = s;
            two_sum(a2, e,  s, e); a2 = s;
            two_sum(a3, e,  s, e); a3 = s;

            // Fold remaining tail into the smallest limb (fast, lossy).
            a3 += e;
        }

        FORCE_INLINE void acc4_prod(double& a0, double& a1, double& a2, double& a3,
                                    double u, double v,
                                    bool keep_err)
        {
            double p, e;
            two_prod(u, v, p, e);

            acc4(a0, a1, a2, a3, p);
            if (keep_err)
                acc4(a0, a1, a2, a3, e);
        }


        // Shewchuk-style expansion primitives (nonoverlapping expansions, ordered by increasing magnitude).

        template<int N>
        FORCE_INLINE int compress(const double (&in)[N], double* out)
        {
            // Two-pass "compress" (make nonoverlapping, remove zeros).
            double q = in[N - 1];
            double temp[N];
            int k = N - 1;

            for (int i = N - 2; i >= 0; --i)
            {
                double s, e;
                two_sum(q, in[i], s, e);
                q = s;
                if (e != 0.0)
                    temp[k--] = e;
            }
            temp[k] = q;

            int m = 0;
            for (int i = k; i < N; ++i)
                out[m++] = temp[i];

            return m;
        }

        template<int NA, int NB, int OUTN>
        FORCE_INLINE int fast_expansion_sum_zeroelim(const double (&a)[NA], int lena,
                                                     const double (&b)[NB], int lenb,
                                                     double (&out)[OUTN])
        {
            // Assumes a and b are ordered by increasing magnitude.
            int ia = 0, ib = 0, n = 0;
            double Q = 0.0;

            auto next = [&](double& x, bool& from_a) -> bool
            {
                if (ia < lena && ib < lenb)
                {
                    if (absd(a[ia]) <= absd(b[ib]))
                    {
                        x = a[ia++];
                        from_a = true;
                    }
                    else
                    {
                        x = b[ib++];
                        from_a = false;
                    }
                    return true;
                }
                if (ia < lena)
                {
                    x = a[ia++];
                    from_a = true;
                    return true;
                }
                if (ib < lenb)
                {
                    x = b[ib++];
                    from_a = false;
                    return true;
                }
                return false;
            };

            double x;
            bool from_a = false;

            if (!next(x, from_a))
                return 0;

            Q = x;

            while (next(x, from_a))
            {
                double s, e;
                two_sum(Q, x, s, e);
                Q = s;
                if (e != 0.0)
                    out[n++] = e;
            }

            if (Q != 0.0 || n == 0)
                out[n++] = Q;

            return n;
        }

        template<int N, int OUTN>
        FORCE_INLINE int scale_expansion_zeroelim(const double (&e)[N], int len,
                                                  double b,
                                                  double (&out)[OUTN])
        {
            // Multiply expansion by scalar b.
            int n = 0;

            double Q = 0.0;
            for (int i = 0; i < len; ++i)
            {
                double p, p_err;
                two_prod(e[i], b, p, p_err);

                double s, e1;
                two_sum(Q, p_err, s, e1);
                if (e1 != 0.0)
                    out[n++] = e1;

                double s2, e2;
                two_sum(p, s, s2, e2);
                Q = s2;
                if (e2 != 0.0)
                    out[n++] = e2;
            }

            if (Q != 0.0 || n == 0)
                out[n++] = Q;

            return n;
        }

        FORCE_INLINE void pack4(const double* e, int len, double& x0, double& x1, double& x2, double& x3)
        {
            // e is increasing magnitude. Take up to 4 largest terms into x0..x3 (largest first).
            x0 = x1 = x2 = x3 = 0.0;
            if (len <= 0) return;

            const int take = (len >= 4) ? 4 : len;
            const int start = len - take;

            double t[4] = {0,0,0,0};
            for (int i = 0; i < take; ++i)
                t[i] = e[start + i];

            // t is increasing. Map to limbs:
            // largest -> x0, ...
            x0 = t[take - 1];
            if (take > 1) x1 = t[take - 2];
            if (take > 2) x2 = t[take - 3];
            if (take > 3) x3 = t[take - 4];
        }

        FORCE_INLINE double to_double(double x0, double /*x1*/, double /*x2*/, double /*x3*/)
        {
            return x0;
        }

        // Constants (quad-double expansions), x0..x3 are largest..smallest
        struct f256_const { double x0, x1, x2, x3; };

        static constexpr f256_const kPi     = { 3.141592653589793116e+00, 1.224646799147353207e-16, -2.994769809718339666e-33, 1.112454220863365282e-49 };
        static constexpr f256_const kPi_2   = { 1.570796326794896558e+00, 6.123233995736766036e-17, -1.497384904859169833e-33, 5.562271104316826410e-50 };
        static constexpr f256_const kTwoPi  = { 6.283185307179586232e+00, 2.449293598294706414e-16, -5.989539619436679332e-33, 2.224908441726730563e-49 };
        static constexpr f256_const kInvPi2 = { 6.366197723675813824e-01, -3.935735335036497176e-17, 2.152264262748829105e-33, -1.281085968010345041e-49 }; // 2/pi
        static constexpr f256_const kLn2    = { 6.931471805599453094e-01, 2.319046813846299558e-17, 5.707708438416212066e-34, -3.582432210601811423e-50 };
        static constexpr f256_const kLog10e = { 4.342944819032518166e-01, 1.010305011872601710e-17, -2.366755089061475057e-34, 1.558385330010468241e-50 };
        static constexpr f256_const kLn10   = { 2.302585092994045901e+00, -2.170756223382249351e-16, -9.984262454465776570e-33, 2.448889984137502700e-49 };

        FORCE_INLINE void const_to_limbs(const f256_const& c, double& x0, double& x1, double& x2, double& x3)
        {
            x0 = c.x0; x1 = c.x1; x2 = c.x2; x3 = c.x3;
        }

        FORCE_INLINE int ilogb_approx(double x)
        {
            return std::ilogb(x);
        }
    }

    struct f256
    {
        double x0, x1, x2, x3; // largest -> smallest

        constexpr f256() : x0(0), x1(0), x2(0), x3(0) {}
        constexpr f256(double a) : x0(a), x1(0), x2(0), x3(0) {}
        constexpr f256(float a) : x0((double)a), x1(0), x2(0), x3(0) {}
        constexpr f256(int a) : x0((double)a), x1(0), x2(0), x3(0) {}
        constexpr f256(std::int64_t a) : x0((double)a), x1(0), x2(0), x3(0) {}

        constexpr f256(double a0, double a1, double a2, double a3) : x0(a0), x1(a1), x2(a2), x3(a3) {}

        static FORCE_INLINE f256 from_limbs(double a0, double a1, double a2, double a3)
        {
            f256 r;
            r.x0 = a0; r.x1 = a1; r.x2 = a2; r.x3 = a3;
            r.renorm();
            return r;
        }

        FORCE_INLINE void renorm()
        {
            // Convert to increasing expansion, compress, then take 4 largest.
            double in[4]  = { x3, x2, x1, x0 };
            double out[4] = { 0,0,0,0 };
            const int len = f256_detail::compress(in, out);
            f256_detail::pack4(out, len, x0, x1, x2, x3);
        }

        FORCE_INLINE double to_double() const { return f256_detail::to_double(x0, x1, x2, x3); }

        // Unary
        FORCE_INLINE f256 operator+() const { return *this; }
        FORCE_INLINE f256 operator-() const { return f256(-x0, -x1, -x2, -x3); }

        // Comparisons (lexicographic limbs)
        friend FORCE_INLINE bool operator==(const f256& a, const f256& b)
        {
            return a.x0 == b.x0 && a.x1 == b.x1 && a.x2 == b.x2 && a.x3 == b.x3;
        }
        friend FORCE_INLINE bool operator!=(const f256& a, const f256& b) { return !(a == b); }

        friend FORCE_INLINE bool operator<(const f256& a, const f256& b)
        {
            if (a.x0 != b.x0) return a.x0 < b.x0;
            if (a.x1 != b.x1) return a.x1 < b.x1;
            if (a.x2 != b.x2) return a.x2 < b.x2;
            return a.x3 < b.x3;
        }
        friend FORCE_INLINE bool operator>(const f256& a, const f256& b) { return b < a; }
        friend FORCE_INLINE bool operator<=(const f256& a, const f256& b) { return !(b < a); }
        friend FORCE_INLINE bool operator>=(const f256& a, const f256& b) { return !(a < b); }

        // Expansion view (increasing magnitude)
        FORCE_INLINE void to_expansion(double (&e)[4]) const { e[0] = x3; e[1] = x2; e[2] = x1; e[3] = x0; }

        // Addition
        friend FORCE_INLINE f256 operator+(const f256& a, const f256& b)
        {
            double ea[4], eb[4];
            a.to_expansion(ea);
            b.to_expansion(eb);

            double s8[8];
            f256_detail::fast_expansion_sum_zeroelim(ea, 4, eb, 4, s8);

            double c8[8];
            const int clen = f256_detail::compress(s8, c8);

            f256 r;
            f256_detail::pack4(c8, clen, r.x0, r.x1, r.x2, r.x3);
            return r;
        }

        friend FORCE_INLINE f256 operator-(const f256& a, const f256& b)
        {
            return a + (-b);
        }

        friend FORCE_INLINE f256 operator+(const f256& a, double b) { return a + f256(b); }
        friend FORCE_INLINE f256 operator-(const f256& a, double b) { return a - f256(b); }
        friend FORCE_INLINE f256 operator+(double a, const f256& b) { return f256(a) + b; }
        friend FORCE_INLINE f256 operator-(double a, const f256& b) { return f256(a) - b; }

        // Multiplication (fixed-schedule quad-double; truncated, performance-first)
        friend FORCE_INLINE f256 operator*(const f256& a, const f256& b)
        {
            // Keep the dominant terms (i+j <= 3). This is intentionally lossy but fast.
            double r0 = 0.0, r1 = 0.0, r2 = 0.0, r3 = 0.0;

            // Order is chosen to roughly match magnitude so renorm stays cheap.
            f256_detail::acc4_prod(r0, r1, r2, r3, a.x0, b.x0, true);

            f256_detail::acc4_prod(r0, r1, r2, r3, a.x0, b.x1, true);
            f256_detail::acc4_prod(r0, r1, r2, r3, a.x1, b.x0, true);

            f256_detail::acc4_prod(r0, r1, r2, r3, a.x0, b.x2, true);
            f256_detail::acc4_prod(r0, r1, r2, r3, a.x1, b.x1, true);
            f256_detail::acc4_prod(r0, r1, r2, r3, a.x2, b.x0, true);

            // Smallest-order group: skip product error for speed
            f256_detail::acc4_prod(r0, r1, r2, r3, a.x0, b.x3, false);
            f256_detail::acc4_prod(r0, r1, r2, r3, a.x1, b.x2, false);
            f256_detail::acc4_prod(r0, r1, r2, r3, a.x2, b.x1, false);
            f256_detail::acc4_prod(r0, r1, r2, r3, a.x3, b.x0, false);

            f256_detail::renorm4(r0, r1, r2, r3);

            return f256(r0, r1, r2, r3);
        }

        friend FORCE_INLINE f256 operator*(const f256& a, double b)
        {
            // Scale limbs then renorm. Keep error for the dominant limb only.
            double p0, e0;
            f256_detail::two_prod(a.x0, b, p0, e0);

            const double p1 = a.x1 * b;
            const double p2 = a.x2 * b;
            const double p3 = a.x3 * b;

            double r0 = p0, r1 = p1, r2 = p2, r3 = p3;
            f256_detail::acc4(r0, r1, r2, r3, e0);
            f256_detail::renorm4(r0, r1, r2, r3);

            return f256(r0, r1, r2, r3);
        }

        friend FORCE_INLINE f256 operator*(double a, const f256& b) { return b * a; }

        // Reciprocal (fast) via 2 Newton steps; mainly for internal use (atan/exp helpers)
        friend FORCE_INLINE f256 inv(const f256& a)
        {
            const double a0 = a.x0;
            f256 r(1.0 / a0);

            // r <- r * (2 - a*r)
            // Two steps are usually enough for fractal workloads.
            for (int i = 0; i < 2; ++i)
            {
                const f256 ar = a * r;
                r = r * (2.0 - ar);
            }

            return r;
        }

        // Division (fixed schedule; quotient digits, performance-first)
        friend FORCE_INLINE f256 operator/(const f256& a, const f256& b)
        {
            const double b0 = b.x0;

            // q0
            const double q0 = a.x0 / b0;
            f256 r = a - b * q0;

            // q1
            const double q1 = r.x0 / b0;
            r -= b * q1;

            // q2
            const double q2 = r.x0 / b0;

            f256 q(q0, q1, q2, 0.0);
            f256_detail::renorm4(q.x0, q.x1, q.x2, q.x3);
            return q;
        }

        friend FORCE_INLINE f256 operator/(const f256& a, double b)
        {
            return a * (1.0 / b);
        }

        friend FORCE_INLINE f256 operator/(double a, const f256& b)
        {
            return f256(a) / b;
        }


        // Compound
        FORCE_INLINE f256& operator+=(const f256& o) { *this = *this + o; return *this; }
        FORCE_INLINE f256& operator-=(const f256& o) { *this = *this - o; return *this; }
        FORCE_INLINE f256& operator*=(const f256& o) { *this = *this * o; return *this; }
        FORCE_INLINE f256& operator/=(const f256& o) { *this = *this / o; return *this; }

        FORCE_INLINE f256& operator+=(double o) { *this = *this + o; return *this; }
        FORCE_INLINE f256& operator-=(double o) { *this = *this - o; return *this; }
        FORCE_INLINE f256& operator*=(double o) { *this = *this * o; return *this; }
        FORCE_INLINE f256& operator/=(double o) { *this = *this / o; return *this; }

        // Helpers
        friend FORCE_INLINE f256 abs(const f256& a) { return (a.x0 < 0.0) ? -a : a; }

        friend FORCE_INLINE f256 floor(const f256& a)
        {
            // Floor using double, then correct with comparisons.
            double fd = std::floor(a.x0);
            f256 r(fd);
            if (r > a) r -= 1.0;
            return r;
        }

        friend FORCE_INLINE f256 trunc(const f256& a)
        {
            double td = std::trunc(a.x0);
            f256 r(td);
            if (a.x0 < 0.0 && r < a) r += 1.0;
            return r;
        }

        friend FORCE_INLINE f256 ldexp(const f256& a, int e)
        {
            const double s = std::ldexp(1.0, e);
            return f256(a.x0 * s, a.x1 * s, a.x2 * s, a.x3 * s);
        }

        friend FORCE_INLINE f256 sqrt(const f256& a)
        {
            if (a.x0 <= 0.0)
            {
                if (a.x0 == 0.0 && a.x1 == 0.0 && a.x2 == 0.0 && a.x3 == 0.0)
                    return f256(0.0);
                return f256(std::numeric_limits<double>::quiet_NaN());
            }

            f256 y(std::sqrt(a.x0));
            for (int i = 0; i < 6; ++i)
                y = (y + a / y) * 0.5;

            return y;
        }

        friend FORCE_INLINE f256 exp(const f256& x)
        {
            // x = k*ln2 + r, exp(x)=2^k*exp(r)
            double ln2_0, ln2_1, ln2_2, ln2_3;
            f256_detail::const_to_limbs(f256_detail::kLn2, ln2_0, ln2_1, ln2_2, ln2_3);
            const f256 ln2 = f256::from_limbs(ln2_0, ln2_1, ln2_2, ln2_3);

            const double xd = x.to_double();
            const double inv_ln2 = 1.0 / std::log(2.0);
            const long long k = (std::isfinite(xd)) ? (long long)std::llround(xd * inv_ln2) : 0LL;

            const f256 r = x - ln2 * (double)k;

            // Taylor exp(r)
            f256 term(1.0);
            f256 sum(1.0);

            const double eps = std::ldexp(1.0, -240);
            for (int n = 1; n < 2000; ++n)
            {
                term = term * (r / (double)n);
                sum += term;

                if (std::fabs(term.to_double()) <= std::fabs(sum.to_double()) * eps)
                    break;
            }

            return ldexp(sum, (int)k);
        }

        friend FORCE_INLINE f256 log(const f256& a)
        {
            if (a.x0 <= 0.0)
            {
                if (a.x0 == 0.0 && a.x1 == 0.0 && a.x2 == 0.0 && a.x3 == 0.0)
                    return f256(-std::numeric_limits<double>::infinity());
                return f256(std::numeric_limits<double>::quiet_NaN());
            }

            // Newton solve exp(y)=a
            f256 y(std::log(a.x0));
            for (int i = 0; i < 8; ++i)
            {
                const f256 ey = exp(y);
                y = y + (a - ey) / ey;
            }
            return y;
        }

        friend FORCE_INLINE f256 log2(const f256& a)
        {
            // log2(a) = log(a)/ln2
            double ln2_0, ln2_1, ln2_2, ln2_3;
            f256_detail::const_to_limbs(f256_detail::kLn2, ln2_0, ln2_1, ln2_2, ln2_3);
            const f256 ln2 = f256::from_limbs(ln2_0, ln2_1, ln2_2, ln2_3);
            return log(a) / ln2;
        }

        friend FORCE_INLINE f256 log10(const f256& a)
        {
            // log10(a) = log(a)/ln10
            double ln10_0, ln10_1, ln10_2, ln10_3;
            f256_detail::const_to_limbs(f256_detail::kLn10, ln10_0, ln10_1, ln10_2, ln10_3);
            const f256 ln10 = f256::from_limbs(ln10_0, ln10_1, ln10_2, ln10_3);
            return log(a) / ln10;
        }

        friend FORCE_INLINE f256 exp2(const f256& x)
        {
            // exp2(x) = exp(x*ln2)
            double ln2_0, ln2_1, ln2_2, ln2_3;
            f256_detail::const_to_limbs(f256_detail::kLn2, ln2_0, ln2_1, ln2_2, ln2_3);
            const f256 ln2 = f256::from_limbs(ln2_0, ln2_1, ln2_2, ln2_3);
            return exp(x * ln2);
        }

        friend FORCE_INLINE void sincos(const f256& x, f256& s, f256& c)
        {
            // Reduce to r = x - k*(pi/2), k = nearest int(x*(2/pi))
            double invpi2_0, invpi2_1, invpi2_2, invpi2_3;
            f256_detail::const_to_limbs(f256_detail::kInvPi2, invpi2_0, invpi2_1, invpi2_2, invpi2_3);
            const f256 invpi2 = f256::from_limbs(invpi2_0, invpi2_1, invpi2_2, invpi2_3);

            double pi2_0, pi2_1, pi2_2, pi2_3;
            f256_detail::const_to_limbs(f256_detail::kPi_2, pi2_0, pi2_1, pi2_2, pi2_3);
            const f256 pi_2 = f256::from_limbs(pi2_0, pi2_1, pi2_2, pi2_3);

            const double xd = x.to_double();
            long long k = 0;
            if (std::isfinite(xd))
            {
                const double kd = (double)std::llround(xd * invpi2.to_double());
                k = (long long)kd;
            }

            const f256 r = x - pi_2 * (double)k;

            // Compute sin/cos via Taylor on r (|r| ~ <= pi/4 after rounding).
            f256 r2 = r * r;

            // sin(r)
            f256 sin_t = r;
            f256 sin_sum = r;
            for (int n = 1; n < 200; ++n)
            {
                const int k2 = 2 * n;
                sin_t = sin_t * (-r2) / (double)(k2 * (k2 + 1));
                sin_sum += sin_t;
                if (std::fabs(sin_t.to_double()) <= std::fabs(sin_sum.to_double()) * std::ldexp(1.0, -240))
                    break;
            }

            // cos(r)
            f256 cos_t(1.0);
            f256 cos_sum(1.0);
            for (int n = 1; n < 200; ++n)
            {
                const int k2 = 2 * n;
                cos_t = cos_t * (-r2) / (double)((k2 - 1) * k2);
                cos_sum += cos_t;
                if (std::fabs(cos_t.to_double()) <= std::fabs(cos_sum.to_double()) * std::ldexp(1.0, -240))
                    break;
            }

            // Rotate based on quadrant k mod 4
            const int q = (int)(k & 3LL);
            switch (q)
            {
                case 0: s = sin_sum;           c = cos_sum;           break;
                case 1: s = cos_sum;           c = -sin_sum;          break;
                case 2: s = -sin_sum;          c = -cos_sum;          break;
                default:s = -cos_sum;          c = sin_sum;           break;
            }
        }

        friend FORCE_INLINE f256 sin(const f256& x) { f256 s, c; sincos(x, s, c); return s; }
        friend FORCE_INLINE f256 cos(const f256& x) { f256 s, c; sincos(x, s, c); return c; }
        friend FORCE_INLINE f256 tan(const f256& x) { f256 s, c; sincos(x, s, c); return s / c; }

        friend FORCE_INLINE f256 atan(const f256& x)
        {
            // atan(x) via series for |x|<=1, else pi/2 - atan(1/x)
            const f256 ax = abs(x);
            if (ax <= f256(1.0))
            {
                f256 t = x;
                f256 sum = x;
                f256 x2 = x * x;
                for (int n = 1; n < 2000; ++n)
                {
                    t = t * (-x2);
                    f256 term = t / (double)(2 * n + 1);
                    sum += term;
                    if (std::fabs(term.to_double()) <= std::fabs(sum.to_double()) * std::ldexp(1.0, -240))
                        break;
                }
                return sum;
            }

            double pi2_0, pi2_1, pi2_2, pi2_3;
            f256_detail::const_to_limbs(f256_detail::kPi_2, pi2_0, pi2_1, pi2_2, pi2_3);
            const f256 pi_2 = f256::from_limbs(pi2_0, pi2_1, pi2_2, pi2_3);

            f256 r = pi_2 - atan(inv(x));
            return (x.x0 < 0.0) ? -r : r;
        }

        friend FORCE_INLINE f256 atan2(const f256& y, const f256& x)
        {
            if (x.x0 > 0.0) return atan(y / x);

            double pi_0, pi_1, pi_2_, pi_3;
            f256_detail::const_to_limbs(f256_detail::kPi, pi_0, pi_1, pi_2_, pi_3);
            const f256 pi = f256::from_limbs(pi_0, pi_1, pi_2_, pi_3);

            if (x.x0 < 0.0)
            {
                f256 a = atan(y / x);
                return (y.x0 >= 0.0) ? (a + pi) : (a - pi);
            }

            // x == 0
            double pi2_0, pi2_1, pi2_2, pi2_3;
            f256_detail::const_to_limbs(f256_detail::kPi_2, pi2_0, pi2_1, pi2_2, pi2_3);
            const f256 pi_2 = f256::from_limbs(pi2_0, pi2_1, pi2_2, pi2_3);

            if (y.x0 > 0.0) return pi_2;
            if (y.x0 < 0.0) return -pi_2;
            return f256(0.0);
        }

        friend FORCE_INLINE f256 asin(const f256& x)
        {
            // asin(x) = atan2(x, sqrt(1-x^2))
            f256 one(1.0);
            f256 t = one - x * x;
            return atan2(x, sqrt(t));
        }

        friend FORCE_INLINE f256 acos(const f256& x)
        {
            // acos(x) = atan2(sqrt(1-x^2), x)
            f256 one(1.0);
            f256 t = one - x * x;
            return atan2(sqrt(t), x);
        }

        friend FORCE_INLINE f256 pow(const f256& a, const f256& b)
        {
            // Handle integer exponent for negative bases.
            if (a.x0 < 0.0)
            {
                const double bd = b.to_double();
                const double rd = std::round(bd);
                if (std::fabs(bd - rd) <= std::ldexp(1.0, -40)) // loose integer test
                {
                    const long long n = (long long)rd;
                    f256 base = -a;
                    f256 res(1.0);
                    long long e = (n < 0) ? -n : n;
                    while (e)
                    {
                        if (e & 1LL) res *= base;
                        base *= base;
                        e >>= 1LL;
                    }
                    if (n < 0) res = inv(res);
                    return (n & 1LL) ? -res : res;
                }
                return f256(std::numeric_limits<double>::quiet_NaN());
            }

            return exp(b * log(a));
        }

        friend FORCE_INLINE f256 sinh(const f256& x)
        {
            const f256 ex = exp(x);
            const f256 em = inv(ex);
            return (ex - em) * 0.5;
        }

        friend FORCE_INLINE f256 cosh(const f256& x)
        {
            const f256 ex = exp(x);
            const f256 em = inv(ex);
            return (ex + em) * 0.5;
        }

        friend FORCE_INLINE f256 tanh(const f256& x)
        {
            const f256 ex = exp(x);
            const f256 em = inv(ex);
            return (ex - em) / (ex + em);
        }
    };

} // namespace bl

namespace std
{
    template<>
    struct numeric_limits<bl::f256>
    {
        static constexpr bool is_specialized = true;

        static constexpr bl::f256 min() noexcept { return bl::f256(std::numeric_limits<double>::min()); }
        static constexpr bl::f256 max() noexcept { return bl::f256(std::numeric_limits<double>::max()); }
        static constexpr bl::f256 lowest() noexcept { return bl::f256(-std::numeric_limits<double>::max()); }

        static constexpr int digits = 212; // approx for quad-double significand bits
        static constexpr int digits10 = 63;
        static constexpr int max_digits10 = 66;

        static constexpr bool is_signed = true;
        static constexpr bool is_integer = false;
        static constexpr bool is_exact = false;
        static constexpr int radix = 2;

        static constexpr bl::f256 epsilon() noexcept { return bl::f256(std::ldexp(1.0, -212)); }
        static constexpr bl::f256 round_error() noexcept { return bl::f256(0.5); }

        static constexpr int min_exponent = std::numeric_limits<double>::min_exponent;
        static constexpr int min_exponent10 = std::numeric_limits<double>::min_exponent10;
        static constexpr int max_exponent = std::numeric_limits<double>::max_exponent;
        static constexpr int max_exponent10 = std::numeric_limits<double>::max_exponent10;

        static constexpr bool has_infinity = true;
        static constexpr bool has_quiet_NaN = true;
        static constexpr bool has_signaling_NaN = false;
        //static constexpr float_denorm_style has_denorm = std::numeric_limits<double>::has_denorm;
        //static constexpr bool has_denorm_loss = std::numeric_limits<double>::has_denorm_loss;

        static constexpr bl::f256 infinity() noexcept { return bl::f256(std::numeric_limits<double>::infinity()); }
        static constexpr bl::f256 quiet_NaN() noexcept { return bl::f256(std::numeric_limits<double>::quiet_NaN()); }
        static constexpr bl::f256 signaling_NaN() noexcept { return bl::f256(std::numeric_limits<double>::signaling_NaN()); }
        static constexpr bl::f256 denorm_min() noexcept { return bl::f256(std::numeric_limits<double>::denorm_min()); }

        static constexpr bool is_iec559 = false;
        static constexpr bool is_bounded = true;
        static constexpr bool is_modulo = false;

        static constexpr bool traps = std::numeric_limits<double>::traps;
        static constexpr bool tinyness_before = std::numeric_limits<double>::tinyness_before;
        static constexpr float_round_style round_style = std::numeric_limits<double>::round_style;
    };
} // namespace std


#endif // BL_F256_H
