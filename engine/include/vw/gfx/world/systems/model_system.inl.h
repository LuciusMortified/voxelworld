#pragma once

#ifndef VW_GFX_WORLD_SYSTEMS_MODEL_SYSTEM_INL_H
#define VW_GFX_WORLD_SYSTEMS_MODEL_SYSTEM_INL_H

#include <algorithm>

#include "vw/gfx/model/model.h"
#include "vw/gfx/world/systems/model_system.h"

namespace vw::gfx {

template <typename... Cs>
model_system<Cs...>::model_system(registry_type& registry, mesh_pool& mesh_pool)
    : registry_(registry), mesh_pool_(mesh_pool) {}

template <typename... Cs>
void model_system<Cs...>::update() {
    mesh_pool_.process_completed();
    update_completed_meshes();
    process_dirty_entities();
}

template <typename... Cs>
auto model_system<Cs...>::modify(entity entity_id) -> typename model_system<Cs...>::model_modifier {
    auto& comp = registry_.template get<model_component>(entity_id);
    return model_modifier(*this, &comp, entity_id);
}

template <typename... Cs>
void model_system<Cs...>::set_model(entity entity_id, std::shared_ptr<vw::gfx::model> model_ptr) {
    auto& comp = registry_.template get<model_component>(entity_id);
    comp.model_ = model_ptr;
    comp.dirty_ = true;
}

template <typename... Cs>
void model_system<Cs...>::mark_dirty(entity entity_id) {
    if (std::find(dirty_entities_.begin(), dirty_entities_.end(), entity_id) == dirty_entities_.end()) {
        dirty_entities_.push_back(entity_id);
    }
}

template <typename... Cs>
void model_system<Cs...>::process_dirty_entities() {
    for (entity entity_id : dirty_entities_) {
        auto& comp = registry_.template get<model_component>(entity_id);
        if (!comp.model_) {
            continue;
        }

        model_hash hash = comp.model_->get_hash();
        if (!mesh_pool_.has(hash) && !mesh_pool_.is_pending(hash)) {
            mesh_pool_.request_mesh(comp.model_);
            pending_entities_.push_back(entity_id);
        }
    }
    dirty_entities_.clear();
}

template <typename... Cs>
void model_system<Cs...>::update_completed_meshes() {
    for (auto iter = pending_entities_.begin(); iter != pending_entities_.end();) {
        entity entity_id = *iter;
        auto& comp = registry_.template get<model_component>(entity_id);
        
        model_hash hash = comp.model_->get_hash();
        if (mesh_pool_.has(hash)) {
            comp.mesh_ = mesh_pool_.get(hash);
            iter = pending_entities_.erase(iter);
        } else {
            ++iter;
        }
    }
}

template <typename... Cs>
model_system<Cs...>::model_modifier::model_modifier(model_system& system, model_component* component, entity entity_id)
    : system_(&system), component_(component), entity_(entity_id) {}

template <typename... Cs>
void model_system<Cs...>::model_modifier::set_voxel(int x, int y, int z, const voxel& voxel_data) {
    if (component_->model_) {
        component_->model_->set_voxel(x, y, z, voxel_data);
        system_->mark_dirty(entity_);
    }
}

template <typename... Cs>
void model_system<Cs...>::model_modifier::set_voxel(int x, int y, int z, color color_data) {
    set_voxel(x, y, z, voxel{color_data});
}

template <typename... Cs>
void model_system<Cs...>::model_modifier::fill(const voxel& voxel_data) {
    if (component_->model_) {
        component_->model_->fill(voxel_data);
        system_->mark_dirty(entity_);
    }
}

template <typename... Cs>
void model_system<Cs...>::model_modifier::clear() {
    if (component_->model_) {
        component_->model_->clear();
        system_->mark_dirty(entity_);
    }
}

}  // namespace vw::gfx

#endif  // VW_GFX_WORLD_SYSTEMS_MODEL_SYSTEM_INL_H
