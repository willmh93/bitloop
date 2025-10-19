#include <functional>
#include <unordered_map>
#include <typeindex>
#include <any>

///--------------------------///
/// Variable Changed Tracker ///
///--------------------------///

class ChangeTracker
{
    template <typename T>
    struct StateMapPair {
        std::unordered_map<T*, T> current;
        std::unordered_map<T*, T> previous;
    };

    struct ClearCommit {
        std::function<void()> clear;
        std::function<void()> commit;
    };

    // one entry per type *in this tracker object*
    mutable std::unordered_map<std::type_index, std::any>       store_;
    mutable std::unordered_map<std::type_index, ClearCommit>    registry_;

    template <typename T>
    StateMapPair<T>& getStateMap() const
    {
        const auto idx = std::type_index(typeid(T));

        auto it = store_.find(idx);
        if (it == store_.end()) {
            // first time we see this T -> create holder and hook up clear/commit
            auto& holder = store_[idx];
            holder.emplace<StateMapPair<T>>();
            auto& maps = std::any_cast<StateMapPair<T>&>(holder);

            registry_[idx] = {
                [&maps]() { maps.current.clear(); maps.previous.clear(); },
                [&maps]() { maps.previous = maps.current; }
            };
            return maps;
        }
        return std::any_cast<StateMapPair<T>&>(it->second);
    }

    template <typename T>
    [[nodiscard]] bool variableChanged(T& var) const
    {
        using NonConstT = std::remove_const_t<T>;
        auto& maps = getStateMap<NonConstT>();

        auto key = const_cast<NonConstT*>(std::addressof(var));
        auto it = maps.previous.find(key);

        if (it != maps.previous.end()) {
            bool changed = (var != it->second);
            maps.current[key] = var;
            return changed;
        }
        else {
            maps.current[key] = var; // todo: Should never *begin* tracking here? Inconsistent results depending on when it's called
            return false;
        }
    }

public:

    ChangeTracker() = default;

    template <typename... Args>
    [[nodiscard]] bool Changed(Args&&... args) const
    {
        return (variableChanged(std::forward<Args>(args)) || ...);
    }

    template <typename T>
    void commitCurrent(T& var)
    {
        getStateMap<T>().current[&var] = var;
    }

    void clearCurrent()
    {
        for (auto& [_, cc] : registry_)
            cc.clear();
    }

    void updateCurrent()
    {
        for (auto& [_, cc] : registry_)
            cc.commit();
    }
};
