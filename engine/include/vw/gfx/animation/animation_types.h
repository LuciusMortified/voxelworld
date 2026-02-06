#pragma once

#ifndef VW_GFX_ANIMATION_TYPES_H
#define VW_GFX_ANIMATION_TYPES_H

#include "vw/core/types.h"

namespace vw::gfx {

// Состояние воспроизведения анимации
enum class animation_state : uint8 {
    stopped,  // Анимация остановлена
    playing,  // Анимация воспроизводится
    paused    // Анимация на паузе
};

// Режим зацикливания анимации
enum class animation_loop_mode : uint8 {
    once,      // Воспроизвести один раз и остановиться
    loop,      // Зациклить воспроизведение
    ping_pong  // Воспроизвести туда-обратно
};

// Тип анимируемого свойства transform
enum class animation_property : uint8 {
    position,  // Позиция (vec3f)
    rotation,  // Поворот (vec3f, углы в радианах)
    scale,     // Масштаб (vec3f)
    origin     // Точка вращения (vec3f)
};

// Тип интерполяции между ключевыми кадрами
enum class interpolation_type : uint8 {
    linear,       // Линейная интерполяция
    step,         // Без интерполяции (дискретные значения)
    ease_in,      // Плавный вход (quadratic)
    ease_out,     // Плавный выход (quadratic)
    ease_in_out,  // Плавный вход-выход (quadratic)
    cubic_bezier  // Кубическая кривая Безье (с контрольными точками)
};

}  // namespace vw::gfx

#endif  // VW_GFX_ANIMATION_TYPES_H
