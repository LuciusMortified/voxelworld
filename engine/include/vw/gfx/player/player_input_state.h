#pragma once

#ifndef VW_GFX_PLAYER_PLAYER_ACTIONS_H
#define VW_GFX_PLAYER_PLAYER_ACTIONS_H

#include "vw/core.h"

namespace vw::gfx {

struct player_input_state {
    vec2f move_input{0.0f, 0.0f};
    vec2f look_delta{0.0f, 0.0f};
    float32 zoom_delta    = 0.0f;
    bool jump_requested   = false;
    bool attack_requested = false;
    bool sprint           = false;
};

}  // namespace vw::gfx

#endif  // VW_GFX_PLAYER_PLAYER_ACTIONS_H
