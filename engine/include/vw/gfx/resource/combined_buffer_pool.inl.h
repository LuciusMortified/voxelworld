#pragma once

#ifndef VW_GFX_COMBINED_BUFFER_POOL_INL_H
#define VW_GFX_COMBINED_BUFFER_POOL_INL_H

#include <algorithm>
#include <numeric>

namespace vw::gfx {

template <typename C>
combined_buffer_pool<C>::combined_buffer_pool(
    vulkan_context& context,
    VkDescriptorPool descriptor_pool,
    VkDescriptorSetLayout descriptor_set_layout
)
    : context_(&context)
    , descriptor_pool_(descriptor_pool)
    , descriptor_set_layout_(descriptor_set_layout) {}

template <typename C>
void combined_buffer_pool<C>::update(
    world_type& world
) {
    update_meshes_(world);
    update_transforms_(world);
}

template <typename C>
const std::vector<std::unique_ptr<combined_buffer>>& combined_buffer_pool<C>::get_buffers() const {
    return buffers_;
}

template <typename C>
buffer_chunk_size combined_buffer_pool<C>::get_chunk_size_for_mesh(
    uint32 vertex_count, uint32 index_count
) {
    uint32 vertex_chunk = 256;
    uint32 index_chunk  = 512;

    while (vertex_chunk < vertex_count && index_chunk < index_count) {
        vertex_chunk *= 2;
        index_chunk *= 2;
    }

    return buffer_chunk_size{vertex_chunk, index_chunk};
}

template <typename C>
combined_buffer* combined_buffer_pool<C>::get_or_create_buffer(
    const buffer_chunk_size& chunk_size
) {
    if (chunk_size_to_buffer_index_.contains(chunk_size)) {
        auto buffer_index = chunk_size_to_buffer_index_[chunk_size];
        return buffers_[buffer_index].get();
    }

    auto buffer_index = buffers_.size();
    buffers_.push_back(
        std::make_unique<combined_buffer>(
            *context_, chunk_size, descriptor_pool_, descriptor_set_layout_
        )
    );
    chunk_size_to_buffer_index_[chunk_size] = buffer_index;

    return buffers_[buffer_index].get();
}

template <typename C>
void combined_buffer_pool<C>::update_meshes_(
    world_type& world
) {
    auto& model_system   = world.get_model_system();
    auto& dirty_entities = model_system.get_render_dirty_entities();
    for (auto it = dirty_entities.begin(); it != dirty_entities.end();) {
        auto ent           = *it;
        bool has_model     = world.template has_component<model_component>(ent);
        bool has_transform = world.template has_component<transform_component>(ent);
        bool is_renderable = has_model && has_transform;

        if (!is_renderable) {
            if (entity_buffer_infos_.contains(ent)) {
                auto& buffer_info = entity_buffer_infos_[ent];
                buffers_[buffer_info.buffer_index]->free(ent);
                entity_buffer_infos_.erase(ent);
            }
            ++it;
            continue;
        }

        const auto& model_comp = world.template get_component<model_component>(ent);
        if (!model_comp.has_model()) {
            ++it;
            continue;
        }

        auto model_id = model_comp.get_identity();
        auto mesh_ptr = world.get_mesh_pool().get(model_id);
        if (!mesh_ptr) {
            ++it;
            continue;
        }

        auto vertex_count = mesh_ptr->vertices.size();
        auto index_count  = mesh_ptr->indices.size();

        buffer_chunk_size required_chunk_size = get_chunk_size_for_mesh(vertex_count, index_count);

        if (entity_buffer_infos_.contains(ent)) {
            auto& buffer_info = entity_buffer_infos_[ent];
            if (buffer_info.chunk_size == required_chunk_size) {
                buffers_[buffer_info.buffer_index]->write_mesh(model_id, *mesh_ptr);
                ++it;
                continue;
            }

            buffers_[buffer_info.buffer_index]->free(ent);
        }

        combined_buffer* buffer = get_or_create_buffer(required_chunk_size);
        auto buffer_index       = chunk_size_to_buffer_index_[required_chunk_size];

        const auto& transform_comp    = world.template get_component<transform_component>(ent);
        const mat4f& transform_matrix = transform_comp.get_world_matrix();
        buffer->allocate(ent, model_id, *mesh_ptr, transform_matrix);

        entity_buffer_infos_[ent] = entity_buffer_info{required_chunk_size, buffer_index};

        it = dirty_entities.erase(it);
    }
}

template <typename C>
void combined_buffer_pool<C>::update_transforms_(
    world_type& world
) {
    auto& transform_system = world.get_transform_system();
    auto& dirty_entities   = transform_system.get_render_dirty_entities();
    for (auto it = dirty_entities.begin(); it != dirty_entities.end();) {
        auto ent           = *it;
        bool has_model     = world.template has_component<model_component>(ent);
        bool has_transform = world.template has_component<transform_component>(ent);
        bool is_renderable = has_model && has_transform;

        if (entity_buffer_infos_.contains(ent) && is_renderable) {
            auto& info = entity_buffer_infos_[ent];

            const auto& transform_comp    = world.template get_component<transform_component>(ent);
            const mat4f& transform_matrix = transform_comp.get_world_matrix();
            buffers_[info.buffer_index]->write_transform(ent, transform_matrix);

            it = dirty_entities.erase(it);
        } else {
            ++it;
        }
    }
}

template <typename C>
const combined_buffer_pool_stats& combined_buffer_pool<C>::get_stats() const {
    stats_.vertex_load_min   = 0.0f;
    stats_.vertex_load_max   = 0.0f;
    stats_.vertex_load_avg   = 0.0f;
    stats_.index_load_min    = 0.0f;
    stats_.index_load_max    = 0.0f;
    stats_.index_load_avg    = 0.0f;
    stats_.mesh_capacity     = 0;
    stats_.mesh_count        = 0;
    stats_.instance_capacity = 0;
    stats_.instance_count    = 0;
    stats_.buffers.clear();

    if (buffers_.empty()) {
        return stats_;
    }

    float32 vertex_load_avg_sum = 0.0f;
    float32 index_load_avg_sum  = 0.0f;

    for (const auto& buffer : buffers_) {
        const auto& buffer_stats = buffer->get_stats();

        stats_.buffers.push_back(buffer_stats);

        if (buffer_stats.vertex_load_min < stats_.vertex_load_min ||
            stats_.vertex_load_min == 0.0f) {
            stats_.vertex_load_min = buffer_stats.vertex_load_min;
        }
        if (buffer_stats.vertex_load_max > stats_.vertex_load_max) {
            stats_.vertex_load_max = buffer_stats.vertex_load_max;
        }
        if (buffer_stats.index_load_min < stats_.index_load_min || stats_.index_load_min == 0.0f) {
            stats_.index_load_min = buffer_stats.index_load_min;
        }
        if (buffer_stats.index_load_max > stats_.index_load_max) {
            stats_.index_load_max = buffer_stats.index_load_max;
        }

        vertex_load_avg_sum += buffer_stats.vertex_load_avg;
        index_load_avg_sum += buffer_stats.index_load_avg;

        stats_.mesh_capacity += buffer_stats.mesh_capacity;
        stats_.mesh_count += buffer_stats.mesh_count;
        stats_.instance_capacity += buffer_stats.instance_capacity;
        stats_.instance_count += buffer_stats.instance_count;
    }

    stats_.vertex_load_avg = vertex_load_avg_sum / buffers_.size();
    stats_.index_load_avg  = index_load_avg_sum / buffers_.size();

    return stats_;
}

}  // namespace vw::gfx

#endif  // VW_GFX_COMBINED_BUFFER_POOL_INL_H
