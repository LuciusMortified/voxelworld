#pragma once
#include <vector>
#include <memory>
#include <vulkan/vulkan.h>

#include <voxel/types.h>

namespace voxel {
    class vulkan_context;

    class buffer {
    public:
        buffer(
            std::shared_ptr<vulkan_context> context,
            VkDeviceSize size,
            VkBufferUsageFlags usage,
            VkMemoryPropertyFlags properties
        );
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
        uint32_t find_memory_type(uint32_t type_filter, VkMemoryPropertyFlags properties);

        std::shared_ptr<vulkan_context> context_;
        VkBuffer buffer_;
        VkDeviceMemory memory_;
        void* mapped_memory_;
    };

    // Специализированные типы буферов
    class vertex_buffer : public buffer {
    public:
        vertex_buffer(
            std::shared_ptr<vulkan_context> context,
            VkDeviceSize size
        ) : buffer(
                context,
                size,
                VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
            ) {}
        
        template<typename T>
        vertex_buffer(
            std::shared_ptr<vulkan_context> context,
            const std::vector<T>& vertices
        ) : buffer(
                context,
                sizeof(T) * vertices.size(),
                VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
            ) {
            copy_from(vertices.data(), size_);
        }
    };

    class index_buffer : public buffer {
    public:
        index_buffer(
            std::shared_ptr<vulkan_context> context,
            VkDeviceSize size
        ) : buffer(
                context,
                size,
                VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
            ) {}

        template<typename T>
        index_buffer(
            std::shared_ptr<vulkan_context> context,
            const std::vector<T>& indices
        ) : buffer(
                context,
                sizeof(T) * indices.size(),
                VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
            ) {
            copy_from(indices.data(), size_);
        }
    };

    class uniform_buffer : public buffer {
    public:
        uniform_buffer(
            std::shared_ptr<vulkan_context> context,
            VkDeviceSize size
        ) : buffer(
                context,
                size,
                VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
            ) {}

        template<typename T>
        uniform_buffer(
            std::shared_ptr<vulkan_context> context,
            const T& data
        ) : buffer(
                context,
                sizeof(T),
                VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
            ) {
            copy_from(&data, size_);
        }
    };
}