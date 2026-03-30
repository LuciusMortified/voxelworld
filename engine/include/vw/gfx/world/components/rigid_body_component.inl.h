#pragma once

#ifndef VW_GFX_WORLD_COMPONENTS_RIGID_BODY_COMPONENT_INL_H
#define VW_GFX_WORLD_COMPONENTS_RIGID_BODY_COMPONENT_INL_H

namespace vw::gfx {

inline auto rigid_body_component::get_velocity() const -> const vec3f& {
    return velocity_;
}

inline auto rigid_body_component::get_impulse() const -> const vec3f& {
    return impulse_;
}

inline auto rigid_body_component::get_gravity_scale() const -> float32 {
    return gravity_scale_;
}

inline auto rigid_body_component::get_drag() const -> float32 {
    return drag_;
}

inline auto rigid_body_component::is_grounded() const -> bool {
    return grounded_;
}

inline auto rigid_body_component::is_frozen() const -> bool {
    return frozen_;
}

}  // namespace vw::gfx

#endif  // VW_GFX_WORLD_COMPONENTS_RIGID_BODY_COMPONENT_INL_H