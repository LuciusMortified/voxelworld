#pragma once

#ifndef VW_GFX_VOX_DESERIALIZER_H
#define VW_GFX_VOX_DESERIALIZER_H
#include <expected>
#include <filesystem>

#include "vw/asset/vox/vox_parser.h"
#include "vw/gfx/world/world.h"

namespace vw::gfx {

template <typename WC = base_world_def>
class vox_deserializer final {
public:
    using world_type = world<WC>;

    using error_type = vw::asset::vox_parser::error_type;

    struct options {
        bool skip_sockets = false;
        bool skip_targets = false;
    };

    struct result {
        std::string root_name;
        std::unordered_map<std::string, entity> name_to_entity;
        std::unordered_map<entity, std::string> entity_to_name;
        std::vector<entity> entities;
    };

    vox_deserializer(world_type& world, vw::asset::vox_parser& parser);

    auto deserialize(const std::filesystem::path& filepath, const options& opts = {})
        -> std::expected<result, error_type>;

private:
    void apply_entity_(const vw::asset::vox_entity_data& data, result& res, const options& opts);

    world_type* world_;
    vw::asset::vox_parser* parser_;
};

}  // namespace vw::gfx

#include "vox_deserializer.inl.h"

#endif  // VW_GFX_VOX_DESERIALIZER_H
