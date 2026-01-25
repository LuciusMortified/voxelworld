#pragma once

#ifndef VW_GFX_MODEL_MODEL_H
#define VW_GFX_MODEL_MODEL_H

#include <vector>

#include "vw/core.h"
#include "vw/core/color.h"
#include "vw/core/voxel.h"
#include "vw/gfx/model/model_identity.h"

namespace vw::gfx {

class model_identity_pool;

class model {
public:
    model(model_identity_pool& identity_pool, int width, int height, int depth);
    ~model();

    model(const model&)            = delete;
    model& operator=(const model&) = delete;

    model(model&&)            = default;
    model& operator=(model&&) = default;

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

    void fill(const voxel& voxel);

    [[nodiscard]] auto get_identity() const -> model_identity;

    void set_voxels(const std::vector<voxel>& voxels);

    [[nodiscard]] auto get_voxels() const -> const std::vector<voxel>&;

private:
    model_identity_pool* identity_pool_;
    int width_{0}, height_{0}, depth_{0};
    std::vector<voxel> voxels_;
    model_identity identity_;

    [[nodiscard]] auto index_at(int x, int y, int z) const -> int;
    [[nodiscard]] auto index_at(vec3i pos) const -> int;

    void increment_generation_();
};

}  // namespace vw::gfx

#include "vw/gfx/model/model.inl.h"

#endif  // VW_GFX_MODEL_MODEL_H
