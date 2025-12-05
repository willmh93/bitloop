#pragma once

/// ======== Debug Features ========
//#define DEBUG_FINITE_DOUBLE_CHECKS
//#define FORCE_RELEASE_FINITE_DOUBLE_CHECKS
//#define DEBUG_DISABLE_PRINT
//#define DEBUG_INCLUDE_LOG_TABS

//#define FORCE_DEBUG


/// ======== Platform Simulation ========
//#define ENABLE_SIMULATED_DEVICE

#ifdef ENABLE_SIMULATED_DEVICE
    struct DebugDevicePreset { int w; int h; float dpr; };

    /// --- flags ---
    #define SIMULATE_MOBILE  // platform()->is_mobile() == true + use mouse to emulate touch
    #define SIMULATE_BROWSER // emscripten

    /// --- presets ---
    #define SIMULATE_DISPLAY                DebugDevicePreset{1080, 1998, 2.625f}  // Google Pixel 9a (Chrome)
    //#define SIMULATE_DISPLAY              DebugDevicePreset{1080, 2424, 2.625f}  // Google Pixel 9a (fullscreen)

    /// --- scale to comfortable size on screen ---
    //#define SIMULATE_DISPLAY_VIEW_SCALE     0.205f // accurate
    #define SIMULATE_DISPLAY_VIEW_SCALE     0.381f // comfortable
#endif

/// ======== Timer filters ========

#define TIMERS_ENABLED

//-- Timer thresholds -- 
constexpr double TIMER_ELAPSED_LIMIT = 1.0; // ms

// -- Filters --
constexpr bool THREAD_LOGGING = false;
constexpr bool THREAD_TIMING = false;
