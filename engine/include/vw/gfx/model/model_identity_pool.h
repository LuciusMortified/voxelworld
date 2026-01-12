#pragma once

#ifndef VW_GFX_MODEL_MODEL_IDENTITY_POOL_H
#define VW_GFX_MODEL_MODEL_IDENTITY_POOL_H

#include <vector>

#include "vw/core/types.h"
#include "vw/gfx/model/model_identity.h"

namespace vw::gfx {

class model_identity_pool final {
public:
    static constexpr size_t default_capacity = 1024;

    explicit model_identity_pool(
        size_t capacity = default_capacity
    ) {
        generations_.reserve(capacity);
    }

    [[nodiscard]] model_identity create() {
        if (!free_indices_.empty()) [[unlikely]] {
            uint32 index = free_indices_.back();
            free_indices_.pop_back();
            return {index, generations_[index]};
        }

        uint32 index = static_cast<uint32>(generations_.size());
        generations_.push_back(0);
        return {index, 0};
    }

    [[nodiscard]] model_identity next_generation(
        model_identity id
    ) {
        if (has(id)) [[likely]] {
            return {id.index, ++generations_[id.index]};
        }
        return invalid_model_identity;
    }

    [[nodiscard]] bool has(
        model_identity id
    ) const {
        return                                            //
            id.index != model_identity::invalid_index &&  //
            id.index < generations_.size() &&             //
            generations_[id.index] == id.generation;
    }

    void destroy(
        model_identity id
    ) {
        if (has(id)) [[likely]] {
            ++generations_[id.index];
            free_indices_.push_back(id.index);
        }
    }

private:
    std::vector<uint32> generations_;
    std::vector<uint32> free_indices_;
};

}  // namespace vw::gfx

#endif  // VW_GFX_MODEL_MODEL_IDENTITY_POOL_H
