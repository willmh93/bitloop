#pragma once

#include <chrono>
#include <string>
#include <vector>
#include <mutex>
#include <unordered_map>
#include <atomic>
#include <cmath>
#include <bitloop/core/debug.h>

namespace bl
{
    // ---------------------------------------------------------------------
    // TimerGroup: global stats per label
    //
    // Meaning:
    //  - total_ns / total_samples -> long-term AVG per sample
    //  - last_group_measured_ns / last_group_samples -> LAST per sample
    //  - PCT ~ fraction of total CPU-core time (all worker threads) spent
    //    in this label during the last group.
    // ---------------------------------------------------------------------
    struct TimerGroup
    {
        std::string name;

        // Accumulated adjusted time over all samples (ns)
        long long total_ns = 0;

        // Accumulated sample count over all groups
        long long total_samples = 0;

        // Last group's adjusted measured time (sum over threads, ns)
        long long last_group_measured_ns = 0;

        // Last group's total sample count (sum over threads)
        long long last_group_samples = 0;

        // Last group's wall-clock duration (ns)
        long long last_group_wall_ns = 0;

        // How many groups have completed
        long long group_count = 0;

        bool group_active = false;
        bool sample_active = false;

        std::chrono::steady_clock::time_point group_start_tp;

        TimerGroup() = default;
        explicit TimerGroup(const std::string& n) : name(n) {}
    };

    // ---------------------------------------------------------------------
    // TimerRegistry: per-thread accumulators + global stats
    // ---------------------------------------------------------------------
    class TimerRegistry
    {
    public:
        using clock = std::chrono::steady_clock;
        using time_point = clock::time_point;

        // One group's state in one thread
        struct ThreadLocalGroupState
        {
            static constexpr int MAX_STACK = 16;

            time_point t0_stack[MAX_STACK];
            int t0_depth = 0;

            // Sum of raw (uncorrected) intervals in this group (ns)
            long long raw_ns_group = 0;

            // Number of t0/t1 pairs in this group
            long long interval_count = 0;

            // Number of logical samples in this group
            long long sample_count = 0;

            // > 0 while inside at least one sample
            int sample_depth = 0;
        };

        // All groups used in one thread
        struct ThreadLocalTimers
        {
            struct Entry {
                TimerGroup* group;
                ThreadLocalGroupState state;
            };

            std::vector<Entry> entries;

            TimerGroup* last_group = nullptr;
            ThreadLocalGroupState* last_state = nullptr;

            ThreadLocalGroupState& get_state(TimerGroup* g)
            {
                if (g == last_group && last_state)
                    return *last_state;

                for (auto& e : entries) {
                    if (e.group == g) {
                        last_group = g;
                        last_state = &e.state;
                        return e.state;
                    }
                }

                entries.push_back({ g, ThreadLocalGroupState{} });
                last_group = g;
                last_state = &entries.back().state;
                return *last_state;
            }
        };

        // -----------------------------------------------------------------
        // Public API
        // -----------------------------------------------------------------

        static TimerGroup* get_or_create_group(const char* name_lit)
        {
            auto& inst = instance();
            std::lock_guard<std::mutex> lock(inst.groups_mutex_);

            auto it = inst.groups_.find(name_lit);
            if (it != inst.groups_.end())
                return &it->second;

            auto [iter, inserted] = inst.groups_.emplace(
                std::string(name_lit),
                TimerGroup{ std::string(name_lit) }
            );
            return &iter->second;
        }

        // ------------------------ Calibration -----------------------------

        // Estimate average overhead of one t0/t1 pair in ns.
        static void calibrate(std::size_t n = 1000000)
        {
            auto& inst = instance();
            if (inst.calibrated_)
                return;

            TimerGroup* cg = get_or_create_group("__timer_calibration__");

            ThreadLocalTimers& tls = tls_for_current_thread();
            auto& st = tls.get_state(cg);
            st = ThreadLocalGroupState{};

            for (std::size_t i = 0; i < n; ++i) {
                t0(cg);
                t1(cg);
            }

            if (st.interval_count > 0) {
                inst.interval_overhead_ns_ =
                    static_cast<double>(st.raw_ns_group) /
                    static_cast<double>(st.interval_count);
            }
            else {
                inst.interval_overhead_ns_ = 0.0;
            }

            inst.calibrated_ = true;
            st = ThreadLocalGroupState{};
        }

        static double interval_overhead_ns()
        {
            return instance().interval_overhead_ns_;
        }

        // ------------------------ Group control ---------------------------

        static void begin_group(TimerGroup* g)
        {
            if (!g) return;

            auto& inst = instance();

            {
                std::lock_guard<std::mutex> glock(inst.groups_mutex_);
                g->group_start_tp = clock::now();
                g->group_active = true;
            }

            // Reset per-thread accumulators for this group
            {
                std::lock_guard<std::mutex> tlock(inst.threads_mutex_);
                for (auto* tls : inst.threads_) {
                    for (auto& e : tls->entries) {
                        if (e.group == g) {
                            e.state = ThreadLocalGroupState{};
                        }
                    }
                }
            }
        }

        static void end_group(TimerGroup* g)
        {
            if (!g) return;

            auto& inst = instance();

            // 1) Wall-clock duration of the group
            long long wall_ns = 0;
            {
                std::lock_guard<std::mutex> glock(inst.groups_mutex_);
                if (g->group_active) {
                    wall_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
                        clock::now() - g->group_start_tp
                    ).count();
                    g->group_active = false;
                }
                else {
                    wall_ns = g->last_group_wall_ns; // fallback
                }
                g->last_group_wall_ns = wall_ns;
            }

            // 2) Aggregate per-thread raw_ns / interval_count / samples
            const double baseline = inst.interval_overhead_ns_;

            long long raw_total_ns = 0;
            long long total_intervals = 0;
            long long total_samples = 0;
            int       active_threads = 0;

            {
                std::lock_guard<std::mutex> tlock(inst.threads_mutex_);

                for (auto* tls : inst.threads_) {
                    bool used = false;

                    for (auto& e : tls->entries) {
                        if (e.group != g)
                            continue;

                        auto& st = e.state;

                        if (st.raw_ns_group != 0 ||
                            st.interval_count != 0 ||
                            st.sample_count != 0)
                        {
                            used = true;
                        }

                        raw_total_ns += st.raw_ns_group;
                        total_intervals += st.interval_count;
                        total_samples += st.sample_count;

                        st = ThreadLocalGroupState{};
                    }

                    if (used)
                        ++active_threads;
                }
            }

            // 3) Apply baseline once at group level
            long double adj_ld = static_cast<long double>(raw_total_ns);
            if (baseline > 0.0 && total_intervals > 0) {
                long double subtract =
                    static_cast<long double>(baseline) *
                    static_cast<long double>(total_intervals);
                adj_ld -= subtract;
                if (adj_ld < 0.0L)
                    adj_ld = 0.0L;
            }

            long long adj_ns = static_cast<long long>(std::llround(adj_ld));

            // 4) Update group stats and print
            {
                std::lock_guard<std::mutex> glock(inst.groups_mutex_);

                g->last_group_measured_ns = adj_ns;
                g->last_group_samples = total_samples;
                g->group_count += 1;

                g->total_ns += adj_ns;
                g->total_samples += total_samples;

                print_locked(*g, active_threads);
            }
        }

        // ---------------------- Sample control ----------------------------

        static void begin_sample(TimerGroup* g)
        {
            if (!g) return;
            if (!g->group_active) return;

            ThreadLocalTimers& tls = tls_for_current_thread();
            auto& st = tls.get_state(g);

            if (st.sample_depth == 0)
                st.sample_count++;

            ++st.sample_depth;
            g->sample_active = true;
        }

        static void end_sample(TimerGroup* g)
        {
            if (!g) return;
            if (!g->group_active) return;

            ThreadLocalTimers& tls = tls_for_current_thread();
            auto& st = tls.get_state(g);

            if (st.sample_depth > 0)
                --st.sample_depth;

            g->sample_active = false;
        }

        // ---------------------- Compiler barrier --------------------------

        static inline void compiler_barrier()
        {
            std::atomic_signal_fence(std::memory_order_seq_cst);
        }

        // ---------------------- Interval measurement ----------------------

        static inline void t0(TimerGroup* g)
        {
            compiler_barrier();

            ThreadLocalTimers& tls = tls_for_current_thread();
            auto& st = tls.get_state(g);

            if (!g->sample_active)
                return; // this "sample" is currently not accepting measurements

            // Minimal hot-path work; no bounds checks
            st.t0_stack[st.t0_depth++] = clock::now();

            compiler_barrier();
        }

        static inline void t1(TimerGroup* g)
        {
            compiler_barrier();
            auto now = clock::now();
            compiler_barrier();

            ThreadLocalTimers& tls = tls_for_current_thread();
            auto& st = tls.get_state(g);

            if (!g->sample_active)
                return; // this "sample" is currently not accepting measurements

            time_point start = st.t0_stack[--st.t0_depth];

            auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
                now - start
            ).count();

            st.raw_ns_group += ns;
            st.interval_count += 1;
        }

        // ---------------------- Maintenance / reporting -------------------

        static void reset(TimerGroup* g)
        {
            if (!g) return;

            auto& inst = instance();

            {
                std::lock_guard<std::mutex> glock(inst.groups_mutex_);
                g->total_ns = 0;
                g->total_samples = 0;
                g->last_group_measured_ns = 0;
                g->last_group_samples = 0;
                g->last_group_wall_ns = 0;
                g->group_count = 0;
                g->group_active = false;
            }

            std::lock_guard<std::mutex> tlock(inst.threads_mutex_);
            for (auto* tls : inst.threads_) {
                for (auto& e : tls->entries) {
                    if (e.group == g) {
                        e.state = ThreadLocalGroupState{};
                    }
                }
            }
        }

        static void report(TimerGroup* g)
        {
            if (!g) return;
            auto& inst = instance();
            std::lock_guard<std::mutex> glock(inst.groups_mutex_);
            // active_threads unknown here; just pass 1 (PCT is less interesting anyway)
            print_locked(*g, 1);
        }

        static void report_all()
        {
            auto& inst = instance();
            std::lock_guard<std::mutex> glock(inst.groups_mutex_);
            for (auto& kv : inst.groups_) {
                print_locked(kv.second, 1);
            }
        }

    private:
        // -----------------------------------------------------------------
        // Singleton + TLS plumbing
        // -----------------------------------------------------------------

        TimerRegistry() = default;

        static TimerRegistry& instance()
        {
            static TimerRegistry inst;
            return inst;
        }

        static ThreadLocalTimers& tls_for_current_thread()
        {
            thread_local ThreadLocalTimers* tls = nullptr;
            if (!tls) {
                tls = new ThreadLocalTimers();
                auto& inst = instance();
                std::lock_guard<std::mutex> lock(inst.threads_mutex_);
                inst.threads_.push_back(tls);
            }
            return *tls;
        }

        static void print_locked(const TimerGroup& g, int active_threads)
        {
            // EXPECTS: groups_mutex_ held by caller.

            double last_ms = 0.0;
            if (g.last_group_samples > 0 && g.last_group_measured_ns > 0) {
                last_ms = (static_cast<double>(g.last_group_measured_ns) /
                    static_cast<double>(g.last_group_samples));// / 1e3;// 1e6;
            }

            double avg_ms = 0.0;
            if (g.total_samples > 0 && g.total_ns > 0) {
                avg_ms = (static_cast<double>(g.total_ns) /
                    static_cast<double>(g.total_samples));// / 1e3; // 1e6;
            }

            double pct = 0.0;
            if (g.last_group_wall_ns > 0 && g.last_group_measured_ns > 0 && active_threads > 0) {
                const long double denom =
                    static_cast<long double>(g.last_group_wall_ns) *
                    static_cast<long double>(active_threads);
                pct = 100.0 *
                    (static_cast<long double>(g.last_group_measured_ns) / denom);
                if (pct > 100.0)
                    pct = 100.0;
            }

            blPrint(
                "Timer '%s': COUNT=%lld, LAST=%.5f ns, AVG=%.5f ns, PCT=%.1f%%\n",
                g.name.c_str(),
                static_cast<long long>(g.last_group_samples),
                last_ms,
                avg_ms,
                pct
            );
        }

        // Global list of groups (by name)
        std::mutex groups_mutex_;
        std::unordered_map<std::string, TimerGroup> groups_;

        // Global list of per-thread timer states
        std::mutex threads_mutex_;
        std::vector<ThreadLocalTimers*> threads_;

        // Baseline overhead for one t0/t1 pair, in ns
        double interval_overhead_ns_ = 0.0;
        bool   calibrated_ = false;
    };

    // ---------------------------------------------------------------------
    // RAII helpers
    // ---------------------------------------------------------------------

    struct ScopedTimerSample
    {
        TimerGroup* group;
        explicit ScopedTimerSample(TimerGroup* g) : group(g)
        {
            TimerRegistry::begin_sample(group);
        }
        ~ScopedTimerSample()
        {
            TimerRegistry::end_sample(group);
        }
    };

    struct ScopedTimer
    {
        TimerGroup* group;
        explicit ScopedTimer(TimerGroup* g) : group(g)
        {
            TimerRegistry::t0(group);
        }
        ~ScopedTimer()
        {
            TimerRegistry::t1(group);
        }
    };

} // namespace bl


#if BL_TIMERS_ENABLED
#define BL_TIMER_GROUP_PTR(name)                                        \
    ([]() -> bl::TimerGroup* {                                                \
        static bl::TimerGroup* _tg =                                          \
            bl::TimerRegistry::get_or_create_group(name);             \
        return _tg;                                                             \
    }())

// Begin / end a group
#define timer_calibrate_overhead(...)  bl::TimerRegistry::calibrate(__VA_ARGS__);
#define timer_begin_group(name)        bl::TimerRegistry::begin_group(BL_TIMER_GROUP_PTR(#name));
#define timer_end_group(name)          bl::TimerRegistry::end_group(BL_TIMER_GROUP_PTR(#name));
#define timer_begin_sample(name)       bl::TimerRegistry::begin_sample(BL_TIMER_GROUP_PTR(#name));
#define timer_end_sample(name)         bl::TimerRegistry::end_sample(BL_TIMER_GROUP_PTR(#name));
#define timer_t0(name)                 bl::TimerRegistry::t0(BL_TIMER_GROUP_PTR(#name));
#define timer_t1(name)                 bl::TimerRegistry::t1(BL_TIMER_GROUP_PTR(#name));
#define timer_reset(name)              bl::TimerRegistry::reset(BL_TIMER_GROUP_PTR(#name));
#define timer_report_all()             bl::TimerRegistry::report_all();
#define timer_sample(name)             bl::ScopedTimerSample __sample_##name(BL_TIMER_GROUP_PTR(#name));
#define timer_scope(name)              bl::ScopedTimer __subsample_##name(BL_TIMER_GROUP_PTR(#name));

#else

// Begin / end a group
#define timer_calibrate_overhead(...)
#define timer_begin_group(name)
#define timer_end_group(name)
#define timer_begin_sample(name)
#define timer_end_sample(name)
#define timer_t0(name)
#define timer_t1(name)
#define timer_reset(name)
#define timer_report_all()
#define timer_sample(name)
#define timer_scope(name)

#endif
