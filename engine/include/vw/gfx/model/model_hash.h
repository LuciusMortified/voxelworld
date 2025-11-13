#pragma once

#ifndef VW_GFX_MODEL_HASH_H
#define VW_GFX_MODEL_HASH_H

#include <functional>

#include "vw/core/types.h"

namespace vw::gfx {

struct model_hash {
    uint64 data;
    
    model_hash() : data(0) {}
    explicit model_hash(uint64 value) : data(value) {}
    
    [[nodiscard]] auto operator==(const model_hash& other) const -> bool {
        return data == other.data;
    }
    
    [[nodiscard]] auto operator!=(const model_hash& other) const -> bool {
        return !(*this == other);
    }
};

}  // namespace vw::gfx

namespace std {
template <>
struct hash<vw::gfx::model_hash> {
    auto operator()(const vw::gfx::model_hash& hash_value) const noexcept -> size_t {
        return static_cast<size_t>(hash_value.data);
    }
};
}  // namespace std

#endif  // VW_GFX_MODEL_HASH_H
