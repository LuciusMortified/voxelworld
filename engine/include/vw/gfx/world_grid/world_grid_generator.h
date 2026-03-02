#pragma once

#ifndef VW_GFX_WORLD_GRID_GENERATOR_H
#define VW_GFX_WORLD_GRID_GENERATOR_H

#include <memory>
#include <vector>

#include "vw/core.h"
#include "vw/gfx/world_grid/chunk.h"

namespace vw::gfx {

struct region_meta {
    region_id id;
    vec2i aabb_min;
    vec2i aabb_max;
};

class model;

struct chunk_data {
    region_id region;
    vec3i coord;
    std::shared_ptr<model> chunk_model;
};

struct region_gen_result {
    region_meta meta;
    std::vector<chunk_data> chunks;
};

class model_identity_pool;

class world_grid_generator {
public:
    virtual ~world_grid_generator() = default;

    void set_identity_pool(model_identity_pool& pool) { pool_ = &pool; }

    virtual auto get_region_id(int32 cx, int32 cz) const -> region_id = 0;
    virtual auto generate_region(region_id id) -> region_gen_result = 0;

protected:
    model_identity_pool* pool_ = nullptr;
};

class flat_world_grid_generator final : public world_grid_generator {
public:
    explicit flat_world_grid_generator(int32 height = 4, int32 region_size = 16,
                                       int32 voxel_scale = 1);

    auto get_region_id(int32 cx, int32 cz) const -> region_id override;
    auto generate_region(region_id id) -> region_gen_result override;

private:
    int32 height_;
    int32 region_size_;
    int32 voxel_scale_;
};

}  // namespace vw::gfx

#include "vw/gfx/world_grid/world_grid_generator.inl.h"

#endif  // VW_GFX_WORLD_GRID_GENERATOR_H
