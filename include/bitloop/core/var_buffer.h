#pragma once

#include "debug.h"

#include <any>
#include <array>
#include <cstring>
#include <sstream>
#include <mutex>
#include <type_traits>
#include <unordered_map>
#include <concepts>

#define VARBUFFER_DEBUG_INFO

template<class U>
concept VarHasHashMethod = requires(const U & u) {
    { u.hash() } -> std::convertible_to<std::size_t>;
};

template<typename T>
concept HasEq = requires(const T & x, const T & y) {
    { x == y } -> std::convertible_to<bool>;
};

template<class T, class Enable = void>
struct TypeOps {
    using Store = std::remove_const_t<T>;

    static void assign(void* dst, const std::any& src)
    {
        const Store& s = std::any_cast<const Store&>(src);
        *static_cast<Store*>(dst) = s;
    }

    static bool equals(const void* a, const std::any& b)
    {
        const Store& A = *static_cast<const Store*>(a);
        const Store& B = std::any_cast<const Store&>(b);

        if constexpr (HasEq<Store>)
            return A == B;
        else if constexpr (std::is_standard_layout_v<Store> && std::is_trivially_copyable_v<Store>)
            return std::memcmp(&A, &B, sizeof(Store)) == 0;
        else
            static_assert(HasEq<Store>, "Store must be equality comparable or trivially copyable");
    }

    // live -> any
    static void store(std::any& dst, const void* src)
    {
        const Store& s = *static_cast<const Store*>(src);

        if (auto* p = std::any_cast<Store>(&dst)) {
            // reuse existing object, use operator=
            *p = s;
        }
        else {
            // first time / wrong type: construct
            dst.emplace<Store>(s);
        }
    }

    // copy any -> any (same underlying type)
    static void copy_any(std::any& dst, const std::any& src)
    {
        const Store& s = std::any_cast<const Store&>(src);

        if (auto* p = std::any_cast<Store>(&dst)) {
            *p = s;
        }
        else {
            dst.emplace<Store>(s);
        }
    }

    // any vs any
    static bool equals_any(const std::any& a, const std::any& b)
    {
        const Store& A = std::any_cast<const Store&>(a);
        const Store& B = std::any_cast<const Store&>(b);

        if constexpr (HasEq<Store>)
            return A == B;
        else if constexpr (std::is_standard_layout_v<Store> && std::is_trivially_copyable_v<Store>)
            return std::memcmp(&A, &B, sizeof(Store)) == 0;
        else
            static_assert(HasEq<Store>, "Store must be equality comparable or trivially copyable");
    }
};


// arrays T[N] => stage as std::array<T,N>, copy element-wise
template<class E, std::size_t N>
struct TypeOps<E[N], void>
{
    using Store = std::array<std::remove_const_t<E>, N>;

    static void assign(void* dst, const std::any& src) {
        const Store& s = std::any_cast<const Store&>(src);
        auto* d = static_cast<std::remove_const_t<E>*>(dst);
        std::copy(s.begin(), s.end(), d);
    }
    static bool equals(const void* dst, const std::any& src) {
        const Store& s = std::any_cast<const Store&>(src);
        const auto* d = static_cast<const E*>(dst);
        return std::equal(s.begin(), s.end(), d);
    }
    static void store(std::any& dst, const void* src) {
        const E* p = static_cast<const E*>(src);

        if (auto* arr = std::any_cast<Store>(&dst)) {
            std::copy(p, p + N, arr->begin());
        }
        else {
            Store s;
            std::copy(p, p + N, s.begin());
            dst.emplace<Store>(std::move(s));
        }
    }
    static void copy_any(std::any& dst, const std::any& src) {
        const Store& s = std::any_cast<const Store&>(src);

        if (auto* arr = std::any_cast<Store>(&dst)) {
            *arr = s; // std::array has operator=
        }
        else {
            dst.emplace<Store>(s);
        }
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

    __PushGuard(const TargetType& t, const T& v) : target(t), target_ref(v) {}
    ~__PushGuard() { target._commit(target_ref); }
};

template<typename TargetType>
class VarBufferInterface
{
public:

    // Note: Live variable reference must be used for all key lookup, even on the UI-side

    template<class T, std::enable_if_t<!std::is_array_v<T>, int> = 0>
    std::remove_const_t<T>& _pull(const T& live_member, bool temp = false, const char* name = nullptr) const
    {
        return __target._pull(live_member, temp, name);
    }

    template<class T, std::size_t N>
    std::array<std::remove_const_t<T>, N>& _pull(const T(&live_member)[N], bool temp = false, const char* name = nullptr) const
    {
        return __target._pull(live_member, temp, name);
    }

    template<class T, std::enable_if_t<!std::is_array_v<T>, int> = 0>
    std::remove_const_t<T>& _temp_pull(const T& live_member, const char* name = nullptr) const
    {
        return __target._pull(live_member, true, name);
    }
    template<class T, std::size_t N>
    auto& _temp_pull(const T(&live_member)[N], const char* name = nullptr) const
    {
        return __target._pull(live_member, true, name);
    }

    // todo: Force a crash/debug break when committing the same member twice before syncing
    template<class T, std::enable_if_t<!std::is_array_v<T>, int> = 0>
    void _commit(const T& live_member, const T& staged) const
    {
        __target._commit(live_member, staged);
    }

    template<class T, std::size_t N>
    void _commit(const T(&live_member)[N], const std::array<T, N>& staged) const
    {
        __target._commit(live_member, staged);
    }

    template<class F>
    void _schedule(F&& f) const
    {
        __target._schedule(std::forward<F>(f));
    }

    const TargetType& __target;

    VarBufferInterface(const TargetType* _target) : __target(*_target) {}
    virtual ~VarBufferInterface() = default;

    virtual void init() {}
    virtual void sidebar() {}
    virtual void overlay() {}

    // push/pull/scope macros (single)
    #define _bl_scoped_one(name)     auto& name = _pull(__target.name, false, #name); __PushGuard<decltype(__target), decltype(__target.name)> __push_guard_##name(__target, __target.name)
    #define _bl_pull_one(name)       auto& name = _pull(__target.name, false, #name)       
    #define _bl_view_one(name)       const auto& name = _pull(__target.name, false, #name)
    #define _bl_pull_temp_one(name)  auto& name = _temp_pull(__target.name, #name)         
    #define _bl_push_one(name)       _commit(__target.name)                                

    // push/pull/scope macros (__VA_ARGS__)
    #define bl_scoped(...)          BL_FOREACH(_bl_scoped_one, __VA_ARGS__)
    #define bl_pull(...)            BL_FOREACH(_bl_pull_one, __VA_ARGS__)        // makes and returns a persistent UI-side clone of variable
    #define bl_view(...)            BL_FOREACH(_bl_view_one, __VA_ARGS__)        // makes and returns a persistent UI-side clone of variable (read-only)
    #define bl_pull_temp(...)       BL_FOREACH(_bl_pull_temp_one, __VA_ARGS__)   // same as bl_pull, but no persistent UI clone (good for one-shot big vars)
    #define bl_push(...)            BL_FOREACH(_bl_push_one, __VA_ARGS__)        // push the pulled-var back to staging area (applied only if var changed)

    #define bl_schedule             _schedule
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
        #ifdef VARBUFFER_DEBUG_INFO
        int id;
        std::string name;
        #endif

        // current shadow value
        std::any value_shadow;

        // snapshots for change detection (size_t hash if value has .hash() method)
        std::any mark_live;    // snapshot of live value
        std::any mark_shadow;  // snapshot of shadow value

        bool hashable = false;
        std::size_t(*hash_from_live_fn)(const void*) = nullptr;   // live ptr -> hash
        std::size_t(*hash_from_any_fn)(const std::any&) = nullptr; // any(value) -> hash

        bool changed = false;  // set by _commit if shadow != mark_shadow
        bool temp = false;

        // type-erased ops
        void (*assign_fn)(void*, const std::any&) = nullptr;                 // any -> live
        bool (*equals_fn)(const void*, const std::any&) = nullptr;           // live vs any
        void (*store_from_live_fn)(std::any&, const void*) = nullptr;        // live -> any
        bool (*equals_any_fn)(const std::any&, const std::any&) = nullptr;   // any vs any

        void (*copy_any_fn)(std::any&, const std::any&) = nullptr;

        void (*print_fn)(std::ostream&, const std::any&) = nullptr;

        // identity
        const void* live_key = nullptr;          // address of tracked (live) member
        const VarBuffer* owner = nullptr;        // back-pointer

        // -------- per-entry helpers --------
        void updateLive() {
            if (!owner || !assign_fn || !value_shadow.has_value() || !live_key) return;
            if (!changed) return;
            assign_fn(const_cast<void*>(live_key), value_shadow);
        }
        void updateShadow() {
            if (!owner || !live_key) return;

            // First time: populate shadow
            if (!value_shadow.has_value()) {
                if (store_from_live_fn) store_from_live_fn(value_shadow, live_key);
                return;
            }

            // only update shadow with live value if the live value actually changed
            if (!liveChanged())
                return; // show_axis (shadow) doesn't turn false because of this return 

            // live changed => update shadow
            if (store_from_live_fn)
                store_from_live_fn(value_shadow, live_key); 
        }

        void markLiveValue() {
            if (!owner || !live_key) return;

            if (hashable && hash_from_live_fn) {
                const std::size_t h = hash_from_live_fn(live_key);

                if (auto* p = std::any_cast<std::size_t>(&mark_live)) {
                    *p = h;                    // reuse existing size_t
                }
                else {
                    mark_live.emplace<std::size_t>(h);
                }
            }
            else if (store_from_live_fn) {
                // this now reuses the contained Store thanks to TypeOps::store
                store_from_live_fn(mark_live, live_key);
            }
        }

        void markShadowValue() {
            if (!value_shadow.has_value()) {
                mark_shadow.reset();
                return;
            }

            if (hashable && hash_from_any_fn) {
                const std::size_t h = hash_from_any_fn(value_shadow);

                if (auto* p = std::any_cast<std::size_t>(&mark_shadow)) {
                    *p = h;                                // reuse size_t
                }
                else {
                    mark_shadow.emplace<std::size_t>(h);   // first time
                }
            }
            else if (copy_any_fn) {
                // reuses the underlying Store when possible, instead of
                // destroying and reconstructing the std::any
                copy_any_fn(mark_shadow, value_shadow);
            }
        }


        bool liveChanged() const {
            if (!live_key) return false;
            if (hashable && hash_from_live_fn && mark_live.has_value()) {
                const std::size_t now = hash_from_live_fn(live_key);
                const std::size_t was = std::any_cast<const std::size_t&>(mark_live);
                return now != was;
            }
            if (!equals_fn || !mark_live.has_value()) return false;
            return !equals_fn(live_key, mark_live);
        }
        bool shadowChanged() const {
            if (!value_shadow.has_value() || !mark_shadow.has_value()) return false;
            if (hashable && hash_from_any_fn) {
                const std::size_t now = hash_from_any_fn(value_shadow);
                const std::size_t was = std::any_cast<const std::size_t&>(mark_shadow);
                return now != was;
            }
            if (!equals_any_fn) return false;
            return !equals_any_fn(value_shadow, mark_shadow);
        }

        std::string to_string_value() const {
            if (!value_shadow.has_value()) return {};
            std::ostringstream oss;
            if (hashable)
                oss << std::any_cast<const std::size_t&>(value_shadow);
            else if (print_fn)
                print_fn(oss, value_shadow);
            return oss.str();
        }

        std::string to_string_marked_shadow() const {
            if (!mark_shadow.has_value()) return {};
            std::ostringstream oss;
            if (hashable)
                oss << std::any_cast<const std::size_t&>(mark_shadow);
            else if (print_fn)
                print_fn(oss, mark_shadow);
            return oss.str();
        }

        std::string to_string_marked_live() const {
            if (!mark_live.has_value()) return {};
            std::ostringstream oss;
            if (hashable)
                oss << std::any_cast<const std::size_t&>(mark_live);
            else if (print_fn)
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

    void invokeScheduledCalls()
    {
        std::vector<Task> tasks;
        tasks.swap(post_commit_tasks);

        TargetType& self = static_cast<TargetType&>(*this);
        for (auto& task : tasks)
            task(self);
    }


    mutable std::unordered_map<const void*, Entry> ui_stage;


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
        if (!e.copy_any_fn)        e.copy_any_fn = +[](std::any& dst, const std::any& src) { Ops::copy_any(dst, src); };

        if constexpr (VarHasHashMethod<Store>) {
            e.hashable = true;
            e.hash_from_live_fn = +[](const void* src) -> std::size_t {
                const Store& s = *static_cast<const Store*>(src);
                return s.hash();
            };
            e.hash_from_any_fn = +[](const std::any& a) -> std::size_t {
                return std::any_cast<const Store&>(a).hash();
            };
        }

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
    std::remove_const_t<T>& _pull(const T& live_member, bool temp = false, const char* name = nullptr) const
    {
        using U = std::remove_const_t<T>;

        const void* live_key = static_cast<const void*>(&live_member);
        auto& e = ui_stage[live_key];

        if (!e.owner) {
            e.owner = this;
            e.live_key = live_key;
            #ifdef VARBUFFER_DEBUG_INFO
            e.id = (int)ui_stage.size();
            e.name = name;
            #endif
        }

        if (!e.value_shadow.has_value())
        {
            e.value_shadow = static_cast<const U&>(live_member);
            bind_ops<T>(e);
        }

        //e.mark_shadow = e.value_shadow;

        e.temp = e.temp || temp;
        return *std::any_cast<U>(&e.value_shadow);
    }

    template<class T, std::size_t N>
    std::array<std::remove_const_t<T>, N>& _pull(const T(&live_member)[N], bool temp = false, const char* name = nullptr) const
    {
        using Store = typename TypeOps<T[N]>::Store;

        const void* live_key = static_cast<const void*>(live_member);
        auto& e = ui_stage[live_key];

        if (!e.owner) {
            e.owner = this;
            e.live_key = live_key;
            #ifdef VARBUFFER_DEBUG_INFO
            e.id = (int)ui_stage.size();
            e.name = name;
            #endif
        }

        if (!e.value_shadow.has_value())
        {
            Store s; std::copy(live_member, live_member + N, s.begin());
            e.value_shadow = std::move(s);
            bind_ops<T[N]>(e);
        }

        // NEW: snapshot "before" shadow value
        //e.mark_shadow = e.value_shadow;

        e.temp = e.temp || temp;
        return *std::any_cast<Store>(&e.value_shadow);
    }

    // scalar
    template<class T, std::enable_if_t<!std::is_array_v<T>, int> = 0>
    void _commit(const T& live_member) const
    {
        const void* live_key = static_cast<const void*>(&live_member);
        auto it = ui_stage.find(live_key);
        if (it == ui_stage.end()) return;
        Entry& e = it->second;

        if (!e.equals_any_fn) bind_ops<T>(e);

        // Use Entry's hash-aware comparator
        e.changed = e.shadowChanged();
    }

    // array (T[N] forwarding overload)
    template<class T, std::size_t N>
    void _commit(const T(&live_member)[N]) const
    {
        const void* live_key = static_cast<const void*>(live_member);
        auto it = ui_stage.find(live_key);
        if (it == ui_stage.end()) return;
        Entry& e = it->second;

        if (!e.equals_any_fn) bind_ops<T[N]>(e);
        e.changed = e.shadowChanged();
    }

    template<class T>
    std::remove_const_t<T>& _temp_pull(const T& live_member) const { return _pull(live_member, true); }
    template<class T, std::size_t N>
    auto& _temp_pull(const T(&live_member)[N]) const { return _pull(live_member, true); }

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

    // only updates shadow vars if the UI hasn't already altered that variable.
    // If UI already altered variable, that takes precedence
    void updateUnchangedShadowVars()
    {
        for (auto& kv : ui_stage)
        {
            if (!kv.second.shadowChanged())
                kv.second.updateShadow();
        }
    }
};
