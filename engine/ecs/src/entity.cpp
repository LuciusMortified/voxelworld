module vw.ecs;

import std;

namespace vw::ecs {

entity_pool::entity_pool(uint32 capacity) {
    generations_.reserve(capacity);
}

auto entity_pool::create() -> entity {
    if (!free_indices_.empty()) [[unlikely]] {
        const uint32 index = free_indices_.back();
        free_indices_.pop_back();
        return {index, generations_[index]};
    }

    const uint32 index = static_cast<uint32>(generations_.size());
    generations_.push_back(0);
    return {index, 0};
}

auto entity_pool::batch_create(uint32 count) -> std::vector<entity> {
    std::vector<entity> result;
    result.reserve(count);
    for (uint32 i = 0; i < count; ++i) {
        result.push_back(create());
    }
    return result;
}

auto entity_pool::has(entity e) const -> bool {
    return e.index != entity::invalid_index && e.index < generations_.size() &&
        generations_[e.index] == e.generation;
}

void entity_pool::destroy(entity e) {
    if (has(e)) [[likely]] {
        ++generations_[e.index];
        free_indices_.push_back(e.index);
    }
}

void entity_pool::batch_destroy(const std::vector<entity>& entities) {
    for (auto e : entities) {
        destroy(e);
    }
}

auto entity_pool::alive_entities() const -> std::vector<entity> {
    const std::unordered_set<uint32> free_set(free_indices_.begin(), free_indices_.end());
    std::vector<entity> result;
    result.reserve(generations_.size() - free_indices_.size());
    for (uint32 i = 0; i < generations_.size(); ++i) {
        if (!free_set.contains(i)) {
            result.push_back({i, generations_[i]});
        }
    }
    return result;
}

}  // namespace vw::ecs
