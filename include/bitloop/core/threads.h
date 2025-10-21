#pragma once

// Fix for MSVC mutex constexpr bug
//#define _DISABLE_CONSTEXPR_MUTEX_CONSTRUCTOR
#include <thread>
#include <mutex>
#include <condition_variable>
#include <future>
#include <functional>
#include <atomic>

#include <vector>
#include <queue>
#include <deque>

#if defined(__EMSCRIPTEN__)
#include <emscripten/threading.h>
#elif defined(_WIN32)
#define NOMINMAX
#include <windows.h>
#elif defined(__unix__) || defined(__APPLE__)
#include <unistd.h>
#include <sys/types.h>
#endif

#include "BS_thread_pool.hpp"
#include "concurrentqueue.h"

#define BL_BEGIN_NS namespace bl {
#define BL_END_NS   }

BL_BEGIN_NS

namespace Thread
{
    using moodycamel::ConcurrentQueue;
    using moodycamel::BlockingConcurrentQueue;

    [[nodiscard]] inline unsigned int idealThreadCount()
    {
        // 1. Try the standard C++ hint
        unsigned int n = std::thread::hardware_concurrency();
        if (n) return (n-1);

        // 2. PlatformManager-specific fall-backs
        #if defined(__EMSCRIPTEN__)
            return static_cast<unsigned int>(emscripten_num_logical_cores());
        #elif defined(_WIN32)
            DWORD w = GetActiveProcessorCount(ALL_PROCESSOR_GROUPS);
            n = w ? static_cast<unsigned int>(w) : 1;
        #elif defined(_SC_NPROCESSORS_ONLN)
            long p = sysconf(_SC_NPROCESSORS_ONLN);
            n = p > 0 ? static_cast<unsigned int>(p) : 1;
        #else
            n = 1;           // last-ditch fall-back
        #endif
            
        return (n > 1) ? (n - 1) : 1;
    }

    [[nodiscard]] static BS::thread_pool<BS::tp::none>& pool()
    {
        static BS::thread_pool pool(Thread::idealThreadCount());
        return pool;
    }

    template<typename sizeT>
    [[nodiscard]] inline std::vector<std::pair<sizeT, sizeT>> splitRanges(sizeT totalSize, sizeT numParts)
    {
        std::vector<std::pair<sizeT, sizeT>> ranges;

        sizeT partSize = totalSize / numParts;
        sizeT remainder = totalSize % numParts;

        sizeT start = 0;
        for (sizeT i = 0; i < numParts; ++i)
        {
            sizeT thisPartSize = partSize + (i < remainder ? 1 : 0);
            ranges.emplace_back(start, start + thisPartSize);
            start += thisPartSize;
        }

        return ranges;
    }

    template<typename sizeT>
    [[nodiscard]] inline std::pair<sizeT, sizeT> splitRange(sizeT totalSize, sizeT numParts, sizeT blockIndex)
    {
        if (numParts == 0) throw std::invalid_argument("numParts must be > 0");
        if (blockIndex >= numParts) throw std::out_of_range("blockIndex out of range");

        const sizeT base = totalSize / numParts;
        const sizeT extra = totalSize % numParts;

        const bool hasExtra = blockIndex < extra;
        const sizeT size = base + (hasExtra ? 1 : 0);

        const sizeT start = hasExtra
            ? blockIndex * (base + 1)
            : extra * (base + 1) + (blockIndex - extra) * base;

        return { start, start + size };   // [start, end)
    }
}

struct SharedSync
{
    std::atomic<bool> quitting{ false };
    std::atomic<bool> updating_live_buffer{ false };

    std::mutex live_buffer_mutex;
    std::mutex shadow_buffer_mutex;
    std::mutex state_mutex;

    std::condition_variable cv;
    std::condition_variable cv_updating_live_buffer;

    bool project_thread_started = false;
    bool frame_ready_to_draw = false;
    bool frame_consumed = false;

    void flag_ready_to_draw()
    {
        std::lock_guard<std::mutex> lock(state_mutex);
        frame_ready_to_draw = true;
        frame_consumed = false;
    }

    void wait_until_gui_consumes_frame()
    {
        std::unique_lock<std::mutex> lock(state_mutex);
        cv.wait(lock, [this] { return frame_consumed || quitting.load(); });
    }

    void wait_until_live_buffer_updated()
    {
        std::unique_lock<std::mutex> live_lock(live_buffer_mutex);
        cv_updating_live_buffer.wait(live_lock, [this]() {
            return !updating_live_buffer.load();
        });
    }

    void quit()
    {
        quitting.store(true);
        cv.notify_all();
    }
};

BL_END_NS
