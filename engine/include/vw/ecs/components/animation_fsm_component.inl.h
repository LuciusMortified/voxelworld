#pragma once

#ifndef VW_ECS_COMPONENTS_ANIMATION_FSM_COMPONENT_INL_H
#define VW_ECS_COMPONENTS_ANIMATION_FSM_COMPONENT_INL_H

namespace vw::ecs {

inline auto animation_fsm_component::machine_count() const -> size_t {
    return machines_.size();
}

inline auto animation_fsm_component::get_machine(size_t index) -> vw::asset::animation_fsm& {
    return machines_[index];
}

inline auto animation_fsm_component::get_machine(size_t index) const -> const vw::asset::animation_fsm& {
    return machines_[index];
}

}  // namespace vw::ecs

#endif  // VW_ECS_COMPONENTS_ANIMATION_FSM_COMPONENT_INL_H