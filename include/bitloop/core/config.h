#pragma once

/// ======== Debug Features ========
//#define DEBUG_FINITE_DOUBLE_CHECKS
//#define FORCE_RELEASE_FINITE_DOUBLE_CHECKS
//#define DEBUG_DISABLE_PRINT
#define DEBUG_INCLUDE_LOG_TABS

/// ======== Platform Simulation ========
//#define DEBUG_SIMULATE_WEB_UI
//#define DEBUG_SIMULATE_MOBILE
//#define DEBUG_SIMULATE_DPR 2.625f
//#define DEBUG_SIMULATE_DPR 1.5f


/// ======== Timer filters ========

#define TIMERS_ENABLED

//-- Timer thresholds -- 
constexpr double TIMER_ELAPSED_LIMIT = 1.0; // ms

// -- Filters --
constexpr bool THREAD_LOGGING = false;
constexpr bool THREAD_TIMING = false;
