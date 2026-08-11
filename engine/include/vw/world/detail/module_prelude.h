#pragma once

#ifndef VW_WORLD_DETAIL_MODULE_PRELUDE_H
#define VW_WORLD_DETAIL_MODULE_PRELUDE_H

// Transitional scaffolding, removed in M6 together with this whole include
// tree. See vw/core/detail/module_prelude.h for why the order matters; this
// adds what the vw.world partitions carry in their global module fragments.
#include "vw/ecs/detail/module_prelude.h"

#include <condition_variable>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <optional>
#include <queue>
#include <sstream>
#include <string>
#include <thread>
#include <unordered_map>
#include <variant>

#endif  // VW_WORLD_DETAIL_MODULE_PRELUDE_H
