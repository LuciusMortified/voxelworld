#pragma once

#ifndef VW_GFX_MODEL_MODEL_INL_H
#define VW_GFX_MODEL_MODEL_INL_H

#include <algorithm>

#include "vw/gfx/model/model.h"
#include "vw/gfx/model/model_identity_pool.h"

namespace vw::gfx {

inline model::model(
    model_identity_pool& identity_pool, int width, int height, int depth
)
    : identity_pool_(&identity_pool), width_(width), height_(height), depth_(depth) {
    voxels_.resize(
        static_cast<size_t>(width_) * static_cast<size_t>(height_) * static_cast<size_t>(depth_)
    );
    identity_ = identity_pool_->create();
}

inline model::~model() {
    if (identity_pool_ && identity_.is_valid()) {
        identity_pool_->destroy(identity_);
    }
}

inline void model::set_voxel(
    int x, int y, int z, const voxel& voxel
) {
    voxels_[index(x, y, z)] = voxel;
    increment_generation();
}

inline auto model::get_voxel(
    int x, int y, int z
) const -> voxel {
    return voxels_[index(x, y, z)];
}

inline auto model::get_voxel(
    vec3i pos
) const -> voxel {
    return voxels_[index(pos.x, pos.y, pos.z)];
}

inline auto model::is_empty(
    int x, int y, int z
) const -> bool {
    return voxels_[index(x, y, z)].is_empty();
}

inline auto model::is_empty(
    vec3i pos
) const -> bool {
    return is_empty(pos.x, pos.y, pos.z);
}

inline void model::clear() {
    std::ranges::fill(voxels_, voxel{});
    increment_generation();
}

inline void model::fill(
    const voxel& voxel
) {
    std::ranges::fill(voxels_, voxel);
    increment_generation();
}

inline auto model::index(
    int x, int y, int z
) const -> int {
    return x + (y * width_) + (z * width_ * height_);
}

inline void model::increment_generation() {
    if (identity_pool_ && identity_.is_valid()) {
        identity_pool_->destroy(identity_);
        identity_ = identity_pool_->create();
    }
}

inline void model::set_voxels(
    const std::vector<voxel>& voxels
) {
    if (voxels.size() != voxels_.size()) {
        throw std::runtime_error("Voxels container size does not match model size.");
    }
    voxels_ = voxels;
    increment_generation();
}

}  // namespace vw::gfx

#endif  // VW_GFX_MODEL_MODEL_INL_H
