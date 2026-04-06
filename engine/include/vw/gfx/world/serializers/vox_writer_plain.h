#pragma once

#ifndef VW_GFX_VOX_WRITER_PLAIN_H
#define VW_GFX_VOX_WRITER_PLAIN_H

#include "vw/gfx/world/serializers/vox_writer.h"

namespace vw::gfx {

inline constexpr std::string_view vox_file_version = "1.0";

/// Plain text format .vox writer.
class vox_writer_plain final : public vox_writer {
public:
    auto write(const std::filesystem::path& filepath, const vox_prefab_data& prefab)
        -> std::expected<void, error_type> override;

private:
    void write_header_(std::ofstream& file, const vox_prefab_data& prefab);
    void write_entity_(std::ofstream& file, const vox_entity_data& ent);
    void write_model_(std::ofstream& file, const vox_model_data& model);
};

}  // namespace vw::gfx

#include "vox_writer_plain.inl.h"

#endif  // VW_GFX_VOX_WRITER_PLAIN_H
