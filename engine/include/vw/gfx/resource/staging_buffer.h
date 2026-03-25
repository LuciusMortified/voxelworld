#pragma once

#ifndef VW_GFX_STAGING_BUFFER_H
#define VW_GFX_STAGING_BUFFER_H

#include <vulkan/vulkan.h>

#include <algorithm>
#include <vector>

#include "vw/core/types.h"

namespace vw::gfx {

class vulkan_context;

class staging_buffer {
public:
    explicit staging_buffer(
        vulkan_context& context,
        VkDeviceSize frame_capacity  = 1 * 1024 * 1024,
        uint32 max_frames_in_flight = 2
    );
    ~staging_buffer();

    staging_buffer(const staging_buffer&)                    = delete;
    auto operator=(const staging_buffer&) -> staging_buffer& = delete;
    staging_buffer(staging_buffer&&)                          = delete;
    auto operator=(staging_buffer&&) -> staging_buffer&       = delete;

    void begin_frame();

    auto stage(const void* data, VkDeviceSize size) -> VkDeviceSize;

    template <typename T>
    auto stage_struct(const T& data) -> VkDeviceSize {
        return stage(&data, sizeof(T));
    }

    template <typename T>
    auto stage_vector(const std::vector<T>& data) -> VkDeviceSize {
        return stage(data.data(), data.size() * sizeof(T));
    }

    void copy_to(
        VkBuffer dst, VkDeviceSize dst_offset, VkDeviceSize staging_offset, VkDeviceSize size
    );

    void zero_region(VkBuffer dst, VkDeviceSize dst_offset, VkDeviceSize size);

    [[nodiscard]] auto available() const -> VkDeviceSize {
        return frame_end_offset_ - write_offset_;
    }

    void replace_buffer(VkBuffer old_buf, VkBuffer new_buf);

    void flush(VkCommandBuffer cmd);

private:
    struct pending_copy {
        VkBuffer src;
        VkBuffer dst;
        VkBufferCopy region;
    };

    vulkan_context* context_;
    VkBuffer buffer_          = VK_NULL_HANDLE;
    VkDeviceMemory memory_    = VK_NULL_HANDLE;
    void* mapped_             = nullptr;

    VkDeviceSize frame_capacity_    = 0;
    uint32 max_frames_in_flight_    = 2;
    uint32 current_frame_index_     = 0;
    VkDeviceSize write_offset_      = 0;
    VkDeviceSize frame_end_offset_  = 0;

    std::vector<pending_copy> pending_copies_;
    std::vector<VkBufferCopy> flush_regions_;
};

}  // namespace vw::gfx

#include "vw/gfx/resource/staging_buffer.inl.h"

#endif  // VW_GFX_STAGING_BUFFER_H
