#pragma once

#ifndef VW_ASSET_ASSET_STORAGE_INL_H
#define VW_ASSET_ASSET_STORAGE_INL_H

#include "vw/asset/voxa/voxa_deserializer.h"
#include "vw/log/logger.h"

namespace vw::asset {

namespace detail {
inline constexpr log::log_category asset_storage_lc{"asset_storage"};
}  // namespace detail

inline asset_storage::asset_storage(vox_parser& parser, model_registry& registry)
    : parser_(&parser), model_registry_(&registry) {}

inline void asset_storage::load_prefab(
    std::string_view name, const std::filesystem::path& filepath
) {
    auto result = parser_->parse(filepath);
    if (!result.has_value()) {
        log::warn(detail::asset_storage_lc, "failed to load prefab '{}': {}", name, filepath.string());
        return;
    }

    std::string name_str(name);

    for (auto& ent : result->entities) {
        if (!ent.model.has_value()) {
            continue;
        }

        std::string model_key = name_str + "/" + ent.name;
        auto m = model_registry_->create(model_key, ent.model->size);

        for (const auto& [pos, v] : ent.model->voxels) {
            m->set_voxel(pos, v);
        }

        models_[model_key] = std::move(m);
        ent.model = std::nullopt;
    }

    prefabs_[name_str] = std::move(*result);
}

inline void asset_storage::load_clip(
    std::string_view name, const std::filesystem::path& filepath
) {
    voxa_deserializer deserializer;
    auto result = deserializer.deserialize(filepath);
    if (!result.has_value()) {
        log::warn(detail::asset_storage_lc, "failed to load clip '{}': {}", name, filepath.string());
        return;
    }

    clips_[std::string(name)] = std::move(*result);
}

inline auto asset_storage::get_entity(
    std::string_view prefab, std::string_view entity_name
) const -> const vox_entity_data& {
    auto pit = prefabs_.find(std::string(prefab));
    for (const auto& ent : pit->second.entities) {
        if (ent.name == entity_name) {
            return ent;
        }
    }

    log::warn(detail::asset_storage_lc, "entity '{}' not found in prefab '{}'", entity_name, prefab);
    return pit->second.entities.front();
}

inline auto asset_storage::get_model(
    std::string_view prefab, std::string_view entity_name
) const -> std::shared_ptr<model> {
    std::string key = std::string(prefab) + "/" + std::string(entity_name);
    auto it = models_.find(key);
    if (it != models_.end()) {
        return it->second;
    }
    return nullptr;
}

inline auto asset_storage::get_clip(std::string_view name) const
    -> std::shared_ptr<animation_clip> {
    auto it = clips_.find(std::string(name));
    if (it != clips_.end()) {
        return it->second;
    }
    return nullptr;
}

inline auto asset_storage::has_clip(std::string_view name) const -> bool {
    return clips_.find(std::string(name)) != clips_.end();
}

}  // namespace vw::asset

#endif  // VW_ASSET_ASSET_STORAGE_INL_H
