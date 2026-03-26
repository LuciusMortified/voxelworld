#pragma once

#ifndef VW_GFX_WORLD_COMPONENTS_SPATIAL_COMPONENT_INL_H
#define VW_GFX_WORLD_COMPONENTS_SPATIAL_COMPONENT_INL_H

#include "vw/gfx/world/components/spatial_component.h"

namespace vw::gfx {

inline auto spatial_component::get_bounds() const -> const aabb& {
    return bounds_;
}

inline auto spatial_component::get_layer() const -> spatial_layer_mask {
    return layer_;
}

inline auto spatial_component::is_dirty() const -> bool {
    return dirty_;
}

}  // namespace vw::gfx

#endif  // VW_GFX_WORLD_COMPONENTS_SPATIAL_COMPONENT_INL_H

