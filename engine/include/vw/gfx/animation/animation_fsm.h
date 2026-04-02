#pragma once

#ifndef VW_GFX_ANIMATION_FSM_H
#define VW_GFX_ANIMATION_FSM_H

#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "vw/core/types.h"
#include "vw/gfx/animation/animation_clip.h"
#include "vw/gfx/animation/animation_layer.h"
#include "vw/gfx/animation/animation_types.h"

namespace vw::gfx {

class animation_fsm final {
public:
    using condition_fn = std::function<bool()>;

    struct transition_rule {
        std::string target_state;
        condition_fn condition;
        std::string trigger_name;
        transition blend;
        bool wait_until_end   = false;
        bool wait_until_blend = false;
    };

    struct state_node {
        std::string name;
        std::shared_ptr<animation_clip> clip;
        animation_loop_mode loop_mode = animation_loop_mode::loop;
        float32 playback_speed        = 1.0f;
        transition layer_blend_in;
        transition layer_blend_out;
        std::vector<transition_rule> transitions;
    };

    struct transition_result {
        std::string target_state;
        std::shared_ptr<animation_clip> clip;
        animation_loop_mode loop_mode;
        float32 playback_speed;
        transition blend;
        transition layer_blend_in;
        transition layer_blend_out;
    };

    using trigger_set = std::unordered_set<std::string>;

    void add_state(state_node state);
    void set_entry_state(std::string_view name);
    [[nodiscard]] auto get_current_state() const -> const std::string&;
    [[nodiscard]] auto get_current_state_node() const -> const state_node*;
    [[nodiscard]] auto evaluate(const animation_layer& layer, trigger_set& triggers)
        -> std::optional<transition_result>;
    void apply_transition(const transition_result& result);

private:
    std::string current_state_;
    std::string entry_state_;
    std::unordered_map<std::string, state_node> states_;
};

}  // namespace vw::gfx

#include "vw/gfx/animation/animation_fsm.inl.h"

#endif  // VW_GFX_ANIMATION_FSM_H