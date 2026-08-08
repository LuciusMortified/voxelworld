#pragma once

#ifndef VW_ECS_COMPONENTS_BOX_COLLIDER_COMPONENT_H
#define VW_ECS_COMPONENTS_BOX_COLLIDER_COMPONENT_H

#include "vw/core.h"

namespace vw::ecs {

class physics_system;

class renderer;

struct box_collider_component final {
private:
    vec3f extents_{1.0f, 1.0f, 1.0f};
    vec3f offset_{0.0f, 0.0f, 0.0f};

        friend class physics_system;

        friend class renderer;

public:
    [[nodiscard]] auto get_extents() const -> const vec3f&;
    [[nodiscard]] auto get_offset() const -> const vec3f&;
};

}  // namespace vw::ecs

#include "box_collider_component.inl.h"

#endif  // VW_ECS_COMPONENTS_BOX_COLLIDER_COMPONENT_H