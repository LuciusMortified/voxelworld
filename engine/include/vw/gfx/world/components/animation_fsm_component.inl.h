#pragma once

#ifndef VW_GFX_WORLD_COMPONENTS_ANIMATION_FSM_COMPONENT_INL_H
#define VW_GFX_WORLD_COMPONENTS_ANIMATION_FSM_COMPONENT_INL_H

namespace vw::gfx {

inline auto animation_fsm_component::machine_count() const -> size_t {
    return machines_.size();
}

inline auto animation_fsm_component::get_machine(size_t index) -> animation_fsm& {
    return machines_[index];
}

inline auto animation_fsm_component::get_machine(size_t index) const -> const animation_fsm& {
    return machines_[index];
}

}  // namespace vw::gfx

#endif  // VW_GFX_WORLD_COMPONENTS_ANIMATION_FSM_COMPONENT_INL_H