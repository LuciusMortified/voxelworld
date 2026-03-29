#pragma once

#ifndef VW_GFX_WORLD_COMPONENTS_BOX_COLLIDER_COMPONENT_INL_H
#define VW_GFX_WORLD_COMPONENTS_BOX_COLLIDER_COMPONENT_INL_H

namespace vw::gfx {

inline auto box_collider_component::get_extents() const -> const vec3f& {
    return extents_;
}

inline auto box_collider_component::get_offset() const -> const vec3f& {
    return offset_;
}

}  // namespace vw::gfx

#endif  // VW_GFX_WORLD_COMPONENTS_BOX_COLLIDER_COMPONENT_INL_H