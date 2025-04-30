#pragma once
#define _DISABLE_CONSTEXPR_MUTEX_CONSTRUCTOR 
#include <thread>
#include <vector>
#include <queue>
#include <deque>
#include <mutex>
#include <condition_variable>
#include <future>
#include <functional>
#include <atomic>

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

        // 2. Platform-specific fall-backs
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
}