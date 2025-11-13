#pragma once

#ifndef VW_GFX_MODEL_REGISTRY_INL_H
#define VW_GFX_MODEL_REGISTRY_INL_H

#include "vw/gfx/model/model_registry.h"

namespace vw::gfx {

inline void model_registry::add(std::string_view name, std::shared_ptr<model> model) {
    models_[std::string(name)] = std::move(model);
}

[[nodiscard]] inline auto model_registry::has(std::string_view name) const -> bool {
    return models_.contains(std::string(name));
}

[[nodiscard]] inline auto model_registry::get(std::string_view name) const -> std::shared_ptr<model> {
    auto iter = models_.find(std::string(name));
    return iter != models_.end() ? iter->second : nullptr;
}

[[nodiscard]] inline auto model_registry::clone(std::string_view name) const -> std::shared_ptr<model> {
    auto original = get(name);
    if (!original) {
        return nullptr;
    }
    return std::make_shared<model>(*original);
}

inline void model_registry::remove(std::string_view name) {
    models_.erase(std::string(name));
}

}  // namespace vw::gfx

#endif  // VW_GFX_MODEL_REGISTRY_INL_H
