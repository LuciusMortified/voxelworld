#pragma once

#ifndef VW_ECS_SYSTEMS_WORLD_GRID_GENERATORS_FLAT_TERRAIN_GENERATOR_INL_H
#define VW_ECS_SYSTEMS_WORLD_GRID_GENERATORS_FLAT_TERRAIN_GENERATOR_INL_H

#include "vw/core/color.h"
#include "vw/asset/model/model.h"
#include "vw/ecs/systems/world_grid/chunk.h"

namespace vw::ecs {


inline flat_terrain_generator::flat_terrain_generator(
    vw::asset::model_identity_pool& identity_pool, vw::asset::page_pool& pool, int32 height, int32 voxel_scale
)
    : identity_pool_(&identity_pool), page_pool_(&pool), height_(height), voxel_scale_(voxel_scale) {}

inline void flat_terrain_generator::generate(
    terrain_context& ctx
) {
    constexpr int32 s = chunk<>::size;

    auto mdl = std::make_shared<vw::asset::model>(*identity_pool_, *page_pool_, s, s, s, voxel_scale_);

    bool checker = ((ctx.cx + ctx.cz) & 1) == 0;
    auto grass_id = checker ? blocks::grass_1 : blocks::grass_2;

    for (int32 x = 0; x < s; ++x) {
        for (int32 z = 0; z < s; ++z) {
            for (int32 y = 0; y < height_; ++y) {
                if (y == height_ - 1) {
                    mdl->set_voxel(x, y, z, voxel{grass_id});
                } else {
                    mdl->set_voxel(x, y, z, voxel{blocks::dirt_2});
                }
            }
        }
    }

    ctx.create_chunk(0) = {vec3i{ctx.cx, 0, ctx.cz}, std::move(mdl)};
}

}  // namespace vw::ecs

#endif  // VW_ECS_SYSTEMS_WORLD_GRID_GENERATORS_FLAT_TERRAIN_GENERATOR_INL_H
