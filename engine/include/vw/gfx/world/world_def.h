#pragma once

#ifndef VW_GFX_WORLD_WORLD_DEF_H
#define VW_GFX_WORLD_WORLD_DEF_H

#include <tuple>

#include "vw/gfx/world/entity_registry.h"
#include "vw/gfx/world/system_trait.h"
#include "vw/gfx/world/tuple_utils.h"

namespace vw::gfx {

template <template <typename> class... Systems>
struct world_def {
    using components = tuple_unique_t<
        tuple_cat_t<typename system_trait<Systems>::components...>>;

    using resources = tuple_unique_t<
        tuple_cat_t<typename system_trait<Systems>::resources...>>;

    using registry_type = entity_registry_from_tuple<components>::type;

    using systems_tuple = std::tuple<Systems<world_def>...>;

    static constexpr std::size_t system_count = sizeof...(Systems);
};

}  // namespace vw::gfx

#endif  // VW_GFX_WORLD_WORLD_DEF_H