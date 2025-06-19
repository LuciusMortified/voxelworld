#pragma once
#include <vector>

#include "types.h"
#include "voxel.h"

namespace voxel {
    class model {
    public:
        model(int width, int height, int depth);
        void set_voxel(const voxel& vxl);
        void set_voxel(const ivec3& pos, const voxel& vxl);
        voxel get_voxel(int x, int y, int z) const;
        voxel get_voxel(const ivec3& pos) const;
        int width() const { return width_; }
        int height() const { return height_; }
        int depth() const { return depth_; }
    private:
        int width_, height_, depth_;
        std::vector<uint32> colors_;
        int index(int x, int y, int z) const;
    };
} 