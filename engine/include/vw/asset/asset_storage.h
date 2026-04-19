#pragma once

#ifndef VW_ASSET_ASSET_STORAGE_H
#define VW_ASSET_ASSET_STORAGE_H

#include <filesystem>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>

#include "vw/asset/animation/animation_clip.h"
#include "vw/asset/model/model.h"
#include "vw/asset/model/model_registry.h"
#include "vw/asset/vox/vox_parser.h"

namespace vw::asset {

/// Stores loaded prefabs (models + metadata) and animation clips.
class asset_storage final {
public:
    asset_storage(vox_parser& parser, model_registry& registry);

    void load_prefab(std::string_view name, const std::filesystem::path& filepath);
    void load_clip(std::string_view name, const std::filesystem::path& filepath);

    [[nodiscard]] auto get_entity(std::string_view prefab, std::string_view entity_name) const
        -> const vox_entity_data&;

    [[nodiscard]] auto get_model(std::string_view prefab, std::string_view entity_name) const
        -> std::shared_ptr<model>;

    [[nodiscard]] auto get_clip(std::string_view name) const
        -> std::shared_ptr<animation_clip>;
    [[nodiscard]] auto has_clip(std::string_view name) const -> bool;

private:
    vox_parser* parser_;
    model_registry* model_registry_;
    std::unordered_map<std::string, vox_prefab_data> prefabs_;
    std::unordered_map<std::string, std::shared_ptr<model>> models_;
    std::unordered_map<std::string, std::shared_ptr<animation_clip>> clips_;
};

}  // namespace vw::asset

#include "vw/asset/asset_storage.inl.h"

#endif  // VW_ASSET_ASSET_STORAGE_H
