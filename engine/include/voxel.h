#pragma once
#include "types.h"

namespace voxel {
    struct voxel {
        uint32 color;

        voxel() : color(0) {}
        voxel(uint32 c) : color(c) {}
    };
}