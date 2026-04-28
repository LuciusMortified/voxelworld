#pragma once

#ifndef VW_GFX_WORLD_SYSTEMS_MODEL_SYSTEM_INL_H
#define VW_GFX_WORLD_SYSTEMS_MODEL_SYSTEM_INL_H

#include "vw/asset/model/model.h"
#include "vw/gfx/world/world.h"

namespace vw::gfx {

template <typename WD>
model_system<WD>::model_system(
    world_type& w
)
    : world_(&w) {}

template <typename WD>
template <typename C>
    requires std::same_as<C, model_component>
void model_system<WD>::on_add(entity e) {
    world_->registry().template request_change<model_component>(e);
}

template <typename WD>
void model_system<WD>::update(float32 /*dt*/) {
    auto& reg       = world_->registry();
    auto& requested = reg.template requested<model_component>();
    for (auto ent : requested) {
        reg.template notify_changed<model_component>(ent);
    }
    reg.template clear_requested<model_component>();
}

template <typename WD>
auto model_system<WD>::modify(
    entity e
) -> model_modifier {
    auto& comp = world_->registry().template get<model_component>(e);
    return model_modifier(*this, &comp, e);
}

template <typename WD>
model_system<WD>::model_modifier::model_modifier(
    model_system& system, model_component* component, entity entity_id
)
    : system_(&system), component_(component), entity_(entity_id) {}

template <typename WD>
auto model_system<WD>::model_modifier::get_model() const -> std::shared_ptr<vw::asset::model> {
    return component_->model_;
}

template <typename WD>
void model_system<WD>::model_modifier::set_model(
    std::shared_ptr<vw::asset::model> model_ptr
) {
    component_->model_ = std::move(model_ptr);
    system_->world_->registry().template request_change<model_component>(entity_);
}

template <typename WD>
void model_system<WD>::model_modifier::set_voxel(
    int x, int y, int z, const voxel& v
) {
    if (component_->model_) {
        component_->model_->set_voxel(x, y, z, v);
        system_->world_->registry().template request_change<model_component>(entity_);
    }
}

template <typename WD>
void model_system<WD>::model_modifier::set_voxel(
    vec3i pos, const voxel& v
) {
    if (component_->model_) {
        component_->model_->set_voxel(pos.x, pos.y, pos.z, v);
        system_->world_->registry().template request_change<model_component>(entity_);
    }
}

template <typename WD>
void model_system<WD>::model_modifier::fill(
    const voxel& v
) {
    if (component_->model_) {
        component_->model_->fill(v);
        system_->world_->registry().template request_change<model_component>(entity_);
    }
}

}  // namespace vw::gfx

#endif  // VW_GFX_WORLD_SYSTEMS_MODEL_SYSTEM_INL_H
