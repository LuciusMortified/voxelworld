#pragma once

#ifndef VW_GFX_PLAYER_PLAYER_INPUT_CONTROLLER_H
#define VW_GFX_PLAYER_PLAYER_INPUT_CONTROLLER_H

#include "vw/core.h"
#include "vw/gfx/player/player_input_state.h"
#include "vw/platform/event.h"
#include "vw/platform/input.h"
#include "vw/platform/window.h"

namespace vw::gfx {

using namespace ::vw::plat;

struct player_input_params {
    float32 mouse_sensitivity = 0.1f;
};

/// Unified input layer that reads keyboard/mouse and produces player_input_state.
class player_input_controller {
public:
    explicit player_input_controller(window& window, player_input_params params = {});
    ~player_input_controller();

    player_input_controller(const player_input_controller&)            = delete;
    auto operator=(const player_input_controller&) -> player_input_controller& = delete;
    player_input_controller(player_input_controller&&)                 = delete;
    auto operator=(player_input_controller&&) -> player_input_controller& = delete;

    [[nodiscard]] auto get_input_state() -> player_input_state;

    [[nodiscard]] auto get_params() -> player_input_params&;

    void set_mouse_captured(bool captured);
    [[nodiscard]] auto is_mouse_captured() const -> bool;

private:
    window* window_;
    player_input_params params_;

    bool mouse_captured_{false};

    double last_mouse_x_{0.0};
    double last_mouse_y_{0.0};
    bool mouse_initialized_{false};

    float32 accumulated_look_x_{0.0f};
    float32 accumulated_look_y_{0.0f};
    float32 accumulated_scroll_{0.0f};

    bool attack_pressed_{false};

    event_sub<mouse_move_event> mouse_move_sub_;
    event_sub<mouse_scroll_event> mouse_scroll_sub_;
    event_sub<mouse_press_event> mouse_press_sub_;
};

}  // namespace vw::gfx

#include "vw/gfx/player/player_input_controller.inl.h"

#endif  // VW_GFX_PLAYER_PLAYER_INPUT_CONTROLLER_H
