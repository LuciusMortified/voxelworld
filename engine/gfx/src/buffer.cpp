module vw.gfx;

import std;
import vulkan;
import vw.core;
import vw.ecs;
import vw.world;
import vw.platform;
import :vk;

namespace vw::gfx {

buffer::buffer(
    vulkan_context& context,
    vk::DeviceSize size,
    vk::BufferUsageFlags usage,
    vk::MemoryPropertyFlags properties
)
    : size_(size),
      context_(&context),
      mapped_memory_(nullptr) {
    const vk::Device device = context_->get_device();

    buffer_ = vk_must(
        device.createBuffer({
            .size        = size,
            .usage       = usage,
            .sharingMode = vk::SharingMode::eExclusive,
        }),
        "create buffer"
    );

    const vk::MemoryRequirements mem_requirements = device.getBufferMemoryRequirements(buffer_);

    memory_ = vk_must(
        device.allocateMemory({
            .allocationSize  = mem_requirements.size,
            .memoryTypeIndex = find_memory_type(mem_requirements.memoryTypeBits, properties),
        }),
        "allocate buffer memory"
    );

    vk_must(device.bindBufferMemory(buffer_, memory_, 0), "bind buffer memory");
}

buffer::~buffer() {
    cleanup();
}

buffer::buffer(buffer&& other) noexcept
    : size_(std::exchange(other.size_, 0)),
      context_(std::exchange(other.context_, nullptr)),
      buffer_(std::exchange(other.buffer_, nullptr)),
      memory_(std::exchange(other.memory_, nullptr)),
      mapped_memory_(std::exchange(other.mapped_memory_, nullptr)) {}

auto buffer::operator=(buffer&& other) noexcept -> buffer& {
    if (this != &other) {
        cleanup();

        size_          = std::exchange(other.size_, 0);
        buffer_        = std::exchange(other.buffer_, nullptr);
        memory_        = std::exchange(other.memory_, nullptr);
        mapped_memory_ = std::exchange(other.mapped_memory_, nullptr);
    }
    return *this;
}

auto buffer::map() -> void* {
    if (mapped_memory_ == nullptr) {
        mapped_memory_ =
            vk_must(context_->get_device().mapMemory(memory_, 0, size_), "map buffer memory");
    }
    return mapped_memory_;
}

auto buffer::unmap() -> void {
    if (mapped_memory_ != nullptr) {
        context_->get_device().unmapMemory(memory_);
        mapped_memory_ = nullptr;
    }
}

auto buffer::copy_from(const void* data, vk::DeviceSize size, vk::DeviceSize offset) -> void {
    if (size == 0)
        return;
    void* mapped = map();
    std::memcpy(static_cast<char*>(mapped) + offset, data, size);
}

auto buffer::copy_to(
    void* data, vk::DeviceSize size, vk::DeviceSize offset
) -> void {
    void* mapped = map();
    std::memcpy(data, static_cast<char*>(mapped) + offset, size);
}

auto buffer::find_memory_type(uint32 type_filter, vk::MemoryPropertyFlags properties) const
    -> uint32 {
    const vk::PhysicalDeviceMemoryProperties mem_properties =
        context_->get_physical_device().getMemoryProperties();

    for (uint32 i = 0; i < mem_properties.memoryTypeCount; i++) {
        if ((type_filter & (1 << i)) &&
            (mem_properties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }

    throw std::runtime_error("failed to find suitable memory type");
}

auto buffer::cleanup() -> void {
    if (mapped_memory_ != nullptr) {
        unmap();
    }
    if (buffer_) {
        context_->get_device().destroyBuffer(buffer_);
    }
    if (memory_) {
        context_->get_device().freeMemory(memory_);
    }
}

}  // namespace vw::gfx
