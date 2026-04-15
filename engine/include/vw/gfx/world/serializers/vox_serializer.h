#pragma once

#ifndef VW_GFX_VOX_SERIALIZER_H
#define VW_GFX_VOX_SERIALIZER_H

#include <expected>
#include <filesystem>
#include <unordered_set>

#include "vw/gfx/world/serializers/vox_writer.h"
#include "vw/gfx/world/world.h"

namespace vw::gfx {

template <typename WC = base_world_def>
class vox_serializer final {
public:
    using world_type        = world<WC>;
    using entity_names_type = std::unordered_map<entity, std::string>;

    using error_type = vox_writer::error_type;

    struct options {
        std::optional<entity_names_type> entity_names;
        std::unordered_set<entity> excluded;
    };

    vox_serializer(world_type& world, vox_writer& writer, entity root, options opts = {});

    auto serialize(const std::filesystem::path& filepath) -> std::expected<void, error_type>;

    [[nodiscard]] auto extract() const -> vox_prefab_data;

private:
    void generate_entity_names_();
    [[nodiscard]] auto extract_entity_(entity ent) const -> vox_entity_data;

    world_type* world_;
    vox_writer* writer_;
    entity root_;
    entity_names_type entity_names_;
    std::unordered_set<entity> excluded_;
};

}  // namespace vw::gfx

#include "vox_serializer.inl.h"

#endif  // VW_GFX_VOX_SERIALIZER_H
