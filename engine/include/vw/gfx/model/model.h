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

namespace vw::gfx {

class model_identity_pool;

class model {
public:
    model(model_identity_pool& identity_pool, int width, int height, int depth,
          int32 voxel_scale = 1);
    ~model();

    model(const model&)            = delete;
    auto operator=(const model&) -> model& = delete;

    model(model&& other) noexcept;
    auto operator=(model&& other) noexcept -> model&;

    [[nodiscard]] auto operator[](vec3i pos) -> voxel&;
    [[nodiscard]] auto operator[](vec3i pos) const -> const voxel&;

    [[nodiscard]] auto operator[](int x, int y, int z) -> voxel&;
    [[nodiscard]] auto operator[](int x, int y, int z) const -> const voxel&;

    void set_voxel(int x, int y, int z, const voxel& voxel);
    void set_voxel(int x, int y, int z, color color);
    void set_voxel(vec3i pos, const voxel& voxel);
    void set_voxel(vec3i pos, color color);

    [[nodiscard]] auto get_voxel(int x, int y, int z) const -> voxel;
    [[nodiscard]] auto get_voxel(vec3i pos) const -> voxel;

    [[nodiscard]] auto is_empty(int x, int y, int z) const -> bool;
    [[nodiscard]] auto is_empty(vec3i pos) const -> bool;

    [[nodiscard]] auto width() const -> int;
    [[nodiscard]] auto height() const -> int;
    [[nodiscard]] auto depth() const -> int;
    [[nodiscard]] auto size() const -> vec3i;
    [[nodiscard]] auto voxel_scale() const -> int32;

    void fill(const voxel& voxel);

    [[nodiscard]] auto get_identity() const -> model_identity;

    void clone_pages_from(const model& source);

    static constexpr int page_size = 8;
    static constexpr int page_volume = page_size * page_size * page_size;
    using page_type = std::array<voxel, page_volume>;

    [[nodiscard]] auto is_page_allocated(int px, int py, int pz) const -> bool;
    [[nodiscard]] auto get_page(int px, int py, int pz) const -> const page_type*;

    [[nodiscard]] auto pages_x() const -> int;
    [[nodiscard]] auto pages_y() const -> int;
    [[nodiscard]] auto pages_z() const -> int;

private:
    model_identity_pool* identity_pool_;
    int width_{0}, height_{0}, depth_{0};
    int32 voxel_scale_{1};
    int pages_x_{0}, pages_y_{0}, pages_z_{0};
    std::vector<std::unique_ptr<page_type>> pages_;
    model_identity identity_;

    [[nodiscard]] auto page_index(int px, int py, int pz) const -> int;
    [[nodiscard]] static auto local_index(int lx, int ly, int lz) -> int;
    [[nodiscard]] auto ensure_page(int px, int py, int pz) -> page_type&;

    void increment_generation_();
};

}  // namespace vw::gfx

#include "vw/gfx/model/model.inl.h"

#endif  // VW_GFX_MODEL_MODEL_H
