module vw.ecs;

import std;
import vw.core;

namespace vw::ecs {

auto pool_base::reserve_slot_(entity e) -> slot {
    if (e.index >= sparse_indices_.size()) [[unlikely]] {
        sparse_indices_.resize(e.index + 1, entity::invalid_index);
    }

    if (has(e)) [[unlikely]] {
        return {.index = sparse_indices_[e.index], .fresh = false};
    }

    const auto index = static_cast<uint32>(dense_entities_.size());
    dense_entities_.push_back(e);
    sparse_indices_[e.index] = index;

    return {.index = index, .fresh = true};
}

auto pool_base::release_slot_(entity e) -> removal {
    if (!has(e)) [[unlikely]] {
        return {};
    }

    const uint32 index = sparse_indices_[e.index];
    const auto last    = static_cast<uint32>(dense_entities_.size() - 1);

    if (index != last) {
        dense_entities_[index]                        = dense_entities_[last];
        sparse_indices_[dense_entities_[index].index] = index;
    }

    dense_entities_.pop_back();
    sparse_indices_[e.index] = entity::invalid_index;

    return {.index = index, .last = last};
}

auto pool_base::clear_slots_() -> void {
    dense_entities_.clear();
    std::ranges::fill(sparse_indices_, entity::invalid_index);
}

}  // namespace vw::ecs
