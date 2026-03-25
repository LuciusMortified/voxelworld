#pragma once

#ifndef VW_GFX_MODEL_MODEL_H
#define VW_GFX_MODEL_MODEL_H

#include <array>
#include <memory>
#include <vector>

#include "vw/core.h"
#include "vw/core/color.h"
#include "vw/core/voxel.h"
#include "vw/gfx/model/model_identity.h"
#include "vw/gfx/model/page_pool.h"

namespace vw::gfx::detail {
struct boundary_slice {
    std::vector<bool> solid;
    bool valid = false;
};
}  // namespace vw::gfx::detail

namespace vw::gfx {

class model_identity_pool;

enum class page_mode : uint8 { empty = 0, uniform = 1, sparse = 2 };

struct page_entry {
    uint32 data = 0;

    [[nodiscard]] auto mode() const -> page_mode;
    [[nodiscard]] auto fill_id() const -> block_id;
    [[nodiscard]] auto pool_index() const -> uint32;

    static auto make_empty() -> page_entry;
    static auto make_uniform(block_id id) -> page_entry;
    static auto make_sparse(uint32 index) -> page_entry;
};

class model {
public:
    model(model_identity_pool& identity_pool, page_pool& pool, int width, int height, int depth,
          int32 voxel_scale = 1);
    ~model();

    model(const model&)            = delete;
    auto operator=(const model&) -> model& = delete;

    model(model&& other) noexcept;
    auto operator=(model&& other) noexcept -> model&;

    void set_voxel(int x, int y, int z, const voxel& voxel);
    void set_voxel(vec3i pos, const voxel& voxel);

    [[nodiscard]] auto get_voxel(int x, int y, int z) const -> voxel;
    [[nodiscard]] auto get_voxel(vec3i pos) const -> voxel;

    [[nodiscard]] auto is_empty(int x, int y, int z) const -> bool;
    [[nodiscard]] auto is_empty(vec3i pos) const -> bool;

    [[nodiscard]] auto width() const -> int;
    [[nodiscard]] auto height() const -> int;
    [[nodiscard]] auto depth() const -> int;
    [[nodiscard]] auto size() const -> vec3i;
    [[nodiscard]] auto voxel_scale() const -> int32;

    void compute_own_boundaries();
    void set_boundary_slice(int face_direction, const model& neighbor);
    [[nodiscard]] auto has_boundary_slice(int face_direction) const -> bool;
    [[nodiscard]] auto is_boundary_solid(int face_direction, int x, int y, int z) const -> bool;
    void invalidate();

    void fill(const voxel& voxel);

    [[nodiscard]] auto get_identity() const -> model_identity;

    void clone_pages_from(const model& source);

    static constexpr int page_size = 8;
    static constexpr int page_volume = page_size * page_size * page_size;
    using page_type = std::array<voxel, page_volume>;

    [[nodiscard]] auto get_page_mode(int px, int py, int pz) const -> page_mode;
    [[nodiscard]] auto get_page_fill_id(int px, int py, int pz) const -> block_id;
    [[nodiscard]] auto get_page(int px, int py, int pz) const -> const page_type*;

    [[nodiscard]] auto pages_x() const -> int;
    [[nodiscard]] auto pages_y() const -> int;
    [[nodiscard]] auto pages_z() const -> int;

private:
    model_identity_pool* identity_pool_;
    page_pool* pool_ptr_;
    int width_{0}, height_{0}, depth_{0};
    int32 voxel_scale_{1};
    int pages_x_{0}, pages_y_{0}, pages_z_{0};
    std::vector<page_entry> pages_;
    std::vector<uint32> owned_pages_;
    model_identity identity_;
    std::array<detail::boundary_slice, 6> boundary_slices_;
    std::array<detail::boundary_slice, 6> own_boundaries_;

    [[nodiscard]] auto page_index(int px, int py, int pz) const -> int;
    [[nodiscard]] static auto local_index(int lx, int ly, int lz) -> int;
    auto alloc_sparse_page() -> uint32;
    void free_sparse_page(uint32 index);
    auto promote_to_sparse(int px, int py, int pz) -> page_type&;

    void increment_generation_();
};

}  // namespace vw::gfx

#include "vw/gfx/model/model.inl.h"

#endif  // VW_GFX_MODEL_MODEL_H
