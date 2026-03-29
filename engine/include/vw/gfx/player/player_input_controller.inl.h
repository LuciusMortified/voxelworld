#pragma once

#ifndef VW_GFX_PLAYER_PLAYER_INPUT_CONTROLLER_INL_H
#define VW_GFX_PLAYER_PLAYER_INPUT_CONTROLLER_INL_H

#include "vw/gfx/window/window.h"

namespace vw::gfx {

inline player_input_controller::player_input_controller(
    window& window, player_input_params params
)
    : window_(&window)
    , params_(params)
    , mouse_move_sub_(0)
    , mouse_scroll_sub_(0) {
    const auto cursor_pos = window_->get_cursor_pos();
    last_mouse_x_         = cursor_pos.x;
    last_mouse_y_         = cursor_pos.y;
    mouse_initialized_    = true;

    mouse_move_sub_ = window_->sub<mouse_move_event>([this](const mouse_move_event& event) -> bool {
        if (!mouse_captured_) {
            return false;
        }

        if (!mouse_initialized_) {
            last_mouse_x_      = event.x;
            last_mouse_y_      = event.y;
            mouse_initialized_ = true;
            return false;
        }

        const auto delta_x = event.x - last_mouse_x_;
        const auto delta_y = event.y - last_mouse_y_;
        last_mouse_x_      = event.x;
        last_mouse_y_      = event.y;

        accumulated_look_x_ += static_cast<float32>(delta_x) * params_.mouse_sensitivity;
        accumulated_look_y_ += static_cast<float32>(delta_y) * params_.mouse_sensitivity;

        return false;
    });

    mouse_scroll_sub_ = window_->sub<mouse_scroll_event>([this](const mouse_scroll_event& event) -> bool {
        accumulated_scroll_ += static_cast<float32>(event.offset_y);
        return false;
    });
}

inline player_input_controller::~player_input_controller() {
    window_->unsub<mouse_move_event>(mouse_move_sub_);
    window_->unsub<mouse_scroll_event>(mouse_scroll_sub_);
}

inline auto player_input_controller::get_input_state() -> player_input_state {
    player_input_state state;

    if (window_->is_key_pressed(keyboard::keys::W)) {
        state.move_input.x += 1.0f;
    }
    if (window_->is_key_pressed(keyboard::keys::S)) {
        state.move_input.x -= 1.0f;
    }
    if (window_->is_key_pressed(keyboard::keys::D)) {
        state.move_input.y += 1.0f;
    }
    if (window_->is_key_pressed(keyboard::keys::A)) {
        state.move_input.y -= 1.0f;
    }

    auto len_sq = state.move_input.x * state.move_input.x +
                  state.move_input.y * state.move_input.y;
    if (len_sq > 0.0f) {
        auto inv_len = 1.0f / std::sqrt(len_sq);
        state.move_input.x *= inv_len;
        state.move_input.y *= inv_len;
    }

    state.jump_requested = window_->is_key_pressed(keyboard::keys::SPACE);
    state.sprint         = window_->is_key_pressed(keyboard::keys::LEFT_SHIFT);

    state.look_delta = {accumulated_look_x_, accumulated_look_y_};
    state.zoom_delta = accumulated_scroll_;

    accumulated_look_x_ = 0.0f;
    accumulated_look_y_ = 0.0f;
    accumulated_scroll_  = 0.0f;

    return state;
}

inline auto player_input_controller::get_params() -> player_input_params& {
    return params_;
}

inline void player_input_controller::set_mouse_captured(bool captured) {
    mouse_captured_ = captured;
    window_->set_cursor_mode(mouse_captured_ ? cursor_modes::DISABLED : cursor_modes::NORMAL);
    window_->set_input_mode(input_modes::RAW_MOUSE_MOTION, mouse_captured_);
    mouse_initialized_ = false;
}

inline auto player_input_controller::is_mouse_captured() const -> bool {
    return mouse_captured_;
}

}  // namespace vw::gfx

#endif  // VW_GFX_PLAYER_PLAYER_INPUT_CONTROLLER_INL_H
