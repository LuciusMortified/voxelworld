#pragma once

#ifndef VW_GFX_ANIMATION_CHANNEL_H
#define VW_GFX_ANIMATION_CHANNEL_H

#include <algorithm>
#include <vector>

#include "vw/core/math.h"
#include "vw/core/types.h"
#include "vw/gfx/animation/animation_types.h"
#include "vw/gfx/animation/keyframe.h"

namespace vw::gfx {

// Канал анимации - анимирует одно свойство (position, rotation, scale или origin)
struct animation_channel {
    animation_property property;         // Какое свойство анимируется
    std::vector<keyframe_vec3> keyframes;  // Ключевые кадры (должны быть отсортированы по
                                           // времени)

    // Вычислить значение в указанное время с интерполяцией
    [[nodiscard]] auto evaluate(float32 time) const -> vec3f;

    // Получить длительность канала (время последнего ключевого кадра)
    [[nodiscard]] auto get_duration() const -> float32;

    // Добавить ключевой кадр и автоматически отсортировать по времени
    void add_keyframe(const keyframe_vec3& keyframe);

    // Проверить, пуст ли канал
    [[nodiscard]] auto is_empty() const -> bool { return keyframes.empty(); }

    // Получить количество ключевых кадров
    [[nodiscard]] auto keyframe_count() const -> size_t { return keyframes.size(); }
};

}  // namespace vw::gfx

#include "vw/gfx/animation/animation_channel.inl.h"

#endif  // VW_GFX_ANIMATION_CHANNEL_H
