#pragma once

#ifndef VW_GFX_WORLD_GRID_GENERATORS_FLAT_WORLD_GRID_GENERATOR_H
#define VW_GFX_WORLD_GRID_GENERATORS_FLAT_WORLD_GRID_GENERATOR_H

#include "vw/gfx/world_grid/world_grid_generator.h"

namespace vw::gfx {

class flat_world_grid_generator final : public world_grid_generator {
public:
    explicit flat_world_grid_generator(int32 height = 4, int32 voxel_scale = 8);

    [[nodiscard]] auto generate_chunk(vec3i coord) -> chunk_data override;

private:
    int32 height_;
    int32 voxel_scale_;
};

}  // namespace vw::gfx

#include "vw/gfx/world_grid/generators/flat_world_grid_generator.inl.h"

#endif  // VW_GFX_WORLD_GRID_GENERATORS_FLAT_WORLD_GRID_GENERATOR_H
