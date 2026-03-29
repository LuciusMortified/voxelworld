#pragma once

#ifndef VW_GFX_WORLD_COMPONENTS_MOVEMENT_INTENT_COMPONENT_INL_H
#define VW_GFX_WORLD_COMPONENTS_MOVEMENT_INTENT_COMPONENT_INL_H

namespace vw::gfx {

inline auto movement_intent_component::get_wish_velocity() const -> const vec3f& {
    return wish_velocity_;
}

inline auto movement_intent_component::get_wish_axes() const -> axis_flags {
    return wish_axes_;
}

}  // namespace vw::gfx

#endif  // VW_GFX_WORLD_COMPONENTS_MOVEMENT_INTENT_COMPONENT_INL_H