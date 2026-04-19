#pragma once

#ifndef VW_ASSET_VOX_PARSER_H
#define VW_ASSET_VOX_PARSER_H

#include <expected>
#include <filesystem>

#include "vw/core/types.h"
#include "vw/asset/vox/vox_prefab_data.h"

namespace vw::asset {

/// Base class for .vox format parsers.
class vox_parser {
public:
    enum class error_type : uint8 { file_open_failed, parse_error };

    virtual ~vox_parser() = default;

    virtual auto parse(const std::filesystem::path& filepath)
        -> std::expected<vox_prefab_data, error_type> = 0;
};

}  // namespace vw::asset

#endif  // VW_ASSET_VOX_PARSER_H
