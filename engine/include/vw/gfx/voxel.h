#pragma once

#include "vw/gfx/color.h"

namespace vw::gfx {
    struct voxel {
        color value = colors::empty;

        constexpr voxel() : value(colors::empty) {}
        constexpr explicit voxel(color c) : value(c) {}
        
        [[nodiscard]]
        constexpr bool is_empty() const { return value.is_empty(); }
    };
} // namespace vw::gfx