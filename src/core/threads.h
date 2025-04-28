#pragma once

#include <thread>
#include <vector>
#include <queue>
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




namespace Thread
{
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

    inline std::vector<std::pair<size_t, size_t>> splitRanges(size_t totalSize, size_t numParts)
    {
        std::vector<std::pair<size_t, size_t>> ranges;

        size_t partSize = totalSize / numParts;
        size_t remainder = totalSize % numParts;

        size_t start = 0;
        for (size_t i = 0; i < numParts; ++i)
        {
            size_t thisPartSize = partSize + (i < remainder ? 1 : 0);
            ranges.emplace_back(start, start + thisPartSize);
            start += thisPartSize;
        }

        return ranges;
    }

    // ---------------------------------------------------------------------------------------------------------------------
    // A minimal lock-free ring buffer for pointers to tasks
    // ---------------------------------------------------------------------------------------------------------------------
    

    // ---------------------------------------------------------------------------------------------------------------------
    // ThreadPool
    // ---------------------------------------------------------------------------------------------------------------------
    

    
}

// --------------------------------------------------------------
// RingBuffer  —  now MoveConstructible (copy disabled)
// --------------------------------------------------------------
class RingBuffer
{
public:
    explicit RingBuffer(std::size_t cap = 1024) : mask(cap - 1), buf(cap)
    {
        if ((cap & mask) != 0)
            throw std::invalid_argument("capacity must be power of two");
        head = tail = 0;
    }

    RingBuffer(RingBuffer&& other) noexcept
        : mask(other.mask), buf(std::move(other.buf))
    {
        head.store(other.head.load(std::memory_order_relaxed),
            std::memory_order_relaxed);
        tail.store(other.tail.load(std::memory_order_relaxed),
            std::memory_order_relaxed);
    }

    RingBuffer& operator=(RingBuffer&& other) noexcept
    {
        if (this != &other)
        {
            buf = std::move(other.buf);
            head.store(other.head.load(std::memory_order_relaxed),
                std::memory_order_relaxed);
            tail.store(other.tail.load(std::memory_order_relaxed),
                std::memory_order_relaxed);
        }
        return *this;
    }

    RingBuffer(const RingBuffer&) = delete;
    RingBuffer& operator=(const RingBuffer&) = delete;

    // ---------- API unchanged -----------------------------------
    bool try_push(std::function<void()> f)
    {
        auto h = head.load(std::memory_order_relaxed);
        auto next = (h + 1) & mask;
        if (next == tail.load(std::memory_order_acquire)) return false;   // full
        buf[h] = std::move(f);
        head.store(next, std::memory_order_release);
        return true;
    }

    bool try_pop(std::function<void()>& out)
    {
        auto t = tail.load(std::memory_order_relaxed);
        if (t == head.load(std::memory_order_acquire)) return false;      // empty
        out = std::move(buf[t]);
        tail.store((t + 1) & mask, std::memory_order_release);
        return true;
    }

    bool empty() const
    {
        return head.load(std::memory_order_acquire) ==
            tail.load(std::memory_order_acquire);
    }

private:
    const std::size_t               mask;
    std::vector<std::function<void()>> buf;
    std::atomic<std::size_t>        head{}, tail{};
};


struct TaskQueue
{
    RingBuffer rb;
    std::mutex m;

    explicit TaskQueue(std::size_t cap = 1024) : rb(cap) {}

    TaskQueue(TaskQueue&& other) noexcept
        : rb(std::move(other.rb)) {}

    TaskQueue& operator=(TaskQueue&& other) noexcept
    {
        if (this != &other)
        {
            std::scoped_lock lock(m, other.m);   // protect both sides
            rb = std::move(other.rb);
        }
        return *this;
    }

    TaskQueue(const TaskQueue&) = delete;
    TaskQueue& operator=(const TaskQueue&) = delete;

    // ------- queue API (unchanged) ------------------------------------
    bool try_push(std::function<void()> f)
    {
        std::lock_guard<std::mutex> lk(m);
        return rb.try_push(std::move(f));
    }

    bool try_pop(std::function<void()>& out)
    {
        std::lock_guard<std::mutex> lk(m);
        return rb.try_pop(out);
    }

    bool empty()
    {
        std::lock_guard<std::mutex> lk(m);
        return rb.empty();
    }
};


class ThreadPool
{
    // Per-thread queues
    std::vector<TaskQueue>              queues;
    std::atomic<std::size_t>            next_queue{ 0 };

    // Global structures
    std::vector<std::thread>            workers;
    std::atomic<bool>                   stop{ false };

    // Overflow for bursts
    std::vector<std::function<void()>>  overflow;
    std::mutex                          overflow_mtx;

    // Sleep/wake
    std::condition_variable             cv;
    std::mutex                          wait_mtx;

public:
    static ThreadPool singleton;                   // stays exactly as you wrote it

    ThreadPool()                                   // default ctor
    {
        _setThreadCount(std::thread::hardware_concurrency());
    }

    ~ThreadPool()
    {
        _shutdown();
    }

    template<typename Func, typename... Args>
    static auto submit(Func&& f, Args&&... args)
        -> std::future<std::invoke_result_t<Func, Args...>>
    {
        return singleton._submit(std::forward<Func>(f),
            std::forward<Args>(args)...);
    }

    // --- allow re-configuration if you really need it ------------
    void _setThreadCount(unsigned int num_threads)
    {
        _shutdown();                               // join old workers if any

        if (num_threads == 0) num_threads = 1;

        queues.clear();
        queues.reserve(num_threads);
        for (unsigned int i = 0; i < num_threads; ++i)
            queues.emplace_back(1024);             // per-queue capacity

        stop.store(false, std::memory_order_relaxed);
        workers.clear();
        workers.reserve(num_threads);

        for (unsigned int i = 0; i < num_threads; ++i)
            workers.emplace_back(&ThreadPool::worker_loop, this, i);
    }

    template<typename Func, typename... Args>
    auto _submit(Func&& f, Args&&... args)
        -> std::future<std::invoke_result_t<Func, Args...>>
    {
        using Ret = std::invoke_result_t<Func, Args...>;

        auto task_ptr = std::make_shared<std::packaged_task<Ret()>>(
            std::bind(std::forward<Func>(f), std::forward<Args>(args)...));

        std::function<void()> wrapped = [task_ptr] { (*task_ptr)(); };

        // round-robin choice
        std::size_t qid = next_queue.fetch_add(1, std::memory_order_relaxed) % queues.size();
        if (!queues[qid].try_push(std::move(wrapped)))
        {
            std::lock_guard<std::mutex> lk(overflow_mtx);
            overflow.emplace_back(std::move(wrapped));
        }

        cv.notify_one();
        return task_ptr->get_future();
    }

private:
    void _shutdown()
    {
        stop.store(true, std::memory_order_release);
        cv.notify_all();

        for (auto& t : workers)
            if (t.joinable()) t.join();

        workers.clear();
        stop.store(false, std::memory_order_relaxed);   // ready for reuse
    }

    void worker_loop(std::size_t id)
    {
        TaskQueue& myq = queues[id];
        std::function<void()> task;

        while (!stop.load(std::memory_order_acquire))
        {
            // 1. Own queue
            if (myq.try_pop(task)) { task(); continue; }

            // 2. Steal
            for (std::size_t n = 0; n < queues.size(); ++n)
            {
                if (queues[(id + n) % queues.size()].try_pop(task))
                {
                    task();
                    continue;                       // back to top
                }
            }

            // 3. Overflow
            {
                std::lock_guard<std::mutex> lk(overflow_mtx);
                if (!overflow.empty())
                {
                    task = std::move(overflow.back());
                    overflow.pop_back();
                    task();
                    continue;
                }
            }

            // 4. Sleep
            {
                std::unique_lock<std::mutex> lk(wait_mtx);
                cv.wait(lk, [this, &myq] {
                    return stop.load(std::memory_order_acquire) || !myq.empty();
                });
            }
        }
    }
};



/*class ThreadPool
{
    // Per-thread queues
    std::vector<RingBuffer> queues;
    std::atomic<std::size_t> next_queue{ 0 };

    // Global structures
    std::vector<std::thread> workers;
    std::atomic<bool> stop;

    // Overflow for bursts
    std::vector<std::function<void()>> overflow;
    std::mutex overflow_mtx;

    // Sleep/wake
    std::condition_variable cv;
    std::mutex wait_mtx;

public:
    static ThreadPool singleton;
    explicit ThreadPool()
    {
        singleton._setThreadCount(Thread::idealThreadCount());
    }

    ~ThreadPool()
    {
        stop.store(true, std::memory_order_release);
        cv.notify_all();
        for (auto& t : workers)
            t.join();
    }

    template<typename Func, typename... Args>
    static auto submit(Func&& f, Args&&... args)
        -> std::future<std::invoke_result_t<Func, Args...>>
    {
        return singleton._submit(std::forward<Func>(f), std::forward<Args>(args)...);
    }

    void _setThreadCount(unsigned int num_threads)
    {
        queues = std::vector<RingBuffer>(num_threads);
        stop = false;

        if (num_threads == 0)
            num_threads = 1;

        workers.reserve(num_threads);
        for (unsigned int i = 0; i < num_threads; ++i)
            workers.emplace_back(&ThreadPool::worker_loop, this, i);
    }

    template<typename Func, typename... Args>
    auto _submit(Func&& f, Args&&... args)
        -> std::future<std::invoke_result_t<Func, Args...>>
    {
        using Ret = std::invoke_result_t<Func, Args...>;
        auto task_ptr = std::make_shared<std::packaged_task<Ret()>>(
            std::bind(std::forward<Func>(f), std::forward<Args>(args)...));

        std::function<void()> wrapper = [task_ptr] { (*task_ptr)(); };

        // Choose the lightest-loaded queue (round-robin is good enough and cheap)
        const std::size_t qid = next_queue.fetch_add(1, std::memory_order_relaxed) % queues.size();
        if (!queues[qid].try_push(std::move(wrapper)))
        {
            // queue full – fallback: push to a global overflow queue with mutex
            std::lock_guard<std::mutex> lk(overflow_mtx);
            overflow.emplace_back(std::move(wrapper));
        }

        cv.notify_one();
        return task_ptr->get_future();
    }

private:
    void worker_loop(std::size_t id)
    {
        RingBuffer& my_queue = queues[id];
        std::function<void()> task;

        while (!stop.load(std::memory_order_acquire))
        {
            // 1. Own queue
            if (my_queue.try_pop(task))
            {
                task();
                continue;
            }

            // 2. Try to steal from other queues
            for (std::size_t n = 0; n < queues.size(); ++n)
            {
                if (queues[(id + n) % queues.size()].try_pop(task))
                {
                    task();
                    goto next_iteration;
                }
            }

            // 3. Overflow queue (protected by mutex)
            {
                std::lock_guard<std::mutex> lk(overflow_mtx);
                if (!overflow.empty())
                {
                    task = std::move(overflow.back());
                    overflow.pop_back();
                    task();
                    continue;
                }
            }

            // 4. Nothing to do – sleep
            {
                std::unique_lock<std::mutex> lk(wait_mtx);
                cv.wait(lk, [this, &my_queue] { return stop.load(std::memory_order_acquire) == true || !my_queue.empty(); });
            }

            next_iteration:
            continue;
        }
    }
};*/