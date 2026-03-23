#pragma once

#ifndef VW_GFX_COMBINED_BUFFER_INL_H
#define VW_GFX_COMBINED_BUFFER_INL_H
#include <array>
#include <numeric>
#include <ranges>

#include "vw/log/logger.h"

namespace vw::gfx {

namespace {
constexpr log::log_category lc_cb_{"combined_buffer"};
}
inline combined_buffer::~combined_buffer() {
    if (descriptor_set_ != VK_NULL_HANDLE && descriptor_pool_ != VK_NULL_HANDLE) {
        vkFreeDescriptorSets(context_->get_device(), descriptor_pool_, 1, &descriptor_set_);
    }
}

inline combined_buffer::combined_buffer(
    vulkan_context& context,
    const buffer_chunk_size& chunk_size,
    VkDescriptorPool descriptor_pool,
    VkDescriptorSetLayout descriptor_set_layout,
    staging_buffer& staging
)
    : context_(&context)
    , staging_(&staging)
    , chunk_size_(chunk_size)
    , descriptor_pool_(descriptor_pool)
    , descriptor_set_layout_(descriptor_set_layout) {
    vertex_buffer_ = std::make_unique<device_vertex_buffer>(
        *context_, mesh_capacity_ * chunk_size_.vertex_count * sizeof(vertex)
    );
    index_buffer_ = std::make_unique<device_index_buffer>(
        *context_, mesh_capacity_ * chunk_size_.index_count * sizeof(uint32)
    );
    instance_index_buffer_ = std::make_unique<device_storage_buffer>(
        *context_,
        instance_capacity_ * sizeof(uint32),
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT
    );
    model_matrix_buffer_ = std::make_unique<device_storage_buffer>(
        *context_, instance_capacity_ * sizeof(mat4f)
    );
    normal_matrix_buffer_ = std::make_unique<device_storage_buffer>(
        *context_, instance_capacity_ * sizeof(mat4f)
    );
    indirect_draw_buffer_ = std::make_unique<device_storage_buffer>(
        *context_,
        instance_capacity_ * sizeof(draw_command),
        VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT
    );

    VkDescriptorSetAllocateInfo alloc_info{};
    alloc_info.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    alloc_info.descriptorPool     = descriptor_pool_;
    alloc_info.descriptorSetCount = 1;
    alloc_info.pSetLayouts        = &descriptor_set_layout_;

    if (vkAllocateDescriptorSets(context_->get_device(), &alloc_info, &descriptor_set_) !=
        VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate descriptor set for combined_buffer!");
    }

    update_descriptor_set_();
}

inline void combined_buffer::allocate(
    entity e, model_identity model_id, const mesh& mesh_data, const mat4f& transform_matrix
) {
    log::debug(
        lc_cb_,
        "ALLOCATE entity {}.{} model {}.{} verts {} idxs {}",
        e.index, e.generation, model_id.index, model_id.generation,
        mesh_data.vertices.size(), mesh_data.indices.size()
    );

    if (!mesh_allocations_.contains(model_id.index)) {
        allocate_mesh(model_id, mesh_data);
    } else {
        write_mesh(model_id, mesh_data);
    }

    auto& mesh_alloc = mesh_allocations_[model_id.index];

    const uint32 instance_index = entity_allocations_.size();
    if (instance_index >= instance_capacity_) {
        expand_instance_buffers_();
    }

    const draw_command cmd{
        .index_count    = chunk_size_.index_count,
        .instance_count = 1,
        .first_index    = mesh_alloc.index_offset,
        .vertex_offset  = static_cast<int32>(mesh_alloc.vertex_offset),
        .first_instance = instance_index,
    };

    const auto instance_staged = staging_->stage_struct(instance_index);
    staging_->copy_to(
        instance_index_buffer_->get_buffer(),
        instance_index * sizeof(uint32),
        instance_staged,
        sizeof(uint32)
    );

    const auto draw_cmd_staged = staging_->stage_struct(cmd);
    staging_->copy_to(
        indirect_draw_buffer_->get_buffer(),
        instance_index * sizeof(draw_command),
        draw_cmd_staged,
        sizeof(draw_command)
    );

    const auto model_staged = staging_->stage_struct(transform_matrix);
    staging_->copy_to(
        model_matrix_buffer_->get_buffer(),
        instance_index * sizeof(mat4f),
        model_staged,
        sizeof(mat4f)
    );

    const auto normal_matrix =
        math::transpose_matrix(math::inverse_matrix(transform_matrix).value_or(transform_matrix));
    const auto normal_staged = staging_->stage_struct(normal_matrix);
    staging_->copy_to(
        normal_matrix_buffer_->get_buffer(),
        instance_index * sizeof(mat4f),
        normal_staged,
        sizeof(mat4f)
    );

    const entity_allocation ent_alloc{
        .instance_index = instance_index,
        .model_index    = model_id.index,
    };
    entity_allocations_[e] = ent_alloc;

    instance_indexes_[instance_index] = e;

    mesh_alloc.ref_count++;
}

inline void combined_buffer::allocate_mesh(
    model_identity model_id, const mesh& mesh_data
) {
    log::debug(
        lc_cb_,
        "ALLOC_MESH model {}.{} verts {} idxs {}",
        model_id.index, model_id.generation,
        mesh_data.vertices.size(), mesh_data.indices.size()
    );

    const auto vertex_count = mesh_data.vertices.size();
    const auto index_count  = mesh_data.indices.size();
    uint32_t vertex_offset  = vertex_used_;
    uint32_t index_offset   = index_used_;

    if (!free_slots_.empty()) {
        const auto& slot = free_slots_.back();
        vertex_offset    = slot.vertex_offset;
        index_offset     = slot.index_offset;
        free_slots_.pop_back();
    } else {
        vertex_used_ += chunk_size_.vertex_count;
        index_used_ += chunk_size_.index_count;
    }

    const bool vertex_out_of_bounds = vertex_used_ > mesh_capacity_ * chunk_size_.vertex_count;
    const bool index_out_of_bounds  = index_used_ > mesh_capacity_ * chunk_size_.index_count;
    if (vertex_out_of_bounds || index_out_of_bounds) {
        expand_mesh_buffers_();
    }

    auto vertices_staged = staging_->stage_vector(mesh_data.vertices);
    staging_->copy_to(
        vertex_buffer_->get_buffer(),
        vertex_offset * sizeof(vertex),
        vertices_staged,
        vertex_count * sizeof(vertex)
    );

    const auto vertex_free_space_offset = (vertex_offset + vertex_count) * sizeof(vertex);
    const auto vertex_free_space_size = (chunk_size_.vertex_count - vertex_count) * sizeof(vertex);
    if (vertex_free_space_size > 0) {
        staging_->zero_region(
            vertex_buffer_->get_buffer(), vertex_free_space_offset, vertex_free_space_size
        );
    }

    const auto indices_staged = staging_->stage_vector(mesh_data.indices);
    staging_->copy_to(
        index_buffer_->get_buffer(),
        index_offset * sizeof(uint32),
        indices_staged,
        index_count * sizeof(uint32)
    );

    const auto index_free_space_offset = (index_offset + index_count) * sizeof(uint32);
    const auto index_free_space_size = (chunk_size_.index_count - index_count) * sizeof(uint32);
    if (index_free_space_size > 0) {
        staging_->zero_region(
            index_buffer_->get_buffer(), index_free_space_offset, index_free_space_size
        );
    }

    mesh_allocation new_mesh_alloc{};
    new_mesh_alloc.vertex_offset = vertex_offset;
    new_mesh_alloc.index_offset  = index_offset;
    new_mesh_alloc.vertex_count  = vertex_count;
    new_mesh_alloc.index_count   = index_count;
    new_mesh_alloc.generation    = model_id.generation;
    new_mesh_alloc.ref_count     = 0;

    mesh_allocations_[model_id.index] = new_mesh_alloc;
}

inline void combined_buffer::write_mesh(
    model_identity model_id, const mesh& mesh_data
) {
    auto& mesh_alloc = mesh_allocations_[model_id.index];
    if (mesh_alloc.generation == model_id.generation) {
        return;
    }

    log::debug(
        lc_cb_,
        "WRITE_MESH model {}.{} (was gen {}) verts {} idxs {}",
        model_id.index, model_id.generation, mesh_alloc.generation,
        mesh_data.vertices.size(), mesh_data.indices.size()
    );

    const auto vertex_count = mesh_data.vertices.size();
    const auto index_count  = mesh_data.indices.size();

    const auto vertices_staged = staging_->stage_vector(mesh_data.vertices);
    staging_->copy_to(
        vertex_buffer_->get_buffer(),
        mesh_alloc.vertex_offset * sizeof(vertex),
        vertices_staged,
        vertex_count * sizeof(vertex)
    );

    const auto vertex_free_space_offset =
        (mesh_alloc.vertex_offset + vertex_count) * sizeof(vertex);
    const auto vertex_free_space_size = (chunk_size_.vertex_count - vertex_count) * sizeof(vertex);
    if (vertex_free_space_size > 0) {
        staging_->zero_region(
            vertex_buffer_->get_buffer(), vertex_free_space_offset, vertex_free_space_size
        );
    }

    const auto indices_staged = staging_->stage_vector(mesh_data.indices);
    staging_->copy_to(
        index_buffer_->get_buffer(),
        mesh_alloc.index_offset * sizeof(uint32_t),
        indices_staged,
        index_count * sizeof(uint32)
    );

    const auto index_free_space_offset = (mesh_alloc.index_offset + index_count) * sizeof(uint32);
    const auto index_free_space_size   = (chunk_size_.index_count - index_count) * sizeof(uint32);
    if (index_free_space_size > 0) {
        staging_->zero_region(
            index_buffer_->get_buffer(), index_free_space_offset, index_free_space_size
        );
    }

    mesh_alloc.vertex_count = vertex_count;
    mesh_alloc.index_count  = index_count;
    mesh_alloc.generation   = model_id.generation;
}

inline void combined_buffer::write_transform(
    entity ent, const mat4f& transform_matrix
) {
    auto& [instance_index, model_index] = entity_allocations_[ent];
    const auto model_staged = staging_->stage_struct(transform_matrix);
    staging_->copy_to(
        model_matrix_buffer_->get_buffer(),
        instance_index * sizeof(mat4f),
        model_staged,
        sizeof(mat4f)
    );

    const auto normal_matrix =
        math::transpose_matrix(math::inverse_matrix(transform_matrix).value_or(transform_matrix));
    const auto normal_staged = staging_->stage_struct(normal_matrix);
    staging_->copy_to(
        normal_matrix_buffer_->get_buffer(),
        instance_index * sizeof(mat4f),
        normal_staged,
        sizeof(mat4f)
    );
}

inline void combined_buffer::write_visibility(entity ent, bool visible) {
    auto& [instance_index, model_index] = entity_allocations_[ent];
    const uint32 instance_count = visible ? 1 : 0;
    const auto staged = staging_->stage_struct(instance_count);
    staging_->copy_to(
        indirect_draw_buffer_->get_buffer(),
        instance_index * sizeof(draw_command) + offsetof(draw_command, instance_count),
        staged,
        sizeof(uint32)
    );
}

inline auto combined_buffer::free(
    entity ent
) -> std::optional<entity> {
    auto& ent_alloc = entity_allocations_[ent];

    log::debug(
        lc_cb_,
        "FREE entity {}.{} model_idx {} instance_idx {}",
        ent.index, ent.generation,
        ent_alloc.model_index, ent_alloc.instance_index
    );

    auto& mesh_alloc = mesh_allocations_[ent_alloc.model_index];
    mesh_alloc.ref_count--;

    if (mesh_alloc.ref_count <= 0) {
        log::debug(
            lc_cb_,
            "  FREE_MESH model_idx {} v_off {} i_off {}",
            ent_alloc.model_index,
            mesh_alloc.vertex_offset, mesh_alloc.index_offset
        );
        free_slots_.push_back({
            .vertex_offset = mesh_alloc.vertex_offset,
            .index_offset  = mesh_alloc.index_offset,
        });
        mesh_allocations_.erase(ent_alloc.model_index);
    }

    std::optional<entity> swapped_entity;

    auto last_index = entity_allocations_.size() - 1;
    bool need_swap =
        ent_alloc.instance_index != last_index && last_index < entity_allocations_.size();
    if (need_swap) {
        entity last_ent      = instance_indexes_[last_index];
        auto& last_ent_alloc = entity_allocations_[last_ent];

        log::debug(
            lc_cb_,
            "  SWAP entity {}.{} instance {} -> {}",
            last_ent.index, last_ent.generation,
            last_index, ent_alloc.instance_index
        );

        auto& last_mesh_alloc = mesh_allocations_[last_ent_alloc.model_index];
        const draw_command new_cmd{
            .index_count    = chunk_size_.index_count,
            .instance_count = 1,
            .first_index    = last_mesh_alloc.index_offset,
            .vertex_offset  = static_cast<int32>(last_mesh_alloc.vertex_offset),
            .first_instance = ent_alloc.instance_index,
        };
        auto draw_cmd_staged = staging_->stage_struct(new_cmd);
        staging_->copy_to(
            indirect_draw_buffer_->get_buffer(),
            ent_alloc.instance_index * sizeof(draw_command),
            draw_cmd_staged,
            sizeof(draw_command)
        );

        last_ent_alloc.instance_index = ent_alloc.instance_index;
        instance_indexes_[ent_alloc.instance_index] = last_ent;

        swapped_entity = last_ent;
    }

    entity_allocations_.erase(ent);
    return swapped_entity;
}
inline auto combined_buffer::get_entity_allocation(
    entity ent
) -> const entity_allocation& {
    return entity_allocations_[ent];
}

inline VkBuffer combined_buffer::get_vertex_buffer() const {
    return vertex_buffer_->get_buffer();
}

inline void combined_buffer::expand_mesh_buffers_() {
    const auto old_vertex_bytes =
        (mesh_capacity_ * chunk_size_.vertex_count) * sizeof(vertex);
    const auto old_index_bytes =
        (mesh_capacity_ * chunk_size_.index_count) * sizeof(uint32);

    mesh_capacity_ *= 2;

    auto new_vertex_buffer = std::make_unique<device_vertex_buffer>(
        *context_, mesh_capacity_ * chunk_size_.vertex_count * sizeof(vertex)
    );
    vertex_buffer_->copy_to_buffer(*new_vertex_buffer, old_vertex_bytes);

    auto new_index_buffer = std::make_unique<device_index_buffer>(
        *context_, mesh_capacity_ * chunk_size_.index_count * sizeof(uint32)
    );
    index_buffer_->copy_to_buffer(*new_index_buffer, old_index_bytes);

    staging_->replace_buffer(vertex_buffer_->get_buffer(), new_vertex_buffer->get_buffer());
    staging_->replace_buffer(index_buffer_->get_buffer(), new_index_buffer->get_buffer());

    vertex_buffer_ = std::move(new_vertex_buffer);
    index_buffer_  = std::move(new_index_buffer);
}

inline void combined_buffer::expand_instance_buffers_() {
    const auto instance_count = entity_allocations_.size();

    instance_capacity_ *= 2;

    auto new_model_matrix_buffer = std::make_unique<device_storage_buffer>(
        *context_, instance_capacity_ * sizeof(mat4f)
    );
    auto new_normal_matrix_buffer = std::make_unique<device_storage_buffer>(
        *context_, instance_capacity_ * sizeof(mat4f)
    );
    auto new_indirect_draw_buffer = std::make_unique<device_storage_buffer>(
        *context_,
        instance_capacity_ * sizeof(draw_command),
        VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT
    );
    auto new_instance_index_buffer = std::make_unique<device_storage_buffer>(
        *context_,
        instance_capacity_ * sizeof(uint32),
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT
    );

    model_matrix_buffer_->copy_to_buffer(
        *new_model_matrix_buffer, instance_count * sizeof(mat4f)
    );
    normal_matrix_buffer_->copy_to_buffer(
        *new_normal_matrix_buffer, instance_count * sizeof(mat4f)
    );
    indirect_draw_buffer_->copy_to_buffer(
        *new_indirect_draw_buffer, instance_count * sizeof(draw_command)
    );
    instance_index_buffer_->copy_to_buffer(
        *new_instance_index_buffer, instance_count * sizeof(uint32)
    );

    staging_->replace_buffer(
        model_matrix_buffer_->get_buffer(), new_model_matrix_buffer->get_buffer()
    );
    staging_->replace_buffer(
        normal_matrix_buffer_->get_buffer(), new_normal_matrix_buffer->get_buffer()
    );
    staging_->replace_buffer(
        indirect_draw_buffer_->get_buffer(), new_indirect_draw_buffer->get_buffer()
    );
    staging_->replace_buffer(
        instance_index_buffer_->get_buffer(), new_instance_index_buffer->get_buffer()
    );

    model_matrix_buffer_   = std::move(new_model_matrix_buffer);
    normal_matrix_buffer_  = std::move(new_normal_matrix_buffer);
    indirect_draw_buffer_  = std::move(new_indirect_draw_buffer);
    instance_index_buffer_ = std::move(new_instance_index_buffer);

    update_descriptor_set_();
}

inline void combined_buffer::update_descriptor_set_() {
    VkDescriptorBufferInfo model_buffer_info{};
    model_buffer_info.buffer = model_matrix_buffer_->get_buffer();
    model_buffer_info.offset = 0;
    model_buffer_info.range  = VK_WHOLE_SIZE;

    VkDescriptorBufferInfo normal_buffer_info{};
    normal_buffer_info.buffer = normal_matrix_buffer_->get_buffer();
    normal_buffer_info.offset = 0;
    normal_buffer_info.range  = VK_WHOLE_SIZE;

    std::array<VkWriteDescriptorSet, 2> descriptor_writes{};

    descriptor_writes[0].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptor_writes[0].dstSet          = descriptor_set_;
    descriptor_writes[0].dstBinding      = 0;
    descriptor_writes[0].dstArrayElement = 0;
    descriptor_writes[0].descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    descriptor_writes[0].descriptorCount = 1;
    descriptor_writes[0].pBufferInfo     = &model_buffer_info;

    descriptor_writes[1].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptor_writes[1].dstSet          = descriptor_set_;
    descriptor_writes[1].dstBinding      = 1;
    descriptor_writes[1].dstArrayElement = 0;
    descriptor_writes[1].descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    descriptor_writes[1].descriptorCount = 1;
    descriptor_writes[1].pBufferInfo     = &normal_buffer_info;

    vkUpdateDescriptorSets(
        context_->get_device(), descriptor_writes.size(), descriptor_writes.data(), 0, nullptr
    );
}

inline uint32 combined_buffer::get_draw_command_count() const {
    return entity_allocations_.size();
}

inline VkBuffer combined_buffer::get_index_buffer() const {
    return index_buffer_->get_buffer();
}

inline VkBuffer combined_buffer::get_instance_index_buffer() const {
    return instance_index_buffer_->get_buffer();
}

inline VkBuffer combined_buffer::get_indirect_draw_buffer() const {
    return indirect_draw_buffer_->get_buffer();
}

inline VkBuffer combined_buffer::get_model_matrix_buffer() const {
    return model_matrix_buffer_->get_buffer();
}

inline VkBuffer combined_buffer::get_normal_matrix_buffer() const {
    return normal_matrix_buffer_->get_buffer();
}

inline bool combined_buffer::is_empty() const {
    return entity_allocations_.empty();
}

inline const combined_buffer_stats& combined_buffer::get_stats() const {
    stats_.chunk_size        = chunk_size_;
    stats_.mesh_capacity     = mesh_capacity_;
    stats_.mesh_count        = static_cast<uint32>(mesh_allocations_.size());
    stats_.instance_capacity = instance_capacity_;
    stats_.instance_count    = static_cast<uint32>(entity_allocations_.size());
    stats_.vertex_load_min   = 0.f;
    stats_.vertex_load_max   = 0.f;
    stats_.vertex_load_avg   = 0.f;
    stats_.index_load_min    = 0.f;
    stats_.index_load_max    = 0.f;
    stats_.index_load_avg    = 0.f;

    if (mesh_allocations_.empty()) {
        return stats_;
    }

    float32 vertex_load_avg_sum = 0.0f;
    float32 index_load_avg_sum  = 0.0f;

    for (const auto& mesh_alloc : mesh_allocations_ | std::views::values) {
        float32 vertex_load = 0.0f;
        if (chunk_size_.vertex_count > 0) {
            vertex_load = static_cast<float32>(mesh_alloc.vertex_count) /
                static_cast<float32>(chunk_size_.vertex_count);
        }
        if (vertex_load < stats_.vertex_load_min || stats_.vertex_load_min == 0.0f) {
            stats_.vertex_load_min = vertex_load;
        }
        if (vertex_load > stats_.vertex_load_max) {
            stats_.vertex_load_max = vertex_load;
        }
        vertex_load_avg_sum += vertex_load;

        float32 index_load = 0.0f;
        if (chunk_size_.index_count > 0) {
            index_load = static_cast<float32>(mesh_alloc.index_count) /
                static_cast<float32>(chunk_size_.index_count);
        }
        if (index_load < stats_.index_load_min || stats_.index_load_min == 0.0f) {
            stats_.index_load_min = index_load;
        }
        if (index_load > stats_.index_load_max) {
            stats_.index_load_max = index_load;
        }
        index_load_avg_sum += index_load;
    }

    stats_.vertex_load_avg = vertex_load_avg_sum / static_cast<float32>(mesh_allocations_.size());
    stats_.index_load_avg  = index_load_avg_sum / static_cast<float32>(mesh_allocations_.size());

    return stats_;
}
}  // namespace vw::gfx

#endif  // VW_GFX_COMBINED_BUFFER_INL_H
