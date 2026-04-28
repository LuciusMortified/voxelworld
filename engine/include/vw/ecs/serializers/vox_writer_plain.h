#pragma once

#ifndef VW_ECS_VOX_WRITER_PLAIN_H
#define VW_ECS_VOX_WRITER_PLAIN_H

#include "vw/ecs/serializers/vox_writer.h"

namespace vw::ecs {

inline constexpr std::string_view vox_file_version = "1.0";

/// Plain text format .vox writer.
class vox_writer_plain final : public vox_writer {
public:
    auto write(const std::filesystem::path& filepath, const vw::asset::vox_prefab_data& prefab)
        -> std::expected<void, error_type> override;

private:
    void write_header_(std::ofstream& file, const vw::asset::vox_prefab_data& prefab);
    void write_entity_(std::ofstream& file, const vw::asset::vox_entity_data& ent);
    void write_model_(std::ofstream& file, const vw::asset::vox_model_data& mdl);
};

}  // namespace vw::ecs

#include "vox_writer_plain.inl.h"

#endif  // VW_ECS_VOX_WRITER_PLAIN_H
