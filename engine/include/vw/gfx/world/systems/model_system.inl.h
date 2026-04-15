#pragma once

#ifndef VW_GFX_WORLD_SYSTEMS_MODEL_SYSTEM_INL_H
#define VW_GFX_WORLD_SYSTEMS_MODEL_SYSTEM_INL_H

#include "vw/gfx/model/model.h"

namespace vw::gfx {

template <typename WD>
model_system<WD>::model_system(
    context_type& context
)
    : context_(&context) {}

template <typename WD>
void model_system<WD>::update(float32 /*dt*/) {
    auto& requested = context_->registry().template requested<model_component>();
    for (auto ent : requested) {
        context_->registry().template notify_changed<model_component>(ent);
    }
    context_->registry().template clear_requested<model_component>();
}

template <typename WD>
auto model_system<WD>::modify(
    entity e
) -> model_modifier {
    auto& comp = context_->registry().template get<model_component>(e);
    return model_modifier(*this, &comp, e);
}

template <typename WD>
model_system<WD>::model_modifier::model_modifier(
    model_system& system, model_component* component, entity entity_id
)
    : system_(&system), component_(component), entity_(entity_id) {}

template <typename WD>
auto model_system<WD>::model_modifier::get_model() const -> std::shared_ptr<model> {
    return component_->model_;
}

template <typename WD>
void model_system<WD>::model_modifier::set_model(
    std::shared_ptr<model> model_ptr
) {
    component_->model_ = std::move(model_ptr);
    system_->context_->registry().template request_change<model_component>(entity_);
}

template <typename WD>
void model_system<WD>::model_modifier::set_voxel(
    int x, int y, int z, const voxel& v
) {
    if (component_->model_) {
        component_->model_->set_voxel(x, y, z, v);
        system_->context_->registry().template request_change<model_component>(entity_);
    }
}

template <typename WD>
void model_system<WD>::model_modifier::set_voxel(
    vec3i pos, const voxel& v
) {
    if (component_->model_) {
        component_->model_->set_voxel(pos.x, pos.y, pos.z, v);
        system_->context_->registry().template request_change<model_component>(entity_);
    }
}

template <typename WD>
void model_system<WD>::model_modifier::fill(
    const voxel& v
) {
    if (component_->model_) {
        component_->model_->fill(v);
        system_->context_->registry().template request_change<model_component>(entity_);
    }
}

}  // namespace vw::gfx

#endif  // VW_GFX_WORLD_SYSTEMS_MODEL_SYSTEM_INL_H
