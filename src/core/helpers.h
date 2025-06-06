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

#if defined(__clang__) || defined(__GNUC__)
#define FORCEINLINE inline __attribute__((always_inline))
#elif defined(_MSC_VER)
#define FORCEINLINE __forceinline
#else
#define FORCEINLINE inline
#endif

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

///--------------------------///
/// Variable Changed Tracker ///
///--------------------------///

class VariableChangedTracker
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
    mutable std::mutex                                  mutex_;   // optional; uncomment locks if needed

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

public:
    VariableChangedTracker() = default;

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
            maps.current[key] = var;
            return false;
        }
    }

    template <typename... Args>
    [[nodiscard]] bool anyChanged(Args&&... args) const
    {
        return (variableChanged(std::forward<Args>(args)) || ...);
    }

    template <typename T>
    void commitValue(T& var)
    {
        getStateMap<T>().current[&var] = var;
    }

    /*template <typename... Args>
    void commitAll(Args&&... args)
    {
        (commitValue(std::forward<Args>(args)), ...);
    }*/

    void variableChangedClearMaps()
    {
        // std::lock_guard<std::mutex> lock(mutex_);
        for (auto& [_, cc] : registry_)
            cc.clear();
    }

    void variableChangedUpdateCurrent()
    {
        // std::lock_guard<std::mutex> lock(mutex_);
        for (auto& [_, cc] : registry_)
            cc.commit();
    }
};