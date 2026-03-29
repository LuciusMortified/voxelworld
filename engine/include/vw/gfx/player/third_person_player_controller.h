#pragma once

#ifndef VW_GFX_PLAYER_THIRD_PERSON_PLAYER_CONTROLLER_H
#define VW_GFX_PLAYER_THIRD_PERSON_PLAYER_CONTROLLER_H

#include "vw/core.h"
#include "vw/gfx/camera/camera.h"
#include "vw/gfx/player/player_input_state.h"
#include "vw/gfx/world/world.h"

namespace vw::gfx {

template <typename WC = base_world_components>
class third_person_player_controller {
public:
    using world_type = world<WC>;
    using character_system_type = character_controller_system<WC>;

    explicit third_person_player_controller(
        camera& camera,
        world_type& world
    );

    void update(const player_input_state& input, entity target);

private:
    camera* camera_;
    world_type* world_;
};

}  // namespace vw::gfx

#include "vw/gfx/player/third_person_player_controller.inl.h"

#endif  // VW_GFX_PLAYER_THIRD_PERSON_PLAYER_CONTROLLER_H
