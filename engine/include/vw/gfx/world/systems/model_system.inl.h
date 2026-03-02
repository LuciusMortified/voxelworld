#pragma once

#ifndef VW_GFX_WORLD_SYSTEMS_MODEL_SYSTEM_INL_H
#define VW_GFX_WORLD_SYSTEMS_MODEL_SYSTEM_INL_H

#include <algorithm>
#include <utility>

#include "vw/gfx/model/model.h"
#include "vw/gfx/world/components/spatial_component.h"

namespace vw::gfx {

template <typename... Cs>
model_system<Cs...>::model_system(
    registry_type& registry, mesh_pool& mesh_pool
)
    : registry_(&registry), mesh_pool_(&mesh_pool) {}

template <typename... Cs>
void model_system<Cs...>::update() {
    using clock = std::chrono::high_resolution_clock;
    auto ms = [](auto start, auto end) -> float32 {
        return std::chrono::duration<float32>(end - start).count() * 1000.0f;
    };

    auto t0 = clock::now();
    mesh_pool_->process_completed();
    auto t1 = clock::now();
    update_completed_meshes();
    auto t2 = clock::now();
    process_dirty_entities();
    auto t3 = clock::now();

    stats_.process_completed_ms  = ms(t0, t1);
    stats_.update_completed_ms   = ms(t1, t2);
    stats_.process_dirty_ms      = ms(t2, t3);
    stats_.pending_entities_count = static_cast<uint32>(pending_entities_.size());
    stats_.pending_meshes_count   = mesh_pool_->get_pending_count();
}

template <typename... Cs>
auto model_system<Cs...>::get_stats() const -> const model_system_stats& {
    return stats_;
}

template <typename... Cs>
auto model_system<Cs...>::modify(
    entity e
) -> model_modifier {
    auto& comp = registry_->template get<model_component>(e);
    return model_modifier(*this, &comp, e);
}

template <typename... Cs>
void model_system<Cs...>::process_dirty_entities() {
    auto& requested = registry_->template requested<model_component>();
    for (auto ent : requested) {
        if (!registry_->template has<model_component>(ent)) {
            continue;
        }

        auto& comp = registry_->template get<model_component>(ent);
        if (!comp.model_) {
            continue;
        }

        auto identity = comp.model_->get_identity();
        if (!mesh_pool_->has(identity) && !mesh_pool_->is_pending(identity)) {
            mesh_pool_->request_mesh(comp.model_);
            pending_entities_.insert(ent);
        }

        registry_->template notify_changed<model_component>(ent);
    }
    registry_->template clear_requested<model_component>();
}

template <typename... Cs>
void model_system<Cs...>::update_completed_meshes() {
    for (auto it = pending_entities_.begin(); it != pending_entities_.end();) {
        auto ent      = *it;

        if (!registry_->template has<model_component>(ent)) {
            it = pending_entities_.erase(it);
            continue;
        }

        auto& comp    = registry_->template get<model_component>(ent);
        auto identity = comp.model_->get_identity();
        if (mesh_pool_->has(identity)) {
            it = pending_entities_.erase(it);
            registry_->template notify_changed<model_component>(ent);
        } else {
            ++it;
        }
    }
}

template <typename... Cs>
model_system<Cs...>::model_modifier::model_modifier(
    model_system& system, model_component* component, entity entity_id
)
    : system_(&system), component_(component), entity_(entity_id) {}

template <typename... Cs>
auto model_system<Cs...>::model_modifier::get_model() const -> std::shared_ptr<model> {
    return component_->model_;
}

template <typename... Cs>
void model_system<Cs...>::model_modifier::set_model(
    std::shared_ptr<model> model_ptr
) {
    component_->model_ = std::move(model_ptr);
    system_->registry_->template request_change<model_component>(entity_);
}

template <typename... Cs>
void model_system<Cs...>::model_modifier::set_voxel(
    int x, int y, int z, const voxel& v
) {
    if (component_->model_) {
        component_->model_->set_voxel(x, y, z, v);
        system_->registry_->template request_change<model_component>(entity_);
    }
}

template <typename... Cs>
void model_system<Cs...>::model_modifier::set_voxel(
    int x, int y, int z, color c
) {
    if (component_->model_) {
        component_->model_->set_voxel(x, y, z, c);
        system_->registry_->template request_change<model_component>(entity_);
    }
}
template <typename... Cs>
void model_system<Cs...>::model_modifier::set_voxel(
    vec3i pos, const voxel& v
) {
    if (component_->model_) {
        component_->model_->set_voxel(pos.x, pos.y, pos.z, v);
        system_->registry_->template request_change<model_component>(entity_);
    }
}
template <typename... Cs>
void model_system<Cs...>::model_modifier::set_voxel(
    vec3i pos, color c
) {
    if (component_->model_) {
        component_->model_->set_voxel(pos.x, pos.y, pos.z, c);
        system_->registry_->template request_change<model_component>(entity_);
    }
}

template <typename... Cs>
void model_system<Cs...>::model_modifier::fill(
    const voxel& v
) {
    if (component_->model_) {
        component_->model_->fill(v);
        system_->registry_->template request_change<model_component>(entity_);
    }
}

}  // namespace vw::gfx

#endif  // VW_GFX_WORLD_SYSTEMS_MODEL_SYSTEM_INL_H
