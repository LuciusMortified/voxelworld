#pragma once

#ifndef VW_GFX_WORLD_GRID_GENERATORS_FLAT_WORLD_GRID_GENERATOR_H
#define VW_GFX_WORLD_GRID_GENERATORS_FLAT_WORLD_GRID_GENERATOR_H

#include "vw/gfx/world_grid/world_grid_generator.h"

namespace vw::gfx {

class model_identity_pool;
class page_pool;

class flat_world_grid_generator final : public world_grid_generator {
public:
    flat_world_grid_generator(model_identity_pool& identity_pool, page_pool& page_pool,
                              int32 height = 4, int32 voxel_scale = 8);

    [[nodiscard]] auto generate_chunk(vec3i coord) -> chunk_data override;
    [[nodiscard]] auto get_chunk_y_range(int32 chunk_x, int32 chunk_z) -> chunk_y_range override;

private:
    model_identity_pool* identity_pool_;
    page_pool* page_pool_;
    int32 height_;
    int32 voxel_scale_;
};

}  // namespace vw::gfx

#include "vw/gfx/world_grid/generators/flat_world_grid_generator.inl.h"

#endif  // VW_GFX_WORLD_GRID_GENERATORS_FLAT_WORLD_GRID_GENERATOR_H
