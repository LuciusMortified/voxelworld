#pragma once

#ifndef VW_GFX_COMBINED_BUFFER_INL_H
#define VW_GFX_COMBINED_BUFFER_INL_H
#include <numeric>
#include <ranges>

namespace vw::gfx {
inline combined_buffer::~combined_buffer() {
    if (descriptor_set_ != VK_NULL_HANDLE && descriptor_pool_ != VK_NULL_HANDLE) {
        vkFreeDescriptorSets(context_->get_device(), descriptor_pool_, 1, &descriptor_set_);
    }
}

inline combined_buffer::combined_buffer(
    vulkan_context& context,
    const buffer_chunk_size& chunk_size,
    VkDescriptorPool descriptor_pool,
    VkDescriptorSetLayout descriptor_set_layout
)
    : context_(&context)
    , chunk_size_(chunk_size)
    , mesh_capacity_(default_mesh_capacity_)
    , instance_capacity_(default_instance_capacity_)
    , vertex_used_(0)
    , index_used_(0)
    , descriptor_pool_(descriptor_pool)
    , descriptor_set_layout_(descriptor_set_layout) {
    vertex_buffer_ = std::make_unique<vertex_buffer>(
        *context_, mesh_capacity_ * chunk_size_.vertex_count * sizeof(vertex)
    );
    index_buffer_ = std::make_unique<index_buffer>(
        *context_, mesh_capacity_ * chunk_size_.index_count * sizeof(uint32)
    );
    instance_index_buffer_ = std::make_unique<storage_buffer>(
        *context_,
        instance_capacity_ * sizeof(uint32),
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |     //
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT |  //
            VK_BUFFER_USAGE_TRANSFER_DST_BIT
    );
    model_matrix_buffer_ = std::make_unique<storage_buffer>(
        *context_,
        instance_capacity_ * sizeof(mat4f),
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT |    //
            VK_BUFFER_USAGE_TRANSFER_DST_BIT  //
    );
    indirect_draw_buffer_ = std::make_unique<storage_buffer>(
        *context_,
        instance_capacity_ * sizeof(draw_command),
        VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT |   //
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT |  //
            VK_BUFFER_USAGE_TRANSFER_DST_BIT    //
    );

    // Выделяем descriptor set из pool
    VkDescriptorSetAllocateInfo alloc_info{};
    alloc_info.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    alloc_info.descriptorPool     = descriptor_pool_;
    alloc_info.descriptorSetCount = 1;
    alloc_info.pSetLayouts        = &descriptor_set_layout_;

    if (vkAllocateDescriptorSets(context_->get_device(), &alloc_info, &descriptor_set_) !=
        VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate descriptor set for combined_buffer!");
    }

    // Обновляем descriptor set с storage buffer
    VkDescriptorBufferInfo storage_buffer_info{};
    storage_buffer_info.buffer = model_matrix_buffer_->get_buffer();
    storage_buffer_info.offset = 0;
    storage_buffer_info.range  = VK_WHOLE_SIZE;

    VkWriteDescriptorSet descriptor_write{};
    descriptor_write.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptor_write.dstSet          = descriptor_set_;
    descriptor_write.dstBinding      = 0;
    descriptor_write.dstArrayElement = 0;
    descriptor_write.descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    descriptor_write.descriptorCount = 1;
    descriptor_write.pBufferInfo     = &storage_buffer_info;

    vkUpdateDescriptorSets(context_->get_device(), 1, &descriptor_write, 0, nullptr);
}

inline void combined_buffer::allocate(
    entity e, model_identity model_id, const mesh& mesh_data, const mat4f& transform_matrix
) {
    auto vertex_count = mesh_data.vertices.size();
    auto index_count  = mesh_data.indices.size();

    if (mesh_allocations_.contains(model_id.index)) {
        auto& mesh_alloc = mesh_allocations_[model_id.index];
        if (mesh_alloc.generation != model_id.generation) {
            mesh_alloc.vertex_count = vertex_count;
            mesh_alloc.index_count  = index_count;
            mesh_alloc.generation   = model_id.generation;

            vertex_buffer_->copy_from_vector(
                mesh_data.vertices, mesh_alloc.vertex_offset * sizeof(vertex)
            );
            index_buffer_->copy_from_vector(
                mesh_data.indices, mesh_alloc.index_offset * sizeof(uint32_t)
            );
        }
    } else {
        uint32_t vertex_offset = vertex_used_;
        uint32_t index_offset  = index_used_;

        if (!free_slots_.empty()) {
            auto& slot    = free_slots_.back();
            vertex_offset = slot.vertex_offset;
            index_offset  = slot.index_offset;
            free_slots_.pop_back();
        } else {
            vertex_used_ += chunk_size_.vertex_count;
            index_used_ += chunk_size_.index_count;
        }

        bool vertex_out_of_bounds = vertex_used_ > mesh_capacity_ * chunk_size_.vertex_count;
        bool index_out_of_bounds  = index_used_ > mesh_capacity_ * chunk_size_.index_count;
        if (vertex_out_of_bounds || index_out_of_bounds) {
            expand_mesh_buffers_();
        }

        vertex_buffer_->copy_from_vector(mesh_data.vertices, vertex_offset * sizeof(vertex));
        auto vertex_free_space_offset =
            (vertex_offset + mesh_data.vertices.size()) * sizeof(vertex);
        auto vertex_free_space_size =
            (chunk_size_.vertex_count - mesh_data.vertices.size()) * sizeof(vertex);
        if (vertex_free_space_size > 0) {
            void* vertex_data = vertex_buffer_->map();
            std::memset(
                static_cast<std::byte*>(vertex_data) + vertex_free_space_offset,
                0,
                vertex_free_space_size
            );
        }

        index_buffer_->copy_from_vector(mesh_data.indices, index_offset * sizeof(uint32));
        auto index_free_space_offset =  //
            (index_offset + mesh_data.indices.size()) * sizeof(uint32);
        auto index_free_space_size =
            (chunk_size_.index_count - mesh_data.indices.size()) * sizeof(uint32);
        if (index_free_space_size > 0) {
            void* index_data = index_buffer_->map();
            std::memset(
                static_cast<std::byte*>(index_data) + index_free_space_offset,
                0,
                index_free_space_size
            );
        }

        mesh_allocation new_mesh_alloc{};
        new_mesh_alloc.vertex_offset = vertex_offset;
        new_mesh_alloc.index_offset  = index_offset;
        new_mesh_alloc.vertex_count  = vertex_count;
        new_mesh_alloc.index_count   = index_count;
        new_mesh_alloc.generation    = model_id.generation;
        new_mesh_alloc.ref_count     = 0;  // Будет увеличен ниже

        mesh_allocations_[model_id.index] = new_mesh_alloc;
    }

    auto& mesh_alloc = mesh_allocations_[model_id.index];

    uint32 instance_index = entity_allocations_.size();
    if (instance_index >= instance_capacity_) {
        expand_instance_buffers_();
    }

    draw_command cmd{
        //.index_count    = mesh_alloc.index_count,
        .index_count    = chunk_size_.index_count,
        .instance_count = 1,
        .first_index    = mesh_alloc.index_offset,
        .vertex_offset  = static_cast<int32>(mesh_alloc.vertex_offset),
        .first_instance = instance_index,
    };

    instance_index_buffer_->copy_from_struct(instance_index, instance_index * sizeof(uint32));

    indirect_draw_buffer_->copy_from_struct(cmd, instance_index * sizeof(draw_command));
    model_matrix_buffer_->copy_from_struct(transform_matrix, instance_index * sizeof(mat4f));

    entity_allocation ent_alloc{
        .instance_index = instance_index,
        .model_index    = model_id.index,
    };
    entity_allocations_[e] = ent_alloc;

    instance_indexes_[instance_index] = e;

    mesh_alloc.ref_count++;
}

inline void combined_buffer::write_mesh(
    model_identity model_id, const mesh& mesh_data
) {
    auto& mesh_alloc = mesh_allocations_[model_id.index];
    if (mesh_alloc.generation != model_id.generation) {
        vertex_buffer_->copy_from_vector(
            mesh_data.vertices, mesh_alloc.vertex_offset * sizeof(vertex)
        );
        auto vertex_free_space_offset =
            (mesh_alloc.vertex_offset + mesh_data.vertices.size()) * sizeof(vertex);
        auto vertex_free_space_size =
            (chunk_size_.vertex_count - mesh_data.vertices.size()) * sizeof(vertex);
        if (vertex_free_space_size > 0) {
            void* vertex_data = vertex_buffer_->map();
            std::memset(
                static_cast<std::byte*>(vertex_data) + vertex_free_space_offset,
                0,
                vertex_free_space_size
            );
        }

        index_buffer_->copy_from_vector(
            mesh_data.indices, mesh_alloc.index_offset * sizeof(uint32_t)
        );
        auto index_free_space_offset =
            (mesh_alloc.index_offset + mesh_data.indices.size()) * sizeof(uint32);
        auto index_free_space_size =
            (chunk_size_.index_count - mesh_data.indices.size()) * sizeof(uint32);
        if (index_free_space_size > 0) {
            void* index_data = index_buffer_->map();
            std::memset(
                static_cast<std::byte*>(index_data) + index_free_space_offset,
                0,
                index_free_space_size
            );
        }

        mesh_alloc.vertex_count = mesh_data.vertices.size();
        mesh_alloc.index_count  = mesh_data.indices.size();
        mesh_alloc.generation   = model_id.generation;
    }
}

inline void combined_buffer::write_transform(
    entity ent, const mat4f& transform_matrix
) {
    auto& ent_alloc = entity_allocations_[ent];
    model_matrix_buffer_->copy_from_struct(
        transform_matrix, ent_alloc.instance_index * sizeof(mat4f)
    );
}

inline void combined_buffer::free(
    entity ent
) {
    auto& ent_alloc = entity_allocations_[ent];

    auto& mesh_alloc = mesh_allocations_[ent_alloc.model_index];
    mesh_alloc.ref_count--;

    if (mesh_alloc.ref_count <= 0) {
        free_slots_.push_back({
            .vertex_offset = mesh_alloc.vertex_offset,
            .index_offset  = mesh_alloc.index_offset,
        });
        mesh_allocations_.erase(ent_alloc.model_index);
    }

    // Удаляем матрицу и команду отрисовки с сохранением плотности
    auto last_index = entity_allocations_.size() - 1;
    bool need_swap =
        ent_alloc.instance_index != last_index && last_index < entity_allocations_.size();
    if (need_swap) {
        entity last_ent      = instance_indexes_[last_index];
        auto& last_ent_alloc = entity_allocations_[last_ent];

        draw_command new_cmd{};
        indirect_draw_buffer_->copy_to_struct(
            new_cmd, last_ent_alloc.instance_index * sizeof(draw_command)
        );
        indirect_draw_buffer_->copy_from_struct(
            new_cmd, ent_alloc.instance_index * sizeof(draw_command)
        );

        mat4f new_matrix{};
        model_matrix_buffer_->copy_to_struct(
            new_matrix, last_ent_alloc.instance_index * sizeof(mat4f)
        );
        model_matrix_buffer_->copy_from_struct(
            new_matrix, ent_alloc.instance_index * sizeof(mat4f)
        );

        last_ent_alloc.instance_index = ent_alloc.instance_index;

        instance_indexes_[ent_alloc.instance_index] = last_ent;
    }

    entity_allocations_.erase(ent);
}

inline VkBuffer combined_buffer::get_vertex_buffer() const {
    return vertex_buffer_->get_buffer();
}

inline void combined_buffer::expand_mesh_buffers_() {
    mesh_capacity_ *= 2;

    auto new_vertex_buffer = std::make_unique<vertex_buffer>(
        *context_, mesh_capacity_ * chunk_size_.vertex_count * sizeof(vertex)
    );
    vertex_buffer_->copy_to_buffer(*new_vertex_buffer, vertex_used_ * sizeof(vertex));

    auto new_index_buffer = std::make_unique<index_buffer>(
        *context_, mesh_capacity_ * chunk_size_.index_count * sizeof(uint32)
    );
    index_buffer_->copy_to_buffer(*new_index_buffer, index_used_ * sizeof(uint32));

    vertex_buffer_ = std::move(new_vertex_buffer);
    index_buffer_  = std::move(new_index_buffer);
}

inline void combined_buffer::expand_instance_buffers_() {
    instance_capacity_ *= 2;

    auto new_model_matrix_buffer = std::make_unique<storage_buffer>(
        *context_,
        instance_capacity_ * sizeof(mat4f),
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT |    //
            VK_BUFFER_USAGE_TRANSFER_DST_BIT  //
    );
    auto new_indirect_draw_buffer = std::make_unique<storage_buffer>(
        *context_,
        instance_capacity_ * sizeof(draw_command),
        VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT |   //
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT |  //
            VK_BUFFER_USAGE_TRANSFER_DST_BIT    //
    );

    auto instance_count = entity_allocations_.size();
    model_matrix_buffer_->copy_to_buffer(*new_model_matrix_buffer, instance_count * sizeof(mat4f));
    indirect_draw_buffer_->copy_to_buffer(
        *new_indirect_draw_buffer, instance_count * sizeof(draw_command)
    );

    model_matrix_buffer_  = std::move(new_model_matrix_buffer);
    indirect_draw_buffer_ = std::move(new_indirect_draw_buffer);

    // Обновляем descriptor set с новым storage buffer
    VkDescriptorBufferInfo storage_buffer_info{};
    storage_buffer_info.buffer = model_matrix_buffer_->get_buffer();
    storage_buffer_info.offset = 0;
    storage_buffer_info.range  = VK_WHOLE_SIZE;

    VkWriteDescriptorSet descriptor_write{};
    descriptor_write.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptor_write.dstSet          = descriptor_set_;
    descriptor_write.dstBinding      = 0;  // Storage buffer binding (в отдельном layout)
    descriptor_write.dstArrayElement = 0;
    descriptor_write.descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    descriptor_write.descriptorCount = 1;
    descriptor_write.pBufferInfo     = &storage_buffer_info;

    vkUpdateDescriptorSets(context_->get_device(), 1, &descriptor_write, 0, nullptr);
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
