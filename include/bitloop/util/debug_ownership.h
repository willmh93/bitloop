#pragma once

#include "hashable.h"
#include <bitloop/core/debug.h>

#include <atomic>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <mutex>
#include <thread>
#include <unordered_map>

namespace bl::debug
{
    using bl::hash_t;

    #if BL_DEBUG_OWNERSHIP

    inline void debug_break() noexcept
    {
        #if defined(_MSC_VER)
        __debugbreak();
        #elif defined(__clang__) || defined(__GNUC__)
        __builtin_trap();
        #else
        //std::abort();
        #endif
    }

    inline std::uint64_t thread_token() noexcept
    {
        static std::atomic<std::uint64_t> g_next{ 1 }; // 0 => "no owner"
        thread_local std::uint64_t t = g_next.fetch_add(1, std::memory_order_relaxed);
        return t;
    }

    struct OwnershipSlot
    {
        std::atomic<std::uint64_t> owner{ 0 }; // thread_token()
        std::atomic<std::uint32_t> depth{ 0 }; // re-entrant depth
        hash_t id{};
        const char* label = nullptr;         // for diagnostics only

        // Debug “held-at” metadata (only meaningful when depth > 0)
        std::atomic<const char*> held_file{ nullptr };
        std::atomic<const char*> held_func{ nullptr };
        std::atomic<int>         held_line{ 0 };
    };

    class OwnershipRegistry
    {
    public:
        static OwnershipRegistry& instance()
        {
            static OwnershipRegistry r;
            return r;
        }

        OwnershipSlot& slot_for(hash_t id, const char* label)
        {
            std::lock_guard<std::mutex> lock(m_);
            auto it = map_.find(id);
            if (it != map_.end())
            {
                // Collision / mismatch detection
                OwnershipSlot* s = it->second;
                if (s->label && label && s->label != label)
                {
                    // Same hash used with different labels; likely collision or inconsistent labeling.
                    std::fprintf(stderr,
                        "[OwnershipRegistry] hash id collision / mismatch\n"
                        "  id=%llu\n"
                        "  existing label=%s\n"
                        "  new      label=%s\n",
                        (unsigned long long)id, s->label, label);
                    debug_break();
                    std::abort();
                }
                return *s;
            }

            OwnershipSlot* s = new OwnershipSlot();
            s->id = id;
            s->label = label;
            map_.emplace(id, s);
            return *s;
        }

    private:
        std::mutex m_;
        std::unordered_map<hash_t, OwnershipSlot*> map_;
    };

    class OwnershipGuard
    {
    public:
        OwnershipGuard(OwnershipSlot& slot, const char* file, int line, const char* func) noexcept
            : slot_(&slot), file_(file), line_(line), func_(func)
        {
            acquire();
        }

        OwnershipGuard(const OwnershipGuard&) = delete;
        OwnershipGuard& operator=(const OwnershipGuard&) = delete;

        ~OwnershipGuard() noexcept
        {
            if (slot_) release();
        }

    private:
        void acquire() noexcept
        {
            const std::uint64_t me = thread_token();

            // Re-entrant by same thread
            const std::uint64_t cur = slot_->owner.load(std::memory_order_acquire);
            if (cur == me)
            {
                slot_->depth.fetch_add(1, std::memory_order_relaxed);
                return;
            }

            std::uint64_t expected = 0;
            if (!slot_->owner.compare_exchange_strong(expected, me,
                std::memory_order_acq_rel,
                std::memory_order_acquire))
            {
                const char* hfile = slot_->held_file.load(std::memory_order_acquire);
                const char* hfunc = slot_->held_func.load(std::memory_order_acquire);
                const int   hline = slot_->held_line.load(std::memory_order_acquire);

                blPrint(
                    "[OwnershipGuard] DATA RACE RISK: resource already owned\n"
                    "  label=%s\n"
                    "  id=%llu\n"
                    "  current-owner token=%llu\n"
                    "  contender     token=%llu\n"
                    "  held at: %s:%d (%s)\n"
                    "  contended at: %s:%d (%s)\n",
                    slot_->label ? slot_->label : "(null)",
                    (unsigned long long)slot_->id,
                    (unsigned long long)expected,
                    (unsigned long long)me,
                    hfile ? hfile : "(unknown)", hline, hfunc ? hfunc : "(unknown)",
                    file_, line_, func_);

            }

            // After successfully claiming ownership:
            slot_->held_file.store(file_, std::memory_order_release);
            slot_->held_func.store(func_, std::memory_order_release);
            slot_->held_line.store(line_, std::memory_order_release);

            slot_->depth.store(1, std::memory_order_relaxed);

        }

        void release() noexcept
        {
            const std::uint64_t me = thread_token();
            const std::uint64_t cur = slot_->owner.load(std::memory_order_relaxed);

            if (cur != me)
            {
                blPrint(
                    "[OwnershipGuard] RELEASE mismatch\n"
                    "  label=%s\n"
                    "  id=%llu\n"
                    "  held-by token=%llu\n"
                    "  releaser token=%llu\n"
                    "  at: %s:%d (%s)\n",
                    slot_->label ? slot_->label : "(null)",
                    (unsigned long long)slot_->id,
                    (unsigned long long)cur,
                    (unsigned long long)me,
                    file_, line_, func_);
                debug_break();
                //std::abort();
            }

            const std::uint32_t d = slot_->depth.fetch_sub(1, std::memory_order_relaxed);
            if (d == 1)
                slot_->owner.store(0, std::memory_order_release);
        }

    private:
        OwnershipSlot* slot_{ nullptr };
        const char* file_{ nullptr };
        int line_{ 0 };
        const char* func_{ nullptr };
    };

    template<hash_t ID>
    inline OwnershipSlot& cached_slot(const char* label)
    {
        // One-time map lookup per (ID) per translation unit; then just a pointer deref
        static OwnershipSlot* p =
            &OwnershipRegistry::instance().slot_for(ID, label);
        return *p;
    }

    #define BL_JOIN_IMPL(a,b) a##b
    #define BL_JOIN(a,b) BL_JOIN_IMPL(a,b)

    #define BL_TAKE_OWNERSHIP(lit) \
      bl::debug::OwnershipGuard BL_JOIN(_bl_own_, __COUNTER__)( \
          bl::debug::cached_slot<bl::StableHasher::hash_lit(lit)>(lit), \
        __FILE__, __LINE__, __func__)

    #define BL_TAKE_OWNERSHIP_ID(id_expr, label_lit) \
    ::bl::debug::OwnershipGuard BL_JOIN(_bl_own_, __COUNTER__)( \
        ::bl::debug::OwnershipRegistry::instance().slot_for((id_expr), (label_lit)), \
        __FILE__, __LINE__, __func__)

    #else

    #define BL_TAKE_OWNERSHIP(lit)                 ((void)0)
    #define BL_TAKE_OWNERSHIP_ID(id_expr,label)    ((void)0)

    #endif
} // namespace bl::debug
