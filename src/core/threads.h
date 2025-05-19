#pragma once

// Fix for MSVC mutex constexpr bug
#define _DISABLE_CONSTEXPR_MUTEX_CONSTRUCTOR
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

namespace Thread
{
    static BS::thread_pool<BS::tp::none>& pool()
    {
        static BS::thread_pool pool;
        return pool;
    }

    inline unsigned int idealThreadCount()
    {
        // 1. Try the standard C++ hint
        unsigned int n = std::thread::hardware_concurrency();
        if (n) return n;

        // 2. PlatformManager-specific fall-backs
        #if defined(__EMSCRIPTEN__)
            return static_cast<unsigned int>(emscripten_num_logical_cores());
        #elif defined(_WIN32)
            DWORD w = GetActiveProcessorCount(ALL_PROCESSOR_GROUPS);
            return w ? static_cast<unsigned int>(w) : 1;
        #elif defined(_SC_NPROCESSORS_ONLN)
            long p = sysconf(_SC_NPROCESSORS_ONLN);
            return p > 0 ? static_cast<unsigned int>(p) : 1;
        #else
            return 1;           // last-ditch fall-back
        #endif
    }

    template<typename sizeT>
    inline std::vector<std::pair<sizeT, sizeT>> splitRanges(sizeT totalSize, sizeT numParts)
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
    inline std::pair<sizeT, sizeT> splitRange(sizeT totalSize, sizeT numParts, sizeT blockIndex)
    {
        // Preconditions (replace with your own error handling if desired)
        if (numParts == 0)          throw std::invalid_argument("numParts must be > 0");
        if (blockIndex >= numParts) throw std::out_of_range("blockIndex out of range");

        const sizeT base = totalSize / numParts;   // size of every block before distributing the remainder
        const sizeT extra = totalSize % numParts;   // how many blocks get one extra element

        // Blocks [0, extra) have size base + 1. The rest have size base.
        const bool hasExtra = blockIndex < extra;
        const sizeT size = base + (hasExtra ? 1 : 0);

        // Start index is:
        //   * blockIndex * (base + 1)          for the first 'extra' blocks
        //   * extra * (base + 1) + (blockIndex - extra) * base  otherwise
        const sizeT start = hasExtra
            ? blockIndex * (base + 1)
            : extra * (base + 1) + (blockIndex - extra) * base;

        return { start, start + size };   // [start, end)
    }
}

struct SharedSync
{
    std::atomic<bool> quitting{ false };
    std::atomic<bool> editing_ui{ false };
    std::atomic<bool> updating_live_buffer{ false };

    std::mutex live_buffer_mutex;
    std::mutex shadow_buffer_mutex;

    std::mutex project_mutex;
    std::mutex state_mutex;
    std::mutex working_mutex;

    std::condition_variable cv;
    std::condition_variable cv_updating_live_buffer;

    bool project_thread_started = false;
    bool frame_ready = false;
    bool frame_consumed = false;
    bool processing_frame = false;

    void wait_until_gui_consumes_frame()
    {
        std::unique_lock<std::mutex> lock(state_mutex);
        cv.wait(lock, [this] {
            return frame_consumed || quitting.load();
        });
    }

    void flag_ready_to_draw()
    {
        std::lock_guard<std::mutex> lock(state_mutex);
        frame_ready = true;
        frame_consumed = false;
    }

    void wait_until_gui_drawn()
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