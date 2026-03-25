#pragma once

#ifndef VW_GFX_STAGING_BUFFER_INL_H
#define VW_GFX_STAGING_BUFFER_INL_H

#include <cassert>
#include <cstring>
#include <stdexcept>

#include "vw/gfx/render/vulkan_context.h"

namespace vw::gfx {

inline staging_buffer::staging_buffer(
    vulkan_context& context, VkDeviceSize frame_capacity, uint32 max_frames_in_flight
)
    : context_(&context)
    , frame_capacity_(frame_capacity)
    , max_frames_in_flight_(max_frames_in_flight)
    , current_frame_index_(max_frames_in_flight - 1) {
    const VkDeviceSize total_capacity = frame_capacity_ * max_frames_in_flight_;

    VkBufferCreateInfo buffer_info{};
    buffer_info.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffer_info.size        = total_capacity;
    buffer_info.usage       = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(context_->get_device(), &buffer_info, nullptr, &buffer_) != VK_SUCCESS) {
        throw std::runtime_error("failed to create staging buffer");
    }

    VkMemoryRequirements mem_requirements;
    vkGetBufferMemoryRequirements(context_->get_device(), buffer_, &mem_requirements);

    VkPhysicalDeviceMemoryProperties mem_properties;
    vkGetPhysicalDeviceMemoryProperties(context_->get_physical_device(), &mem_properties);

    uint32 memory_type_index = 0;
    bool found               = false;
    for (uint32 i = 0; i < mem_properties.memoryTypeCount; i++) {
        auto required = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
        if ((mem_requirements.memoryTypeBits & (1 << i)) &&
            (mem_properties.memoryTypes[i].propertyFlags & required) == required) {
            memory_type_index = i;
            found             = true;
            break;
        }
    }
    if (!found) {
        throw std::runtime_error("failed to find suitable memory type for staging buffer");
    }

    VkMemoryAllocateInfo alloc_info{};
    alloc_info.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    alloc_info.allocationSize  = mem_requirements.size;
    alloc_info.memoryTypeIndex = memory_type_index;

    if (vkAllocateMemory(context_->get_device(), &alloc_info, nullptr, &memory_) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate staging buffer memory");
    }

    vkBindBufferMemory(context_->get_device(), buffer_, memory_, 0);
    vkMapMemory(context_->get_device(), memory_, 0, total_capacity, 0, &mapped_);
}

inline staging_buffer::~staging_buffer() {
    if (mapped_) {
        vkUnmapMemory(context_->get_device(), memory_);
    }
    if (buffer_ != VK_NULL_HANDLE) {
        vkDestroyBuffer(context_->get_device(), buffer_, nullptr);
    }
    if (memory_ != VK_NULL_HANDLE) {
        vkFreeMemory(context_->get_device(), memory_, nullptr);
    }
}

inline void staging_buffer::begin_frame() {
    current_frame_index_ = (current_frame_index_ + 1) % max_frames_in_flight_;
    write_offset_        = current_frame_index_ * frame_capacity_;
    frame_end_offset_    = write_offset_ + frame_capacity_;
}

inline auto staging_buffer::stage(
    const void* data, VkDeviceSize size
) -> VkDeviceSize {
    assert(write_offset_ + size <= frame_end_offset_);
    const auto offset = write_offset_;
    std::memcpy(static_cast<std::byte*>(mapped_) + offset, data, size);
    write_offset_ += size;
    return offset;
}

inline void staging_buffer::copy_to(
    VkBuffer dst, VkDeviceSize dst_offset, VkDeviceSize staging_offset, VkDeviceSize size
) {
    pending_copies_.push_back({buffer_, dst, {staging_offset, dst_offset, size}});
}

inline void staging_buffer::zero_region(
    VkBuffer dst, VkDeviceSize dst_offset, VkDeviceSize size
) {
    assert(write_offset_ + size <= frame_end_offset_);
    auto offset = write_offset_;
    std::memset(static_cast<std::byte*>(mapped_) + offset, 0, size);
    write_offset_ += size;
    copy_to(dst, dst_offset, offset, size);
}

inline void staging_buffer::replace_buffer(VkBuffer old_buf, VkBuffer new_buf) {
    for (auto& copy : pending_copies_) {
        if (copy.src == old_buf) copy.src = new_buf;
        if (copy.dst == old_buf) copy.dst = new_buf;
    }
}

inline void staging_buffer::flush(
    VkCommandBuffer cmd
) {
    if (pending_copies_.empty()) {
        return;
    }

    std::ranges::stable_sort(
        pending_copies_, [this](const pending_copy& a, const pending_copy& b) {
            const bool a_is_staging = (a.src == buffer_);
            const bool b_is_staging = (b.src == buffer_);
            if (a_is_staging != b_is_staging) return !a_is_staging;
            if (a.src != b.src) return a.src < b.src;
            return a.dst < b.dst;
        }
    );

    for (auto it = pending_copies_.begin(); it != pending_copies_.end();) {
        auto batch_end = std::find_if(it + 1, pending_copies_.end(), [&](const pending_copy& c) {
            return c.src != it->src || c.dst != it->dst;
        });

        flush_regions_.clear();
        for (auto r = it; r != batch_end; ++r) {
            bool duplicate = false;
            for (auto later = r + 1; later != batch_end; ++later) {
                if (later->region.dstOffset == r->region.dstOffset &&
                    later->region.size == r->region.size) {
                    duplicate = true;
                    break;
                }
            }
            if (!duplicate) {
                flush_regions_.push_back(r->region);
            }
        }

        vkCmdCopyBuffer(
            cmd, it->src, it->dst,
            static_cast<uint32_t>(flush_regions_.size()),
            flush_regions_.data()
        );

        it = batch_end;
    }

    pending_copies_.clear();
}

}  // namespace vw::gfx

#endif  // VW_GFX_STAGING_BUFFER_INL_H
