#pragma once

#ifndef VW_GFX_MODEL_MODEL_H
#define VW_GFX_MODEL_MODEL_H

#include <vector>

#include "vw/core/color.h"
#include "vw/core/voxel.h"
#include "vw/gfx/model/model_hash.h"

namespace vw::gfx {

class model {
public:
    model(int width, int height, int depth);

    void set_voxel(int x, int y, int z, const voxel& voxel);
    void set_voxel(int x, int y, int z, color color) {
        set_voxel(x, y, z, voxel{color});
    }

    [[nodiscard]]
    auto get_voxel(int x, int y, int z) const -> voxel;

    [[nodiscard]]
    auto is_empty(int x, int y, int z) const -> bool;

    [[nodiscard]]
    auto width() const -> int {
        return width_;
    }

    [[nodiscard]]
    auto height() const -> int {
        return height_;
    }

    [[nodiscard]]
    auto depth() const -> int {
        return depth_;
    }

    void fill(const voxel& voxel);

    void clear();

    [[nodiscard]]
    auto get_hash() const -> model_hash;

private:
    int width_{0}, height_{0}, depth_{0};

    std::vector<voxel> voxels_;

    mutable model_hash hash_;
    mutable bool hash_dirty_ = true;

    [[nodiscard]]
    auto index(int x, int y, int z) const -> int;

    void mark_hash_dirty();
    
    [[nodiscard]]
    auto calculate_hash() const -> model_hash;
};

}  // namespace vw::gfx

#include "vw/gfx/model/model.inl.h"

#endif  // VW_GFX_MODEL_MODEL_H
