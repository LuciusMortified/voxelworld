#pragma once

#ifndef VW_ECS_BASE_WORLD_DEF_H
#define VW_ECS_BASE_WORLD_DEF_H

#include "vw/ecs/systems/animation_fsm_system.h"
#include "vw/ecs/systems/animation_system.h"
#include "vw/ecs/systems/character_controller_system.h"
#include "vw/ecs/systems/hierarchy_system.h"
#include "vw/ecs/systems/light_system.h"
#include "vw/ecs/systems/model_system.h"
#include "vw/ecs/systems/physics_system.h"
#include "vw/ecs/systems/socket_system.h"
#include "vw/ecs/systems/spatial_system.h"
#include "vw/ecs/systems/transform_system.h"
#include "vw/ecs/systems/world_grid_system.h"
#include "vw/ecs/world_def.h"

namespace vw::ecs {

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

}  // namespace vw::ecs

#endif  // VW_ECS_BASE_WORLD_DEF_H
