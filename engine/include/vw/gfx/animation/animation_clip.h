#pragma once

#ifndef VW_GFX_ANIMATION_CLIP_H
#define VW_GFX_ANIMATION_CLIP_H

#include <memory>
#include <string>
#include <vector>

#include "vw/core/transform.h"
#include "vw/core/types.h"
#include "vw/gfx/animation/animation_track.h"

namespace vw::gfx {

// Анимационный клип - набор треков для множественных целей (entity)
// Представляет полную анимацию (например, "walk", "run", "jump")
class animation_clip final {
public:
    explicit animation_clip(std::string name = "unnamed");

    // ========== Управление треками ==========

    // Добавить трек для цели
    void add_track(animation_track track);

    // Получить трек по имени цели
    [[nodiscard]] auto get_track(const std::string& target_name) const -> const animation_track*;

    // Получить трек по имени цели (mutable)
    [[nodiscard]] auto get_track_mut(const std::string& target_name) -> animation_track*;

    // Проверить наличие трека
    [[nodiscard]] auto has_track(const std::string& target_name) const -> bool;

    // Получить все треки
    [[nodiscard]] auto get_tracks() const -> const std::vector<animation_track>&;

    // Удалить трек
    void remove_track(const std::string& target_name);

    // ========== Удобные методы ==========

    // Добавить канал к существующему или новому треку
    void add_channel_to_track(const std::string& target_name, const animation_channel& channel);

    // Получить длительность клипа (максимальная из всех треков)
    [[nodiscard]] auto get_duration() const -> float32;

    // ========== Компиляция ==========

    // Компилировать все треки
    void compile(const animation_track::compile_settings& settings);
    void compile();  // Компилировать с настройками по умолчанию

    // Очистить компиляцию всех треков
    void clear_compiled();

    // Проверка компиляции
    [[nodiscard]] auto is_compiled() const -> bool { return is_compiled_; }

    // ========== Имя клипа ==========

    [[nodiscard]] auto get_name() const -> const std::string&;
    void set_name(std::string name);

    // ========== Обратная совместимость (для анимации одиночного entity) ==========

    // Добавить канал к треку с пустым именем (корневой entity)
    void add_channel(const animation_channel& channel);

    // Оценить трек с пустым именем в указанное время
    [[nodiscard]] auto evaluate_single(float32 time) const -> transform;

private:
    std::string name_;
    std::vector<animation_track> tracks_;
    bool is_compiled_ = false;
};

}  // namespace vw::gfx

#include "vw/gfx/animation/animation_clip.inl.h"

#endif  // VW_GFX_ANIMATION_CLIP_H
