#pragma once

#ifndef VW_GFX_WORLD_COMPONENTS_CHARACTER_CONTROLLER_COMPONENT_H
#define VW_GFX_WORLD_COMPONENTS_CHARACTER_CONTROLLER_COMPONENT_H

#include "vw/core.h"

namespace vw::gfx {

template <typename...>
class character_controller_system;

struct character_controller_component final {
private:
    vec3f move_input_{0.0f, 0.0f, 0.0f};
    float32 move_speed_ = 50.0f;
    float32 jump_impulse_ = 50.0f;
    bool jump_requested_ = false;

    template <typename...>
    friend class character_controller_system;

public:
    [[nodiscard]] auto get_move_speed() const -> float32;
    [[nodiscard]] auto get_jump_impulse() const -> float32;
};

}  // namespace vw::gfx

#include "character_controller_component.inl.h"

#endif  // VW_GFX_WORLD_COMPONENTS_CHARACTER_CONTROLLER_COMPONENT_H
