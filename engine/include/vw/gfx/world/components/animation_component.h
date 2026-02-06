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
    // ========== Текущая анимация ==========

    std::shared_ptr<animation_clip> clip_;  // Текущий анимационный клип

    // ========== Состояние воспроизведения ==========

    animation_state state_ = animation_state::stopped;  // Состояние воспроизведения

    float32 current_time_ = 0.0f;  // Текущее время в анимации (секунды)

    float32 playback_speed_ = 1.0f;  // Скорость воспроизведения (1.0 = нормальная)

    animation_loop_mode loop_mode_ = animation_loop_mode::once;  // Режим зацикливания

    float32 direction_ = 1.0f;  // Направление воспроизведения (1.0 = вперед, -1.0 = назад)

    // ========== Блендинг анимаций ==========

    float32 blend_time_ = 0.0f;         // Текущее время блендинга
    float32 blend_duration_ = 0.0f;     // Длительность блендинга
    std::shared_ptr<animation_clip> previous_clip_;  // Предыдущий клип для блендинга
    float32 previous_time_ = 0.0f;      // Время в предыдущем клипе

public:
    // ========== Getters ==========

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

    // ========== Friend декларация ==========

    template <typename... Cs>
    friend class animation_system;
};

}  // namespace vw::gfx

#include "vw/gfx/world/components/animation_component.inl.h"

#endif  // VW_GFX_ANIMATION_COMPONENT_H
