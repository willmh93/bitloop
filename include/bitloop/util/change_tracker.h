#include <functional>
#include <unordered_map>
#include <typeindex>
#include <type_traits>
#include <any>
#include <memory>
#include <concepts>

template<class U>
concept HasHashMethod = requires(const U & u) {
    { u.hash() } -> std::convertible_to<std::size_t>;
};

///--------------------------///
/// Variable Changed Tracker ///
///--------------------------///

class ChangeTracker
{
    template <typename T>
    struct StateMapPair {
        using Baseline = std::conditional_t<HasHashMethod<T>, std::size_t, T>;
        std::unordered_map<T*, Baseline> previous;
        std::unordered_map<T*, unsigned char> current;
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
            auto& holder = store_[idx];
            holder.emplace<StateMapPair<T>>();

            registry_[idx] = {
                // clear: wipe both sets
                [this, idx]() {
                    auto it2 = store_.find(idx);
                    if (it2 == store_.end()) return;
                    auto& maps = std::any_cast<StateMapPair<T>&>(it2->second);
                    maps.current.clear();
                    maps.previous.clear();
                },
                // commit: update baseline only when changed; add first-time snapshots
                [this, idx]() {
                    auto it2 = store_.find(idx);
                    if (it2 == store_.end()) return;
                    auto& maps = std::any_cast<StateMapPair<T>&>(it2->second);

                    // 1) Existing tracked vars => write only if changed
                    for (auto& kv : maps.previous) {
                        T* p = kv.first;
                        if (!p) continue;

                        if constexpr (HasHashMethod<T>) {
                            const std::size_t h = p->hash();
                            if (kv.second != h) kv.second = h;
                        } else {
                            const T& live = *p;
                            if (!(kv.second == live)) kv.second = live; // LHS non-const map value
                        }
                    }

                    // 2) Newly staged vars => add snapshot if not present
                    for (auto& kv : maps.current) {
                        T* p = kv.first;
                        if (!p) continue;
                        if (maps.previous.find(p) == maps.previous.end()) {
                            if constexpr (HasHashMethod<T>) {
                                maps.previous.emplace(p, p->hash());
                            } else {
                                maps.previous.emplace(p, *p);
                            }
                        }
                    }
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
            // not in baseline yet => start tracking.
            // report "no change" until first snapshot
            maps.current[key] = 0;
            return false;
        }

        if constexpr (HasHashMethod<NonConstT>) {
            // compare hashes
            return it->second != var.hash();
        }
        else {
            // keep non-const object on LHS to allow non-const member operator==
            return !(it->second == var);
        }
    }


public:
    ChangeTracker() = default;

    template <typename... Args>
    [[nodiscard]] bool Changed(Args&&... args) const
    {
        // Pure check: no "current" writes here
        return (variableChanged(std::forward<Args>(args)) || ...);
    }

    template <typename T>
    void commitCurrent(T& var) const
    {
        getStateMap<std::remove_const_t<T>>().current[std::addressof(var)] = 0;
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
