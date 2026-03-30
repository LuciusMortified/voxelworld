#pragma once

#ifndef VW_GFX_WORLD_COMPONENTS_RIGID_BODY_COMPONENT_H
#define VW_GFX_WORLD_COMPONENTS_RIGID_BODY_COMPONENT_H

#include "vw/core.h"

namespace vw::gfx {

template <typename>
class physics_system;

struct rigid_body_component final {
private:
    vec3f velocity_{0.0f, 0.0f, 0.0f};
    vec3f impulse_{0.0f, 0.0f, 0.0f};
    float32 gravity_scale_ = 1.0f;
    float32 drag_ = 5.0f;
    bool grounded_ = false;
    bool frozen_ = false;

    template <typename>
    friend class physics_system;

public:
    [[nodiscard]] auto get_velocity() const -> const vec3f&;
    [[nodiscard]] auto get_impulse() const -> const vec3f&;
    [[nodiscard]] auto get_gravity_scale() const -> float32;
    [[nodiscard]] auto get_drag() const -> float32;
    [[nodiscard]] auto is_grounded() const -> bool;
    [[nodiscard]] auto is_frozen() const -> bool;
};

}  // namespace vw::gfx

#include "rigid_body_component.inl.h"

#endif  // VW_GFX_WORLD_COMPONENTS_RIGID_BODY_COMPONENT_H