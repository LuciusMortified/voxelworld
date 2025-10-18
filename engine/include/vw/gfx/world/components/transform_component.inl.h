#pragma once

#ifndef VW_GFX_TRANSFORM_COMPONENT_INL_H
#define VW_GFX_TRANSFORM_COMPONENT_INL_H

#include "vw/gfx/world/components/transform_component.h"

namespace vw::gfx {

inline auto transform_component::get_local_matrix() const -> mat4f {
    if (local_dirty_) {
        local_matrix_ = transform_.calc_matrix();
        local_dirty_  = false;
    }
    return local_matrix_;
}

inline auto transform_component::get_world_matrix() const -> mat4f {
    return world_matrix_;
}

inline auto transform_component::get_position() const -> const vec3f& {
    return transform_.get_position();
}

inline auto transform_component::get_rotation() const -> const vec3f& {
    return transform_.get_rotation();
}

inline auto transform_component::get_scale() const -> const vec3f& {
    return transform_.get_scale();
}

inline auto transform_component::get_origin() const -> const vec3f& {
    return transform_.get_origin();
}


}  // namespace vw::gfx

#endif  // VW_GFX_TRANSFORM_COMPONENT_INL_H