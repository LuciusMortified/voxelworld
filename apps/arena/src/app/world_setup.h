#pragma once

#ifndef VW_ARENA_WORLD_SETUP_H
#define VW_ARENA_WORLD_SETUP_H

#include <vw/gfx.h>

#include "vw/ecs/systems/world_grid/generators/perlin_terrain_generator.h"
#include "vw/ecs/systems/world_grid/world_grid.h"

namespace vw::arena {

struct world_setup_result {
    gfx::perlin_terrain_generator::params generator_params;
};

auto setup_world_grid(gfx::engine<>& engine) -> world_setup_result;

}  // namespace vw::arena

#include "world_setup.inl.h"

#endif  // VW_ARENA_WORLD_SETUP_H
