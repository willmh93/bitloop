#pragma once
#define _DISABLE_CONSTEXPR_MUTEX_CONSTRUCTOR 
#include <mutex>
#include <array>
#include <tuple>
#include <typeindex>
#include <functional>
#include <unordered_map>
#include <memory>

#include "debug.h"


namespace Helpers
{
    std::string floatToCleanString(float value, int decimal_places = 6, float precision = 0.0f, bool minimize = true);

    // Wrapping/Unwrapping strings with '\n' at given length
    std::string wrapString(const std::string& input, size_t width);
    std::string unwrapString(const std::string& input);
}

///--------------------------------///
/// constexpr function dispatcher  ///
///--------------------------------///

// Notes:
// > Supports only boolean template args for now
// > Table size grows exponentially, lots of bools will result in larger binary size

template <size_t I, size_t N>
constexpr auto makeBoolsTuple()
{
    // We expand over [0..N-1] and produce a tuple of integral_constant<bool, …>
    return[]<size_t... Bs>(std::index_sequence<Bs...>) {
        return std::tuple{
            std::integral_constant<bool, ((I >> (N - 1 - Bs)) & 1)>{}...
        };
    }(std::make_index_sequence<N>{});
}

// Step 1: the "core" – what does f(bool_c<0>, bool_c<1>...) return?
template<typename F, std::size_t... Is>
using dispatch_result_t_ =
std::invoke_result_t<F,
    std::integral_constant<bool, ((void)Is, false)>...>;

// Step 2: unwrap std::index_sequence into the pack <Is...> 
template<typename F, typename Seq>
struct dispatch_result_impl;                     // primary template

template<typename F, std::size_t... Is>
struct dispatch_result_impl<F, std::index_sequence<Is...>>   // specialization
{
    using type = dispatch_result_t_<F, Is...>;
};

// Step 3: public alias
template<typename F, std::size_t N>
using dispatch_result_t =
typename dispatch_result_impl<F, std::make_index_sequence<N>>::type;

// Build the dispatch table by enumerating all bitmask combinations
template<typename F, std::size_t... Is>
auto dispatchBooleansImpl(const std::array<bool, sizeof...(Is)>& flags,
    F&& f,
    std::index_sequence<Is...>) -> dispatch_result_t<F, sizeof...(Is)>
{
    using Ret = dispatch_result_t<F, sizeof...(Is)>;

    // Compute the flat index from the actual runtime flags
    int index = 0;
    const size_t shift = sizeof...(Is) - 1;
    ((index |= (flags[Is] ? 1 : 0) << (shift - Is)), ...);

    using FnPtr = Ret(*)(F&);

    static constexpr std::array<FnPtr, 1 << sizeof...(Is)> dispatch_table = [] {
        std::array<FnPtr, 1 << sizeof...(Is)> table{};

        [&] <std::size_t... Idxs>(std::index_sequence<Idxs...>)
        {
            ((table[Idxs] = +[](F& f_inner) -> Ret
            {
                constexpr auto bools =
                    makeBoolsTuple<Idxs, sizeof...(Is)>();

                if constexpr (std::is_void_v<Ret>)
                {
                    std::apply([&](auto... Bs) { f_inner(Bs...); }, bools);
                }
                else
                {
                    return std::apply(
                        [&](auto... Bs) { return f_inner(Bs...); }, bools);
                }
            }), ...);
        }(std::make_index_sequence<1 << sizeof...(Is)>{});

        return table;
    }();

    if constexpr (std::is_void_v<Ret>)
    {
        dispatch_table[index](f);
    }
    else
    {
        return dispatch_table[index](f);
    }
}

template <typename... Bools, typename F>
decltype(auto) dispatchBooleans(F&& f, Bools... flags)
{
    static_assert((std::is_same_v<Bools, bool> && ...), "All flags must be bool");
    auto flagArray = std::array{ flags... };
    return dispatchBooleansImpl(flagArray,
        std::forward<F>(f),
        std::make_index_sequence<sizeof...(Bools)>{});
}
// Helper that creates a function lookup table for each boolean combination
// usage:  dispatchBooleans( bools_template(FUNC_NAME, [CAPTURE], FUNC_ARGS), BOOL_ARGS )
// note:   The function call itself is not likely to get inlined

//#define boolsTemplate(func, capture, ...) capture<typename... Bools>(Bools... passed) { func<Bools::value...>(__VA_ARGS__); }

#define boolsTemplate(func, capture, ...)            \
    capture <typename... Bools>(Bools...)            \
    -> decltype(auto) {                              \
        return func<Bools::value...>(__VA_ARGS__);   \
    }

class VariableChangedTracker
{

    /// Holds the two maps for a particular type T
    template <typename T>
    struct StateMapPair {
        std::unordered_map<T*, T> current;
        std::unordered_map<T*, T> previous;
    };

    /// A simple record to store how we clear and commit for each type
    struct ClearCommit {
        std::function<void()> clearer;
        std::function<void()> committer;
    };

    mutable std::unordered_map<std::type_index, ClearCommit> registry_;
    std::mutex mutex_;


    /// The per-type map is allocated once per type T via a static local; we also
    /// register it (lazily) so that global clear/commit can iterate over it.
    template <typename T>
    StateMapPair<T>& getStateMap() const
    {
        static StateMapPair<T> maps;
        static bool registered = registerType<T>(maps);
        (void)registered; // silence unused warning
        return maps;
    }

    /// Register the given maps clear/commit methods in our registry
    template <typename T>
    bool registerType(StateMapPair<T>& maps) const
    {
        //std::lock_guard<std::mutex> lock(mutex_);

        registry_[std::type_index(typeid(T))] = {
            // Clearer: reset both 'current' and 'previous'
            [&]() {
                maps.current.clear();
                maps.previous.clear();
            },
            // Committer: copy 'current' => 'previous'
            [&]() {
                maps.previous = maps.current;
            }
        };
        return true;
    }

public:

    VariableChangedTracker() = default;

    /// Obtain a single global instance if you want to use it as a singleton
    static VariableChangedTracker& instance()
    {
        static VariableChangedTracker s_instance;
        return s_instance;
    }

    /// Returns true if 'var' differs from its last committed value, false otherwise.
    /*template <typename T>
    bool variableChanged(T& var) const
    {
        using NonConstT = typename std::remove_const<T>::type;
        auto& maps = getStateMap<NonConstT>();

        // Remove constness from the address
        auto key = const_cast<NonConstT*>(&var);

        auto it = maps.previous.find(key);
        if (it != maps.previous.end())
        {
            // Compare against the previously committed value
            bool changed = (var != it->second);
            // Always update 'current' so that commit will be correct
            maps.current[key] = var;
            return changed;
        }
        else
        {
            // First time we see this variable; store it but it doesn't "count" as changed yet
            maps.current[key] = var;
            return false;
        }
    }*/

    template <typename T>
    bool variableChanged(T& var) const
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
            maps.current[key] = var;        // first sighting
            return false;
        }
    }


    /// Returns true if any variable among args... has changed
    template <typename... Args>
    bool anyChanged(Args&&... args) const
    {
        return (variableChanged(std::forward<Args>(args)) || ...);
    }

    /// Explicitly update the 'current' value of a single variable
    template<typename T>
    void commitValue(T& var)
    {
        getStateMap<T>().current[&var] = var;
    }

    /// Explicitly update the 'current' values of all variables passed in
    template<typename... Args>
    void commitAll(Args&&... args)
    {
        (commitValue(std::forward<Args>(args)), ...);
    }

    /// Clears all tracked data (for every type) so we effectively start fresh
    void variableChangedClearMaps()
    {
        //std::lock_guard<std::mutex> lock(mutex_);
        for (auto& [_, cc] : registry_)
        {
            cc.clearer();
        }
    }

    /// Commits all tracked data (for every type), i.e. current => previous
    /// so that subsequent calls to variableChanged() compare against the newly committed data
    void variableChangedUpdateCurrent()
    {
        //std::lock_guard<std::mutex> lock(mutex_);
        for (auto& [_, cc] : registry_)
        {
            cc.committer();
        }
    }
};