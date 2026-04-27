#pragma once

#ifndef VW_GFX_WORLD_BASE_WORLD_DEF_H
#define VW_GFX_WORLD_BASE_WORLD_DEF_H

#include "vw/gfx/world/systems/animation_fsm_system.h"
#include "vw/gfx/world/systems/animation_system.h"
#include "vw/gfx/world/systems/character_controller_system.h"
#include "vw/gfx/world/systems/hierarchy_system.h"
#include "vw/gfx/world/systems/light_system.h"
#include "vw/gfx/world/systems/model_system.h"
#include "vw/gfx/world/systems/physics_system.h"
#include "vw/gfx/world/systems/socket_system.h"
#include "vw/gfx/world/systems/spatial_system.h"
#include "vw/gfx/world/systems/transform_system.h"
#include "vw/gfx/world/systems/world_grid_system.h"
#include "vw/gfx/world/world_def.h"

namespace vw::gfx {

using base_world_def = world_def<
    hierarchy_system,
    character_controller_system,
    animation_fsm_system,
    physics_system,
    transform_system,
    model_system,
    spatial_system,
    light_system,
    socket_system,
    world_grid_system,
    animation_system>;

}  // namespace vw::gfx

#endif  // VW_GFX_WORLD_BASE_WORLD_DEF_H
