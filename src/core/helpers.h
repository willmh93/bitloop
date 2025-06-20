#pragma once
#define _DISABLE_CONSTEXPR_MUTEX_CONSTRUCTOR 
#include <mutex>
#include <array>
#include <tuple>
#include <typeindex>
#include <functional>
#include <unordered_map>
#include <memory>
#include <variant>
#include "debug.h"

#if defined(__clang__) || defined(__GNUC__)
#define FORCEINLINE inline __attribute__((always_inline))
#elif defined(_MSC_VER)
#define FORCEINLINE __forceinline
#else
#define FORCEINLINE inline
#endif

#if defined(__GNUC__) || defined(__clang__)
# define FAST_MATH_FN __attribute__((optimize("fast-math")))
#elif defined(_MSC_VER)
# define FAST_MATH_FN __pragma(float_control(precise, off)) \
                      __forceinline
#else
# define FAST_MATH_FN
#endif


namespace Helpers
{
    template<typename T>
    std::string floatToCleanString(T value, int max_decimal_places, T precision, bool minimize=true)
    {
        // Optional snapping
        if (precision > 0)
            value = std::round(value / precision) * precision;

        // Use fixed formatting
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(max_decimal_places) << value;
        std::string str = oss.str();

        // Remove trailing zeros
        size_t dot = str.find('.');
        if (dot != std::string::npos)
        {
            size_t last_non_zero = str.find_last_not_of('0');
            if (last_non_zero != std::string::npos)
                str.erase(last_non_zero + 1);

            // Remove trailing dot
            if (str.back() == '.')
                str.pop_back();
        }

        if (minimize)
        {
            if (str == "-0")
                str = "0";

            // Remove the leading '0' for values between -1 and 1
            // (e.g. 0.7351 -> .7351)
            if (!str.empty())
            {
                bool negative = (str[0] == '-');
                std::size_t first = negative ? 1 : 0;
                if (first + 1 < str.size() && str[first] == '0' && str[first + 1] == '.')
                    str.erase(first, 1);
            }
        }

        return str;
    }

    // Wrapping/Unwrapping strings with '\n' at given length
    std::string wrapString(const std::string& input, size_t width);
    std::string unwrapString(const std::string& input);
}

///------------------------------------------------///
/// constexpr typed bool/enum function dispatcher  ///
///------------------------------------------------///

template<typename T>
concept DispatchArg = std::is_same_v<T, bool> || (std::is_enum_v<T> && requires { T::COUNT; });

// Determine size of domain (2 for bool, Enum::COUNT for enums)
template<typename T>
consteval std::size_t domain_size_helper()
{
    if constexpr (std::is_same_v<T, bool>)
        return 2;
    else
        return static_cast<std::size_t>(T::COUNT);
}

template<DispatchArg T> inline constexpr std::size_t domain_size_v = domain_size_helper<T>();
template<std::size_t Idx, DispatchArg... Es> struct idx_to_tuple; 

template<std::size_t Idx, DispatchArg First, DispatchArg... Rest>
struct idx_to_tuple<Idx, First, Rest...>
{
    static constexpr std::size_t rest_prod = (domain_size_v<Rest> * ... * 1);
    static constexpr std::size_t digit = Idx / rest_prod;
    static constexpr std::size_t rem = Idx % rest_prod;

    using head = std::integral_constant<First, static_cast<First>(digit)>;
    using tail = typename idx_to_tuple<rem, Rest...>::type;
    using type = decltype(std::tuple_cat(std::tuple<head>{}, tail{}));
};

template<std::size_t Idx, DispatchArg Last>
struct idx_to_tuple<Idx, Last>
{
    static constexpr std::size_t digit = Idx;
    using type = std::tuple<std::integral_constant<Last, static_cast<Last>(digit)>>;
};

template<typename Fun, typename... Ts>
struct enum_table_leading
{
    template<DispatchArg... Es>
    struct impl
    {
        using Ret = decltype(
            std::declval<Fun>().template operator() < Ts... > (
            std::integral_constant<Es, static_cast<Es>(0)>()...));

        using FnPtr = Ret(*)(Fun&);
        static constexpr std::size_t total = (domain_size_v<Es> * ...);

        template<std::size_t I>
        static consteval FnPtr make_entry()
        {
            return +[](Fun& f) -> Ret {
                using Tup = typename idx_to_tuple<I, Es...>::type;
                if constexpr (std::is_void_v<Ret>)
                    std::apply([&](auto... Cs) { f.template operator()<Ts...> (Cs...); }, Tup{});
                else
                    return std::apply([&](auto... Cs) { return f.template operator()<Ts...> (Cs...); }, Tup{});
            };
        }

        template<std::size_t... Is>
        static consteval auto build(std::index_sequence<Is...>) {
            return std::array<FnPtr, total>{ make_entry<Is>()... };
        }

        static constexpr auto table = build(std::make_index_sequence<total>{});
    };
};

template<typename... Ts, typename F, DispatchArg... Es> 
decltype(auto) table_invoke(F&& f, Es... vs)
{
    using Fun = std::decay_t<F>;
    using Tab = typename enum_table_leading<Fun, Ts...>::template impl<Es...>;

    std::size_t idx = 0;
    ((idx = idx * domain_size_v<Es> +static_cast<std::size_t>(vs)), ...);

    if constexpr (std::is_void_v<typename Tab::Ret>)
        Tab::table[idx](f);
    else
        return Tab::table[idx](f);
}

#define build_table(func, capture, ...)                         \
    capture <typename... Ts>(auto... Cs) -> decltype(auto) {    \
        return [&]<typename... Us>(std::tuple<Us...>*) {        \
            if constexpr (sizeof...(Ts) == 0)                   \
                return func<Us::value...>(__VA_ARGS__);         \
            else                                                \
                return func<Ts..., Us::value...>(__VA_ARGS__);  \
        }((std::tuple<decltype(Cs)...>*)nullptr);               \
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
            maps.current[key] = var; // todo: Should never begin tracking here? Inconsistent results depending on when it's called
            return false;
        }
    }

    template <typename... Args>
    [[nodiscard]] bool Changed(Args&&... args) const
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