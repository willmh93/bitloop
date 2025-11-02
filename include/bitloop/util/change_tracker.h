#include <functional>
#include <unordered_map>
#include <typeindex>
#include <type_traits>
#include <any>
#include <memory>

///--------------------------///
/// Variable Changed Tracker ///
///--------------------------///

class ChangeTracker
{
    template <typename T>
    struct StateMapPair {
        // previous: baseline snapshot taken by the last updateCurrent()
        std::unordered_map<T*, T> previous;
        // current: set of variables we intend to track (we ignore the stored value here)
        std::unordered_map<T*, T> current;
    };

    struct ClearCommit {
        std::function<void()> clear;
        std::function<void()> commit;
    };

    // one entry per type *in this tracker object*
    mutable std::unordered_map<std::type_index, std::any>    store_;
    mutable std::unordered_map<std::type_index, ClearCommit> registry_;

    template <typename T>
    StateMapPair<T>& getStateMap() const
    {
        const auto idx = std::type_index(typeid(T));

        auto it = store_.find(idx);
        if (it == store_.end()) {
            // first time we see this T -> create holder and hook up clear/commit
            auto& holder = store_[idx];
            holder.emplace<StateMapPair<T>>();
            // IMPORTANT: capture [this, idx] so we re-lookup maps each time (safe vs rehash/move)
            registry_[idx] = {
                // clear: wipe "current" (tracked set) AND "previous" (baseline)
                [this, idx]() {
                    auto it2 = store_.find(idx);
                    if (it2 == store_.end()) return;
                    auto& maps = std::any_cast<StateMapPair<T>&>(it2->second);
                    maps.current.clear();
                    maps.previous.clear();
                },
                // commit: build a fresh baseline by dereferencing tracked pointers *now*
                [this, idx]() {
                    auto it2 = store_.find(idx);
                    if (it2 == store_.end()) return;
                    auto& maps = std::any_cast<StateMapPair<T>&>(it2->second);

                    std::unordered_map<T*, T> next_prev;
                    next_prev.reserve(maps.previous.size() + maps.current.size());

                    auto add_snapshot = [&](T* p) {
                        if (p) next_prev[p] = *p; // snapshot live value at commit time
                    };

                    // keep previously known vars, even if not re-registered this frame
                    for (auto& kv : maps.previous) add_snapshot(kv.first);
                    // include any newly tracked vars
                    for (auto& kv : maps.current)  add_snapshot(kv.first);

                    maps.previous.swap(next_prev);
                    // optional: leave maps.current as the "tracked set" persistent across commits
                    // If you prefer re-declare each frame, uncomment the next line:
                    // maps.current.clear();
                }
            };
            return std::any_cast<StateMapPair<T>&>(holder);
        }
        return std::any_cast<StateMapPair<T>&>(it->second);
    }

    template <typename T>
    [[nodiscard]] bool variableChanged(T& var) const
    {
        using NonConstT = std::remove_const_t<T>;
        auto& maps = getStateMap<NonConstT>();

        auto* key = const_cast<NonConstT*>(std::addressof(var));
        auto  it = maps.previous.find(key);

        if (it == maps.previous.end()) {
            // Not in baseline yet -> treat as "no change" until first updateCurrent() snapshot.
            // (Call commitCurrent(var) once to start tracking, or rely on prior updateCurrent().)
            //commitCurrent(var);
            maps.current[key] = var;
            return false;
        }
        return !(var == it->second);
    }

public:
    ChangeTracker() = default;

    template <typename... Args>
    [[nodiscard]] bool Changed(Args&&... args) const
    {
        // Pure check: no "current" writes here
        return (variableChanged(std::forward<Args>(args)) || ...);
    }

    // Register/stage a variable so updateCurrent() will snapshot it.
    // You can call this once at setup (the stored value is ignored at commit time).
    template <typename T>
    void commitCurrent(T& var) const
    {
        getStateMap<std::remove_const_t<T>>().current[std::addressof(var)] = var;
    }

    // Convenience: register many at once
    template <typename... Args>
    void commitCurrentMany(Args&... args) const
    {
        (commitCurrent(args), ...);
    }

    // Clear both tracked set and baseline for all types
    void clearCurrent()
    {
        for (auto& [_, cc] : registry_) cc.clear();
    }

    // Snapshot live values of all tracked variables *now* into the baseline.
    void updateCurrent()
    {
        for (auto& [_, cc] : registry_) cc.commit();
    }
};
