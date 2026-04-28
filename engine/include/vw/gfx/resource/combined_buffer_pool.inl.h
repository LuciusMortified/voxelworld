#pragma once

#ifndef VW_GFX_COMBINED_BUFFER_POOL_INL_H
#define VW_GFX_COMBINED_BUFFER_POOL_INL_H

#include <algorithm>
#include <numeric>
#include <ranges>

#include "vw/core/timing.h"
#include "vw/log/logger.h"

namespace vw::gfx {

namespace {

constexpr log::log_category lc_cbp_{"cbuf_pool"};

inline void sorted_insert(std::vector<entity>& v, entity e) {
    auto it = std::lower_bound(v.begin(), v.end(), e);
    if (it == v.end() || *it != e) {
        v.insert(it, e);
    }
}

inline void sorted_erase(std::vector<entity>& v, entity e) {
    auto it = std::lower_bound(v.begin(), v.end(), e);
    if (it != v.end() && *it == e) {
        v.erase(it);
    }
}

template <typename Iter>
inline void sorted_merge_range(
    std::vector<entity>& dst, Iter first, Iter last
) {
    if (first == last) return;
    auto old_size = static_cast<std::ptrdiff_t>(dst.size());
    dst.insert(dst.end(), first, last);
    std::sort(dst.begin() + old_size, dst.end());
    std::inplace_merge(dst.begin(), dst.begin() + old_size, dst.end());
    dst.erase(std::unique(dst.begin(), dst.end()), dst.end());
}

}  // namespace

template <typename C>
combined_buffer_pool<C>::combined_buffer_pool(
    vulkan_context& context,
    VkDescriptorPool descriptor_pool,
    VkDescriptorSetLayout descriptor_set_layout,
    VkDescriptorSetLayout compute_descriptor_set_layout
)
    : context_(&context)
    , staging_(context, 32 * 1024 * 1024)
    , descriptor_pool_(descriptor_pool)
    , descriptor_set_layout_(descriptor_set_layout)
    , compute_descriptor_set_layout_(compute_descriptor_set_layout) {}

template <typename C>
void combined_buffer_pool<C>::update(
    world_type& world,
    const camera& camera,
    VkCommandBuffer cmd,
    mesh_pool& pool
) {
    staging_.begin_frame();

    stats_.timing.destroyed_ms = measure_ms([&] {
        process_destroyed_(world);
    });

    stats_.timing.meshes_ms = measure_ms([&] {
        update_meshes_(world, camera.get_position(), pool);
    });

    stats_.timing.transforms_ms = measure_ms([&] {
        update_transforms_(world);
    });

    stats_.timing.staging_flush_ms = measure_ms([&] {
        staging_.flush(cmd);
    });
}

template <typename C>
const std::vector<std::unique_ptr<combined_buffer>>& combined_buffer_pool<C>::get_buffers() const {
    return buffers_;
}

template <typename C>
void combined_buffer_pool<C>::process_destroyed_(world_type& world) {
    for (auto ent : world.destroyed()) {
        if (entity_buffer_infos_.contains(ent)) {
            auto& info = entity_buffer_infos_[ent];
            auto swapped = buffers_[info.buffer_index]->free(ent);
            if (swapped && world.template has<transform_component>(*swapped)) {
                auto& tc = world.template get<transform_component>(*swapped);
                vw::spatial::aabb bounds{};
                if (world.template has<spatial_component>(*swapped)) {
                    bounds = world.template get<spatial_component>(*swapped)
                                 .get_bounds();
                }
                buffers_[info.buffer_index]->write_transform(
                    *swapped, tc.get_world_matrix(), bounds);
            }
            entity_buffer_infos_.erase(ent);
        }
        sorted_erase(mesh_pending_entities_, ent);
        sorted_erase(transform_pending_entities_, ent);
    }
}

template <typename C>
buffer_chunk_size combined_buffer_pool<C>::get_chunk_size_for_mesh(
    uint32 vertex_count, uint32 index_count
) {
    uint32 vertex_chunk = 256;
    while (vertex_chunk < vertex_count) {
        vertex_chunk *= 2;
    }

    uint32 index_chunk = 512;
    while (index_chunk < index_count) {
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
            *context_, chunk_size, descriptor_pool_, descriptor_set_layout_,
            compute_descriptor_set_layout_, staging_
        )
    );
    chunk_size_to_buffer_index_[chunk_size] = buffer_index;

    return buffers_[buffer_index].get();
}

template <typename C>
void combined_buffer_pool<C>::update_meshes_(
    world_type& world, const vec3f& camera_pos, mesh_pool& pool
) {
    auto& model_changed = world.template changed<model_component>();
    sorted_merge_range(mesh_pending_entities_, model_changed.begin(), model_changed.end());

    entities_to_process_.assign(
        mesh_pending_entities_.begin(), mesh_pending_entities_.end());

    std::sort(entities_to_process_.begin(), entities_to_process_.end(),
        [&](entity a, entity b) {
            auto get_dist_sq = [&](entity e) -> float32 {
                if (!world.template has<spatial_component>(e)) {
                    return 0.0f;
                }
                auto& sc = world.template get<spatial_component>(e);
                auto diff = sc.get_bounds().center() - camera_pos;
                return diff.x * diff.x + diff.y * diff.y + diff.z * diff.z;
            };
            return get_dist_sq(a) < get_dist_sq(b);
        });

    constexpr uint32 estimated_avg_mesh_cost = 32 * 1024;
    const uint32 max_mesh_writes = std::clamp(
        static_cast<uint32>(staging_.available() / estimated_avg_mesh_cost), 4u, 16u);
    uint32 mesh_writes = 0;

    merge_buffer_.clear();
    merge_buffer_.reserve(entities_to_process_.size());

    for (size_t i = 0; i < entities_to_process_.size(); ++i) {
        entity ent = entities_to_process_[i];

        if (mesh_writes >= max_mesh_writes) {
            merge_buffer_.insert(
                merge_buffer_.end(),
                entities_to_process_.begin() + static_cast<std::ptrdiff_t>(i),
                entities_to_process_.end());
            break;
        }

        const bool has_model     = world.template has<model_component>(ent);
        const bool has_transform = world.template has<transform_component>(ent);

        if (!has_model || !has_transform) {
            if (entity_buffer_infos_.contains(ent)) {
                auto& buffer_info = entity_buffer_infos_[ent];
                auto swapped = buffers_[buffer_info.buffer_index]->free(ent);
                if (swapped && world.template has<transform_component>(*swapped)) {
                    auto& tc = world.template get<transform_component>(*swapped);
                    vw::spatial::aabb swap_bounds{};
                    if (world.template has<spatial_component>(*swapped)) {
                        swap_bounds = world.template get<spatial_component>(*swapped)
                                          .get_bounds();
                    }
                    buffers_[buffer_info.buffer_index]->write_transform(
                        *swapped, tc.get_world_matrix(), swap_bounds);
                }
                entity_buffer_infos_.erase(ent);
            }
            continue;
        }

        const auto& model_comp = world.template get<model_component>(ent);
        if (!model_comp.has_model()) {
            merge_buffer_.push_back(ent);
            continue;
        }

        auto model_id = model_comp.get_identity();
        auto mesh_ptr = pool.get(model_id);
        if (!mesh_ptr) {
            merge_buffer_.push_back(ent);
            continue;
        }

        auto vertex_count = mesh_ptr->vertices.size();
        auto index_count  = mesh_ptr->indices.size();

        if (vertex_count == 0 || index_count == 0) {
            continue;
        }

        buffer_chunk_size required_chunk_size = get_chunk_size_for_mesh(vertex_count, index_count);

        const VkDeviceSize mesh_staging_cost =
            required_chunk_size.vertex_count * sizeof(vertex) +
            required_chunk_size.index_count * sizeof(uint32) +
            sizeof(uint32) + sizeof(draw_command) + sizeof(mat4f) * 2;

        const auto& transform_comp    = world.template get<transform_component>(ent);
        const mat4f& transform_matrix = transform_comp.get_world_matrix();

        if (entity_buffer_infos_.contains(ent)) {
            auto& buffer_info = entity_buffer_infos_[ent];
            auto& buffer      = buffers_[buffer_info.buffer_index];

            if (buffer_info.chunk_size == required_chunk_size) {
                const auto& ent_alloc = buffer->get_entity_allocation(ent);
                if (ent_alloc.model_index == model_id.index) {
                    if (staging_.available() < mesh_staging_cost) {
                        merge_buffer_.push_back(ent);
                        continue;
                    }
                    buffer->write_mesh(model_id, *mesh_ptr);
                    pool.evict(model_id);
                    ++mesh_writes;
                    continue;
                }
            }

            if (staging_.available() < mesh_staging_cost) {
                merge_buffer_.push_back(ent);
                continue;
            }

            auto swapped = buffer->free(ent);
            if (swapped && world.template has<transform_component>(*swapped)) {
                auto& tc = world.template get<transform_component>(*swapped);
                vw::spatial::aabb sw_bounds{};
                if (world.template has<spatial_component>(*swapped)) {
                    sw_bounds = world.template get<spatial_component>(*swapped)
                                    .get_bounds();
                }
                buffer->write_transform(*swapped, tc.get_world_matrix(), sw_bounds);
            }
        } else if (staging_.available() < mesh_staging_cost) {
            merge_buffer_.push_back(ent);
            continue;
        }

        auto* buffer            = get_or_create_buffer(required_chunk_size);
        const auto buffer_index = chunk_size_to_buffer_index_[required_chunk_size];

        vw::spatial::aabb ent_bounds{};
        if (world.template has<spatial_component>(ent)) {
            ent_bounds = world.template get<spatial_component>(ent).get_bounds();
        }
        buffer->allocate(ent, model_id, *mesh_ptr, transform_matrix, ent_bounds);
        pool.evict(model_id);

        entity_buffer_infos_[ent] = entity_buffer_info{required_chunk_size, buffer_index};
        ++mesh_writes;
    }

    std::sort(merge_buffer_.begin(), merge_buffer_.end());
    mesh_pending_entities_.swap(merge_buffer_);
}

template <typename C>
void combined_buffer_pool<C>::update_transforms_(
    world_type& world
) {
    auto& transform_changed = world.template changed<transform_component>();
    sorted_merge_range(
        transform_pending_entities_, transform_changed.begin(), transform_changed.end());

    entities_to_process_.assign(
        transform_pending_entities_.begin(), transform_pending_entities_.end());

    merge_buffer_.clear();
    merge_buffer_.reserve(entities_to_process_.size());

    for (size_t i = 0; i < entities_to_process_.size(); ++i) {
        entity ent = entities_to_process_[i];

        if (!entity_buffer_infos_.contains(ent)) {
            continue;
        }

        const bool has_model     = world.template has<model_component>(ent);
        const bool has_transform = world.template has<transform_component>(ent);
        if (!has_model || !has_transform) {
            continue;
        }

        if (staging_.available() < sizeof(mat4f) * 2) {
            merge_buffer_.insert(
                merge_buffer_.end(),
                entities_to_process_.begin() + static_cast<std::ptrdiff_t>(i),
                entities_to_process_.end());
            break;
        }

        auto& info = entity_buffer_infos_[ent];
        const auto& transform_comp = world.template get<transform_component>(ent);
        vw::spatial::aabb tr_bounds{};
        if (world.template has<spatial_component>(ent)) {
            tr_bounds = world.template get<spatial_component>(ent).get_bounds();
        }
        buffers_[info.buffer_index]->write_transform(
            ent, transform_comp.get_world_matrix(), tr_bounds);
    }

    std::sort(merge_buffer_.begin(), merge_buffer_.end());
    transform_pending_entities_.swap(merge_buffer_);
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
