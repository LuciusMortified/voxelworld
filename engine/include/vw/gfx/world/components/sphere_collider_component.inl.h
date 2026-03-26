#pragma once

#ifndef VW_GFX_WORLD_COMPONENTS_SPHERE_COLLIDER_COMPONENT_INL_H
#define VW_GFX_WORLD_COMPONENTS_SPHERE_COLLIDER_COMPONENT_INL_H

namespace vw::gfx {

inline auto sphere_collider_component::get_radius() const -> float32 {
    return radius_;
}

inline auto sphere_collider_component::get_offset() const -> const vec3f& {
    return offset_;
}

}  // namespace vw::gfx

#endif  // VW_GFX_WORLD_COMPONENTS_SPHERE_COLLIDER_COMPONENT_INL_H
