#pragma once

#ifndef VW_GFX_WORLD_COMPONENTS_BOX_COLLIDER_COMPONENT_H
#define VW_GFX_WORLD_COMPONENTS_BOX_COLLIDER_COMPONENT_H

#include "vw/core.h"

namespace vw::gfx {

template <typename>
class physics_system;

template <typename>
class renderer;

struct box_collider_component final {
private:
    vec3f extents_{1.0f, 1.0f, 1.0f};
    vec3f offset_{0.0f, 0.0f, 0.0f};

    template <typename>
    friend class physics_system;

    template <typename>
    friend class renderer;

public:
    [[nodiscard]] auto get_extents() const -> const vec3f&;
    [[nodiscard]] auto get_offset() const -> const vec3f&;
};

}  // namespace vw::gfx

#include "box_collider_component.inl.h"

#endif  // VW_GFX_WORLD_COMPONENTS_BOX_COLLIDER_COMPONENT_H