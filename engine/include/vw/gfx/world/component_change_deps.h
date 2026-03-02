#pragma once

#ifndef VW_GFX_WORLD_COMPONENT_CHANGE_DEPS_H
#define VW_GFX_WORLD_COMPONENT_CHANGE_DEPS_H

#include <tuple>

namespace vw::gfx {

struct transform_component;
struct model_component;
struct spatial_component;
struct light_component;
struct world_view_component;

template <typename>
struct component_change_deps {
    using types = std::tuple<>;
};

template <>
struct component_change_deps<transform_component> {
    using types = std::tuple<spatial_component, world_view_component, light_component>;
};

template <>
struct component_change_deps<model_component> {
    using types = std::tuple<spatial_component>;
};

}  // namespace vw::gfx

#endif  // VW_GFX_WORLD_COMPONENT_CHANGE_DEPS_H
