#pragma once

#ifndef VW_CORE_TUPLE_UTILS_H
#define VW_CORE_TUPLE_UTILS_H

#include <array>
#include <cstddef>
#include <tuple>
#include <type_traits>

namespace vw {

namespace detail {

template <typename Tuple>
struct tuple_cat_impl;

template <>
struct tuple_cat_impl<std::tuple<>> {
    using type = std::tuple<>;
};

template <typename T>
struct tuple_cat_impl<std::tuple<T>> {
    using type = T;
};

template <typename T1, typename T2, typename... Rest>
struct tuple_cat_impl<std::tuple<T1, T2, Rest...>> {
    using type = tuple_cat_impl<
        std::tuple<decltype(std::tuple_cat(std::declval<T1>(), std::declval<T2>())), Rest...>
    >::type;
};

template <typename Acc, typename Tuple>
struct tuple_unique_impl;

template <typename... AccTs>
struct tuple_unique_impl<std::tuple<AccTs...>, std::tuple<>> {
    using type = std::tuple<AccTs...>;
};

template <typename... AccTs, typename Head, typename... Tail>
struct tuple_unique_impl<std::tuple<AccTs...>, std::tuple<Head, Tail...>> {
    using type = std::conditional_t<
        (std::is_same_v<Head, AccTs> || ...),
        typename tuple_unique_impl<std::tuple<AccTs...>, std::tuple<Tail...>>::type,
        typename tuple_unique_impl<std::tuple<AccTs..., Head>, std::tuple<Tail...>>::type
    >;
};

template <typename Head, typename... Tail>
struct tuple_unique_impl<std::tuple<>, std::tuple<Head, Tail...>> {
    using type = tuple_unique_impl<std::tuple<Head>, std::tuple<Tail...>>::type;
};

template <typename T, typename... Ts>
constexpr auto pack_index_of_impl() -> std::size_t {
    constexpr std::array<bool, sizeof...(Ts)> matches = {std::is_same_v<T, Ts>...};
    for (std::size_t i = 0; i < sizeof...(Ts); ++i) {
        if (matches[i]) {
            return i;
        }
    }
    return sizeof...(Ts);
}

template <typename T, typename Tuple>
struct tuple_contains;

template <typename T, typename... Ts>
struct tuple_contains<T, std::tuple<Ts...>>
    : std::bool_constant<(std::is_same_v<T, Ts> || ...)> {};

template <typename T, typename Tuple>
struct tuple_index_of;

template <typename T, typename... Ts>
struct tuple_index_of<T, std::tuple<Ts...>> {
    static_assert((std::is_same_v<T, Ts> || ...), "type not found in tuple");
    static constexpr std::size_t value = pack_index_of_impl<T, Ts...>();
};

}  // namespace detail

template <typename... Tuples>
using tuple_cat_t = detail::tuple_cat_impl<std::tuple<Tuples...>>::type;

template <typename Tuple>
using tuple_unique_t = detail::tuple_unique_impl<std::tuple<>, Tuple>::type;

template <typename T, typename Tuple>
inline constexpr bool tuple_contains_v = detail::tuple_contains<T, Tuple>::value;

template <typename T, typename Tuple>
inline constexpr std::size_t tuple_index_of_v = detail::tuple_index_of<T, Tuple>::value;

template <typename T, typename... Ts>
inline constexpr std::size_t pack_index_of_v = []() -> std::size_t {
    static_assert((std::is_same_v<T, Ts> || ...), "type not in parameter pack");
    static_assert(
        ((std::is_same_v<T, Ts> ? 1 : 0) + ...) == 1,
        "type must appear exactly once in parameter pack"
    );
    return detail::pack_index_of_impl<T, Ts...>();
}();

}  // namespace vw

#endif  // VW_CORE_TUPLE_UTILS_H
