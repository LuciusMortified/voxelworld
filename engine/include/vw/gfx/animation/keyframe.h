#pragma once

#ifndef VW_GFX_ANIMATION_KEYFRAME_H
#define VW_GFX_ANIMATION_KEYFRAME_H

#include "vw/core/math.h"
#include "vw/core/types.h"
#include "vw/core/vec2.h"
#include "vw/gfx/animation/animation_types.h"

namespace vw::gfx {

// Ключевой кадр для vec3f свойства (position, rotation, scale, origin)
struct keyframe_vec3 {
    float32 time;  // Время ключевого кадра в секундах

    vec3f value;  // Значение в этом ключевом кадре

    interpolation_type interp = interpolation_type::linear;  // Тип интерполяции к следующему
                                                             // кадру

    // Контрольные точки для cubic_bezier интерполяции
    // Для других типов интерполяции игнорируются
    vec2f tangent_in{0.0f, 0.0f};   // Входящая касательная (control point 1)
    vec2f tangent_out{1.0f, 1.0f};  // Исходящая касательная (control point 2)

    // Оператор сравнения для сортировки по времени
    [[nodiscard]] auto operator<(const keyframe_vec3& other) const -> bool {
        return time < other.time;
    }

    [[nodiscard]] auto operator==(const keyframe_vec3& other) const -> bool {
        return time == other.time;
    }
};

}  // namespace vw::gfx

#endif  // VW_GFX_ANIMATION_KEYFRAME_H
