#pragma once

#ifndef VW_GFX_WORLD_COMPONENTS_SPHERE_COLLIDER_COMPONENT_H
#define VW_GFX_WORLD_COMPONENTS_SPHERE_COLLIDER_COMPONENT_H

#include "vw/core.h"

namespace vw::gfx {

template <typename, typename...>
class physics_system;

struct sphere_collider_component final {
private:
    float32 radius_ = 0.5f;
    vec3f offset_{0.0f, 0.0f, 0.0f};

    template <typename, typename...>
    friend class physics_system;

public:
    [[nodiscard]] auto get_radius() const -> float32;
    [[nodiscard]] auto get_offset() const -> const vec3f&;
};

}  // namespace vw::gfx

#include "sphere_collider_component.inl.h"

#endif  // VW_GFX_WORLD_COMPONENTS_SPHERE_COLLIDER_COMPONENT_H
