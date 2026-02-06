#pragma once

#ifndef VW_GFX_ANIMATION_COMPONENT_H
#define VW_GFX_ANIMATION_COMPONENT_H

#include <memory>

#include "vw/core/types.h"
#include "vw/gfx/animation/animation_clip.h"
#include "vw/gfx/animation/animation_types.h"

namespace vw::gfx {

template <typename... Cs>
class animation_system;

// Компонент анимации - хранит состояние воспроизведения анимации
// Прикрепляется к корневому entity иерархии (например, body персонажа)
struct animation_component final {
private:
    std::shared_ptr<animation_clip> clip_;
    animation_state state_ = animation_state::stopped;
    float32 current_time_ = 0.0f;
    float32 playback_speed_ = 1.0f;
    animation_loop_mode loop_mode_ = animation_loop_mode::once;
    float32 direction_ = 1.0f;
    float32 blend_time_ = 0.0f;
    float32 blend_duration_ = 0.0f;
    std::shared_ptr<animation_clip> previous_clip_;
    float32 previous_time_ = 0.0f;

public:
    [[nodiscard]] auto get_clip() const -> std::shared_ptr<animation_clip>;
    [[nodiscard]] auto get_state() const -> animation_state;
    [[nodiscard]] auto get_current_time() const -> float32;
    [[nodiscard]] auto get_playback_speed() const -> float32;
    [[nodiscard]] auto get_loop_mode() const -> animation_loop_mode;
    [[nodiscard]] auto get_duration() const -> float32;
    [[nodiscard]] auto is_playing() const -> bool;
    [[nodiscard]] auto is_paused() const -> bool;
    [[nodiscard]] auto is_stopped() const -> bool;
    [[nodiscard]] auto is_finished() const -> bool;

    template <typename... Cs>
    friend class animation_system;
};

}  // namespace vw::gfx

#include "vw/gfx/world/components/animation_component.inl.h"

#endif  // VW_GFX_ANIMATION_COMPONENT_H
