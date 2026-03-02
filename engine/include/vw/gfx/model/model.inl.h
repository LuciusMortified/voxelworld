#pragma once

#ifndef VW_GFX_MODEL_MODEL_INL_H
#define VW_GFX_MODEL_MODEL_INL_H

#include <algorithm>

#include "vw/gfx/model/model_identity_pool.h"

namespace vw::gfx {

inline model::model(
    model_identity_pool& identity_pool, int width, int height, int depth, int32 voxel_scale
)
    : identity_pool_(&identity_pool),
      width_(width),
      height_(height),
      depth_(depth),
      voxel_scale_(voxel_scale),
      pages_x_((width + page_size - 1) / page_size),
      pages_y_((height + page_size - 1) / page_size),
      pages_z_((depth + page_size - 1) / page_size) {
    pages_.resize(
        static_cast<size_t>(pages_x_) * static_cast<size_t>(pages_y_) *
        static_cast<size_t>(pages_z_)
    );
    identity_ = identity_pool_->create();
}

inline model::~model() {
    if (identity_pool_) {
        identity_pool_->destroy(identity_);
    }
}

inline model::model(
    model&& other
) noexcept
    : identity_pool_(other.identity_pool_),
      width_(other.width_),
      height_(other.height_),
      depth_(other.depth_),
      voxel_scale_(other.voxel_scale_),
      pages_x_(other.pages_x_),
      pages_y_(other.pages_y_),
      pages_z_(other.pages_z_),
      pages_(std::move(other.pages_)),
      identity_(other.identity_) {
    other.identity_pool_ = nullptr;
}

inline auto model::operator=(
    model&& other
) noexcept -> model& {
    if (this != &other) {
        if (identity_pool_) {
            identity_pool_->destroy(identity_);
        }
        identity_pool_ = other.identity_pool_;
        width_ = other.width_;
        height_ = other.height_;
        depth_ = other.depth_;
        voxel_scale_ = other.voxel_scale_;
        pages_x_ = other.pages_x_;
        pages_y_ = other.pages_y_;
        pages_z_ = other.pages_z_;
        pages_ = std::move(other.pages_);
        identity_ = other.identity_;
        other.identity_pool_ = nullptr;
    }
    return *this;
}

inline auto model::operator[](
    vec3i pos
) -> voxel& {
    int px = pos.x / page_size;
    int py = pos.y / page_size;
    int pz = pos.z / page_size;
    auto& page = ensure_page(px, py, pz);
    return page[local_index(pos.x % page_size, pos.y % page_size, pos.z % page_size)];
}

inline auto model::operator[](
    vec3i pos
) const -> const voxel& {
    int px = pos.x / page_size;
    int py = pos.y / page_size;
    int pz = pos.z / page_size;
    auto& ptr = pages_[page_index(px, py, pz)];
    if (!ptr) {
        static const voxel empty{};
        return empty;
    }
    return (*ptr)[local_index(pos.x % page_size, pos.y % page_size, pos.z % page_size)];
}

inline auto model::operator[](
    int x, int y, int z
) -> voxel& {
    return (*this)[vec3i{x, y, z}];
}

inline auto model::operator[](
    int x, int y, int z
) const -> const voxel& {
    return (*this)[vec3i{x, y, z}];
}

inline void model::set_voxel(
    int x, int y, int z, const voxel& voxel
) {
    int px = x / page_size;
    int py = y / page_size;
    int pz = z / page_size;
    if (voxel.is_empty()) {
        auto& ptr = pages_[page_index(px, py, pz)];
        if (!ptr) return;
        (*ptr)[local_index(x % page_size, y % page_size, z % page_size)] = voxel;
    } else {
        auto& page = ensure_page(px, py, pz);
        page[local_index(x % page_size, y % page_size, z % page_size)] = voxel;
    }
    increment_generation_();
}

inline void model::set_voxel(
    int x, int y, int z, color color
) {
    set_voxel(x, y, z, voxel{color});
}

inline void model::set_voxel(
    vec3i pos, const voxel& voxel
) {
    set_voxel(pos.x, pos.y, pos.z, voxel);
}

inline void model::set_voxel(
    vec3i pos, color color
) {
    set_voxel(pos.x, pos.y, pos.z, voxel{color});
}

inline auto model::get_voxel(
    int x, int y, int z
) const -> voxel {
    int px = x / page_size;
    int py = y / page_size;
    int pz = z / page_size;
    auto& ptr = pages_[page_index(px, py, pz)];
    if (!ptr) return voxel{};
    return (*ptr)[local_index(x % page_size, y % page_size, z % page_size)];
}

inline auto model::get_voxel(
    vec3i pos
) const -> voxel {
    return get_voxel(pos.x, pos.y, pos.z);
}

inline auto model::is_empty(
    int x, int y, int z
) const -> bool {
    int px = x / page_size;
    int py = y / page_size;
    int pz = z / page_size;
    auto& ptr = pages_[page_index(px, py, pz)];
    if (!ptr) return true;
    return (*ptr)[local_index(x % page_size, y % page_size, z % page_size)].is_empty();
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

inline auto model::voxel_scale() const -> int32 {
    return voxel_scale_;
}

inline void model::fill(
    const voxel& voxel
) {
    if (voxel.is_empty()) {
        for (auto& ptr : pages_) {
            ptr.reset();
        }
    } else {
        for (auto& ptr : pages_) {
            if (!ptr) {
                ptr = std::make_unique<page_type>();
            }
            ptr->fill(voxel);
        }
    }
    increment_generation_();
}

inline auto model::get_identity() const -> model_identity {
    return identity_;
}

inline auto model::is_page_allocated(
    int px, int py, int pz
) const -> bool {
    return pages_[page_index(px, py, pz)] != nullptr;
}

inline auto model::get_page(
    int px, int py, int pz
) const -> const page_type* {
    auto& ptr = pages_[page_index(px, py, pz)];
    return ptr ? ptr.get() : nullptr;
}

inline auto model::pages_x() const -> int {
    return pages_x_;
}

inline auto model::pages_y() const -> int {
    return pages_y_;
}

inline auto model::pages_z() const -> int {
    return pages_z_;
}

inline auto model::page_index(
    int px, int py, int pz
) const -> int {
    return px + (py * pages_x_) + (pz * pages_x_ * pages_y_);
}

inline auto model::local_index(
    int lx, int ly, int lz
) -> int {
    return lx + (ly * page_size) + (lz * page_size * page_size);
}

inline auto model::ensure_page(
    int px, int py, int pz
) -> page_type& {
    auto& ptr = pages_[page_index(px, py, pz)];
    if (!ptr) {
        ptr = std::make_unique<page_type>();
        ptr->fill(voxel{});
    }
    return *ptr;
}

inline void model::increment_generation_() {
    identity_ = identity_pool_->next_generation(identity_);
}

inline void model::clone_pages_from(
    const model& source
) {
    auto count = static_cast<size_t>(pages_x_) * static_cast<size_t>(pages_y_) *
                 static_cast<size_t>(pages_z_);
    for (size_t i = 0; i < count; ++i) {
        if (source.pages_[i]) {
            pages_[i] = std::make_unique<page_type>(*source.pages_[i]);
        } else {
            pages_[i].reset();
        }
    }
    increment_generation_();
}


}  // namespace vw::gfx

#endif  // VW_GFX_MODEL_MODEL_INL_H
