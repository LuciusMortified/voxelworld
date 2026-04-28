#pragma once

#ifndef VW_ECS_WORLD_GRID_GENERATORS_FLAT_WORLD_GRID_GENERATOR_H
#define VW_ECS_WORLD_GRID_GENERATORS_FLAT_WORLD_GRID_GENERATOR_H

#include "vw/ecs/world_grid/world_grid_generator.h"

namespace vw::asset {
class model_identity_pool;
class page_pool;
}  // namespace vw::asset

namespace vw::ecs {


class flat_world_grid_generator final : public world_grid_generator {
public:
    flat_world_grid_generator(vw::asset::model_identity_pool& identity_pool, vw::asset::page_pool& pool,
                              int32 height = 4, int32 voxel_scale = 8);

    [[nodiscard]] auto generate_chunk(vec3i coord) -> chunk_data override;
    [[nodiscard]] auto get_chunk_y_range(int32 chunk_x, int32 chunk_z) -> chunk_y_range override;

private:
    vw::asset::model_identity_pool* identity_pool_;
    vw::asset::page_pool* page_pool_;
    int32 height_;
    int32 voxel_scale_;
};

}  // namespace vw::ecs

#include "vw/ecs/world_grid/generators/flat_world_grid_generator.inl.h"

#endif  // VW_ECS_WORLD_GRID_GENERATORS_FLAT_WORLD_GRID_GENERATOR_H
