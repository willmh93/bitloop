#include <array>
#include <tuple>
#include <type_traits>
#include <utility>

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
                    std::apply([&](auto... Cs) { f.template operator() < Ts... > (Cs...); }, Tup{});
                else
                    return std::apply([&](auto... Cs) { return f.template operator() < Ts... > (Cs...); }, Tup{});
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

