#pragma once

#ifndef VW_GFX_MODEL_MODEL_INL_H
#define VW_GFX_MODEL_MODEL_INL_H

#include <algorithm>

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
    identity_pool_->destroy(identity_);
}

inline auto model::operator[](
    vec3i pos
) -> voxel& {
    return voxels_[index_at(pos)];
}

inline auto model::operator[](
    vec3i pos
) const -> const voxel& {
    return voxels_[index_at(pos)];
}

inline auto model::operator[](
    int x, int y, int z
) -> voxel& {
    return voxels_[index_at(x, y, z)];
}

inline auto model::operator[](
    int x, int y, int z
) const -> const voxel& {
    return voxels_[index_at(x, y, z)];
}

inline void model::set_voxel(
    int x, int y, int z, const voxel& voxel
) {
    voxels_[index_at(x, y, z)] = voxel;
    increment_generation_();
}

inline void model::set_voxel(
    int x, int y, int z, color color
) {
    voxels_[index_at(x, y, z)] = voxel{color};
    increment_generation_();
}

inline void model::set_voxel(
    vec3i pos, const voxel& voxel
) {
    voxels_[index_at(pos)] = voxel;
    increment_generation_();
}

inline void model::set_voxel(
    vec3i pos, color color
) {
    voxels_[index_at(pos)] = voxel{color};
    increment_generation_();
}

inline auto model::get_voxel(
    int x, int y, int z
) const -> voxel {
    return voxels_[index_at(x, y, z)];
}

inline auto model::get_voxel(
    vec3i pos
) const -> voxel {
    return voxels_[index_at(pos)];
}

inline auto model::is_empty(
    int x, int y, int z
) const -> bool {
    return voxels_[index_at(x, y, z)].is_empty();
}

inline auto model::is_empty(
    vec3i pos
) const -> bool {
    return is_empty(pos.x, pos.y, pos.z);
}

inline auto model::width() const -> int {
    return width_;
}

inline auto model::height() const -> int {
    return height_;
}

inline auto model::depth() const -> int {
    return depth_;
}

inline auto model::size() const -> vec3i {
    return vec3i{width_, height_, depth_};
}

inline void model::fill(
    const voxel& voxel
) {
    std::ranges::fill(voxels_, voxel);
    increment_generation_();
}

inline auto model::get_identity() const -> model_identity {
    return identity_;
}

inline auto model::index_at(
    int x, int y, int z
) const -> int {
    return x + (y * width_) + (z * width_ * height_);
}

inline auto model::index_at(
    vec3i pos
) const -> int {
    return index_at(pos.x, pos.y, pos.z);
}

inline void model::increment_generation_() {
    identity_ = identity_pool_->next_generation(identity_);
}

inline void model::set_voxels(
    const std::vector<voxel>& voxels
) {
    if (voxels.size() != voxels_.size()) {
        throw std::runtime_error("Voxels container size does not match model size.");
    }
    voxels_ = voxels;
    increment_generation_();
}

inline auto model::get_voxels() const -> const std::vector<voxel>& {
    return voxels_;
}

}  // namespace vw::gfx

#endif  // VW_GFX_MODEL_MODEL_INL_H
