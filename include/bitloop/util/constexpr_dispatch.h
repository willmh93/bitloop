#pragma once

#include <array>
#include <tuple>
#include <type_traits>
#include <utility>
#include <functional>
#include <cstdio>

#include <bitloop/core/types.h>

#ifndef FORCE_INLINE
#if defined(_MSC_VER)
#define FORCE_INLINE __forceinline
#elif defined(__clang__) || defined(__GNUC__)
#define FORCE_INLINE inline __attribute__((always_inline))
#else
#define FORCE_INLINE inline
#endif
#endif

// ─────────────────────────────────────────────────────────────────────────────
// dispatch domain traits
//
// by default, enums are treated as a dense domain [0, E::COUNT]
// you may specialize enum_domain_traits<E> to
//   - shrink the domain (e.g. a subset)
//   - remap arbitrary enum values to a dense index
//
// this keeps "COUNT bigger than last declared enumerator" tricks working
// (e.g. bitmask enums), while allowing explicit shrinking elsewhere
// ─────────────────────────────────────────────────────────────────────────────

template<class T>
struct is_dispatch_arg : std::bool_constant<std::is_enum_v<T> ||
                                            std::is_same_v<std::remove_cv_t<T>, bool>> {};

template<class T>
inline constexpr bool is_dispatch_arg_v = is_dispatch_arg<T>::value;

template<class E>
struct enum_domain_traits
{
    static_assert(std::is_enum_v<E>, "enum_domain_traits<E> requires enum E");

    static constexpr std::size_t size = static_cast<std::size_t>(E::COUNT);

    static FORCE_INLINE constexpr std::size_t index(E v)
    {
        const auto i = static_cast<std::size_t>(v);
        #if !defined(NDEBUG)
        if (i >= size)
        {
            #if defined(_MSC_VER)
            __debugbreak();
            #else
            __builtin_trap();
            #endif
        }
        #endif
        return i;
    }

    static FORCE_INLINE constexpr E value(std::size_t i)
    {
        #if !defined(NDEBUG)
        if (i >= size)
        {
            #if defined(_MSC_VER)
            __debugbreak();
            #else
            __builtin_trap();
            #endif
        }
        #endif
        return static_cast<E>(i);
    }
};

template<>
struct enum_domain_traits<bool>
{
    static constexpr std::size_t size = 2;

    static FORCE_INLINE constexpr std::size_t index(bool v)
    {
        return v ? 1u : 0u;
    }

    static FORCE_INLINE constexpr bool value(std::size_t i)
    {
        return i != 0;
    }
};

// helper for sparse/subset domains
template<class E, E... Vs>
struct enum_domain_values
{
    static_assert(std::is_enum_v<E>, "enum_domain_values requires enum E");
    static constexpr std::size_t size = sizeof...(Vs);
    static constexpr std::array<E, size> values = { Vs... };

    static FORCE_INLINE constexpr std::size_t index(E v)
    {
        for (std::size_t i = 0; i < size; ++i)
        {
            if (values[i] == v)
            {
                return i;
            }
        }
        #if defined(_MSC_VER)
        __debugbreak();
        #else
        __builtin_trap();
        #endif
        return 0;
    }

    static FORCE_INLINE constexpr E value(std::size_t i)
    {
        #if !defined(NDEBUG)
        if (i >= size)
        {
            #if defined(_MSC_VER)
            __debugbreak();
            #else
            __builtin_trap();
            #endif
        }
        #endif
        return values[i];
    }
};

template<class T>
inline constexpr std::size_t domain_size_v = enum_domain_traits<std::remove_cv_t<T>>::size;

namespace _bl_detail
{
    template<class... Ts>
    struct type_list {};

    template<class List, class T>
    struct list_push_back;

    template<class... Ts, class T>
    struct list_push_back<type_list<Ts...>, T>
    {
        using type = type_list<Ts..., T>;
    };

    template<class T>
    FORCE_INLINE constexpr std::size_t domain_index(T v)
    {
        return enum_domain_traits<std::remove_cv_t<T>>::index(v);
    }

    template<class E>
    using default_constant_t = std::conditional_t<
        std::is_same_v<std::remove_cv_t<E>, bool>,
        std::integral_constant<bool, false>,
        std::integral_constant<std::remove_cv_t<E>, enum_domain_traits<std::remove_cv_t<E>>::value(0)>
    >;
}

// ─────────────────────────────────────────────────────────────────────────────
// enum => type mapping
// 
// specialize enum_type_map<E, V> to map a specific enum value V to a type
// used by map_enum(...)
// ─────────────────────────────────────────────────────────────────────────────

template<class E, E V>
struct enum_type_map;

template<class E, E V>
using enum_type_map_t = typename enum_type_map<E, V>::type;

// default float mappings
template<> struct enum_type_map<bl::FloatingPointType, bl::FloatingPointType::F32>  { using type = bl::f32;  };
template<> struct enum_type_map<bl::FloatingPointType, bl::FloatingPointType::F64>  { using type = bl::f64;  };
template<> struct enum_type_map<bl::FloatingPointType, bl::FloatingPointType::F128> { using type = bl::f128; };

// ─────────────────────────────────────────────────────────────────────────────
// type selector tags
// ─────────────────────────────────────────────────────────────────────────────

namespace bl
{
    template<class T>
    struct type_tag { using type = T; };

    template<class T>
    inline constexpr type_tag<T> type{};

    template<class E>
    struct mapped_enum
    {
        static_assert(std::is_enum_v<E>, "mapped_enum requires enum type");
        E value{};
    };

    template<class E>
    FORCE_INLINE constexpr mapped_enum<E> map_enum(E v)
    {
        return mapped_enum<E>{ v };
    }
}

template<class T>
struct is_type_tag : std::false_type {};

template<class T>
struct is_type_tag<bl::type_tag<T>> : std::true_type {};

template<class T>
inline constexpr bool is_type_tag_v = is_type_tag<std::remove_cv_t<T>>::value;

template<class T>
struct is_mapped_enum : std::false_type {};

template<class E>
struct is_mapped_enum<bl::mapped_enum<E>> : std::true_type {};

template<class T>
inline constexpr bool is_mapped_enum_v = is_mapped_enum<std::remove_cv_t<T>>::value;

// ─────────────────────────────────────────────────────────────────────────────
// enum_expandN maps runtime enum values to types using enum_type_map + enum_domain_traits
// ─────────────────────────────────────────────────────────────────────────────

namespace _bl_detail
{
    template<class E, class Cont, class TsList, class... More>
    struct enum_expand_step;

    template<class E, class Cont, class... Ts>
    struct enum_expand_step<E, Cont, type_list<Ts...>>
    {
        static FORCE_INLINE decltype(auto) invoke(Cont& cont)
        {
            return cont.template operator()<Ts...>();
        }
    };

    template<class E, class Cont, class... Ts, class... More>
    struct enum_expand_step<E, Cont, type_list<Ts...>, E, More...>
    {
        using Ret = decltype(std::declval<Cont&>().template operator()<Ts..., enum_type_map_t<E, enum_domain_traits<E>::value(0)>>());
        using Fn = Ret(*)(Cont&, More...);

        template<std::size_t I>
        static FORCE_INLINE Ret entry(Cont& cont, More... rest)
        {
            using Mapped = enum_type_map_t<E, enum_domain_traits<E>::value(I)>;
            return enum_expand_step<E, Cont, type_list<Ts..., Mapped>, More...>::invoke(cont, rest...);
        }

        template<std::size_t... Is>
        static consteval auto build(std::index_sequence<Is...>)
        {
            return std::array<Fn, enum_domain_traits<E>::size>{ &entry<Is>... };
        }

        static constexpr auto table = build(std::make_index_sequence<enum_domain_traits<E>::size>{});

        static FORCE_INLINE decltype(auto) invoke(Cont& cont, E v0, More... rest)
        {
            return table[enum_domain_traits<E>::index(v0)](cont, rest...);
        }
    };

    template<class E, class Cont, class TsList, class... More>
    FORCE_INLINE decltype(auto) enum_expandN_impl(Cont& cont, More... xs)
    {
        return enum_expand_step<E, Cont, TsList, More...>::invoke(cont, xs...);
    }
}

template<class E, class Cont, class... Ts, class... More>
FORCE_INLINE decltype(auto) enum_expandN(Cont& cont, More... xs)
{
    static_assert(std::is_enum_v<E>, "E must be an enum");
    return _bl_detail::enum_expandN_impl<E, Cont, _bl_detail::type_list<Ts...>>(cont, xs...);
}

// ─────────────────────────────────────────────────────────────────────────────
// stepwise 1-D dispatch for value domains (bool + enums)
// ─────────────────────────────────────────────────────────────────────────────

namespace _bl_detail
{
    template<class E, std::size_t I>
    using domain_constant_t = std::conditional_t<
        std::is_same_v<std::remove_cv_t<E>, bool>,
        std::integral_constant<bool, (I != 0)>,
        std::integral_constant<std::remove_cv_t<E>, enum_domain_traits<std::remove_cv_t<E>>::value(I)>
    >;

    template<class Ret, class Fun, class TsList, class CsList, class... Es>
    struct step_dispatch;

    template<class Ret, class Fun, class... Ts, class... Cs>
    struct step_dispatch<Ret, Fun, type_list<Ts...>, type_list<Cs...>>
    {
        static FORCE_INLINE Ret invoke(Fun& func)
        {
            if constexpr (std::is_void_v<Ret>)
            {
                func.template operator()<Ts...>(Cs{}...);
                return;
            }
            else
            {
                return func.template operator()<Ts...>(Cs{}...);
            }
        }
    };

    template<class Ret, class Fun, class TsList, class CsList, class First, class... Rest>
    struct step_dispatch<Ret, Fun, TsList, CsList, First, Rest...>
    {
        using FirstT = std::remove_cv_t<First>;
        using Fn = Ret(*)(Fun&, Rest...);

        template<std::size_t I>
        static FORCE_INLINE Ret entry(Fun& f, Rest... rest)
        {
            using C = domain_constant_t<FirstT, I>;
            using NextCs = typename list_push_back<CsList, C>::type;
            return step_dispatch<Ret, Fun, TsList, NextCs, Rest...>::invoke(f, rest...);
        }

        template<std::size_t... Is>
        static consteval auto build(std::index_sequence<Is...>)
        {
            return std::array<Fn, enum_domain_traits<FirstT>::size>{ &entry<Is>... };
        }

        static constexpr auto table = build(std::make_index_sequence<enum_domain_traits<FirstT>::size>{});

        static FORCE_INLINE Ret invoke(Fun& func, First v0, Rest... rest)
        {
            return table[enum_domain_traits<FirstT>::index(v0)](func, rest...);
        }
    };
}

template<typename... Ts, typename F, typename... Es>
    requires ((is_dispatch_arg_v<std::remove_cvref_t<Es>> && ...))
decltype(auto) _table_invoke(F& func, Es... vs)
{
    using Fun = std::decay_t<F>;
    using RetProbe = decltype(func.template operator()<Ts...>(_bl_detail::default_constant_t<std::remove_cvref_t<Es>>{}...));
    return _bl_detail::step_dispatch<RetProbe, Fun, _bl_detail::type_list<Ts...>, _bl_detail::type_list<>, std::remove_cvref_t<Es>...>::invoke(
        func,
        static_cast<std::remove_cvref_t<Es>>(vs)...);
}

// ─────────────────────────────────────────────────────────────────────────────
// public table_invoke parses leading type selectors, then dispatch domains
// 
// supported leading type selectors
//   - bl::type_tag<T> (bl::type<T>)
//   - bl::mapped_enum<E> (bl::map_enum(v))
// 
// after the first non-type-selector, all remaining args must be dispatch args
// ─────────────────────────────────────────────────────────────────────────────

namespace _bl_detail
{
    template<class Fun, class TsList, class... Args>
    struct table_invoke_parse;

    template<class Fun, class... Ts>
    struct table_invoke_parse<Fun, type_list<Ts...>>
    {
        static FORCE_INLINE decltype(auto) invoke(Fun& func)
        {
            return _table_invoke<Ts...>(func);
        }
    };

    template<class Fun, class... Ts, class T, class... Rest>
    struct table_invoke_parse<Fun, type_list<Ts...>, bl::type_tag<T>, Rest...>
    {
        static FORCE_INLINE decltype(auto) invoke(Fun& func, bl::type_tag<T>, Rest... rest)
        {
            return table_invoke_parse<Fun, type_list<Ts..., T>, Rest...>::invoke(func, rest...);
        }
    };

    template<class Fun, class... Ts, class E, class... Rest>
    struct table_invoke_parse<Fun, type_list<Ts...>, bl::mapped_enum<E>, Rest...>
    {
        static FORCE_INLINE decltype(auto) invoke(Fun& func, bl::mapped_enum<E> me, Rest... rest)
        {
            auto cont = [&]<class... AllTypes>() -> decltype(auto) {
                return table_invoke_parse<Fun, type_list<AllTypes...>, Rest...>::invoke(func, rest...);
            };
            return enum_expandN<E, decltype(cont), Ts...>(cont, me.value);
        }
    };

    template<class Fun, class... Ts, class... Rest>
    struct table_invoke_parse<Fun, type_list<Ts...>, bl::FloatingPointType, Rest...>
    {
        static FORCE_INLINE decltype(auto) invoke(Fun& func, bl::FloatingPointType fp, Rest... rest)
        {
            return table_invoke_parse<Fun, type_list<Ts...>, bl::mapped_enum<bl::FloatingPointType>, Rest...>::invoke(
                func,
                bl::map_enum(fp),
                rest...);
        }
    };

    template<class Fun, class... Ts, class First, class... Rest>
    struct table_invoke_parse<Fun, type_list<Ts...>, First, Rest...>
    {
        static FORCE_INLINE decltype(auto) invoke(Fun& func, First first, Rest... rest)
        {
            static_assert(is_dispatch_arg_v<std::remove_cvref_t<First>>, "type selectors must come before dispatch args");
            static_assert(((is_dispatch_arg_v<std::remove_cvref_t<Rest>> && ...)), "all trailing args must be dispatch args");
            return _table_invoke<Ts...>(
                func,
                static_cast<std::remove_cvref_t<First>>(first),
                static_cast<std::remove_cvref_t<Rest>>(rest)...);
        }
    };
}

template<typename... Ts, typename F, typename... Args>
decltype(auto) table_invoke(F&& f, Args... args)
{
    using Fun = std::decay_t<F>;
    Fun& func = const_cast<Fun&>(static_cast<const Fun&>(f));
    return _bl_detail::table_invoke_parse<Fun, _bl_detail::type_list<Ts...>, Args...>::invoke(func, args...);
}

// ─────────────────────────────────────────────────────────────────────────────
// dispatch table helpers
// ─────────────────────────────────────────────────────────────────────────────

#define dispatch_table(func, ...)                                  \
    [&] <typename... Ts>(auto... Cs) -> decltype(auto) {           \
        return [&]<typename... Us>(std::tuple<Us...>*) {           \
            if constexpr (sizeof...(Ts) == 0)                      \
                return func<Us::value...>(__VA_ARGS__);            \
            else                                                   \
                return func<Ts..., Us::value...>(__VA_ARGS__);     \
        }((std::tuple<decltype(Cs)...>*)nullptr);                  \
    }

#define dispatch_table_targ(obj, method, ...)                                      \
    [&] <typename... Ts>(auto... Cs) -> decltype(auto) {                           \
        auto&& _obj = (obj);                                                       \
        return [&]<typename... Us>(std::tuple<Us...>*) {                           \
            if constexpr (sizeof...(Ts) == 0) {                                    \
                return std::invoke(&method<Us::value...>, _obj, __VA_ARGS__);      \
            } else {                                                               \
                return std::invoke(&method<Ts..., Us::value...>, _obj, __VA_ARGS__);\
            }                                                                      \
        }((std::tuple<decltype(Cs)...>*)nullptr);                                  \
    }

#define lambda_table(func, ...)                                     \
    [&] <typename... Ts>(auto... Cs) -> decltype(auto) {            \
        return [&]<typename... Us>(std::tuple<Us...>*) {            \
            if constexpr (sizeof...(Ts) == 0)                       \
                return func.template operator()<Us::value...>(__VA_ARGS__);        \
            else                                                    \
                return func.template operator()<Ts..., Us::value...>(__VA_ARGS__); \
        }((std::tuple<decltype(Cs)...>*)nullptr);                   \
    }


namespace dispatch_debug
{
    template<class T>
    struct mapped_underlying;

    template<class E>
    struct mapped_underlying<bl::mapped_enum<E>> { using type = E; };

    template<class T>
    using mapped_underlying_t = typename mapped_underlying<std::remove_cv_t<T>>::type;

    template<class Arg>
    consteval std::size_t arg_domain_size()
    {
        using A = std::remove_cvref_t<Arg>;

        if constexpr (is_type_tag_v<A>)
        {
            return 1;
        }
        else if constexpr (is_mapped_enum_v<A>)
        {
            using E = mapped_underlying_t<A>;
            return domain_size_v<E>;
        }
        else if constexpr (std::is_same_v<A, bl::FloatingPointType>)
        {
            // legacy direct float selector (if you pass it directly)
            return domain_size_v<bl::FloatingPointType>;
        }
        else if constexpr (is_dispatch_arg_v<A>)
        {
            return domain_size_v<A>;
        }
        else
        {
            return 1;
        }
    }

    template<class... Args>
    consteval std::size_t variant_count_types()
    {
        return (std::size_t{ 1 } * ... * arg_domain_size<Args>());
    }

    template<class... Args>
    void print_from_args(const char* label, Args&&...)
    {
        constexpr std::size_t sizes[] = { arg_domain_size<std::remove_cvref_t<Args>>()... };
        constexpr std::size_t total = variant_count_types<std::remove_cvref_t<Args>...>();

        blPrint("%s: %zu variants\n", label ? label : "dispatch", total);
        for (std::size_t i = 0; i < sizeof...(Args); ++i)
            blPrint("  arg[%zu] domain=%zu\n", i, sizes[i]);
    }
}
