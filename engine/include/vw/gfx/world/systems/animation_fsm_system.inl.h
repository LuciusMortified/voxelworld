#pragma once

#ifndef VW_GFX_WORLD_SYSTEMS_ANIMATION_FSM_SYSTEM_INL_H
#define VW_GFX_WORLD_SYSTEMS_ANIMATION_FSM_SYSTEM_INL_H

#include "vw/gfx/world/systems/animation_system.h"

namespace vw::gfx {

template <typename WC>
animation_fsm_system<WC>::animation_fsm_system(context_type& context)
    : context_(&context) {}

template <typename WC>
void animation_fsm_system<WC>::update(float32 /*dt*/) {
    auto view =
        context_->registry()
            .template view<animation_fsm_component, animation_player_component>();

    for (auto [ent, fsm_comp, player_comp] : view) {
        auto& triggers = fsm_comp.triggers_;
        for (size_t i = 0; i < fsm_comp.machine_count(); ++i) {
            auto pm = context_->template get_system<animation_system>().modify_player(ent);
            if (!player_comp.has_layer(i)) {
                pm.add_layer(i);
                pm.rebuild_target_map();
            }

            auto& machine     = fsm_comp.machines_[i];
            const auto& layer = player_comp.get_layer(i);

            auto lm = pm.layer(i);

            if (layer.state == animation_state::stopped && !layer.clip) {
                const auto* state = machine.get_current_state_node();
                if (!state) {
                    continue;
                }

                if (state->clip) {
                    lm.blend_to(state->clip);
                    lm.set_loop_mode(state->loop_mode);
                    lm.set_playback_speed(state->playback_speed);
                    lm.set_fade_out(state->layer_blend_out);

                    lm.play(state->layer_blend_in);
                    continue;
                }
            }

            auto result = machine.evaluate(layer, triggers);
            if (!result) {
                continue;
            }

            if (result->clip) {
                lm.blend_to(result->clip, result->blend);
                lm.set_loop_mode(result->loop_mode);
                lm.set_playback_speed(result->playback_speed);
                lm.set_fade_out(result->layer_blend_out);

                lm.play(result->layer_blend_in);
            }

            machine.apply_transition(*result);
        }
        triggers.clear();
    }
}

template <typename WC>
animation_fsm_system<WC>::modifier::modifier(
    animation_fsm_component* component
)
    : component_(component) {}

template <typename WC>
void animation_fsm_system<WC>::modifier::add_machine(
    size_t index, animation_fsm machine
) const {
    if (index >= component_->machines_.size()) {
        component_->machines_.resize(index + 1);
    }
    component_->machines_[index] = std::move(machine);
}

template <typename WC>
void animation_fsm_system<WC>::modifier::fire_trigger(
    std::string_view name
) const {
    component_->triggers_.emplace(name);
}

template <typename WC>
auto animation_fsm_system<WC>::modify(
    entity ent
) -> modifier {
    auto& comp = context_->registry().template get<animation_fsm_component>(ent);
    return modifier(&comp);
}

}  // namespace vw::gfx

#endif  // VW_GFX_WORLD_SYSTEMS_ANIMATION_FSM_SYSTEM_INL_H