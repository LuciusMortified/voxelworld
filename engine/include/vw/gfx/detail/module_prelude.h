#pragma once

#ifndef VW_GFX_DETAIL_MODULE_PRELUDE_H
#define VW_GFX_DETAIL_MODULE_PRELUDE_H

// Transitional scaffolding, removed in M6 together with this whole include
// tree. See vw/core/detail/module_prelude.h for why the order matters; this
// adds what the vw.gfx partitions carry in their global module fragments.
// No Vulkan header here: the binding reaches vw.gfx as `import vulkan`, which
// the partitions never re-export, so a consumer cannot name a vk:: type at all.
#include "vw/world/detail/module_prelude.h"
#include "vw/platform/detail/module_prelude.h"

#include <future>

#endif  // VW_GFX_DETAIL_MODULE_PRELUDE_H
