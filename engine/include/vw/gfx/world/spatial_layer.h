#pragma once

#ifndef VW_GFX_WORLD_SPATIAL_LAYER_H
#define VW_GFX_WORLD_SPATIAL_LAYER_H

#include "vw/core/types.h"

namespace vw::gfx {

using spatial_layer_mask = uint16;

namespace spatial_layer {

inline constexpr spatial_layer_mask none       = 0;
inline constexpr spatial_layer_mask terrain    = 1 << 0;
inline constexpr spatial_layer_mask character  = 1 << 1;
inline constexpr spatial_layer_mask prop       = 1 << 2;
inline constexpr spatial_layer_mask trigger    = 1 << 3;
inline constexpr spatial_layer_mask projectile = 1 << 4;
inline constexpr spatial_layer_mask all        = 0xFFFF;

}  // namespace spatial_layer

}  // namespace vw::gfx

#endif  // VW_GFX_WORLD_SPATIAL_LAYER_H
