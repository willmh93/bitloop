#pragma once

#include "debug.h"

#include <any>
#include <array>
#include <cstring>
#include <sstream>
#include <mutex>
#include <algorithm>
#include <type_traits>
#include <unordered_map>


template<class T, class Enable = void>
struct TypeOps {
    using Store = std::remove_const_t<T>;

    static void assign(void* dst, const std::any& src) {
        const Store& s = std::any_cast<const Store&>(src);
        *static_cast<Store*>(dst) = s;
    }
    static bool equals(const void* dst, const std::any& src) {
        const Store& s = std::any_cast<const Store&>(src);
        const Store& d = *static_cast<const Store*>(dst);
        if constexpr (std::is_standard_layout_v<Store> && std::is_trivially_copyable_v<Store>)
            return std::memcmp(&d, &s, sizeof(Store)) == 0;
        else
            return d == s;
    }
    // copy live memory -> any
    static void store(std::any& dst, const void* src) {
        const Store& s = *static_cast<const Store*>(src);
        dst = s;

        /// For operator=(...)
        ///if (dst.type() == typeid(Store)) {
        ///    std::any_cast<Store&>(dst) = s;
        ///}
        ///else {
        ///    dst.emplace<Store>(s);
        ///}
    }
    // compare any vs any (same underlying type)
    static bool equals_any(const std::any& a, const std::any& b) {
        const Store& A = std::any_cast<const Store&>(a);
        const Store& B = std::any_cast<const Store&>(b);
        if constexpr (std::is_standard_layout_v<Store> && std::is_trivially_copyable_v<Store>)
            return std::memcmp(&A, &B, sizeof(Store)) == 0;
        else
            return A == B;
    }
};

// arrays T[N] => stage as std::array<T,N>, copy element-wise
template<class E, std::size_t N>
struct TypeOps<E[N], void>
{
    using Store = std::array<std::remove_const_t<E>, N>;

    static void assign(void* dst, const std::any& src) {
        const Store& s = std::any_cast<const Store&>(src);
        auto* d = static_cast<std::remove_const_t<E>*>(dst); // points to E[N]
        std::copy(s.begin(), s.end(), d);
    }
    static bool equals(const void* dst, const std::any& src) {
        const Store& s = std::any_cast<const Store&>(src);
        const auto* d = static_cast<const E*>(dst);
        return std::equal(s.begin(), s.end(), d);
    }
    static void store(std::any& dst, const void* src) {
        const E* p = static_cast<const E*>(src);
        Store s; std::copy(p, p + N, s.begin());
        dst = std::move(s);
    }
    static bool equals_any(const std::any& a, const std::any& b) {
        const Store& A = std::any_cast<const Store&>(a);
        const Store& B = std::any_cast<const Store&>(b);
        return std::equal(A.begin(), A.end(), B.begin());
    }
};

template<class TargetType, typename T>
struct __PushGuard
{
    const TargetType& target;
    const T& target_ref;
    const T& staged_ref;

    __PushGuard(const TargetType& t, const T& v, const T& s) : target(t), target_ref(v), staged_ref(s) {}
    ~__PushGuard() { target._commit(target_ref, staged_ref); }
};

template<typename TargetType>
class VarBufferInterface
{
public:

    template<class T, std::enable_if_t<!std::is_array_v<T>, int> = 0>
    std::remove_const_t<T>& _pull(const T& member, bool temp = false, const char* name = nullptr) const
    {
        return __target._pull(member, temp, name);
    }

    template<class T, std::size_t N>
    std::array<std::remove_const_t<T>, N>& _pull(const T(&arr)[N], bool temp = false, const char* name = nullptr) const
    {
        return __target._pull(arr, temp, name);
    }

    template<class T, std::enable_if_t<!std::is_array_v<T>, int> = 0>
    std::remove_const_t<T>& _temp_pull(const T& member, bool /*ignored*/ = true, const char* name = nullptr) const
    {
        return __target._pull(member, true, name);
    }
    template<class T, std::size_t N>
    auto& _temp_pull(const T(&arr)[N], bool /*ignored*/ = true, const char* name = nullptr) const
    {
        return __target._pull(arr, true, name);
    }

    // todo: Force a crash/debug break when committing the same member twice before syncing
    template<class T, std::enable_if_t<!std::is_array_v<T>, int> = 0>
    void _commit(const T& member, const T& staged) const
    {
        __target._commit(member, staged);
    }

    // todo: Force a crash/debug break when committing the same member twice before syncing
    template<class T, std::size_t N>
    void _commit(const T(&member)[N], const std::array<T, N>& staged) const
    {
        __target._commit(member, staged);
    }

    template<class F>
    void _schedule(F&& f) const
    {
        __target._schedule(std::forward<F>(f));
    }

    const TargetType& __target;

    VarBufferInterface(const TargetType* _target) : __target(*_target) {}
    virtual ~VarBufferInterface() = default;

    virtual void sidebar() = 0;
    virtual void overlay() {}

    // 4) Public macro
    #define bl_scoped_one(name) \
        auto& name = _pull(__target.name, false, #name); __PushGuard<decltype(__target), decltype(__target.name)> __push_guard_##name(__target, __target.name, name)
    #define bl_scoped(...) \
        BL_FOREACH(bl_scoped_one, __VA_ARGS__)

    #define bl_schedule         _schedule
    #define bl_pull(name)       auto& name = _pull(__target.name, false, #name)
    #define bl_pull_temp(name)  auto& name = _temp_pull(__target.name, true, #name)
    #define bl_push(name)       _commit(__target.name, name)
};

// detect ostream support
template<class T>
concept Ostreamable = requires(std::ostream & os, const T & t) { os << t; };

template<class T> struct is_std_array : std::false_type {};
template<class U, std::size_t N> struct is_std_array<std::array<U, N>> : std::true_type {};
template<class T> inline constexpr bool is_std_array_v = is_std_array<T>::value;

template<class TargetType>
struct VarBuffer
{
    VarBuffer() = default;
    VarBuffer(const VarBuffer&) = delete;
    VarBuffer& operator=(const VarBuffer&) = delete;
    VarBuffer(VarBuffer&&) = delete;
    VarBuffer& operator=(VarBuffer&&) = delete;

    struct Entry
    {
        std::string name;

        // current staged value (shadow for ui_stage; live->ui for live_stage)
        std::any value;

        // snapshots for change detection
        std::any mark_live;    // snapshot of *live* memory
        std::any mark_shadow;  // snapshot of current shadow (value)

        bool changed = false;  // set by _commit if shadow != mark_shadow
        bool temp = false;

        // type-erased ops
        void (*assign_fn)(void*, const std::any&) = nullptr;                 // any -> live
        bool (*equals_fn)(const void*, const std::any&) = nullptr;           // live vs any
        void (*store_from_live_fn)(std::any&, const void*) = nullptr;        // live -> any
        bool (*equals_any_fn)(const std::any&, const std::any&) = nullptr;   // any vs any

        void (*print_fn)(std::ostream&, const std::any&) = nullptr;

        // identity
        const void* key = nullptr;               // address of tracked member
        const VarBuffer* owner = nullptr;        // back-pointer

        // -------- per-entry helpers --------
        void updateLive() {
            if (!owner || !assign_fn || !value.has_value() || !key) return;
            if (!changed) return;
            assign_fn(const_cast<void*>(key), value);
        }
        void updateShadow() {
            if (!owner || !store_from_live_fn || !key) return;
            store_from_live_fn(value, key);
        }
        void markLiveValue() {
            if (!owner || !store_from_live_fn || !key) return;
            store_from_live_fn(mark_live, key);
        }
        void markShadowValue() {
            if (value.has_value()) mark_shadow = value;
            else mark_shadow.reset();
        }
        bool liveChanged() const {
            if (!equals_fn || !mark_live.has_value() || !key) return false;
            return !equals_fn(key, mark_live);
        }
        bool shadowChanged() const {
            if (!equals_any_fn || !value.has_value() || !mark_shadow.has_value()) return false;
            return !equals_any_fn(value, mark_shadow);
        }
        std::string to_string_value() const {
            if (!value.has_value() || !print_fn) return {};
            std::ostringstream oss;
            print_fn(oss, value);
            return oss.str();
        }
        std::string to_string_marked_shadow() const {
            if (!value.has_value() || !print_fn) return {};
            std::ostringstream oss;
            print_fn(oss, mark_shadow);
            return oss.str();
        }
        std::string to_string_marked_live() const {
            if (!value.has_value() || !print_fn) return {};
            std::ostringstream oss;
            print_fn(oss, mark_live);
            return oss.str();
        }
    };


    // post-commit (after UI->Live) task queue
    using Task = std::function<void(TargetType&)>;
    mutable std::mutex m_tasks;
    mutable std::vector<Task> post_commit_tasks;

    template<class F>
    void _schedule(F&& f) const
    {
        static_assert(std::is_invocable_v<F&, TargetType&>, "_schedule expects a callable(TargetType&)");
        post_commit_tasks.emplace_back(std::forward<F>(f));
    }

    void runScheduledCalls()
    {
        std::vector<Task> tasks;
        tasks.swap(post_commit_tasks);

        TargetType& self = static_cast<TargetType&>(*this);
        for (auto& task : tasks)
            task(self);
    }


    mutable std::unordered_map<const void*, Entry> ui_stage;
    mutable std::unordered_map<const void*, Entry> live_stage;
    //mutable std::mutex m_ui, m_live;

    template<class T> std::remove_const_t<T>& _pull(T&&) const = delete;
    template<class T> void _commit(T&&, const T&) const = delete;

    // ---- bind helpers ----
    template<class T>
    static void bind_ops(Entry& e)
    {
        using Ops = TypeOps<T>;
        using Store = typename TypeOps<T>::Store;

        if (!e.assign_fn)          e.assign_fn = +[](void* d, const std::any& a) { if (!Ops::equals(d, a)) Ops::assign(d, a); };
        if (!e.equals_fn)          e.equals_fn = +[](const void* d, const std::any& a) { return Ops::equals(d, a); };
        if (!e.store_from_live_fn) e.store_from_live_fn = +[](std::any& dst, const void* src) { Ops::store(dst, src); };
        if (!e.equals_any_fn)      e.equals_any_fn = +[](const std::any& a, const std::any& b) { return Ops::equals_any(a, b); };

        if (!e.print_fn) {
            e.print_fn = +[](std::ostream& os, const std::any& a [[maybe_unused]] ) {
                if constexpr (Ostreamable<Store>) {
                    os << std::any_cast<const Store&>(a);
                }
                else if constexpr (is_std_array_v<Store>) {
                    const auto& arr = std::any_cast<const Store&>(a);
                    os << '[';
                    for (std::size_t i = 0; i < arr.size(); ++i) {
                        if (i) os << ',';
                        if constexpr (Ostreamable<typename Store::value_type>) {
                            os << arr[i];
                        }
                        else {
                            os << "<elem>";
                        }
                    }
                    os << ']';
                }
                else {
                    os << "<invalid>";
                }
            };
        }
    }

    // ---- UI side ----
    template<class T, std::enable_if_t<!std::is_array_v<T>, int> = 0>
    std::remove_const_t<T>& _pull(const T& member, bool temp = false, const char* name = nullptr) const
    {
        using U = std::remove_const_t<T>;

        const void* key = static_cast<const void*>(&member);
        auto& e = ui_stage[key];

        if (!e.owner) {
            e.owner = this;
            e.key = key;
            e.name = name;
        }

        if (!e.value.has_value())
        {
            e.value = static_cast<const U&>(member);
            bind_ops<T>(e);
        }

        //e.mark_shadow = e.value;

        e.temp = e.temp || temp;
        return *std::any_cast<U>(&e.value);
    }

    template<class T, std::size_t N>
    std::array<std::remove_const_t<T>, N>& _pull(const T(&arr)[N], bool temp = false, const char* name = nullptr) const
    {
        using Store = typename TypeOps<T[N]>::Store;

        const void* key = static_cast<const void*>(arr);
        auto& e = ui_stage[key];

        if (!e.owner) {
            e.owner = this;
            e.key = key;
            e.name = name;
        }

        if (!e.value.has_value())
        {
            Store s; std::copy(arr, arr + N, s.begin());
            e.value = std::move(s);
            bind_ops<T[N]>(e);
        }

        // NEW: snapshot "before" shadow value
        //e.mark_shadow = e.value;

        e.temp = e.temp || temp;
        return *std::any_cast<Store>(&e.value);
    }

    // _commit now ONLY compares shadow vs snapshot; no writes.
    template<class T, std::enable_if_t<!std::is_array_v<T>, int> = 0>
    void _commit(const T& member, const T& /*staged_value*/) const
    {
        const void* key = static_cast<const void*>(&member);
        auto it = ui_stage.find(key);
        if (it == ui_stage.end()) return;
        Entry& e = it->second;

        if (!e.equals_any_fn) bind_ops<T>(e);
        e.changed = (e.value.has_value() && e.mark_shadow.has_value())
            ? !e.equals_any_fn(e.value, e.mark_shadow)
            : false;
    }

    template<class T, std::size_t N>
    void _commit(const T(&member)[N], const std::array<T, N>& /*staged*/) const
    {
        const void* key = static_cast<const void*>(member);
        auto it = ui_stage.find(key);
        if (it == ui_stage.end()) return;
        Entry& e = it->second;

        if (!e.equals_any_fn) bind_ops<T[N]>(e);
        e.changed = (e.value.has_value() && e.mark_shadow.has_value())
            ? !e.equals_any_fn(e.value, e.mark_shadow)
            : false;
    }

    template<class T, std::size_t N>
    void _commit(const T(&member)[N], const T(&)[N]) const
    {
        const void* key = static_cast<const void*>(member);
        auto it = ui_stage.find(key);
        if (it == ui_stage.end()) return;
        Entry& e = it->second;

        if (!e.equals_any_fn) bind_ops<T[N]>(e);
        e.changed = (e.value.has_value() && e.mark_shadow.has_value())
            ? !e.equals_any_fn(e.value, e.mark_shadow)
            : false;
    }

    template<class T>
    std::remove_const_t<T>& _temp_pull(const T& member) const { return _pull(member, true); }
    template<class T, std::size_t N>
    auto& _temp_pull(const T(&arr)[N]) const { return _pull(arr, true); }

    // ---- Live -> UI staging (unchanged except owner/key/bind) ----
    template<class T, std::enable_if_t<!std::is_array_v<T>, int> = 0>
    void stage_to_ui(const T& member, bool temp = false)
    {
        using U = std::remove_const_t<T>;

        const void* key = static_cast<const void*>(&member);
        auto& e = live_stage[key];

        if (!e.owner)
        {
            e.owner = this;
            e.key = key;
        }

        e.value = static_cast<const U&>(member);
        bind_ops<T>(e);
        e.changed = true;
        e.temp = e.temp || temp;
    }

    template<class T, std::size_t N>
    void stage_to_ui(const T(&member)[N], bool temp = false)
    {
        using Store = typename TypeOps<T[N]>::Store;
        Store s; std::copy(member, member + N, s.begin());

        const void* key = static_cast<const void*>(member);
        auto& e = live_stage[key];
        if (!e.owner)
        {
            e.owner = this;
            e.key = key;
        }
        e.value = std::move(s);
        bind_ops<T[N]>(e);
        e.changed = true;
        e.temp = e.temp || temp;
    }

    // -------- apply to all tracked vars in ui_stage --------
    void updateLive() {
        for (auto& kv : ui_stage) kv.second.updateLive();
    }
    void updateShadow() {
        for (auto& kv : ui_stage) kv.second.updateShadow();
    }
    void markLiveValue() {
        for (auto& kv : ui_stage) kv.second.markLiveValue();
    }
    void markShadowValue() {
        for (auto& kv : ui_stage) kv.second.markShadowValue();
    }
    bool liveChanged() const {
        for (auto const& kv : ui_stage)
            if (kv.second.liveChanged()) return true;
        return false;
    }
    bool shadowChanged() const {
        for (auto const& kv : ui_stage)
            if (kv.second.shadowChanged()) return true;
        return false;
    }
};
