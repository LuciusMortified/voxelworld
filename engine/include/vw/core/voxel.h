#pragma once

#ifndef VW_CORE_VOXEL_H
#define VW_CORE_VOXEL_H

#include "vw/core/color.h"

namespace vw {
struct voxel {
    color value = colors::empty;

    constexpr voxel() : value(colors::empty) {}
    constexpr explicit voxel(color c) : value(c) {}

    [[nodiscard]]
    constexpr bool is_empty() const {
        return value.is_empty();
    }
};

static constexpr auto empty_voxel = voxel{colors::empty};

}  // namespace vw

#endif  // VW_CORE_VOXEL_H
