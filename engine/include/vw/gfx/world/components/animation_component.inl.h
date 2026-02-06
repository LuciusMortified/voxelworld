#pragma once

namespace vw::gfx {

inline auto animation_component::get_clip() const -> std::shared_ptr<animation_clip> {
    return clip_;
}

inline auto animation_component::get_state() const -> animation_state {
    return state_;
}

inline auto animation_component::get_current_time() const -> float32 {
    return current_time_;
}

inline auto animation_component::get_playback_speed() const -> float32 {
    return playback_speed_;
}

inline auto animation_component::get_loop_mode() const -> animation_loop_mode {
    return loop_mode_;
}

inline auto animation_component::get_duration() const -> float32 {
    if (!clip_) {
        return 0.0f;
    }
    return clip_->get_duration();
}

inline auto animation_component::is_playing() const -> bool {
    return state_ == animation_state::playing;
}

inline auto animation_component::is_paused() const -> bool {
    return state_ == animation_state::paused;
}

inline auto animation_component::is_stopped() const -> bool {
    return state_ == animation_state::stopped;
}

inline auto animation_component::is_finished() const -> bool {
    if (!clip_) {
        return true;
    }

    if (state_ == animation_state::stopped) {
        return true;
    }

    // Анимация завершена если достигнут конец и режим не loop
    if (loop_mode_ == animation_loop_mode::once) {
        float32 duration = clip_->get_duration();
        return current_time_ >= duration;
    }

    return false;
}

}  // namespace vw::gfx
