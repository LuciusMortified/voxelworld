#pragma once

#ifndef VW_GFX_ANIMATION_CLIP_REGISTRY_H
#define VW_GFX_ANIMATION_CLIP_REGISTRY_H

#include <memory>
#include <string>
#include <unordered_map>

#include "vw/gfx/animation/animation_clip.h"

namespace vw::gfx {

// Реестр анимационных клипов
// Хранит и управляет всеми анимационными клипами в движке
// Аналог model_registry для анимаций
class animation_clip_registry final {
public:
    animation_clip_registry() = default;

    // ========== Регистрация и получение ==========

    // Зарегистрировать клип по имени
    void register_clip(const std::string& name, std::shared_ptr<animation_clip> clip);

    // Получить клип по имени
    [[nodiscard]] auto get_clip(const std::string& name) const
        -> std::shared_ptr<animation_clip>;

    // Проверить существование клипа
    [[nodiscard]] auto has_clip(const std::string& name) const -> bool;

    // Удалить клип из реестра
    void unregister_clip(const std::string& name);

    // ========== Создание клипа ==========

    // Создать новый клип и зарегистрировать его
    [[nodiscard]] auto create_clip(const std::string& name) -> std::shared_ptr<animation_clip>;

    // ========== Получение всех клипов ==========

    // Получить все зарегистрированные клипы
    [[nodiscard]] auto get_all_clips() const
        -> const std::unordered_map<std::string, std::shared_ptr<animation_clip>>&;

    // Получить количество зарегистрированных клипов
    [[nodiscard]] auto clip_count() const -> size_t;

    // Очистить все клипы
    void clear();

private:
    std::unordered_map<std::string, std::shared_ptr<animation_clip>> clips_;
};

}  // namespace vw::gfx

#include "vw/gfx/animation/animation_clip_registry.inl.h"

#endif  // VW_GFX_ANIMATION_CLIP_REGISTRY_H
