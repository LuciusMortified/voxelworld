#pragma once

#ifndef VW_GFX_WORLD_GRID_GENERATORS_FLAT_WORLD_GRID_GENERATOR_INL_H
#define VW_GFX_WORLD_GRID_GENERATORS_FLAT_WORLD_GRID_GENERATOR_INL_H

#include "vw/core/color.h"
#include "vw/gfx/model/model.h"

namespace vw::gfx {

inline flat_world_grid_generator::flat_world_grid_generator(
    int32 height, int32 voxel_scale
)
    : height_(height), voxel_scale_(voxel_scale) {}

inline auto flat_world_grid_generator::generate_chunk(
    vec3i coord
) -> chunk_data {
    constexpr int32 s = chunk<>::size;

    auto mdl = std::make_shared<model>(*pool_, *page_pool_, s, s, s, voxel_scale_);

    bool checker = ((coord.x + coord.z) & 1) == 0;
    auto grass_id = checker ? static_cast<uint8>(block_id::grass_1)
                            : static_cast<uint8>(block_id::grass_2);

    for (int32 x = 0; x < s; ++x) {
        for (int32 z = 0; z < s; ++z) {
            for (int32 y = 0; y < height_; ++y) {
                if (y == height_ - 1) {
                    mdl->set_voxel(x, y, z, voxel{grass_id});
                } else {
                    mdl->set_voxel(x, y, z, voxel{static_cast<uint8>(block_id::dirt_2)});
                }
            }
        }
    }

    return {coord, std::move(mdl)};
}

}  // namespace vw::gfx

#endif  // VW_GFX_WORLD_GRID_GENERATORS_FLAT_WORLD_GRID_GENERATOR_INL_H
