#pragma once

#ifndef VW_GFX_WORLD_SYSTEMS_MODEL_SYSTEM_INL_H
#define VW_GFX_WORLD_SYSTEMS_MODEL_SYSTEM_INL_H

#include <algorithm>
#include <utility>

#include "vw/gfx/model/model.h"
#include "vw/gfx/world/components/spatial_component.h"

namespace vw::gfx {

template <typename WC>
model_system<WC>::model_system(
    context_type& context
)
    : context_(&context) {}

template <typename WC>
void model_system<WC>::update(float32 /*dt*/) {
    using clock = std::chrono::high_resolution_clock;
    auto ms = [](auto start, auto end) -> float32 {
        return std::chrono::duration<float32>(end - start).count() * 1000.0f;
    };

    auto t0 = clock::now();
    context_->get_mesh_pool()->process_completed();
    auto t1 = clock::now();
    update_completed_meshes();
    auto t2 = clock::now();
    process_dirty_entities();
    auto t3 = clock::now();

    stats_.process_completed_ms  = ms(t0, t1);
    stats_.update_completed_ms   = ms(t1, t2);
    stats_.process_dirty_ms      = ms(t2, t3);
    stats_.pending_entities_count = static_cast<uint32>(pending_entities_.size());
    stats_.pending_meshes_count   = context_->get_mesh_pool()->get_pending_count();
}

template <typename WC>
auto model_system<WC>::get_stats() const -> const model_system_stats& {
    return stats_;
}

template <typename WC>
auto model_system<WC>::modify(
    entity e
) -> model_modifier {
    auto& comp = context_->registry().template get<model_component>(e);
    return model_modifier(*this, &comp, e);
}

template <typename WC>
void model_system<WC>::process_dirty_entities() {
    auto& requested = context_->registry().template requested<model_component>();
    for (auto ent : requested) {
        if (!context_->registry().template has<model_component>(ent)) {
            continue;
        }

        auto& comp = context_->registry().template get<model_component>(ent);
        if (!comp.model_) {
            continue;
        }

        auto identity = comp.model_->get_identity();
        if (!context_->get_mesh_pool()->has(identity) && !context_->get_mesh_pool()->is_pending(identity)) {
            context_->get_mesh_pool()->request_mesh(comp.model_);
            pending_entities_.insert(ent);
        }

        context_->registry().template notify_changed<model_component>(ent);
    }
    context_->registry().template clear_requested<model_component>();
}

template <typename WC>
void model_system<WC>::update_completed_meshes() {
    for (auto it = pending_entities_.begin(); it != pending_entities_.end();) {
        auto ent      = *it;

        if (!context_->registry().template has<model_component>(ent)) {
            it = pending_entities_.erase(it);
            continue;
        }

        auto& comp    = context_->registry().template get<model_component>(ent);
        auto identity = comp.model_->get_identity();
        if (context_->get_mesh_pool()->has(identity)) {
            it = pending_entities_.erase(it);
            context_->registry().template notify_changed<model_component>(ent);
        } else {
            ++it;
        }
    }
}

template <typename WC>
model_system<WC>::model_modifier::model_modifier(
    model_system& system, model_component* component, entity entity_id
)
    : system_(&system), component_(component), entity_(entity_id) {}

template <typename WC>
auto model_system<WC>::model_modifier::get_model() const -> std::shared_ptr<model> {
    return component_->model_;
}

template <typename WC>
void model_system<WC>::model_modifier::set_model(
    std::shared_ptr<model> model_ptr
) {
    component_->model_ = std::move(model_ptr);
    system_->context_->registry().template request_change<model_component>(entity_);
}

template <typename WC>
void model_system<WC>::model_modifier::set_voxel(
    int x, int y, int z, const voxel& v
) {
    if (component_->model_) {
        component_->model_->set_voxel(x, y, z, v);
        system_->context_->registry().template request_change<model_component>(entity_);
    }
}

template <typename WC>
void model_system<WC>::model_modifier::set_voxel(
    vec3i pos, const voxel& v
) {
    if (component_->model_) {
        component_->model_->set_voxel(pos.x, pos.y, pos.z, v);
        system_->context_->registry().template request_change<model_component>(entity_);
    }
}

template <typename WC>
void model_system<WC>::model_modifier::fill(
    const voxel& v
) {
    if (component_->model_) {
        component_->model_->fill(v);
        system_->context_->registry().template request_change<model_component>(entity_);
    }
}

}  // namespace vw::gfx

#endif  // VW_GFX_WORLD_SYSTEMS_MODEL_SYSTEM_INL_H
