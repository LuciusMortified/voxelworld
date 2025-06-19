#pragma once
#include <vulkan/vulkan.h>
#include "types.h"

namespace voxel {
    class vulkan_context;

    class buffer {
    public:
        buffer(vulkan_context& context, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties);
        ~buffer();

        // Запретить копирование
        buffer(const buffer&) = delete;
        buffer& operator=(const buffer&) = delete;

        // Перемещение разрешено
        buffer(buffer&& other) noexcept;
        buffer& operator=(buffer&& other) noexcept;

        VkBuffer get_buffer() const { return buffer_; }
        VkDeviceMemory get_memory() const { return memory_; }
        VkDeviceSize get_size() const { return size_; }

        void* map();
        void unmap();
        void copy_from(const void* data, VkDeviceSize size, VkDeviceSize offset = 0);

        void copy_to_buffer(buffer& dst, VkDeviceSize size, VkDeviceSize src_offset = 0, VkDeviceSize dst_offset = 0);

    private:
        void cleanup();
        uint32_t find_memory_type(uint32_t type_filter, VkMemoryPropertyFlags properties);

        vulkan_context& context_;
        VkBuffer buffer_;
        VkDeviceMemory memory_;
        VkDeviceSize size_;
        void* mapped_memory_;
    };

    // Специализированные типы буферов
    class vertex_buffer : public buffer {
    public:
        vertex_buffer(vulkan_context& context, VkDeviceSize size);
        template<typename T>
        vertex_buffer(vulkan_context& context, const std::vector<T>& vertices);
    };

    class index_buffer : public buffer {
    public:
        index_buffer(vulkan_context& context, VkDeviceSize size);
        template<typename T>
        index_buffer(vulkan_context& context, const std::vector<T>& indices);
    };

    class uniform_buffer : public buffer {
    public:
        uniform_buffer(vulkan_context& context, VkDeviceSize size);
        template<typename T>
        uniform_buffer(vulkan_context& context, const T& data);
    };
}