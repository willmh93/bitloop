#pragma once

#ifndef BL_BEGIN_NS
#define BL_BEGIN_NS namespace bl {
#endif
#ifndef BL_END_NS
#define BL_END_NS   }
#endif

// ======== FAST_INLINE ========
/*
#if defined(_MSC_VER)
# define FAST_INLINE __forceinline

#elif defined(__EMSCRIPTEN__)
# define FAST_INLINE inline __attribute__((always_inline))
#elif defined(__clang__) || defined(__GNUC__)
# define FAST_INLINE inline __attribute__((always_inline, optimize("fast-math")))

#else
# define FAST_INLINE inline
#endif*/

// ===== Inline (no fast-math attr baked in) =====
#if defined(_MSC_VER)
#define FAST_INLINE __forceinline
#elif defined(__clang__) || defined(__GNUC__)
#define FAST_INLINE inline __attribute__((always_inline))
#else
#define FAST_INLINE inline
#endif


/* ------------------------------------------------------------------ */
/*  Fast-math override helpers                                        */
/* ------------------------------------------------------------------ */
/*
#if defined(_MSC_VER) // MSVC or clang-cl
#define BL_PUSH_PRECISE  __pragma(float_control(precise, on, push)) \
                         __pragma(fp_contract(off))
#define BL_POP_PRECISE   __pragma(float_control(pop))

#elif defined(__EMSCRIPTEN__)
#define BL_PUSH_PRECISE  _Pragma("clang optimize off") \
                         _Pragma("fp_contract(off)")
#define BL_POP_PRECISE   _Pragma("clang optimize on")

#elif defined(__clang__) || defined(__GNUC__)
#define BL_PUSH_PRECISE  _Pragma("GCC push_options") \
                         _Pragma("GCC optimize(\"no-fast-math\")") \
                         _Pragma("fp_contract(off)")
#define BL_POP_PRECISE   _Pragma("GCC pop_options")

#else
#define BL_PUSH_PRECISE
#define BL_POP_PRECISE
#endif
*/


#if defined(_MSC_VER)
#if defined(_M_FP_FAST)
#define BL_FAST_MATH
#endif
#elif defined(__FAST_MATH__)
#define BL_FAST_MATH
#endif


#if defined(_MSC_VER)
#define BL_PUSH_PRECISE  __pragma(float_control(precise, on, push)) \
                         __pragma(fp_contract(off))
#define BL_POP_PRECISE   __pragma(float_control(pop))


#elif defined(__EMSCRIPTEN__)
#define BL_PUSH_PRECISE  _Pragma("clang fp reassociate(off)") \
                            _Pragma("clang fp contract(off)")
#define BL_POP_PRECISE   _Pragma("clang fp reassociate(on)")  \
                            _Pragma("clang fp contract(fast)")

// Other Clang (desktop) that supports the richer set
#elif defined(__clang__)
#define BL_PUSH_PRECISE  _Pragma("clang fp reassociate(off)") \
                            _Pragma("clang fp contract(off)")
#define BL_POP_PRECISE   _Pragma("clang fp reassociate(on)")  \
                            _Pragma("clang fp contract(fast)")

// GCC fallback
#elif defined(__GNUC__)
#define BL_PUSH_PRECISE  _Pragma("GCC push_options")                \
                            _Pragma("GCC optimize(\"no-fast-math\")") \
                            _Pragma("STDC FP_CONTRACT OFF")
#define BL_POP_PRECISE   _Pragma("GCC pop_options")

#else
#define BL_PUSH_PRECISE
#define BL_POP_PRECISE
#endif

template<typename T>
constexpr T bl_infinity() {
    //static_assert(bl::is_floating_point_v<T>, "bl_infinity<T> requires a floating-point type");
    #ifdef BL_FAST_MATH
    return T(1e300); // Safe large finite fallback
    #else
    return std::numeric_limits<T>::infinity();
    #endif
}

// ======== blBreak ========

#if defined(_MSC_VER)
#define blBreak() __debugbreak()
#elif defined(__GNUC__) || defined(__clang__)
#include <csignal>
#define blBreak() std::raise(SIGTRAP)
#else
#define blBreak() ((void)0)  // fallback: no-op
#endif



// Returns true if the compiler enabled a "fast math" mode for this TU.
constexpr bool fast_math_enabled() {
#if defined(__FAST_MATH__)                 // GCC/Clang: -ffast-math
    return true;
#elif defined(_MSC_VER) && defined(_M_FP_FAST)  // MSVC: /fp:fast
    return true;
#else
    return false;
#endif
}

// stringify helpers
#define STR(x) #x
#define VAL(x) STR(x)

#if defined(_MSC_VER)
#define PP_MESSAGE(msg) __pragma(message(msg))
#define PP_SHOW(name)   __pragma(message(#name "=" VAL(name)))
#else
  // Clang/GCC/Emscripten
#define PP_MESSAGE(msg) _Pragma("message \"" msg "\"")
#define PP_SHOW(name)   _Pragma("message \"" #name "=" VAL(name) "\"")
#endif
