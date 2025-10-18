#pragma once

#ifndef VW_GFX_COMPONENT_POOL_H
#define VW_GFX_COMPONENT_POOL_H
#include <vector>

#include "entity.h"

namespace vw::gfx {
template <typename T>
class component_pool final {
public:
    static constexpr size_t default_sparse_capacity = 1024;

    explicit component_pool(size_t sparse_capacity = default_sparse_capacity) {
        sparse_indices_.resize(sparse_capacity, entity::invalid_index);
    }

    [[nodiscard]]
    bool has(entity ent) const {
        return ent.index != entity::invalid_index && ent.index < sparse_indices_.size() &&
            sparse_indices_[ent.index] != entity::invalid_index &&
            dense_entities_[sparse_indices_[ent.index]] == ent;
    }

    void add(entity ent, T&& value = {}) {
        if (ent.index >= sparse_indices_.size()) [[unlikely]] {
            sparse_indices_.resize(ent.index + 1, entity::invalid_index);
        }

        if (has(ent)) [[unlikely]] {
            dense_data_[sparse_indices_[ent.index]] = std::forward<T>(value);
            return;
        }

        const uint32 index = dense_data_.size();
        dense_data_.push_back(std::forward<T>(value));
        dense_entities_.push_back(ent);
        sparse_indices_[ent.index] = index;
    }

    void remove(entity ent) {
        if (!has(ent)) [[unlikely]] {
            return;
        }

        const uint32 index      = sparse_indices_[ent.index];
        const uint32 last_index = dense_data_.size() - 1;

        if (index != last_index) {
            std::swap(dense_data_[index], dense_data_[last_index]);
            std::swap(dense_entities_[index], dense_entities_[last_index]);
            sparse_indices_[dense_entities_[index].index] = index;
        }

        dense_data_.pop_back();
        dense_entities_.pop_back();

        sparse_indices_[index] = entity::invalid_index;
    }

    [[nodiscard]]
    T& get(entity e) {
        return dense_data_[sparse_indices_[e.index]];
    }

    [[nodiscard]]
    const T& get(entity e) const {
        return dense_data_[sparse_indices_[e.index]];
    }

    [[nodiscard]]
    const std::vector<entity>& entities() const {
        return dense_entities_;
    }

private:
    std::vector<T> dense_data_;
    std::vector<entity> dense_entities_;
    std::vector<uint32> sparse_indices_;
};
}  // namespace vw::gfx

#endif  // VW_GFX_COMPONENT_POOL_H
