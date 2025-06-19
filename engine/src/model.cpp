#include <stdexcept>

#include "types.h"
#include "model.h"

namespace voxel {
    voxel_model::voxel_model(int width, int height, int depth)
        : width_(width), height_(height), depth_(depth), colors_(width * height * depth, 0) {}

    void voxel_model::set_voxel(const voxel& vxl) {
        if (vxl.x < 0 || vxl.x >= width_ || vxl.y < 0 || vxl.y >= height_ || vxl.z < 0 || vxl.z >= depth_)
            throw std::out_of_range("voxel_model::set_voxel: coordinates out of range");
        colors_[index(vxl.x, vxl.y, vxl.z)] = vxl.color;
    }

    voxel voxel_model::get_voxel(int x, int y, int z) const {
        if (x < 0 || x >= width_ || y < 0 || y >= height_ || z < 0 || z >= depth_)
            throw std::out_of_range("voxel_model::get_voxel: coordinates out of range");
        return voxel{x, y, z, colors_[index(x, y, z)]};
    }

    int voxel_model::index(int x, int y, int z) const {
        return x + y * width_ + z * width_ * height_;
    }
} 