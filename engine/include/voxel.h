#pragma once
#include "types.h"

namespace voxel {
    struct voxel {
        ivec3 pos;
        uint32 color;

        voxel() : pos(), color(0) {}
        voxel(int x, int y, int z, uint32 c) : pos(x, y, z), color(c) {}
        voxel(const ivec3& position, uint32 c) : pos(position), color(c) {}
    };
}