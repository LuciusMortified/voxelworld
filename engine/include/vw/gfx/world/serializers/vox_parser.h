#pragma once

#ifndef VW_GFX_VOX_PARSER_H
#define VW_GFX_VOX_PARSER_H

#include <expected>
#include <filesystem>

#include "vw/core/types.h"
#include "vw/gfx/world/serializers/vox_prefab_data.h"

namespace vw::gfx {

/// Base class for .vox format parsers.
class vox_parser {
public:
    enum class error_type : uint8 { file_open_failed, parse_error };

    virtual ~vox_parser() = default;

    virtual auto parse(const std::filesystem::path& filepath)
        -> std::expected<vox_prefab_data, error_type> = 0;
};

}  // namespace vw::gfx

#endif  // VW_GFX_VOX_PARSER_H
