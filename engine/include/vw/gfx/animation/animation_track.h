#pragma once

#ifndef VW_GFX_ANIMATION_TRACK_H
#define VW_GFX_ANIMATION_TRACK_H

#include <string>
#include <vector>

#include "vw/core/math.h"
#include "vw/core/transform.h"
#include "vw/core/types.h"
#include "vw/gfx/animation/animation_channel.h"

namespace vw::gfx {

// Трек анимации - набор каналов для одной цели (entity)
// Анимирует один entity в иерархии персонажа
struct animation_track {
    // Имя цели (например, "head", "left_arm", "body")
    // Пустая строка "" означает корневой entity
    std::string target_name;

    // Каналы для разных свойств (position, rotation, scale, origin)
    std::vector<animation_channel> channels;

    // ========== Настройки компиляции ==========

    struct compile_settings {
        uint32 fps = 60;              // Частота кадров (30, 60, 120)
        bool compile_matrices = true;  // Вычислить матрицы заранее
    };

    // ========== Методы ==========

    // Компилировать трек (прекомпиляция всех кадров)
    void compile(const compile_settings& settings);
    void compile();  // Компилировать с настройками по умолчанию

    // Очистить скомпилированные данные
    void clear_compiled();

    // Проверка компиляции
    [[nodiscard]] auto is_compiled() const -> bool { return is_compiled_; }

    // Получить FPS компиляции
    [[nodiscard]] auto get_compiled_fps() const -> uint32 { return compiled_fps_; }

    // Вычислить transform в указанное время (runtime или из кэша)
    [[nodiscard]] auto evaluate(float32 time) const -> transform;

    // Получить скомпилированный transform по времени (быстро!)
    [[nodiscard]] auto get_compiled_transform(float32 time) const -> const transform&;

    // Получить скомпилированную матрицу по времени (еще быстрее!)
    [[nodiscard]] auto get_compiled_matrix(float32 time) const -> const mat4f&;

    // Получить длительность трека (максимум из всех каналов)
    [[nodiscard]] auto get_duration() const -> float32;

    // Добавить канал
    void add_channel(const animation_channel& channel);

    // Получить канал по свойству
    [[nodiscard]] auto get_channel(animation_property prop) const -> const animation_channel*;

    // Проверить наличие канала
    [[nodiscard]] auto has_channel(animation_property prop) const -> bool;

private:
    // ========== Скомпилированные данные ==========

    bool is_compiled_ = false;
    uint32 compiled_fps_ = 0;
    float32 frame_time_ = 0.0f;  // Время одного кадра (1.0 / fps)

    std::vector<transform> compiled_transforms_;    // Предвычисленные transforms
    std::vector<mat4f> compiled_local_matrices_;  // Предвычисленные матрицы (опционально)
};

}  // namespace vw::gfx

#include "vw/gfx/animation/animation_track.inl.h"

#endif  // VW_GFX_ANIMATION_TRACK_H
