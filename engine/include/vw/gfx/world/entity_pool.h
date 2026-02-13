#pragma once

#ifndef VW_GFX_ENTITY_MANAGER_H
#define VW_GFX_ENTITY_MANAGER_H

#include <vector>

#include "vw/gfx/world/entity.h"

namespace vw::gfx {
class entity_pool final {
public:
    static constexpr size_t default_capacity = 1024;

    explicit entity_pool(size_t capacity = default_capacity) {
        generations_.reserve(capacity);
    }

    [[nodiscard]]
    entity create() {
        if (!free_indices_.empty()) [[unlikely]] {
            uint32 index = free_indices_.back();
            free_indices_.pop_back();
            return {index, generations_[index]};
        }

        uint32 index = generations_.size();
        generations_.push_back(0);
        return {index, 0};
    }

    [[nodiscard]]
    auto batch_create(uint32 count) -> std::vector<entity> {
        std::vector<entity> result;
        result.reserve(count);
        for (uint32 i = 0; i < count; ++i) {
            result.push_back(create());
        }
        return result;
    }

    [[nodiscard]]
    bool has(entity e) const {
        return e.index != entity::invalid_index && e.index < generations_.size() &&
            generations_[e.index] == e.generation;
    }

    void destroy(entity e) {
        if (has(e)) [[likely]] {
            ++generations_[e.index];
            free_indices_.push_back(e.index);
        }
    }

    void batch_destroy(const std::vector<entity>& entities) {
        for (auto e : entities) {
            destroy(e);
        }
    }

private:
    std::vector<uint32> generations_;
    std::vector<uint32> free_indices_;
};
}  // namespace vw::gfx

#endif  // VW_GFX_ENTITY_MANAGER_H
