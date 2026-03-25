#pragma once

#ifndef VW_GFX_WORLD_COMPONENTS_H
#define VW_GFX_WORLD_COMPONENTS_H

#include <tuple>

#include "vw/gfx/world/components/animation_player_component.h"
#include "vw/gfx/world/components/animation_target_component.h"
#include "vw/gfx/world/components/hierarchy_component.h"
#include "vw/gfx/world/components/light_component.h"
#include "vw/gfx/world/components/model_component.h"
#include "vw/gfx/world/components/socket_component.h"
#include "vw/gfx/world/components/spatial_component.h"
#include "vw/gfx/world/components/transform_component.h"
#include "vw/gfx/world/components/world_view_component.h"

namespace vw::gfx {

using base_world_components = std::tuple<
    hierarchy_component,
    transform_component,
    model_component,
    spatial_component,
    light_component,
    socket_component,
    animation_player_component,
    animation_target_component,
    world_view_component>;

}

#endif  // VW_GFX_WORLD_COMPONENTS_H
