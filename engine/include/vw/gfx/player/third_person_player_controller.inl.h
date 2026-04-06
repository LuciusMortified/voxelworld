#pragma once

#ifndef VW_GFX_PLAYER_THIRD_PERSON_PLAYER_CONTROLLER_INL_H
#define VW_GFX_PLAYER_THIRD_PERSON_PLAYER_CONTROLLER_INL_H

namespace vw::gfx {

template <typename WC>
third_person_player_controller<WC>::third_person_player_controller(
    camera& camera, world_type& world
)
    : camera_(&camera), world_(&world) {}

template <typename WC>
void third_person_player_controller<WC>::update(
    const player_input_state& input, entity target
) {
    auto forward = camera_->get_forward();
    forward.y    = 0.0f;
    forward      = math::normalize(forward);

    auto right = camera_->get_right();
    right.y    = 0.0f;
    right      = math::normalize(right);

    vec3f move_dir = math::normalize(forward * input.move_input.x + right * input.move_input.y);

    auto modifier = world_->get_character_controller_system().modify(target);
    modifier.set_move_input(move_dir);

    if (math::length(move_dir) > math::epsilon) {
        modifier.set_facing_direction(move_dir);
    }

    if (input.jump_requested) {
        modifier.request_jump();
    }
}

}  // namespace vw::gfx

#endif  // VW_GFX_PLAYER_THIRD_PERSON_PLAYER_CONTROLLER_INL_H
