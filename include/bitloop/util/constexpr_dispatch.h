#include <array>
#include <tuple>
#include <type_traits>
#include <utility>
#include <initializer_list>
#include <bitloop/core/types.h>

// ──────────────────────────────────────────────────────────────────────────────
// DispatchArg concept/trait + domain size utilities
// ─────────────────────────────────────────────────────────────────────────────-

// Treat enums and bools as dispatch domains.
template<class T>
struct is_dispatch_arg
    : std::bool_constant<std::is_enum_v<T> || std::is_same_v<std::remove_cv_t<T>, bool>> {
};

template<class T>
inline constexpr bool is_dispatch_arg_v = is_dispatch_arg<T>::value;

// Simple "concept" for pre-C++20: use SFINAE in templates.
template<class T>
using EnableIfDispatchArg = std::enable_if_t<is_dispatch_arg_v<T>, int>;

// domain_size_v<T>:
//  - for enum E, expects E::COUNT to exist (last enumerator)
//  - for bool, domain size is 2
template<class T, class = void>
struct domain_size;

template<class E>
struct domain_size<E, std::enable_if_t<std::is_enum_v<E>>>
{
    static constexpr std::size_t value = static_cast<std::size_t>(E::COUNT);
};

template<>
struct domain_size<bool, void>
{
    static constexpr std::size_t value = 2;
};

template<class T>
inline constexpr std::size_t domain_size_v = domain_size<T>::value;

// ──────────────────────────────────────────────────────────────────────────────
/* idx_to_tuple: given a linear index I and a pack of dispatch domains Es...
   build a tuple< std::integral_constant<E0, d0>, ... > where each digit di
   is the mixed-radix digit for domain Ei. Works for Es... possibly empty. */
   // ─────────────────────────────────────────────────────────────────────────────-

template<std::size_t Idx, class... Es>
struct idx_to_tuple;

template<std::size_t Idx>
struct idx_to_tuple<Idx>
{
    using type = std::tuple<>;
};

template<std::size_t Idx, class First, class... Rest>
struct idx_to_tuple<Idx, First, Rest...>
{
    static_assert(is_dispatch_arg_v<First>, "DispatchArg must be enum or bool");

    static constexpr std::size_t rest_prod = (std::size_t{ 1 } * ... * domain_size_v<Rest>);
    static constexpr std::size_t digit = rest_prod ? (Idx / rest_prod) : 0;
    static constexpr std::size_t rem = rest_prod ? (Idx % rest_prod) : 0;

    using head = std::integral_constant<First, static_cast<First>(digit)>;
    using tail = typename idx_to_tuple<rem, Rest...>::type;
    using type = decltype(std::tuple_cat(std::tuple<head>{}, tail{}));
};


template<typename Fun, typename... Ts>
struct enum_table_leading
{
    template<class... Es>
    struct impl
    {
        static_assert((is_dispatch_arg_v<Es> && ...), "All Es must be enums or bool");

        // Return type probe (two branches; never forms operator()<,>).
        using Ret = decltype(
            std::declval<Fun>().template operator() < Ts... > (
            std::declval<std::integral_constant<Es, static_cast<Es>(0)>>()...));

        //using Ret = decltype(
        //    std::declval<Fun>().template operator() < Ts... > (
        //    std::integral_constant<Es, static_cast<Es>(0)>()...));

        using FnPtr = Ret(*)(Fun&);

        // Identity product so empty pack => total == 1
        static constexpr std::size_t total = (std::size_t{ 1 } * ... * domain_size_v<Es>);

        template<std::size_t I>
        static consteval FnPtr make_entry()
        {
            if constexpr (sizeof...(Es) == 0)
            {
                return +[](Fun& f) -> Ret {
                    if constexpr (std::is_void_v<Ret>)
                        f.template operator() < Ts... > ();
                    else
                        return f.template operator() < Ts... > ();
                };
            }
            else
            {
                return +[](Fun& f) -> Ret {
                    using Tup = typename idx_to_tuple<I, Es...>::type; // tuple of Cs...
                    if constexpr (std::is_void_v<Ret>)
                        std::apply([&](auto... Cs) { f.template operator() < Ts... > (Cs...); }, Tup{});
                    else
                        return std::apply([&](auto... Cs) { return f.template operator() < Ts... > (Cs...); }, Tup{});
                };
            }
        }

        template<std::size_t... Is>
        static consteval auto build(std::index_sequence<Is...>) {
            return std::array<FnPtr, total>{ make_entry<Is>()... };
        }

        static constexpr auto table = build(std::make_index_sequence<total>{});
    };
};

// ─────────────────────────────────────────────────────────────────────────────-
/* table_invoke:
   Entry point you already had: forwards to enum_table_leading and calls the
   correct thunk. Works when Es... is empty or not. */
   // ─────────────────────────────────────────────────────────────────────────────-

template<typename... Ts, typename F, typename... Es> requires (is_dispatch_arg_v<Es> && ...)
decltype(auto) _table_invoke(F&& f, Es... vs)
{
    using Fun = std::decay_t<F>;
    using Tab = typename enum_table_leading<Fun, Ts...>::template impl<Es...>;

    if constexpr (sizeof...(Es) == 0)
    {
        if constexpr (std::is_void_v<typename Tab::Ret>)
            Tab::table[0](f);
        else
            return Tab::table[0](f);
    }
    else
    {
        std::size_t idx = 0;
        ((idx = idx * domain_size_v<Es> +static_cast<std::size_t>(vs)), ...);

        if constexpr (std::is_void_v<typename Tab::Ret>)
            Tab::table[idx](f);
        else
            return Tab::table[idx](f);
    }
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

// Own the storage; avoid initializer_list iterator/pointer quirks.
struct fp_seq {
    bl::FloatingPointType buf[8];
    std::size_t n = 0;

    fp_seq(std::initializer_list<bl::FloatingPointType> v) {
        n = v.size();
        std::size_t i = 0;
        for (auto x : v) buf[i++] = x;
    }
    const bl::FloatingPointType* data() const { return buf; }
    std::size_t size() const { return n; }
};
// Map enum → type (fully qualified)
template<bl::FloatingPointType> struct _bl_fp_map;
template<> struct _bl_fp_map<bl::FloatingPointType::F32> { using type = bl::f32; };
template<> struct _bl_fp_map<bl::FloatingPointType::F64> { using type = bl::f64; };
template<> struct _bl_fp_map<bl::FloatingPointType::F128> { using type = bl::f128; };

inline bl::FloatingPointType _norm_fp(bl::FloatingPointType e) {
    switch (e) {
    case bl::FloatingPointType::F32:  return bl::FloatingPointType::F32;
    case bl::FloatingPointType::F128: return bl::FloatingPointType::F128;
    default:                          return bl::FloatingPointType::F64;
    }
}



// Map N runtime bl::FloatingPointType tokens to Ts... then call a continuation<Ts...>()
template<class Cont, class... Ts>
static FAST_INLINE decltype(auto) _fp_expandN(Cont&& cont) {
    return std::forward<Cont>(cont).template operator() < Ts... > ();
}

template<class Cont, class... Ts>
static FAST_INLINE decltype(auto) _fp_expandN(Cont&& cont, bl::FloatingPointType t0) {
    switch (_norm_fp(t0)) {
    case bl::FloatingPointType::F32:
        return _fp_expandN<Cont, Ts..., typename _bl_fp_map<bl::FloatingPointType::F32>::type>(std::forward<Cont>(cont));
    case bl::FloatingPointType::F128:
        return _fp_expandN<Cont, Ts..., typename _bl_fp_map<bl::FloatingPointType::F128>::type>(std::forward<Cont>(cont));
    default:
        return _fp_expandN<Cont, Ts..., typename _bl_fp_map<bl::FloatingPointType::F64>::type>(std::forward<Cont>(cont));
    }
}

template<class Cont, class... Ts, class... More>
static FAST_INLINE decltype(auto) _fp_expandN(Cont&& cont, bl::FloatingPointType t0, bl::FloatingPointType t1, More... rest) {
    switch (_norm_fp(t0)) {
    case bl::FloatingPointType::F32:
        return _fp_expandN<Cont, Ts..., typename _bl_fp_map<bl::FloatingPointType::F32>::type>(std::forward<Cont>(cont), t1, rest...);
    case bl::FloatingPointType::F128:
        return _fp_expandN<Cont, Ts..., typename _bl_fp_map<bl::FloatingPointType::F128>::type>(std::forward<Cont>(cont), t1, rest...);
    default:
        return _fp_expandN<Cont, Ts..., typename _bl_fp_map<bl::FloatingPointType::F64>::type>(std::forward<Cont>(cont), t1, rest...);
    }
}


// 0 float-type template params
template<typename F, typename... Es> requires (is_dispatch_arg_v<Es> && ...)
decltype(auto) table_invoke(F&& f, Es... vs)
{
    using Fun = std::decay_t<F>;
    Fun& func = const_cast<Fun&>(static_cast<const Fun&>(f));
    return _table_invoke<>(func, vs...);
}

// 1 float-type template param first
template<typename F, typename... Es> requires (is_dispatch_arg_v<Es> && ...)
decltype(auto) table_invoke(F&& f, bl::FloatingPointType a0, Es... vs)
{
    using Fun = std::decay_t<F>;
    Fun& func = const_cast<Fun&>(static_cast<const Fun&>(f));
    auto cont = [&]<typename T0>() -> decltype(auto) { return _table_invoke<T0>(func, vs...); };
    return _fp_expandN(cont, a0);
}


// 2 float-type template params first
template<typename F, typename... Es> requires (is_dispatch_arg_v<Es> && ...)
decltype(auto) table_invoke(F&& f, bl::FloatingPointType a0, bl::FloatingPointType a1, Es... vs)
{
    using Fun = std::decay_t<F>;
    Fun& func = const_cast<Fun&>(static_cast<const Fun&>(f));
    auto cont = [&]<typename T0, typename T1>() -> decltype(auto) { return _table_invoke<T0, T1>(func, vs...); };
    return _fp_expandN(cont, a0, a1);
}

// 3 float-type template params first
template<typename F, typename... Es> requires (is_dispatch_arg_v<Es> && ...)
decltype(auto) table_invoke(F&& f, bl::FloatingPointType a0, bl::FloatingPointType a1, bl::FloatingPointType a2, Es... vs)
{
    using Fun = std::decay_t<F>;
    Fun& func = const_cast<Fun&>(static_cast<const Fun&>(f));
    auto cont = [&]<typename T0, typename T1, typename T2>() -> decltype(auto) { return _table_invoke<T0, T1, T2>(func, vs...); };
    return _fp_expandN(cont, a0, a1, a2);
}

// 4 float-type template params first
template<typename F, typename... Es> requires (is_dispatch_arg_v<Es> && ...)
decltype(auto) table_invoke(F&& f, bl::FloatingPointType a0, bl::FloatingPointType a1, bl::FloatingPointType a2, bl::FloatingPointType a3, Es... vs)
{
    using Fun = std::decay_t<F>;
    Fun& func = const_cast<Fun&>(static_cast<const Fun&>(f));
    auto cont = [&]<typename T0, typename T1, typename T2, typename T3>() -> decltype(auto) { return _table_invoke<T0, T1, T2, T3>(func, vs...); };
    return _fp_expandN(cont, a0, a1, a2, a3);
}
