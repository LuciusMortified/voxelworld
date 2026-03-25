#pragma once

#ifndef VW_CORE_VOXEL_H
#define VW_CORE_VOXEL_H

#include "block_registry.h"
#include "vw/core/types.h"

namespace vw {

struct voxel {
    block_id id = blocks::air;

    constexpr voxel() = default;
    constexpr explicit voxel(block_id block_id) : id(block_id) {}

    [[nodiscard]] constexpr auto is_empty() const -> bool { return id == blocks::air; }

    constexpr auto operator==(const voxel&) const -> bool = default;
};

static constexpr auto empty_voxel = voxel{};

}  // namespace vw

#endif  // VW_CORE_VOXEL_H