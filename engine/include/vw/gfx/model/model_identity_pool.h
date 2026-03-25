#pragma once

#ifndef VW_GFX_MODEL_MODEL_IDENTITY_POOL_H
#define VW_GFX_MODEL_MODEL_IDENTITY_POOL_H

#include <mutex>
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

    [[nodiscard]] auto create() -> model_identity {
        std::scoped_lock lock(mutex_);
        if (!free_indices_.empty()) [[unlikely]] {
            uint32 index = free_indices_.back();
            free_indices_.pop_back();
            return {.index = index, .generation = generations_[index]};
        }

        const auto index = static_cast<uint32>(generations_.size());
        generations_.push_back(0);
        return {.index = index, .generation = 0};
    }

    [[nodiscard]] auto next_generation(
        model_identity id
    ) -> model_identity {
        std::scoped_lock lock(mutex_);
        if (has_unlocked_(id)) [[likely]] {
            return {.index = id.index, .generation = ++generations_[id.index]};
        }
        return invalid_model_identity;
    }

    [[nodiscard]] auto has(
        model_identity id
    ) const -> bool {
        std::scoped_lock lock(mutex_);
        return has_unlocked_(id);
    }

    void destroy(
        model_identity id
    ) {
        std::scoped_lock lock(mutex_);
        if (has_unlocked_(id)) [[likely]] {
            ++generations_[id.index];
            free_indices_.push_back(id.index);
        }
    }

private:
    [[nodiscard]] auto has_unlocked_(
        model_identity id
    ) const -> bool {
        return                                            //
            id.index != model_identity::invalid_index &&  //
            id.index < generations_.size() &&             //
            generations_[id.index] == id.generation;
    }

    mutable std::mutex mutex_;
    std::vector<uint32> generations_;
    std::vector<uint32> free_indices_;
};

}  // namespace vw::gfx

#endif  // VW_GFX_MODEL_MODEL_IDENTITY_POOL_H
