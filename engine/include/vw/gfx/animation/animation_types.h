#pragma once

#ifndef VW_GFX_ANIMATION_TYPES_H
#define VW_GFX_ANIMATION_TYPES_H

#include "vw/core/types.h"

namespace vw::gfx {

enum class animation_state : uint8 {
    stopped,
    playing,
    paused
};

enum class animation_loop_mode : uint8 {
    once,
    loop,
    ping_pong
};

enum class animation_property : uint8 {
    position,
    rotation,
    scale,
    origin
};

}  // namespace vw::gfx

#endif  // VW_GFX_ANIMATION_TYPES_H
