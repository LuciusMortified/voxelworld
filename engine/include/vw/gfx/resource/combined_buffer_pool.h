#pragma once

#ifndef VW_GFX_COMBINED_BUFFER_POOL_H
#define VW_GFX_COMBINED_BUFFER_POOL_H

#include <vulkan/vulkan.h>

#include <map>
#include <memory>
#include <unordered_map>
#include <vector>

#include "vw/gfx/resource/combined_buffer.h"
#include "vw/gfx/world/entity.h"
#include "vw/gfx/world/world.h"


namespace vw::gfx {

class vulkan_context;

struct entity_buffer_info {
    buffer_chunk_size chunk_size;
    size_t buffer_index;  // Индекс буфера в buffers_
};

struct combined_buffer_pool_stats {
    float32 vertex_load_min  = 0.0f;
    float32 vertex_load_max  = 0.0f;
    float32 vertex_load_avg  = 0.0f;
    float32 index_load_min   = 0.0f;
    float32 index_load_max   = 0.0f;
    float32 index_load_avg   = 0.0f;
    uint32 mesh_capacity     = 0;
    uint32 mesh_count        = 0;
    uint32 instance_capacity = 0;
    uint32 instance_count    = 0;
    std::vector<combined_buffer_stats> buffers;
};

template <typename C>
class combined_buffer_pool {
public:
    using world_type = world<C>;

    explicit combined_buffer_pool(
        vulkan_context& context,
        VkDescriptorPool descriptor_pool,
        VkDescriptorSetLayout descriptor_set_layout
    );
    ~combined_buffer_pool() = default;

    combined_buffer_pool(const combined_buffer_pool&)            = delete;
    combined_buffer_pool& operator=(const combined_buffer_pool&) = delete;
    combined_buffer_pool(combined_buffer_pool&&)                 = delete;
    combined_buffer_pool& operator=(combined_buffer_pool&&)      = delete;

    void update(world_type& world);

    [[nodiscard]] auto get_buffers() const -> const std::vector<std::unique_ptr<combined_buffer>>&;

    [[nodiscard]]
    static buffer_chunk_size get_chunk_size_for_mesh(uint32 vertex_count, uint32 index_count);

    [[nodiscard]]
    const combined_buffer_pool_stats& get_stats() const;

private:
    combined_buffer* get_or_create_buffer(const buffer_chunk_size& chunk_size);

    void update_meshes_(world_type& world);
    void update_transforms_(world_type& world);

    vulkan_context* context_;
    std::vector<std::unique_ptr<combined_buffer>> buffers_;
    std::unordered_map<entity, entity_buffer_info> entity_buffer_infos_;
    std::map<buffer_chunk_size, size_t> chunk_size_to_buffer_index_;

    VkDescriptorPool descriptor_pool_            = VK_NULL_HANDLE;
    VkDescriptorSetLayout descriptor_set_layout_ = VK_NULL_HANDLE;

    mutable combined_buffer_pool_stats stats_;
};

}  // namespace vw::gfx

#include "vw/gfx/resource/combined_buffer_pool.inl.h"

#endif  // VW_GFX_COMBINED_BUFFER_POOL_H
