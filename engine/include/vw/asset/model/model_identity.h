#pragma once

#ifndef VW_ASSET_MODEL_MODEL_IDENTITY_H
#define VW_ASSET_MODEL_MODEL_IDENTITY_H

#include <functional>
#include <limits>

#include "vw/core/types.h"

namespace vw::asset {

struct model_identity {
    static constexpr uint32 invalid_index = std::numeric_limits<uint32>::max();

    uint32 index      = invalid_index;
    uint32 generation = 0;

    auto operator==(
        const model_identity& other
    ) const -> bool {
        return index == other.index && generation == other.generation;
    }

    auto operator!=(
        const model_identity& other
    ) const -> bool {
        return !(*this == other);
    }

    [[nodiscard]]
    auto is_valid() const -> bool {
        return index != invalid_index;
    }
};

static constexpr auto invalid_model_identity = model_identity{};

}  // namespace vw::asset

template <>
struct std::hash<vw::asset::model_identity> {
    auto operator()(
        const vw::asset::model_identity& id
    ) const noexcept -> size_t {
        size_t x = (size_t{id.generation} << 32) | size_t{id.index};

        // splitmix64 finalizer
        x += 0x9e3779b97f4a7c15ull;
        x = (x ^ (x >> 30)) * 0xbf58476d1ce4e5b9ull;
        x = (x ^ (x >> 27)) * 0x94d049bb133111ebull;
        x =  x ^ (x >> 31);

        return x;
    }
};

#endif  // VW_ASSET_MODEL_MODEL_IDENTITY_H
