#pragma once

#ifndef VW_GFX_BUFFER_H
#define VW_GFX_BUFFER_H

#include <vulkan/vulkan.h>
#include <stdexcept>
#include <vector>

namespace vw::gfx {
class vulkan_context;

class buffer {
public:
    buffer(
        vulkan_context& context,
        VkDeviceSize size,
        VkBufferUsageFlags usage,
        VkMemoryPropertyFlags properties
    );
    virtual ~buffer();

    buffer(const buffer&)            = delete;
    buffer& operator=(const buffer&) = delete;

    buffer(buffer&& other) noexcept;
    buffer& operator=(buffer&& other) noexcept;

    [[nodiscard]]
    VkBuffer get_buffer() const {
        return buffer_;
    }

    [[nodiscard]]
    VkDeviceMemory get_memory() const {
        return memory_;
    }

    [[nodiscard]]
    VkDeviceSize get_size() const {
        return size_;
    }

    [[nodiscard]]
    void* map();

    void unmap();

    void copy_from(const void* data, VkDeviceSize size, VkDeviceSize offset = 0);

    template <typename T>
    void copy_from_struct(const T& data, VkDeviceSize offset = 0) {
        if (sizeof(T) > size_) {
            throw std::runtime_error("data size exceeds buffer size");
        }
        copy_from(&data, sizeof(T), offset);
    }

    template <typename T>
    void copy_from_vector(const std::vector<T>& data, VkDeviceSize offset = 0) {
        if (data.size() * sizeof(T) > size_) {
            throw std::runtime_error("data size exceeds buffer size");
        }
        copy_from(data.data(), data.size() * sizeof(T), offset);
    }

    void copy_to_buffer(
        buffer& dst,
        VkDeviceSize size,
        VkDeviceSize src_offset = 0,
        VkDeviceSize dst_offset = 0
    );

protected:
    VkDeviceSize size_;

private:
    void cleanup();

    [[nodiscard]]
    uint32_t find_memory_type(uint32_t type_filter, VkMemoryPropertyFlags properties) const;

    vulkan_context* context_;

    VkBuffer buffer_;
    VkDeviceMemory memory_;
    void* mapped_memory_;
};

// Специализированные типы буферов
class vertex_buffer final : public buffer {
public:
    vertex_buffer(vulkan_context& context, VkDeviceSize size)
        : buffer(
              context,
              size,
              VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
              VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
          ) {}

    template <typename T>
    vertex_buffer(vulkan_context& context, const std::vector<T>& vertices)
        : buffer(
              context,
              sizeof(T) * vertices.size(),
              VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
              VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
          ) {
        copy_from_vector(vertices);
    }
};

class index_buffer final : public buffer {
public:
    index_buffer(vulkan_context& context, VkDeviceSize size)
        : buffer(
              context,
              size,
              VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
              VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
          ) {}

    template <typename T>
    index_buffer(vulkan_context& context, const std::vector<T>& indices)
        : buffer(
              context,
              sizeof(T) * indices.size(),
              VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
              VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
          ) {
        copy_from(indices.data(), size_);
    }
};

class uniform_buffer final : public buffer {
public:
    uniform_buffer(vulkan_context& context, VkDeviceSize size)
        : buffer(
              context,
              size,
              VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
              VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
          ) {}

    template <typename T>
    uniform_buffer(vulkan_context& context, const T& data)
        : buffer(
              context,
              sizeof(T),
              VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
              VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
          ) {
        copy_from(&data, size_);
    }
};
}  // namespace vw::gfx

#endif  // VW_GFX_BUFFER_H
