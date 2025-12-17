#include "vw/gfx/resource/combined_buffer.h"

#include <algorithm>
#include <cstring>
#include <numeric>

#include "vw/gfx/render/vulkan_context.h"
#include "vw/gfx/resource/mesh.h"
#include "vw/gfx/world/components/model_component.h"
#include "vw/gfx/world/components/transform_component.h"
#include "vw/gfx/world/world.h"

namespace vw::gfx {

combined_buffer::combined_buffer(
    vulkan_context& context, const buffer_chunk_size& chunk_size
)
    : context_(&context)
    , chunk_size_(chunk_size)
    , mesh_capacity_(default_mesh_capacity_)
    , instance_capacity_(default_instance_capacity_)
    , vertex_used_(0)
    , index_used_(0) {
    vertex_buffer_ = std::make_unique<vertex_buffer>(
        *context_, mesh_capacity_ * chunk_size_.vertex_count * sizeof(vertex)
    );
    index_buffer_ = std::make_unique<index_buffer>(
        *context_, mesh_capacity_ * chunk_size_.index_count * sizeof(uint32)
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
}

void combined_buffer::allocate(
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
        index_buffer_->copy_from_vector(mesh_data.indices, index_offset * sizeof(uint32_t));

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
        .index_count    = mesh_alloc.index_count,
        .instance_count = 1,
        .first_index    = mesh_alloc.index_offset,
        .vertex_offset  = static_cast<int32>(mesh_alloc.vertex_offset),
        .first_instance = 0,
    };

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

void combined_buffer::write_mesh(
    model_identity model_id, const mesh& mesh_data
) {
    auto& mesh_alloc = mesh_allocations_[model_id.index];
    if (mesh_alloc.generation != model_id.generation) {
        vertex_buffer_->copy_from_vector(
            mesh_data.vertices, mesh_alloc.vertex_offset * sizeof(vertex)
        );
        index_buffer_->copy_from_vector(
            mesh_data.indices, mesh_alloc.index_offset * sizeof(uint32_t)
        );
        mesh_alloc.generation = model_id.generation;
    }
}

void combined_buffer::write_transform(
    entity ent, const mat4f& transform_matrix
) {
    auto& ent_alloc = entity_allocations_[ent];
    model_matrix_buffer_->copy_from_struct(
        transform_matrix, ent_alloc.instance_index * sizeof(mat4f)
    );
}

void combined_buffer::free(
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

void combined_buffer::expand_mesh_buffers_() {
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

void combined_buffer::expand_instance_buffers_() {
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
}

combined_buffer_stats combined_buffer::get_stats() const {
    combined_buffer_stats stats;
    stats.chunk_size        = chunk_size_;
    stats.mesh_capacity     = mesh_capacity_;
    stats.mesh_count        = static_cast<uint32>(mesh_allocations_.size());
    stats.instance_capacity = instance_capacity_;
    stats.instance_count    = static_cast<uint32>(entity_allocations_.size());

    if (mesh_allocations_.empty()) {
        return stats;
    }

    std::vector<float32> vertex_loads;
    std::vector<float32> index_loads;
    vertex_loads.reserve(mesh_allocations_.size());
    index_loads.reserve(mesh_allocations_.size());

    for (const auto& [model_index, mesh_alloc] : mesh_allocations_) {
        float32 vertex_load = 0.0f;
        if (chunk_size_.vertex_count > 0) {
            vertex_load = static_cast<float32>(mesh_alloc.vertex_count) /
                static_cast<float32>(chunk_size_.vertex_count);
        }
        vertex_loads.push_back(vertex_load);

        float32 index_load = 0.0f;
        if (chunk_size_.index_count > 0) {
            index_load = static_cast<float32>(mesh_alloc.index_count) /
                static_cast<float32>(chunk_size_.index_count);
        }
        index_loads.push_back(index_load);
    }

    if (!vertex_loads.empty()) {
        stats.vertex_load_min = *std::ranges::min_element(vertex_loads);
        stats.vertex_load_max = *std::ranges::max_element(vertex_loads);
        stats.vertex_load_avg = std::accumulate(vertex_loads.begin(), vertex_loads.end(), 0.0f) /
            static_cast<float32>(vertex_loads.size());
    }

    if (!index_loads.empty()) {
        stats.index_load_min = *std::ranges::min_element(index_loads);
        stats.index_load_max = *std::ranges::max_element(index_loads);
        stats.index_load_avg = std::accumulate(index_loads.begin(), index_loads.end(), 0.0f) /
            static_cast<float32>(index_loads.size());
    }

    return stats;
}

}  // namespace vw::gfx
