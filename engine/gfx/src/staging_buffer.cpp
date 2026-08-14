module;

#include <cassert>

module vw.gfx;

import std;
import vulkan;
import vw.core;
import vw.ecs;
import vw.world;
import vw.platform;
import :vk;

namespace vw::gfx {

staging_buffer::staging_buffer(
    vulkan_context& context, vk::DeviceSize frame_capacity, uint32 max_frames_in_flight
)
    : context_(&context)
    , frame_capacity_(frame_capacity)
    , max_frames_in_flight_(max_frames_in_flight)
    , current_frame_index_(max_frames_in_flight - 1) {
    const vk::DeviceSize total_capacity = frame_capacity_ * max_frames_in_flight_;

    const vk::Device device = context_->get_device();

    buffer_ = vk_must(
        device.createBuffer({
            .size        = total_capacity,
            .usage       = vk::BufferUsageFlagBits::eTransferSrc,
            .sharingMode = vk::SharingMode::eExclusive,
        }),
        "create staging buffer"
    );

    const vk::MemoryRequirements mem_requirements = device.getBufferMemoryRequirements(buffer_);
    const vk::PhysicalDeviceMemoryProperties mem_properties =
        context_->get_physical_device().getMemoryProperties();

    uint32 memory_type_index = 0;
    bool found               = false;
    for (uint32 i = 0; i < mem_properties.memoryTypeCount; i++) {
        auto required = vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent;
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

    memory_ = vk_must(
        device.allocateMemory({
            .allocationSize  = mem_requirements.size,
            .memoryTypeIndex = memory_type_index,
        }),
        "allocate staging buffer memory"
    );

    vk_must(device.bindBufferMemory(buffer_, memory_, 0), "bind staging buffer memory");
    mapped_ = vk_must(device.mapMemory(memory_, 0, total_capacity), "map staging buffer");
}

staging_buffer::~staging_buffer() {
    const vk::Device device = context_->get_device();
    if (mapped_ != nullptr) {
        device.unmapMemory(memory_);
    }
    if (buffer_) {
        device.destroyBuffer(buffer_);
    }
    if (memory_) {
        device.freeMemory(memory_);
    }
}

void staging_buffer::begin_frame() {
    current_frame_index_ = (current_frame_index_ + 1) % max_frames_in_flight_;
    write_offset_        = current_frame_index_ * frame_capacity_;
    frame_end_offset_    = write_offset_ + frame_capacity_;
}

auto staging_buffer::stage(
    const void* data, vk::DeviceSize size
) -> vk::DeviceSize {
    assert(write_offset_ + size <= frame_end_offset_);
    const auto offset = write_offset_;
    std::memcpy(static_cast<std::byte*>(mapped_) + offset, data, size);
    write_offset_ += size;
    return offset;
}

void staging_buffer::copy_to(
    vk::Buffer dst, vk::DeviceSize dst_offset, vk::DeviceSize staging_offset, vk::DeviceSize size
) {
    pending_copies_.push_back({buffer_, dst, {staging_offset, dst_offset, size}});
}

void staging_buffer::copy_buffer(
    vk::Buffer src, vk::DeviceSize src_offset,
    vk::Buffer dst, vk::DeviceSize dst_offset,
    vk::DeviceSize size
) {
    if (size == 0) {
        return;
    }
    pending_copies_.push_back({src, dst, {src_offset, dst_offset, size}});
}

void staging_buffer::zero_region(
    vk::Buffer dst, vk::DeviceSize dst_offset, vk::DeviceSize size
) {
    assert(write_offset_ + size <= frame_end_offset_);
    auto offset = write_offset_;
    std::memset(static_cast<std::byte*>(mapped_) + offset, 0, size);
    write_offset_ += size;
    copy_to(dst, dst_offset, offset, size);
}

void staging_buffer::replace_buffer(vk::Buffer old_buf, vk::Buffer new_buf) {
    for (auto& copy : pending_copies_) {
        if (copy.src == old_buf) copy.src = new_buf;
        if (copy.dst == old_buf) copy.dst = new_buf;
    }
}

void staging_buffer::flush(
    vk::CommandBuffer cmd
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

    bool saw_device_copy = false;
    bool barrier_emitted = false;

    for (auto it = pending_copies_.begin(); it != pending_copies_.end();) {
        auto batch_end = std::find_if(it + 1, pending_copies_.end(), [&](const pending_copy& c) {
            return c.src != it->src || c.dst != it->dst;
        });

        // Growth copies run first and may target the same regions this frame's
        // staging writes are about to touch, so the two groups need ordering.
        if (it->src == buffer_) {
            if (saw_device_copy && !barrier_emitted) {
                const vk::MemoryBarrier barrier{
                    .srcAccessMask = vk::AccessFlagBits::eTransferWrite,
                    .dstAccessMask = vk::AccessFlagBits::eTransferWrite,
                };
                cmd.pipelineBarrier(
                    vk::PipelineStageFlagBits::eTransfer,
                    vk::PipelineStageFlagBits::eTransfer,
                    {},
                    barrier,
                    {},
                    {}
                );
                barrier_emitted = true;
            }
        } else {
            saw_device_copy = true;
        }

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

        cmd.copyBuffer(it->src, it->dst, flush_regions_);

        it = batch_end;
    }

    pending_copies_.clear();
}

}  // namespace vw::gfx
