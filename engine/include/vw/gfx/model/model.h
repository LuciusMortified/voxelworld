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
    model(model&&)                 = delete;
    model& operator=(model&&)      = delete;

    void set_voxel(int x, int y, int z, const voxel& voxel);
    void set_voxel(
        int x, int y, int z, color color
    ) {
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

    [[nodiscard]]
    auto size() const -> vw::vec3i {
        return vw::vec3i{width_, height_, depth_};
    }

    void fill(const voxel& voxel);

    void clear();

    [[nodiscard]]
    auto get_identity() const -> model_identity {
        return identity_;
    }

private:
    model_identity_pool* identity_pool_;
    int width_{0}, height_{0}, depth_{0};
    std::vector<voxel> voxels_;
    model_identity identity_;

    [[nodiscard]]
    auto index(int x, int y, int z) const -> int;

    void increment_generation();
};

}  // namespace vw::gfx

#include "vw/gfx/model/model.inl.h"

#endif  // VW_GFX_MODEL_MODEL_H
