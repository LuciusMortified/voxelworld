#pragma once

#ifndef VW_ASSET_MODEL_PAGE_POOL_H
#define VW_ASSET_MODEL_PAGE_POOL_H

#include <array>
#include <memory>
#include <mutex>
#include <span>
#include <vector>

#include "vw/core/types.h"
#include "vw/core/voxel.h"

namespace vw::asset {

class page_pool final {
public:
    using page_type = std::array<voxel, 512>;
    static constexpr uint32 block_size = 4096;
    static constexpr uint32 max_blocks = 256;

    page_pool() { blocks_.reserve(max_blocks); }

    [[nodiscard]] auto alloc() -> uint32;
    void free(uint32 index);

    [[nodiscard]] auto alloc_batch(uint32 count) -> std::vector<uint32>;
    void free_batch(std::span<const uint32> indices);

    [[nodiscard]] auto get(uint32 index) -> page_type&;
    [[nodiscard]] auto get(uint32 index) const -> const page_type&;

    [[nodiscard]] auto allocated_count() const -> uint32;
    [[nodiscard]] auto free_count() const -> uint32;

private:
    void ensure_capacity_(uint32 index);

    mutable std::mutex mutex_;
    std::vector<std::unique_ptr<std::array<page_type, block_size>>> blocks_;
    std::vector<uint32> free_indices_;
    uint32 next_index_ = 0;
};

}  // namespace vw::asset

#include "vw/asset/model/page_pool.inl.h"

#endif  // VW_ASSET_MODEL_PAGE_POOL_H
