#pragma once

#ifndef VW_GFX_WORLD_GRID_TERRAIN_GENERATOR_H
#define VW_GFX_WORLD_GRID_TERRAIN_GENERATOR_H

#include <functional>
#include <memory>

#include "vw/core.h"

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

struct terrain_context {
    int32 cx;
    int32 cz;
    std::function<auto(int32 y) -> chunk_data&> create_chunk;
};

class terrain_generator {
public:
    virtual ~terrain_generator() = default;
    virtual void generate(terrain_context& ctx) = 0;
};

}  // namespace vw::gfx

#endif  // VW_GFX_WORLD_GRID_TERRAIN_GENERATOR_H
