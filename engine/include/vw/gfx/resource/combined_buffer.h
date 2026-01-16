#pragma once

#ifndef VW_GFX_COMBINED_BUFFER_H
#define VW_GFX_COMBINED_BUFFER_H

#include <vulkan/vulkan.h>

#include <memory>
#include <unordered_map>
#include <vector>

#include "vw/core/mat4.h"
#include "vw/core/types.h"
#include "vw/gfx/model/model_identity.h"
#include "vw/gfx/resource/buffer.h"
#include "vw/gfx/resource/mesh.h"
#include "vw/gfx/world/entity.h"

namespace vw::gfx {

class vulkan_context;

struct draw_command {
    uint32 index_count;
    uint32 instance_count;
    uint32 first_index;
    int32 vertex_offset;
    uint32 first_instance;
};

struct buffer_chunk_size {
    uint32 vertex_count;
    uint32 index_count;

    bool operator==(
        const buffer_chunk_size& rhs
    ) const {
        return vertex_count == rhs.vertex_count && index_count == rhs.index_count;
    }

    bool operator<(
        const buffer_chunk_size& rhs
    ) const {
        if (vertex_count != rhs.vertex_count) {
            return vertex_count < rhs.vertex_count;
        }
        return index_count < rhs.index_count;
    }
};

struct free_slot {
    uint32 vertex_offset;
    uint32 index_offset;
};

struct entity_allocation {
    uint32 instance_index;
    uint32 model_index;
};

struct mesh_allocation {
    uint32 vertex_offset;
    uint32 index_offset;
    uint32 vertex_count;
    uint32 index_count;
    uint32 generation;
    uint32 ref_count;
};

struct combined_buffer_stats {
    buffer_chunk_size chunk_size{};
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
};

class combined_buffer {
public:
    explicit combined_buffer(
        vulkan_context& context,
        const buffer_chunk_size& chunk_size,
        VkDescriptorPool descriptor_pool,
        VkDescriptorSetLayout descriptor_set_layout
    );
    ~combined_buffer();

    combined_buffer(const combined_buffer&)            = delete;
    combined_buffer& operator=(const combined_buffer&) = delete;

    combined_buffer(combined_buffer&&) noexcept            = default;
    combined_buffer& operator=(combined_buffer&&) noexcept = default;

    void allocate(
        entity e, model_identity model_id, const mesh& mesh_data, const mat4f& transform_matrix
    );
    void write_mesh(model_identity model_id, const mesh& mesh_data);
    void write_transform(entity ent, const mat4f& transform_matrix);
    void free(entity ent);

    [[nodiscard]] VkBuffer get_vertex_buffer() const;
    [[nodiscard]] VkBuffer get_index_buffer() const;
    [[nodiscard]] VkBuffer get_instance_index_buffer() const;
    [[nodiscard]] VkBuffer get_indirect_draw_buffer() const;
    [[nodiscard]] VkBuffer get_model_matrix_buffer() const;
    [[nodiscard]] uint32 get_draw_command_count() const;
    [[nodiscard]] bool is_empty() const;
    [[nodiscard]] const combined_buffer_stats& get_stats() const;
    [[nodiscard]] VkDescriptorSet get_descriptor_set() const {
        return descriptor_set_;
    }

private:
    void expand_mesh_buffers_();
    void expand_instance_buffers_();

    static constexpr uint32_t default_mesh_capacity_     = 32;
    static constexpr uint32_t default_instance_capacity_ = 64;

    vulkan_context* context_;
    buffer_chunk_size chunk_size_;

    uint32 mesh_capacity_;
    std::unique_ptr<vertex_buffer> vertex_buffer_;
    std::unique_ptr<index_buffer> index_buffer_;
    std::unique_ptr<storage_buffer> instance_index_buffer_;

    uint32 instance_capacity_;
    std::unique_ptr<storage_buffer> model_matrix_buffer_;
    std::unique_ptr<storage_buffer> indirect_draw_buffer_;

    std::unordered_map<entity, entity_allocation> entity_allocations_;
    std::unordered_map<uint32, mesh_allocation> mesh_allocations_;
    std::unordered_map<uint32, entity> instance_indexes_;
    std::vector<free_slot> free_slots_;
    uint32_t vertex_used_;
    uint32_t index_used_;

    VkDescriptorSet descriptor_set_              = VK_NULL_HANDLE;
    VkDescriptorPool descriptor_pool_            = VK_NULL_HANDLE;
    VkDescriptorSetLayout descriptor_set_layout_ = VK_NULL_HANDLE;

    mutable combined_buffer_stats stats_;
};

}  // namespace vw::gfx

#include "vw/gfx/resource/combined_buffer.inl.h"

#endif  // VW_GFX_COMBINED_BUFFER_H
