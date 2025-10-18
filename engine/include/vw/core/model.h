#pragma once

#ifndef VW_CORE_MODEL_H
#define VW_CORE_MODEL_H

#include <vector>

#include "vw/core/color.h"
#include "vw/core/voxel.h"

namespace vw {
class model {
public:
    model(int width, int height, int depth);

    void set_voxel(int x, int y, int z, const voxel& voxel);
    void set_voxel(int x, int y, int z, color color) {
        set_voxel(x, y, z, voxel{color});
    }

    [[nodiscard]]
    voxel get_voxel(int x, int y, int z) const;

    [[nodiscard]]
    bool is_empty(int x, int y, int z) const;

    [[nodiscard]]
    int width() const {
        return width_;
    }

    [[nodiscard]]
    int height() const {
        return height_;
    }

    [[nodiscard]]
    int depth() const {
        return depth_;
    }

    void fill(const voxel& voxel);

    void clear();

private:
    int width_{0}, height_{0}, depth_{0};

    std::vector<voxel> voxels_;

    [[nodiscard]]
    int index(int x, int y, int z) const;
};
}  // namespace vw

#endif  // VW_CORE_MODEL_H
