#include <stdexcept>

#include <voxel/types.h>
#include <voxel/model.h>

namespace voxel {
    model::model(int width, int height, int depth)
        : width_(width), height_(height), depth_(depth), 
          voxels_(width * height * depth, voxel()) {}

    void model::set_voxel(int x, int y, int z, const voxel& voxel) {
        if (x < 0 || x >= width_ || y < 0 || y >= height_ || z < 0 || z >= depth_)
            throw std::out_of_range("model::set_voxel: coordinates out of range");
        voxels_[index(x, y, z)] = voxel;
    }

    voxel model::get_voxel(int x, int y, int z) const {
        if (x < 0 || x >= width_ || y < 0 || y >= height_ || z < 0 || z >= depth_)
            throw std::out_of_range("model::get_voxel: coordinates out of range");
        return voxels_[index(x, y, z)];
    }

    bool model::has_voxel(int x, int y, int z) const {
        if (x < 0 || x >= width_ || y < 0 || y >= height_ || z < 0 || z >= depth_)
            return false;
        return voxels_[index(x, y, z)].color != 0;
    }

    bool model::is_empty(int x, int y, int z) const {
        if (x < 0 || x >= width_ || y < 0 || y >= height_ || z < 0 || z >= depth_)
            return true;
        return voxels_[index(x, y, z)].color == 0;
    }

    void model::clear() {
        std::fill(voxels_.begin(), voxels_.end(), voxel());
    }

    void model::fill(const voxel& voxel) {
        std::fill(voxels_.begin(), voxels_.end(), voxel);
    }

    int model::index(int x, int y, int z) const {
        return x + y * width_ + z * width_ * height_;
    }
} 