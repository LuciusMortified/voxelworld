#pragma once

#ifndef VW_GFX_WORLD_COMPONENTS_RIGID_BODY_COMPONENT_H
#define VW_GFX_WORLD_COMPONENTS_RIGID_BODY_COMPONENT_H

#include "vw/core.h"

namespace vw::gfx {

template <typename, typename...>
class physics_system;

template <typename...>
class character_controller_system;

struct rigid_body_component final {
private:
    vec3f velocity_{0.0f, 0.0f, 0.0f};
    float32 gravity_scale_ = 1.0f;
    bool grounded_ = false;
    bool frozen_ = false;

    template <typename, typename...>
    friend class physics_system;

    template <typename...>
    friend class character_controller_system;

public:
    [[nodiscard]] auto get_velocity() const -> const vec3f&;
    [[nodiscard]] auto get_gravity_scale() const -> float32;
    [[nodiscard]] auto is_grounded() const -> bool;
    [[nodiscard]] auto is_frozen() const -> bool;
};

}  // namespace vw::gfx

#include "rigid_body_component.inl.h"

#endif  // VW_GFX_WORLD_COMPONENTS_RIGID_BODY_COMPONENT_H
