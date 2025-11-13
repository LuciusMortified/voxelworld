#pragma once

#ifndef VW_GFX_MODEL_MODEL_INL_H
#define VW_GFX_MODEL_MODEL_INL_H

#include "vw/gfx/model/model.h"

#include <algorithm>
#include <functional>

namespace vw::gfx {

inline model::model(int width, int height, int depth)
    : width_(width), height_(height), depth_(depth) {
    voxels_.resize(
        static_cast<size_t>(width_) * static_cast<size_t>(height_) * static_cast<size_t>(depth_)
    );
    mark_hash_dirty();
}

inline void model::set_voxel(int x, int y, int z, const voxel& voxel) {
    voxels_[index(x, y, z)] = voxel;
    mark_hash_dirty();
}

inline auto model::get_voxel(int x, int y, int z) const -> voxel {
    return voxels_[index(x, y, z)];
}

inline auto model::is_empty(int x, int y, int z) const -> bool {
    if (x < 0 || x >= width_ || y < 0 || y >= height_ || z < 0 || z >= depth_) {
        return true;
    }
    return voxels_[index(x, y, z)].is_empty();
}

inline void model::clear() {
    std::ranges::fill(voxels_, voxel{});
    mark_hash_dirty();
}

inline void model::fill(const voxel& voxel) {
    std::fill(voxels_.begin(), voxels_.end(), voxel);
    mark_hash_dirty();
}

inline auto model::get_hash() const -> model_hash {
    if (hash_dirty_) {
        hash_       = calculate_hash();
        hash_dirty_ = false;
    }
    return hash_;
}

inline auto model::index(int x, int y, int z) const -> int {
    return x + (y * width_) + (z * width_ * height_);
}

inline void model::mark_hash_dirty() {
    hash_dirty_ = true;
}

inline auto model::calculate_hash() const -> model_hash {
    // Используем простой хеш для voxels_
    uint64 hash_value = 0;
    for (const auto& vox : voxels_) {
        hash_value ^= static_cast<uint64>(vox.value.value) + 0x9e3779b9 + (hash_value << 6) +
            (hash_value >> 2);
    }
    return model_hash(hash_value);
}

}  // namespace vw::gfx

#endif  // VW_GFX_MODEL_MODEL_INL_H
