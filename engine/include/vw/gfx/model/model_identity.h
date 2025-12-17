#pragma once

#ifndef VW_GFX_MODEL_MODEL_IDENTITY_H
#define VW_GFX_MODEL_MODEL_IDENTITY_H

#include <limits>

#include "vw/core/types.h"
#include "vw/gfx/world/entity.h"

namespace vw::gfx {

struct model_identity {
    static constexpr uint32 invalid_index = std::numeric_limits<uint32>::max();

    uint32 index      = invalid_index;
    uint32 generation = 0;

    bool operator==(
        const model_identity& other
    ) const {
        return index == other.index && generation == other.generation;
    }

    bool operator!=(
        const model_identity& other
    ) const {
        return !(*this == other);
    }

    [[nodiscard]]
    bool is_valid() const {
        return index != invalid_index;
    }
};

static constexpr auto invalid_model_identity = model_identity{};

}  // namespace vw::gfx

template <>
struct std::hash<vw::gfx::model_identity> {
    size_t operator()(
        const vw::gfx::model_identity& id
    ) const noexcept {
        size_t x = (size_t{id.generation} << 32) | size_t{id.index};

        // splitmix64 finalizer (хороший миксер)
        x += 0x9e3779b97f4a7c15ull;
        x = (x ^ (x >> 30)) * 0xbf58476d1ce4e5b9ull;
        x = (x ^ (x >> 27)) * 0x94d049bb133111ebull;
        x =  x ^ (x >> 31);

        return x;
    }
};

#endif  // VW_GFX_MODEL_MODEL_IDENTITY_H
