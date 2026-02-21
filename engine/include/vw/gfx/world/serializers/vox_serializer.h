#pragma once

#ifndef VW_GFX_VOX_SERIALIZER_H
#define VW_GFX_VOX_SERIALIZER_H

#include <expected>
#include <filesystem>
#include <string_view>
#include <unordered_set>

#include "vw/gfx/world/world.h"

namespace vw::gfx {

inline constexpr std::string_view vox_file_version = "1.0";

template <typename WC = base_world_components>
class vox_serializer final {
public:
    using world_type        = world<WC>;
    using entity_names_type = std::unordered_map<entity, std::string>;

    enum class error_type : uint8 { file_open_failed, write_failed };

    struct options {
        std::optional<entity_names_type> entity_names;
        std::unordered_set<entity> excluded;
    };

    vox_serializer(world_type& world, entity root, options opts = {});

    auto serialize(const std::filesystem::path& filepath) -> std::expected<void, error_type>;

private:
    void generate_entity_names_();
    void write_header_(std::ofstream& file);
    void write_entity_(std::ofstream& file, entity ent);
    void write_model_(std::ofstream& file, entity ent);

    world_type* world_;
    entity root_;
    entity_names_type entity_names_;
    std::unordered_set<entity> excluded_;
};

}  // namespace vw::gfx

#include "vox_serializer.inl.h"

#endif  // VW_GFX_VOX_SERIALIZER_H
