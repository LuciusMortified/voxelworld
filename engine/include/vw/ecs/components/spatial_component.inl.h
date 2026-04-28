#pragma once

#ifndef VW_ECS_COMPONENTS_SPATIAL_COMPONENT_INL_H
#define VW_ECS_COMPONENTS_SPATIAL_COMPONENT_INL_H

#include "vw/ecs/components/spatial_component.h"

namespace vw::ecs {

inline auto spatial_component::get_bounds() const -> const vw::spatial::aabb& {
    return bounds_;
}

inline auto spatial_component::get_layer() const -> spatial_layer_mask {
    return layer_;
}

inline auto spatial_component::is_dirty() const -> bool {
    return dirty_;
}

}  // namespace vw::ecs

#endif  // VW_ECS_COMPONENTS_SPATIAL_COMPONENT_INL_H

