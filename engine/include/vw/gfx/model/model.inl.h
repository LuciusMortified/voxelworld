#pragma once

#ifndef VW_GFX_MODEL_MODEL_INL_H
#define VW_GFX_MODEL_MODEL_INL_H

#include <algorithm>

#include "vw/gfx/model/model_identity_pool.h"

namespace vw::gfx {

// ==================== page_entry ====================

inline auto page_entry::mode() const -> page_mode {
    return static_cast<page_mode>(data & 0x3u);
}

inline auto page_entry::fill_id() const -> uint8 {
    return static_cast<uint8>((data >> 2) & 0xFFu);
}

inline auto page_entry::pool_index() const -> uint16 {
    return static_cast<uint16>((data >> 10) & 0xFFFFu);
}

inline auto page_entry::make_empty() -> page_entry { return {0}; }

inline auto page_entry::make_uniform(uint8 id) -> page_entry {
    return {1u | (static_cast<uint32>(id) << 2)};
}

inline auto page_entry::make_sparse(uint16 index) -> page_entry {
    return {2u | (static_cast<uint32>(index) << 10)};
}

// ==================== model ====================

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
      page_pool_(std::move(other.page_pool_)),
      free_pool_indices_(std::move(other.free_pool_indices_)),
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
        page_pool_ = std::move(other.page_pool_);
        free_pool_indices_ = std::move(other.free_pool_indices_);
        identity_ = other.identity_;
        other.identity_pool_ = nullptr;
    }
    return *this;
}

inline void model::set_voxel(
    int x, int y, int z, const voxel& v
) {
    int px = x / page_size;
    int py = y / page_size;
    int pz = z / page_size;
    int li = local_index(x % page_size, y % page_size, z % page_size);
    auto& entry = pages_[page_index(px, py, pz)];

    switch (entry.mode()) {
        case page_mode::empty:
            if (v.is_empty()) return;
            promote_to_sparse(px, py, pz)[li] = v;
            break;
        case page_mode::uniform:
            if (v.id == entry.fill_id()) return;
            promote_to_sparse(px, py, pz)[li] = v;
            break;
        case page_mode::sparse:
            page_pool_[entry.pool_index()][li] = v;
            break;
    }
    increment_generation_();
}

inline void model::set_voxel(
    vec3i pos, const voxel& v
) {
    set_voxel(pos.x, pos.y, pos.z, v);
}

inline auto model::get_voxel(
    int x, int y, int z
) const -> voxel {
    int px = x / page_size;
    int py = y / page_size;
    int pz = z / page_size;
    const auto& entry = pages_[page_index(px, py, pz)];

    switch (entry.mode()) {
        case page_mode::empty:   return voxel{};
        case page_mode::uniform: return voxel{entry.fill_id()};
        case page_mode::sparse:
            return page_pool_[entry.pool_index()]
                [local_index(x % page_size, y % page_size, z % page_size)];
    }
    return voxel{};
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
    const auto& entry = pages_[page_index(px, py, pz)];

    switch (entry.mode()) {
        case page_mode::empty:   return true;
        case page_mode::uniform: return false;
        case page_mode::sparse:
            return page_pool_[entry.pool_index()]
                [local_index(x % page_size, y % page_size, z % page_size)].is_empty();
    }
    return true;
}

inline auto model::is_empty(
    vec3i pos
) const -> bool {
    return is_empty(pos.x, pos.y, pos.z);
}

inline auto model::width() const -> int { return width_; }
inline auto model::height() const -> int { return height_; }
inline auto model::depth() const -> int { return depth_; }
inline auto model::size() const -> vec3i { return vec3i{width_, height_, depth_}; }
inline auto model::voxel_scale() const -> int32 { return voxel_scale_; }

inline void model::compute_own_boundaries() {
    constexpr int ps = page_size;

    auto fill_yz = [&](detail::boundary_slice& slice, int nx) {
        slice.solid.assign(height_ * depth_, false);
        slice.valid = true;
        int px = nx / ps;
        int lx = nx % ps;
        for (int py = 0; py < pages_y_; ++py) {
            for (int pz = 0; pz < pages_z_; ++pz) {
                auto m = get_page_mode(px, py, pz);
                int y0 = py * ps, z0 = pz * ps;
                if (m == page_mode::empty) continue;
                if (m == page_mode::uniform) {
                    for (int ly = 0; ly < ps && y0 + ly < height_; ++ly)
                        for (int lz = 0; lz < ps && z0 + lz < depth_; ++lz)
                            slice.solid[(y0 + ly) * depth_ + (z0 + lz)] = true;
                    continue;
                }
                auto* page = get_page(px, py, pz);
                for (int ly = 0; ly < ps && y0 + ly < height_; ++ly) {
                    for (int lz = 0; lz < ps && z0 + lz < depth_; ++lz) {
                        if (!(*page)[lx + ly * ps + lz * ps * ps].is_empty()) {
                            slice.solid[(y0 + ly) * depth_ + (z0 + lz)] = true;
                        }
                    }
                }
            }
        }
    };

    auto fill_xz = [&](detail::boundary_slice& slice, int ny) {
        slice.solid.assign(width_ * depth_, false);
        slice.valid = true;
        int py = ny / ps;
        int ly = ny % ps;
        for (int px = 0; px < pages_x_; ++px) {
            for (int pz = 0; pz < pages_z_; ++pz) {
                auto m = get_page_mode(px, py, pz);
                int x0 = px * ps, z0 = pz * ps;
                if (m == page_mode::empty) continue;
                if (m == page_mode::uniform) {
                    for (int lx = 0; lx < ps && x0 + lx < width_; ++lx)
                        for (int lz = 0; lz < ps && z0 + lz < depth_; ++lz)
                            slice.solid[(x0 + lx) * depth_ + (z0 + lz)] = true;
                    continue;
                }
                auto* page = get_page(px, py, pz);
                for (int lx = 0; lx < ps && x0 + lx < width_; ++lx) {
                    for (int lz = 0; lz < ps && z0 + lz < depth_; ++lz) {
                        if (!(*page)[lx + ly * ps + lz * ps * ps].is_empty()) {
                            slice.solid[(x0 + lx) * depth_ + (z0 + lz)] = true;
                        }
                    }
                }
            }
        }
    };

    auto fill_xy = [&](detail::boundary_slice& slice, int nz) {
        slice.solid.assign(width_ * height_, false);
        slice.valid = true;
        int pz = nz / ps;
        int lz = nz % ps;
        for (int px = 0; px < pages_x_; ++px) {
            for (int py = 0; py < pages_y_; ++py) {
                auto m = get_page_mode(px, py, pz);
                int x0 = px * ps, y0 = py * ps;
                if (m == page_mode::empty) continue;
                if (m == page_mode::uniform) {
                    for (int lx = 0; lx < ps && x0 + lx < width_; ++lx)
                        for (int ly = 0; ly < ps && y0 + ly < height_; ++ly)
                            slice.solid[(x0 + lx) * height_ + (y0 + ly)] = true;
                    continue;
                }
                auto* page = get_page(px, py, pz);
                for (int lx = 0; lx < ps && x0 + lx < width_; ++lx) {
                    for (int ly = 0; ly < ps && y0 + ly < height_; ++ly) {
                        if (!(*page)[lx + ly * ps + lz * ps * ps].is_empty()) {
                            slice.solid[(x0 + lx) * height_ + (y0 + ly)] = true;
                        }
                    }
                }
            }
        }
    };

    fill_yz(own_boundaries_[0], 0);
    fill_yz(own_boundaries_[1], width_ - 1);
    fill_xz(own_boundaries_[2], 0);
    fill_xz(own_boundaries_[3], height_ - 1);
    fill_xy(own_boundaries_[4], 0);
    fill_xy(own_boundaries_[5], depth_ - 1);
}

inline void model::set_boundary_slice(int face_direction, const model& neighbor) {
    auto& src = neighbor.own_boundaries_[face_direction];
    if (!src.valid) return;
    auto& dst = boundary_slices_[face_direction];
    dst.solid = src.solid;
    dst.valid = true;
}

inline auto model::has_boundary_slice(int face_direction) const -> bool {
    return boundary_slices_[face_direction].valid;
}

inline auto model::is_boundary_solid(int face_direction, int x, int y, int z) const -> bool {
    const auto& slice = boundary_slices_[face_direction];
    int idx;
    switch (face_direction / 2) {
        case 0: idx = y * depth_ + z; break;
        case 1: idx = x * depth_ + z; break;
        default: idx = x * height_ + y; break;
    }
    return slice.solid[idx];
}

inline void model::invalidate() {
    increment_generation_();
}

inline void model::fill(
    const voxel& v
) {
    page_pool_.clear();
    free_pool_indices_.clear();

    if (v.is_empty()) {
        std::fill(pages_.begin(), pages_.end(), page_entry::make_empty());
    } else {
        std::fill(pages_.begin(), pages_.end(), page_entry::make_uniform(v.id));
    }
    increment_generation_();
}

inline auto model::get_identity() const -> model_identity {
    return identity_;
}

inline auto model::get_page_mode(
    int px, int py, int pz
) const -> page_mode {
    return pages_[page_index(px, py, pz)].mode();
}

inline auto model::get_page_fill_id(
    int px, int py, int pz
) const -> uint8 {
    return pages_[page_index(px, py, pz)].fill_id();
}

inline auto model::get_page(
    int px, int py, int pz
) const -> const page_type* {
    const auto& entry = pages_[page_index(px, py, pz)];
    if (entry.mode() == page_mode::sparse)
        return &page_pool_[entry.pool_index()];
    return nullptr;
}

inline auto model::pages_x() const -> int { return pages_x_; }
inline auto model::pages_y() const -> int { return pages_y_; }
inline auto model::pages_z() const -> int { return pages_z_; }

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

inline auto model::alloc_sparse_page() -> uint16 {
    if (!free_pool_indices_.empty()) {
        uint16 idx = free_pool_indices_.back();
        free_pool_indices_.pop_back();
        return idx;
    }
    auto idx = static_cast<uint16>(page_pool_.size());
    page_pool_.emplace_back();
    return idx;
}

inline void model::free_sparse_page(uint16 index) {
    free_pool_indices_.push_back(index);
}

inline auto model::promote_to_sparse(
    int px, int py, int pz
) -> page_type& {
    auto& entry = pages_[page_index(px, py, pz)];
    uint16 idx = alloc_sparse_page();
    auto& page = page_pool_[idx];
    if (entry.mode() == page_mode::uniform) {
        page.fill(voxel{entry.fill_id()});
    } else {
        page.fill(voxel{});
    }
    entry = page_entry::make_sparse(idx);
    return page;
}

inline void model::increment_generation_() {
    identity_ = identity_pool_->next_generation(identity_);
}

inline void model::clone_pages_from(
    const model& source
) {
    pages_ = source.pages_;
    page_pool_ = source.page_pool_;
    free_pool_indices_ = source.free_pool_indices_;
    increment_generation_();
}

}  // namespace vw::gfx

#endif  // VW_GFX_MODEL_MODEL_INL_H