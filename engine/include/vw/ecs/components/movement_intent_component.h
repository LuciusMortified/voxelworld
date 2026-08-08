#pragma once

#ifndef VW_ECS_COMPONENTS_MOVEMENT_INTENT_COMPONENT_H
#define VW_ECS_COMPONENTS_MOVEMENT_INTENT_COMPONENT_H

#include "vw/core.h"

namespace vw::ecs {

using axis_flags = uint8;

namespace axis_flag {
    static constexpr axis_flags none = 0;
    static constexpr axis_flags x    = 1;
    static constexpr axis_flags y    = 2;
    static constexpr axis_flags z    = 4;
    static constexpr axis_flags xz   = x | z;
    static constexpr axis_flags xyz  = x | y | z;
}  // namespace axis_flag

class physics_system;

class character_controller_system;

struct movement_intent_component final {
private:
    vec3f wish_velocity_{0.0f, 0.0f, 0.0f};
    axis_flags wish_axes_ = axis_flag::xz;

        friend class physics_system;

        friend class character_controller_system;

public:
    [[nodiscard]] auto get_wish_velocity() const -> const vec3f&;
    [[nodiscard]] auto get_wish_axes() const -> axis_flags;
};

}  // namespace vw::ecs

#include "movement_intent_component.inl.h"

#endif  // VW_ECS_COMPONENTS_MOVEMENT_INTENT_COMPONENT_H