#pragma once

#ifndef VW_CORE_TIMING_H
#define VW_CORE_TIMING_H

#include <chrono>

#include "vw/core/types.h"

namespace vw {

template <typename F>
auto measure_ms(F&& fn) -> float32 {
    using clock = std::chrono::high_resolution_clock;
    const auto start = clock::now();
    fn();
    return std::chrono::duration<float32>(clock::now() - start).count() * 1000.0f;
}

}  // namespace vw

#endif  // VW_CORE_TIMING_H