#pragma once

#ifndef VW_GFX_WORLD_GRID_GENERATOR_H
#define VW_GFX_WORLD_GRID_GENERATOR_H

#include <memory>

#include "vw/core.h"
#include "vw/gfx/world_grid/chunk.h"

namespace vw::asset { class model; }

namespace vw::gfx {


struct chunk_data {
    vec3i coord;
    std::shared_ptr<vw::asset::model> chunk_model;
};

struct chunk_y_range {
    int32 min_y = 0;
    int32 max_y = 0;
};

class world_grid_generator {
public:
    virtual ~world_grid_generator() = default;

    virtual auto generate_chunk(vec3i coord) -> chunk_data = 0;
    [[nodiscard]] virtual auto get_chunk_y_range(int32 chunk_x, int32 chunk_z) -> chunk_y_range = 0;
};

}  // namespace vw::gfx

#endif  // VW_GFX_WORLD_GRID_GENERATOR_H
