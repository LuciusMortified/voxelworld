#pragma once

#ifndef VW_GFX_ANIMATION_CLIP_REGISTRY_H
#define VW_GFX_ANIMATION_CLIP_REGISTRY_H

#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>

#include "vw/gfx/animation/animation_clip.h"

namespace vw::gfx {

// Реестр анимационных клипов
// Хранит и управляет всеми анимационными клипами в движке
// Аналог model_registry для анимаций
class animation_clip_registry final {
public:
    animation_clip_registry() = default;

    [[nodiscard]] auto create(std::string_view name) -> std::shared_ptr<animation_clip>;
    [[nodiscard]] auto get(std::string_view name) const -> std::shared_ptr<animation_clip>;
    [[nodiscard]] auto has(std::string_view name) const -> bool;
    void remove(std::string_view name);
    [[nodiscard]] auto all() const
        -> const std::unordered_map<std::string, std::shared_ptr<animation_clip>>&;
    [[nodiscard]] auto count() const -> size_t;
    void clear();

private:
    std::unordered_map<std::string, std::shared_ptr<animation_clip>> clips_;
};

}  // namespace vw::gfx

#include "vw/gfx/animation/animation_clip_registry.inl.h"

#endif  // VW_GFX_ANIMATION_CLIP_REGISTRY_H
