#include <stdexcept>

#include "types.h"
#include "model.h"

namespace voxel {
    model::model(int width, int height, int depth)
        : width_(width), height_(height), depth_(depth), colors_(width * height * depth, 0) {}

    void model::set_voxel(int x, int y, int z, uint32 color) {
        if (x < 0 || x >= width_ || y < 0 || y >= height_ || z < 0 || z >= depth_)
            throw std::out_of_range("model::set_voxel: coordinates out of range");
        colors_[index(x, y, z)] = color;
    }

    uint32 model::get_voxel(int x, int y, int z) const {
        if (x < 0 || x >= width_ || y < 0 || y >= height_ || z < 0 || z >= depth_)
            throw std::out_of_range("model::get_voxel: coordinates out of range");
        return colors_[index(x, y, z)];
    }

    int model::index(int x, int y, int z) const {
        return x + y * width_ + z * width_ * height_;
    }
} 