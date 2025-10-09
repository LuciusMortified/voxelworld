#pragma once

#ifndef VW_GFX_ENTITY_H
#define VW_GFX_ENTITY_H

#include <limits>

#include "vw/core.h"

namespace vw::gfx {
struct entity final {
    static constexpr uint32 invalid_index = std::numeric_limits<uint32>::max();

    uint32 index;
    uint32 generation;

    entity() : index(invalid_index), generation(0) {}
    entity(uint32 index, uint32 generation) : index(index), generation(generation) {}

    [[nodiscard]]
    bool operator==(const entity& rhs) const {
        return index == rhs.index && generation == rhs.generation;
    }

    [[nodiscard]]
    bool operator!=(const entity& rhs) const {
        return !(*this == rhs);
    }
};
}  // namespace vw::gfx

#endif  // VW_GFX_ENTITY_H
