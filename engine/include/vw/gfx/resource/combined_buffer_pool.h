#pragma once

#ifndef VW_GFX_COMBINED_BUFFER_POOL_H
#define VW_GFX_COMBINED_BUFFER_POOL_H

#include <vulkan/vulkan.h>

#include <chrono>
#include <map>
#include <memory>
#include <unordered_map>
#include <vector>

#include "vw/gfx/resource/combined_buffer.h"
#include "vw/gfx/resource/mesh_pool.h"
#include "vw/gfx/resource/staging_buffer.h"
#include "vw/ecs/entity.h"
#include "vw/ecs/world.h"

namespace vw::gfx {

using namespace ::vw::ecs;

class vulkan_context;

struct entity_buffer_info {
    buffer_chunk_size chunk_size;
    size_t buffer_index;
};

struct buffer_pool_timing_stats {
    float32 destroyed_ms     = 0.0f;
    float32 meshes_ms        = 0.0f;
    float32 transforms_ms    = 0.0f;
    float32 staging_flush_ms = 0.0f;
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
    buffer_pool_timing_stats timing;
};

template <typename C>
class combined_buffer_pool {
public:
    using world_type = world<C>;

    explicit combined_buffer_pool(
        vulkan_context& context,
        VkDescriptorPool descriptor_pool,
        VkDescriptorSetLayout descriptor_set_layout,
        VkDescriptorSetLayout compute_descriptor_set_layout = VK_NULL_HANDLE
    );
    ~combined_buffer_pool() = default;

    combined_buffer_pool(const combined_buffer_pool&)            = delete;
    auto operator=(const combined_buffer_pool&) -> combined_buffer_pool& = delete;
    combined_buffer_pool(combined_buffer_pool&&)                 = delete;
    auto operator=(combined_buffer_pool&&) -> combined_buffer_pool&      = delete;

    void update(
        world_type& world,
        const camera& camera,
        VkCommandBuffer cmd,
        mesh_pool& pool
    );

    [[nodiscard]] auto get_buffers() const -> const std::vector<std::unique_ptr<combined_buffer>>&;

    [[nodiscard]] static auto get_chunk_size_for_mesh(
        uint32 vertex_count, uint32 index_count
    ) -> buffer_chunk_size;

    [[nodiscard]] auto get_stats() const -> const combined_buffer_pool_stats&;

private:
    auto get_or_create_buffer(const buffer_chunk_size& chunk_size) -> combined_buffer*;

    void process_destroyed_(world_type& world);
    void update_meshes_(world_type& world, const vec3f& camera_pos, mesh_pool& pool);
    void update_transforms_(world_type& world);

    vulkan_context* context_;
    staging_buffer staging_;
    std::vector<std::unique_ptr<combined_buffer>> buffers_;
    std::unordered_map<entity, entity_buffer_info> entity_buffer_infos_;
    std::map<buffer_chunk_size, size_t> chunk_size_to_buffer_index_;

    VkDescriptorPool descriptor_pool_                     = VK_NULL_HANDLE;
    VkDescriptorSetLayout descriptor_set_layout_          = VK_NULL_HANDLE;
    VkDescriptorSetLayout compute_descriptor_set_layout_  = VK_NULL_HANDLE;

    std::vector<entity> entities_to_process_;
    std::vector<entity> mesh_pending_entities_;
    std::vector<entity> transform_pending_entities_;
    std::vector<entity> merge_buffer_;

    mutable combined_buffer_pool_stats stats_;
};

}  // namespace vw::gfx

#include "vw/gfx/resource/combined_buffer_pool.inl.h"

#endif  // VW_GFX_COMBINED_BUFFER_POOL_H
