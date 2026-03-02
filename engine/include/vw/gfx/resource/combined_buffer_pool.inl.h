#pragma once

#ifndef VW_GFX_COMBINED_BUFFER_POOL_INL_H
#define VW_GFX_COMBINED_BUFFER_POOL_INL_H

#include <algorithm>
#include <numeric>
#include <ranges>

namespace vw::gfx {

template <typename C>
combined_buffer_pool<C>::combined_buffer_pool(
    vulkan_context& context,
    VkDescriptorPool descriptor_pool,
    VkDescriptorSetLayout descriptor_set_layout
)
    : context_(&context)
    , staging_(context)
    , descriptor_pool_(descriptor_pool)
    , descriptor_set_layout_(descriptor_set_layout) {}

template <typename C>
void combined_buffer_pool<C>::update(
    world_type& world,
    const camera& camera,
    std::span<const frustum> shadow_frustums,
    VkCommandBuffer cmd
) {
    staging_.begin_frame();

    for (auto ent : world.destroyed()) {
        if (entity_buffer_infos_.contains(ent)) {
            auto& info = entity_buffer_infos_[ent];
            auto swapped = buffers_[info.buffer_index]->free(ent);
            if (swapped && world.template has_component<transform_component>(*swapped)) {
                auto& tc = world.template get_component<transform_component>(*swapped);
                buffers_[info.buffer_index]->write_transform(*swapped, tc.get_world_matrix());
            }
            entity_buffer_infos_.erase(ent);
        }
        mesh_pending_entities_.erase(ent);
        transform_pending_entities_.erase(ent);
        visibility_cache_.visible.erase(ent);
    }

    const frustum& view_frustum = camera.get_frustum();
    update_visibility_cache_(world, view_frustum, shadow_frustums);
    update_meshes_(world);
    update_transforms_(world);

    staging_.flush(cmd);
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

    while (vertex_chunk < vertex_count || index_chunk < index_count) {
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
            *context_, chunk_size, descriptor_pool_, descriptor_set_layout_, staging_
        )
    );
    chunk_size_to_buffer_index_[chunk_size] = buffer_index;

    return buffers_[buffer_index].get();
}

template <typename C>
void combined_buffer_pool<C>::update_meshes_(
    world_type& world
) {
    auto& model_changed = world.template changed<model_component>();
    mesh_pending_entities_.insert(model_changed.begin(), model_changed.end());

    entities_to_process_.clear();
    entities_to_process_.insert(visibility_cache_.changed.begin(), visibility_cache_.changed.end());
    entities_to_process_.insert(mesh_pending_entities_.begin(), mesh_pending_entities_.end());

    for (entity ent : entities_to_process_) {
        const bool has_spatial = world.template has_component<spatial_component>(ent);
        const bool is_visible  = visibility_cache_.visible.contains(ent);
        if (has_spatial && !is_visible) {
            if (entity_buffer_infos_.contains(ent)) {
                const auto& [chunk_size, buffer_index] = entity_buffer_infos_[ent];
                auto swapped = buffers_[buffer_index]->free(ent);
                if (swapped && world.template has_component<transform_component>(*swapped)) {
                    auto& tc = world.template get_component<transform_component>(*swapped);
                    buffers_[buffer_index]->write_transform(*swapped, tc.get_world_matrix());
                }
                entity_buffer_infos_.erase(ent);
            }
            mesh_pending_entities_.erase(ent);
            continue;
        }

        bool has_model     = world.template has_component<model_component>(ent);
        bool has_transform = world.template has_component<transform_component>(ent);
        bool is_renderable = has_model && has_transform;

        if (!is_renderable) {
            if (entity_buffer_infos_.contains(ent)) {
                auto& buffer_info = entity_buffer_infos_[ent];
                auto swapped = buffers_[buffer_info.buffer_index]->free(ent);
                if (swapped && world.template has_component<transform_component>(*swapped)) {
                    auto& tc = world.template get_component<transform_component>(*swapped);
                    buffers_[buffer_info.buffer_index]->write_transform(
                        *swapped, tc.get_world_matrix()
                    );
                }
                entity_buffer_infos_.erase(ent);
            }
            mesh_pending_entities_.erase(ent);
            continue;
        }

        const auto& model_comp = world.template get_component<model_component>(ent);
        if (!model_comp.has_model()) {
            continue;
        }

        auto model_id = model_comp.get_identity();
        auto mesh_ptr = world.get_mesh_pool().get(model_id);
        if (!mesh_ptr) {
            continue;
        }

        auto vertex_count = mesh_ptr->vertices.size();
        auto index_count  = mesh_ptr->indices.size();

        buffer_chunk_size required_chunk_size = get_chunk_size_for_mesh(vertex_count, index_count);

        const VkDeviceSize mesh_staging_cost =
            required_chunk_size.vertex_count * sizeof(vertex) +
            required_chunk_size.index_count * sizeof(uint32) +
            sizeof(uint32) + sizeof(draw_command) + sizeof(mat4f);

        const auto& transform_comp    = world.template get_component<transform_component>(ent);
        const mat4f& transform_matrix = transform_comp.get_world_matrix();

        if (entity_buffer_infos_.contains(ent)) {
            auto& buffer_info = entity_buffer_infos_[ent];
            auto& buffer      = buffers_[buffer_info.buffer_index];

            if (buffer_info.chunk_size == required_chunk_size) {
                const auto& ent_alloc = buffer->get_entity_allocation(ent);
                if (ent_alloc.model_index == model_id.index) {
                    if (staging_.available() < mesh_staging_cost) {
                        mesh_pending_entities_.insert(ent);
                        continue;
                    }
                    buffer->write_mesh(model_id, *mesh_ptr);
                    mesh_pending_entities_.erase(ent);
                    continue;
                }
            }

            if (staging_.available() < mesh_staging_cost) {
                mesh_pending_entities_.insert(ent);
                continue;
            }

            auto swapped = buffer->free(ent);
            if (swapped && world.template has_component<transform_component>(*swapped)) {
                auto& tc = world.template get_component<transform_component>(*swapped);
                buffer->write_transform(*swapped, tc.get_world_matrix());
            }
        } else if (staging_.available() < mesh_staging_cost) {
            mesh_pending_entities_.insert(ent);
            continue;
        }

        auto* buffer            = get_or_create_buffer(required_chunk_size);
        const auto buffer_index = chunk_size_to_buffer_index_[required_chunk_size];

        buffer->allocate(ent, model_id, *mesh_ptr, transform_matrix);

        entity_buffer_infos_[ent] = entity_buffer_info{required_chunk_size, buffer_index};
        mesh_pending_entities_.erase(ent);
    }
}

template <typename C>
void combined_buffer_pool<C>::update_transforms_(
    world_type& world
) {
    auto& transform_changed = world.template changed<transform_component>();
    transform_pending_entities_.insert(transform_changed.begin(), transform_changed.end());

    entities_to_process_.clear();
    entities_to_process_.insert(visibility_cache_.changed.begin(), visibility_cache_.changed.end());
    entities_to_process_.insert(
        transform_pending_entities_.begin(), transform_pending_entities_.end()
    );

    for (entity ent : entities_to_process_) {
        const bool has_spatial = world.template has_component<spatial_component>(ent);
        const bool is_visible  = visibility_cache_.visible.contains(ent);
        if (has_spatial && !is_visible) {
            transform_pending_entities_.erase(ent);
            continue;
        }

        const bool has_model     = world.template has_component<model_component>(ent);
        const bool has_transform = world.template has_component<transform_component>(ent);
        const bool is_renderable = has_model && has_transform;

        if (entity_buffer_infos_.contains(ent) && is_renderable) {
            if (staging_.available() < sizeof(mat4f)) {
                transform_pending_entities_.insert(ent);
                continue;
            }

            auto& info = entity_buffer_infos_[ent];

            const auto& transform_comp    = world.template get_component<transform_component>(ent);
            const mat4f& transform_matrix = transform_comp.get_world_matrix();
            buffers_[info.buffer_index]->write_transform(ent, transform_matrix);

            transform_pending_entities_.erase(ent);
        }
    }
}

template <typename C>
void combined_buffer_pool<C>::update_visibility_cache_(
    world_type& world, const frustum& view_frustum, std::span<const frustum> shadow_frustums
) {
    auto& spatial_system = world.get_spatial_system();

    constexpr float angle_threshold    = math::radians(2.f);
    constexpr float distance_threshold = 0.5f;

    const bool has_spatial_changed = !world.template changed<spatial_component>().empty();

    const bool frustum_changed = !visibility_cache_.view_frustum.approximately_equal(
        view_frustum, angle_threshold, distance_threshold
    );

    visibility_cache_.changed.clear();
    if (frustum_changed || has_spatial_changed) {
        spatial_system.query_all(view_frustum, visibility_cache_.tmp_visible);

        for (const auto& shadow_frustum : shadow_frustums) {
            spatial_system.query_all(shadow_frustum, shadow_query_tmp_);
            visibility_cache_.tmp_visible.insert(
                shadow_query_tmp_.begin(), shadow_query_tmp_.end()
            );
        }

        for (auto ent : visibility_cache_.visible) {
            if (!visibility_cache_.tmp_visible.contains(ent)) {
                visibility_cache_.changed.insert(ent);
            }
        }
        for (auto ent : visibility_cache_.tmp_visible) {
            if (!visibility_cache_.visible.contains(ent)) {
                visibility_cache_.changed.insert(ent);
            }
        }

        visibility_cache_.visible      = visibility_cache_.tmp_visible;
        visibility_cache_.view_frustum = view_frustum;
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
