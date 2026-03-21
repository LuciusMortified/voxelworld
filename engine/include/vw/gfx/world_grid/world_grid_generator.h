#pragma once

#ifndef VW_GFX_WORLD_GRID_GENERATOR_H
#define VW_GFX_WORLD_GRID_GENERATOR_H

#include <memory>

#include "vw/core.h"
#include "vw/gfx/world_grid/chunk.h"

namespace vw::gfx {

class model;

struct chunk_data {
    vec3i coord;
    std::shared_ptr<model> chunk_model;
};

struct chunk_y_range {
    int32 min_y = 0;
    int32 max_y = 0;
};

class model_identity_pool;

class world_grid_generator {
public:
    virtual ~world_grid_generator() = default;

    void set_identity_pool(model_identity_pool& pool) { pool_ = &pool; }

    virtual auto generate_chunk(vec3i coord) -> chunk_data = 0;
    [[nodiscard]] virtual auto get_chunk_y_range(int32 chunk_x, int32 chunk_z) -> chunk_y_range {
        (void)chunk_x;
        (void)chunk_z;
        return {0, 0};
    }

protected:
    model_identity_pool* pool_ = nullptr;
};

}  // namespace vw::gfx

#endif  // VW_GFX_WORLD_GRID_GENERATOR_H
