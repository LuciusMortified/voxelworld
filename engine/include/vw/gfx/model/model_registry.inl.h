#pragma once

#ifndef VW_GFX_MODEL_REGISTRY_INL_H
#define VW_GFX_MODEL_REGISTRY_INL_H

#include "vw/gfx/model/model_registry.h"

namespace vw::gfx {

[[nodiscard]] inline auto model_registry::has(
    std::string_view name
) const -> bool {
    return models_.contains(std::string(name));
}

[[nodiscard]] inline auto model_registry::get(
    std::string_view name
) const -> std::shared_ptr<model> {
    auto iter = models_.find(std::string(name));
    return iter != models_.end() ? iter->second : nullptr;
}

[[nodiscard]] inline auto model_registry::create(
    std::string_view name, int width, int height, int depth
) -> std::shared_ptr<model> {
    auto new_model             = std::make_shared<model>(identity_pool_, width, height, depth);
    models_[std::string(name)] = new_model;
    return new_model;
}

inline auto model_registry::create(
    std::string_view name, vec3i size
) -> std::shared_ptr<model> {
    return create(name, size.x, size.y, size.z);
}

[[nodiscard]] inline auto model_registry::create_unnamed(
    int width, int height, int depth
) -> std::shared_ptr<model> {
    return std::make_shared<model>(identity_pool_, width, height, depth);
}

inline auto model_registry::create_unnamed(
    vec3i size
) -> std::shared_ptr<model> {
    return create_unnamed(size.x, size.y, size.z);
}

[[nodiscard]] inline auto model_registry::create_clone(
    std::string_view name
) -> std::shared_ptr<model> {
    auto original = get(name);
    if (!original) {
        return nullptr;
    }
    auto cloned_model = std::make_shared<model>(
        identity_pool_, original->width(), original->height(), original->depth()
    );
    // Manually copy voxel data
    for (int z = 0; z < original->depth(); ++z) {
        for (int y = 0; y < original->height(); ++y) {
            for (int x = 0; x < original->width(); ++x) {
                cloned_model->set_voxel(x, y, z, original->get_voxel(x, y, z));
            }
        }
    }
    return cloned_model;
}

inline void model_registry::erase(std::string_view name) {
    models_.erase(std::string(name));
}

}  // namespace vw::gfx

#endif  // VW_GFX_MODEL_REGISTRY_INL_H
