#pragma once

#ifndef VW_ASSET_ANIMATION_FSM_INL_H
#define VW_ASSET_ANIMATION_FSM_INL_H

#include <algorithm>

namespace vw::asset {

inline void animation_fsm::add_state(state_node state) {
    auto name = state.name;
    states_.emplace(std::move(name), std::move(state));
}

inline void animation_fsm::set_entry_state(std::string_view name) {
    entry_state_   = std::string(name);
    current_state_ = entry_state_;
}

inline auto animation_fsm::get_current_state() const -> const std::string& {
    return current_state_;
}

inline auto animation_fsm::get_current_state_node() const -> const state_node* {
    auto it = states_.find(current_state_);
    if (it == states_.end()) return nullptr;
    return &it->second;
}

inline auto animation_fsm::evaluate(const animation_layer& layer, trigger_set& triggers)
    -> std::optional<transition_result> {
    const auto it = states_.find(current_state_);
    if (it == states_.end()) return std::nullopt;

    const auto& current = it->second;
    for (const auto& rule : current.transitions) {
        if (rule.wait_until_end && layer.state != animation_state::stopped) continue;
        if (rule.wait_until_blend && layer.is_blending()) continue;

        bool triggered = false;
        if (!rule.trigger_name.empty()) {
            if (triggers.contains(rule.trigger_name)) {
                triggered = true;
            }
        }
        if (!triggered && rule.condition) {
            triggered = rule.condition();
        }
        if (!triggered) continue;

        auto target_it = states_.find(rule.target_state);
        if (target_it == states_.end()) continue;

        if (!rule.trigger_name.empty()) {
            triggers.erase(rule.trigger_name);
        }

        const auto& target = target_it->second;
        return transition_result{
            .target_state    = rule.target_state,
            .clip            = target.clip,
            .loop_mode       = target.loop_mode,
            .playback_speed  = target.playback_speed,
            .blend           = rule.blend,
            .layer_blend_in  = target.layer_blend_in,
            .layer_blend_out = target.layer_blend_out,
        };
    }

    return std::nullopt;
}

inline void animation_fsm::apply_transition(const transition_result& result) {
    current_state_ = result.target_state;
}

}  // namespace vw::asset

#endif  // VW_ASSET_ANIMATION_FSM_INL_H