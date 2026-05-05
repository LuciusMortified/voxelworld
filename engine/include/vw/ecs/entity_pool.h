#pragma once

#ifndef VW_ECS_ENTITY_MANAGER_H
#define VW_ECS_ENTITY_MANAGER_H

#include <unordered_set>
#include <vector>

#include "vw/ecs/entity.h"

namespace vw::ecs {
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

    [[nodiscard]]
    auto alive_entities() const -> std::vector<entity> {
        std::unordered_set<uint32> free_set(free_indices_.begin(), free_indices_.end());
        std::vector<entity> result;
        result.reserve(generations_.size() - free_indices_.size());
        for (uint32 i = 0; i < generations_.size(); ++i) {
            if (!free_set.contains(i)) {
                result.push_back({i, generations_[i]});
            }
        }
        return result;
    }

private:
    std::vector<uint32> generations_;
    std::vector<uint32> free_indices_;
};
}  // namespace vw::ecs

#endif  // VW_ECS_ENTITY_MANAGER_H
