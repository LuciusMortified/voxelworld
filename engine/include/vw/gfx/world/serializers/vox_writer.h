#pragma once

#ifndef VW_GFX_VOX_WRITER_H
#define VW_GFX_VOX_WRITER_H

#include <expected>
#include <filesystem>

#include "vw/core/types.h"
#include "vw/asset/vox/vox_prefab_data.h"

namespace vw::gfx {


/// Base class for .vox format writers.
class vox_writer {
public:
    enum class error_type : uint8 { file_open_failed, write_failed };

    virtual ~vox_writer() = default;

    virtual auto write(const std::filesystem::path& filepath, const vw::asset::vox_prefab_data& prefab)
        -> std::expected<void, error_type> = 0;
};

}  // namespace vw::gfx

#endif  // VW_GFX_VOX_WRITER_H
