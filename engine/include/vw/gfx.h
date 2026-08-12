#pragma once

#ifndef VW_GFX_H
#define VW_GFX_H

#include "vw/log.h"

#include "vw/gfx/camera/camera.h"
#include "vw/gfx/camera/free_camera_controller.h"
#include "vw/gfx/camera/third_person_camera_controller.h"
#include "vw/gfx/debug/debug_primitive.h"
#include "vw/gfx/debug/debug_window.h"
#include "vw/gfx/engine/app.h"
#include "vw/gfx/engine/engine.h"
#include "vw/gfx/player/player_input_controller.h"
#include "vw/gfx/player/player_input_state.h"
#include "vw/gfx/render/renderer.h"
#include "vw/gfx/render/vulkan_context.h"
#include "vw/gfx/resource/buffer.h"
#include "vw/gfx/resource/mesh.h"
#include "vw/gfx/resource/mesh_pool.h"
#include "vw/gfx/resource/shader.h"
#include "vw/platform/event.h"
#include "vw/platform/input.h"
#include "vw/platform/window.h"

#endif  //  VW_GFX_H
