#pragma once

#ifndef VW_GFX_WORLD_GRID_GENERATORS_FLAT_TERRAIN_GENERATOR_H
#define VW_GFX_WORLD_GRID_GENERATORS_FLAT_TERRAIN_GENERATOR_H

#include "vw/gfx/world_grid/terrain_generator.h"

namespace vw::asset {
class model_identity_pool;
class page_pool;
}  // namespace vw::asset

namespace vw::gfx {


class flat_terrain_generator final : public terrain_generator {
public:
    flat_terrain_generator(vw::asset::model_identity_pool& identity_pool, vw::asset::page_pool& pool,
                           int32 height = 4, int32 voxel_scale = 8);

    void generate(terrain_context& ctx) override;

private:
    vw::asset::model_identity_pool* identity_pool_;
    vw::asset::page_pool* page_pool_;
    int32 height_;
    int32 voxel_scale_;
};

}  // namespace vw::gfx

#include "vw/gfx/world_grid/generators/flat_terrain_generator.inl.h"

#endif  // VW_GFX_WORLD_GRID_GENERATORS_FLAT_TERRAIN_GENERATOR_H
