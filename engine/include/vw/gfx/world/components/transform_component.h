#pragma once

#ifndef VW_GFX_TRANSFORM_COMPONENT_H
#define VW_GFX_TRANSFORM_COMPONENT_H

#include "vw/core.h"

namespace vw::gfx {

struct transform_component final {
private:
    transform transform_;

    mutable mat4f local_matrix_;
    mutable bool local_dirty_ = true;

    mutable mat4f world_matrix_;
    mutable bool world_dirty_ = true;

public:
    [[nodiscard]] auto get_local_matrix() const -> mat4f;
    [[nodiscard]] auto get_world_matrix() const -> mat4f;

    [[nodiscard]] auto get_transform() const -> const transform&;
    [[nodiscard]] auto get_position() const -> const vec3f&;
    [[nodiscard]] auto get_rotation() const -> const quat&;
    [[nodiscard]] auto get_rotation_euler() const -> vec3f;
    [[nodiscard]] auto get_scale() const -> const vec3f&;
    [[nodiscard]] auto get_origin() const -> const vec3f&;

    template <typename>
    friend class hierarchy_system;

    template <typename>
    friend class transform_system;
};

}  // namespace vw::gfx

#include "vw/gfx/world/components/transform_component.inl.h"

#endif  // VW_GFX_TRANSFORM_COMPONENT_H
