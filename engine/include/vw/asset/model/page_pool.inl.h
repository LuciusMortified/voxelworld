#pragma once

#ifndef VW_ASSET_MODEL_PAGE_POOL_INL_H
#define VW_ASSET_MODEL_PAGE_POOL_INL_H

namespace vw::asset {

inline auto page_pool::alloc() -> uint32 {
    std::scoped_lock lock(mutex_);
    if (!free_indices_.empty()) {
        uint32 idx = free_indices_.back();
        free_indices_.pop_back();
        return idx;
    }
    uint32 idx = next_index_++;
    ensure_capacity_(idx);
    return idx;
}

inline void page_pool::free(uint32 index) {
    std::scoped_lock lock(mutex_);
    free_indices_.push_back(index);
}

inline auto page_pool::alloc_batch(uint32 count) -> std::vector<uint32> {
    std::scoped_lock lock(mutex_);
    std::vector<uint32> result;
    result.reserve(count);

    uint32 consecutive = count - static_cast<uint32>(std::min(
        static_cast<size_t>(count), free_indices_.size()
    ));

    uint32 bump_start = next_index_;
    for (uint32 i = 0; i < consecutive; ++i) {
        result.push_back(bump_start + i);
    }
    next_index_ = bump_start + consecutive;
    if (consecutive > 0) {
        ensure_capacity_(next_index_ - 1);
    }

    uint32 remaining = count - consecutive;
    for (uint32 i = 0; i < remaining; ++i) {
        uint32 idx = free_indices_.back();
        free_indices_.pop_back();
        result.push_back(idx);
    }

    return result;
}

inline void page_pool::free_batch(std::span<const uint32> indices) {
    std::scoped_lock lock(mutex_);
    free_indices_.reserve(free_indices_.size() + indices.size());
    for (uint32 idx : indices) {
        free_indices_.push_back(idx);
    }
}

inline auto page_pool::get(uint32 index) -> page_type& {
    return (*blocks_[index / block_size])[index % block_size];
}

inline auto page_pool::get(uint32 index) const -> const page_type& {
    return (*blocks_[index / block_size])[index % block_size];
}

inline auto page_pool::allocated_count() const -> uint32 {
    std::scoped_lock lock(mutex_);
    return next_index_ - static_cast<uint32>(free_indices_.size());
}

inline auto page_pool::free_count() const -> uint32 {
    std::scoped_lock lock(mutex_);
    return static_cast<uint32>(free_indices_.size());
}

inline void page_pool::ensure_capacity_(uint32 index) {
    uint32 block_idx = index / block_size;
    while (blocks_.size() <= block_idx) {
        blocks_.push_back(std::make_unique<std::array<page_type, block_size>>());
    }
}

}  // namespace vw::asset

#endif  // VW_ASSET_MODEL_PAGE_POOL_INL_H
