#pragma once

#ifndef VW_GFX_WORLD_COMPONENTS_ANIMATION_STATE_MACHINE_COMPONENT_INL_H
#define VW_GFX_WORLD_COMPONENTS_ANIMATION_STATE_MACHINE_COMPONENT_INL_H

namespace vw::gfx {

inline auto animation_state_machine_component::machine_count() const -> size_t {
    return machines_.size();
}

inline auto animation_state_machine_component::get_machine(size_t index) -> animation_state_machine& {
    return machines_[index];
}

inline auto animation_state_machine_component::get_machine(size_t index) const -> const animation_state_machine& {
    return machines_[index];
}

}  // namespace vw::gfx

#endif  // VW_GFX_WORLD_COMPONENTS_ANIMATION_STATE_MACHINE_COMPONENT_INL_H
