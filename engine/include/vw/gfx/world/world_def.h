#pragma once

#ifndef VW_GFX_WORLD_WORLD_DEF_H
#define VW_GFX_WORLD_WORLD_DEF_H

#include <tuple>

#include "vw/gfx/world/entity_registry.h"
#include "vw/gfx/world/system_trait.h"
#include "vw/gfx/world/tuple_utils.h"

namespace vw::gfx {

namespace detail {

template <template <typename> class Target, std::size_t I,
          template <typename> class... Ss>
struct system_position_in;

template <template <typename> class Target, std::size_t I>
struct system_position_in<Target, I> {
    static constexpr std::size_t value = static_cast<std::size_t>(-1);
};

template <template <typename> class Target, std::size_t I,
          template <typename> class Head, template <typename> class... Tail>
struct system_position_in<Target, I, Head, Tail...> {
    static constexpr std::size_t value =
        std::is_same_v<Head<void>, Target<void>>
            ? I
            : system_position_in<Target, I + 1, Tail...>::value;
};

template <std::size_t MyPos, typename DepsList,
          template <typename> class... AllSystems>
struct validate_deps;

template <std::size_t MyPos, template <typename> class... AllSystems>
struct validate_deps<MyPos, system_list<>, AllSystems...> : std::true_type {};

template <std::size_t MyPos,
          template <typename> class DepHead, template <typename> class... DepTail,
          template <typename> class... AllSystems>
struct validate_deps<MyPos, system_list<DepHead, DepTail...>, AllSystems...> {
    static constexpr std::size_t dep_pos =
        system_position_in<DepHead, 0, AllSystems...>::value;
    static_assert(dep_pos != static_cast<std::size_t>(-1),
        "dependency not found in system list");
    static_assert(dep_pos < MyPos,
        "dependency must appear before dependent system in world_def");
    static constexpr bool value =
        (dep_pos < MyPos) &&
        validate_deps<MyPos, system_list<DepTail...>, AllSystems...>::value;
};

}  // namespace detail

template <template <typename> class... Systems>
struct world_def {
    using components = tuple_unique_t<
        tuple_cat_t<typename system_trait<Systems>::components...>>;

    using resources = tuple_unique_t<
        tuple_cat_t<typename system_trait<Systems>::resources...>>;

    using registry_type = typename entity_registry_from_tuple<components>::type;

    using systems_tuple = std::tuple<Systems<components>...>;

    static constexpr std::size_t system_count = sizeof...(Systems);

    template <template <typename> class S>
    static constexpr std::size_t system_index =
        detail::system_position_in<S, 0, Systems...>::value;

    static constexpr bool deps_valid = (
        detail::validate_deps<
            detail::system_position_in<Systems, 0, Systems...>::value,
            typename system_trait<Systems>::depends_on,
            Systems...
        >::value && ...);

    static_assert(deps_valid,
        "system dependency ordering violated in world_def");
};

}  // namespace vw::gfx

#endif  // VW_GFX_WORLD_WORLD_DEF_H
