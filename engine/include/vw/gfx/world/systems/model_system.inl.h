#pragma once

#ifndef VW_GFX_WORLD_SYSTEMS_MODEL_SYSTEM_INL_H
#define VW_GFX_WORLD_SYSTEMS_MODEL_SYSTEM_INL_H

#include <algorithm>
#include <utility>

#include "vw/gfx/model/model.h"
#include "vw/gfx/world/components/spatial_component.h"
#include "vw/gfx/world/systems/model_system.h"

namespace vw::gfx {

template <typename... Cs>
model_system<Cs...>::model_system(
    registry_type& registry, mesh_pool& mesh_pool, spatial_system_type& spatial_sys
)
    : registry_(&registry), mesh_pool_(&mesh_pool), spatial_system_(&spatial_sys) {}

template <typename... Cs>
void model_system<Cs...>::update() {
    mesh_pool_->process_completed();
    update_completed_meshes();
    process_dirty_entities();
}

template <typename... Cs>
auto model_system<Cs...>::modify(
    entity e
) -> model_modifier {
    auto& comp = registry_->template get<model_component>(e);
    return model_modifier(*this, &comp, e);
}

template <typename... Cs>
void model_system<Cs...>::mark_dirty(
    entity ent
) {
    dirty_entities_.insert(ent);
}

template <typename... Cs>
auto model_system<Cs...>::get_render_dirty_entities() -> std::unordered_set<entity>& {
    return render_dirty_entities_;
}

template <typename... Cs>
void model_system<Cs...>::mark_render_dirty(
    entity ent
) {
    render_dirty_entities_.insert(ent);
}

template <typename... Cs>
void model_system<Cs...>::process_dirty_entities() {
    for (auto ent : dirty_entities_) {
        auto& comp = registry_->template get<model_component>(ent);
        if (!comp.model_) {
            continue;
        }

        auto identity = comp.model_->get_identity();
        if (!mesh_pool_->has(identity) && !mesh_pool_->is_pending(identity)) {
            mesh_pool_->request_mesh(comp.model_);
            pending_entities_.insert(ent);
        }
    }
    dirty_entities_.clear();
}

template <typename... Cs>
void model_system<Cs...>::update_completed_meshes() {
    for (auto it = pending_entities_.begin(); it != pending_entities_.end();) {
        auto ent      = *it;
        auto& comp    = registry_->template get<model_component>(ent);
        auto identity = comp.model_->get_identity();
        if (mesh_pool_->has(identity)) {
            it = pending_entities_.erase(it);
            render_dirty_entities_.insert(ent);
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
std::shared_ptr<model> model_system<Cs...>::model_modifier::get_model() const {
    return component_->model_;
}

template <typename... Cs>
void model_system<Cs...>::model_modifier::set_model(
    std::shared_ptr<model> model_ptr
) {
    component_->model_ = std::move(model_ptr);
    system_->mark_dirty(entity_);
    system_->mark_render_dirty(entity_);

    if (system_->registry_->template has<spatial_component>(entity_)) {
        system_->spatial_system_->mark_dirty(entity_);
    }
}

template <typename... Cs>
void model_system<Cs...>::model_modifier::set_voxel(
    int x, int y, int z, const voxel& v
) {
    if (component_->model_) {
        component_->model_->set_voxel(x, y, z, v);
        system_->mark_dirty(entity_);
        system_->mark_render_dirty(entity_);

        if (system_->registry_->template has<spatial_component>(entity_)) {
            system_->spatial_system_->mark_dirty(entity_);
        }
    }
}

template <typename... Cs>
void model_system<Cs...>::model_modifier::set_voxel(
    int x, int y, int z, color c
) {
    if (component_->model_) {
        component_->model_->set_voxel(x, y, z, c);
        system_->mark_dirty(entity_);
        system_->mark_render_dirty(entity_);

        if (system_->registry_->template has<spatial_component>(entity_)) {
            system_->spatial_system_->mark_dirty(entity_);
        }
    }
}
template <typename... Cs>
void model_system<Cs...>::model_modifier::set_voxel(
    vec3i pos, const voxel& v
) {
    if (component_->model_) {
        component_->model_->set_voxel(pos.x, pos.y, pos.z, v);
        system_->mark_dirty(entity_);
        system_->mark_render_dirty(entity_);

        if (system_->registry_->template has<spatial_component>(entity_)) {
            system_->spatial_system_->mark_dirty(entity_);
        }
    }
}
template <typename... Cs>
void model_system<Cs...>::model_modifier::set_voxel(
    vec3i pos, color c
) {
    if (component_->model_) {
        component_->model_->set_voxel(pos.x, pos.y, pos.z, c);
        system_->mark_dirty(entity_);
        system_->mark_render_dirty(entity_);

        if (system_->registry_->template has<spatial_component>(entity_)) {
            system_->spatial_system_->mark_dirty(entity_);
        }
    }
}

template <typename... Cs>
void model_system<Cs...>::model_modifier::fill(
    const voxel& v
) {
    if (component_->model_) {
        component_->model_->fill(v);
        system_->mark_dirty(entity_);
        system_->mark_render_dirty(entity_);

        // Пометить spatial_component как dirty
        if (system_->spatial_system_ &&
            system_->registry_->template has<spatial_component>(entity_)) {
            system_->spatial_system_->mark_dirty(entity_);
        }
    }
}

}  // namespace vw::gfx

#endif  // VW_GFX_WORLD_SYSTEMS_MODEL_SYSTEM_INL_H
