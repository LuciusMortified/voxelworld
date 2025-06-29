#include <stdexcept>
#include <cstring>

#include <voxel/buffer.h>
#include <voxel/vulkan_context.h>

namespace voxel {

// ================== buffer ==================

buffer::buffer(
    std::shared_ptr<vulkan_context> context,
    VkDeviceSize size,
    VkBufferUsageFlags usage,
    VkMemoryPropertyFlags properties
)
    : context_(std::move(context)), buffer_(VK_NULL_HANDLE), memory_(VK_NULL_HANDLE), 
      size_(size), mapped_memory_(nullptr) {
    
    VkBufferCreateInfo buffer_info{};
    buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffer_info.size = size;
    buffer_info.usage = usage;
    buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(context_->get_device(), &buffer_info, nullptr, &buffer_) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create buffer");
    }

    VkMemoryRequirements mem_requirements;
    vkGetBufferMemoryRequirements(context_->get_device(), buffer_, &mem_requirements);

    VkMemoryAllocateInfo alloc_info{};
    alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    alloc_info.allocationSize = mem_requirements.size;
    alloc_info.memoryTypeIndex = find_memory_type(mem_requirements.memoryTypeBits, properties);

    if (vkAllocateMemory(context_->get_device(), &alloc_info, nullptr, &memory_) != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate buffer memory");
    }

    vkBindBufferMemory(context_->get_device(), buffer_, memory_, 0);
}

buffer::~buffer() {
    cleanup();
}

buffer::buffer(buffer&& other) noexcept 
    : context_(other.context_), buffer_(other.buffer_), memory_(other.memory_), 
      size_(other.size_), mapped_memory_(other.mapped_memory_) {
    other.buffer_ = VK_NULL_HANDLE;
    other.memory_ = VK_NULL_HANDLE;
    other.size_ = 0;
    other.mapped_memory_ = nullptr;
}

buffer& buffer::operator=(buffer&& other) noexcept {
    if (this != &other) {
        cleanup();
        // `context_` является ссылкой и не может быть переприсвоена, 
        // предполагается, что оба буфера находятся в одном контексте.
        buffer_ = other.buffer_;
        memory_ = other.memory_;
        size_ = other.size_;
        mapped_memory_ = other.mapped_memory_;

        other.buffer_ = VK_NULL_HANDLE;
        other.memory_ = VK_NULL_HANDLE;
        other.size_ = 0;
        other.mapped_memory_ = nullptr;
    }
    return *this;
}

void* buffer::map() {
    if (mapped_memory_ == nullptr) {
        if (vkMapMemory(context_->get_device(), memory_, 0, size_, 0, &mapped_memory_) != VK_SUCCESS) {
            throw std::runtime_error("Failed to map buffer memory");
        }
    }
    return mapped_memory_;
}

void buffer::unmap() {
    if (mapped_memory_ != nullptr) {
        vkUnmapMemory(context_->get_device(), memory_);
        mapped_memory_ = nullptr;
    }
}

void buffer::copy_from(const void* data, VkDeviceSize size, VkDeviceSize offset) {
    if (size == 0) return;
    void* mapped = map();
    memcpy(static_cast<char*>(mapped) + offset, data, size);
}

void buffer::copy_to_buffer(
    buffer& dst,
    VkDeviceSize size,
    VkDeviceSize src_offset,
    VkDeviceSize dst_offset
) {
    VkCommandBufferAllocateInfo alloc_info{};
    alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    alloc_info.commandPool = context_->get_command_pool();
    alloc_info.commandBufferCount = 1;

    VkCommandBuffer command_buffer;
    vkAllocateCommandBuffers(context_->get_device(), &alloc_info, &command_buffer);

    VkCommandBufferBeginInfo begin_info{};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(command_buffer, &begin_info);

    VkBufferCopy copy_region{};
    copy_region.srcOffset = src_offset;
    copy_region.dstOffset = dst_offset;
    copy_region.size = size;
    vkCmdCopyBuffer(command_buffer, buffer_, dst.buffer_, 1, &copy_region);

    vkEndCommandBuffer(command_buffer);

    VkSubmitInfo submit_info{};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &command_buffer;

    vkQueueSubmit(context_->get_graphics_queue(), 1, &submit_info, VK_NULL_HANDLE);
    vkQueueWaitIdle(context_->get_graphics_queue());

    vkFreeCommandBuffers(context_->get_device(), context_->get_command_pool(), 1, &command_buffer);
}

uint32 buffer::find_memory_type(uint32 type_filter, VkMemoryPropertyFlags properties) {
    VkPhysicalDeviceMemoryProperties mem_properties;
    vkGetPhysicalDeviceMemoryProperties(context_->get_physical_device(), &mem_properties);

    for (uint32 i = 0; i < mem_properties.memoryTypeCount; i++) {
        if ((type_filter & (1 << i)) && (mem_properties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }

    throw std::runtime_error("Failed to find suitable memory type");
}

void buffer::cleanup() {
    if (mapped_memory_) {
        unmap();
    }
    if (buffer_ != VK_NULL_HANDLE) {
        vkDestroyBuffer(context_->get_device(), buffer_, nullptr);
    }
    if (memory_ != VK_NULL_HANDLE) {
        vkFreeMemory(context_->get_device(), memory_, nullptr);
    }
}

} // namespace voxel 