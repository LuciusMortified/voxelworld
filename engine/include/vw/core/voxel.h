#pragma once

#ifndef VW_CORE_VOXEL_H
#define VW_CORE_VOXEL_H

#include "vw/core/types.h"

namespace vw {

struct voxel {
    uint8 id = 0;

    constexpr voxel() = default;
    constexpr explicit voxel(uint8 block_id) : id(block_id) {}

    [[nodiscard]] constexpr auto is_empty() const -> bool { return id == 0; }

    constexpr auto operator==(const voxel&) const -> bool = default;
};

static constexpr auto empty_voxel = voxel{};

}  // namespace vw

#endif  // VW_CORE_VOXEL_H