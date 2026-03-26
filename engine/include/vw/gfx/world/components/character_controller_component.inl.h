#pragma once

#ifndef VW_GFX_WORLD_COMPONENTS_CHARACTER_CONTROLLER_COMPONENT_INL_H
#define VW_GFX_WORLD_COMPONENTS_CHARACTER_CONTROLLER_COMPONENT_INL_H

namespace vw::gfx {

inline auto character_controller_component::get_move_speed() const -> float32 {
    return move_speed_;
}

inline auto character_controller_component::get_jump_impulse() const -> float32 {
    return jump_impulse_;
}

inline auto character_controller_component::get_rotation_speed() const -> float32 {
    return rotation_speed_;
}

inline auto character_controller_component::get_facing_direction() const -> const vec3f& {
    return facing_direction_;
}

}  // namespace vw::gfx

#endif  // VW_GFX_WORLD_COMPONENTS_CHARACTER_CONTROLLER_COMPONENT_INL_H
