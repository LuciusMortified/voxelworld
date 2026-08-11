#pragma once

#ifndef VW_CORE_DETAIL_MODULE_PRELUDE_H
#define VW_CORE_DETAIL_MODULE_PRELUDE_H

// Transitional scaffolding, removed in M6 together with this whole include tree.
//
// MSVC merges declarations that a module carries in its global module fragment
// with the importer's textual includes only when the textual include comes
// first; otherwise the same entities are declared twice and the TU fails. Every
// shim therefore reproduces the module's global module fragment before
// importing, so the ordering holds no matter which shim a consumer reaches
// first. Keep in sync with the `module;` sections of engine/core/src/*.cppm.
#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <expected>
#include <functional>
#include <limits>
#include <span>
#include <string_view>
#include <vector>

#endif  // VW_CORE_DETAIL_MODULE_PRELUDE_H
