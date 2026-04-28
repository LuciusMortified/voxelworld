#pragma once

#ifndef VW_GFX_WORLD_GRID_CHUNK_INL_H
#define VW_GFX_WORLD_GRID_CHUNK_INL_H

#include "vw/asset/model/model.h"
#include "vw/gfx/world/world.h"

namespace vw::gfx {


template <typename WD>
chunk<WD>::chunk(
    world_type& w, vec3i coord, std::shared_ptr<vw::asset::model> mdl, int32 voxel_scale
)
    : world_(&w)
    , ent_(w.create()
        .template with<transform_component>()
        .template with<model_component>()
        .template with<spatial_component>()
        .get_entity())
    , model_(std::move(mdl)) {
    auto world_pos = vec3f{
        static_cast<float32>(coord.x * size * voxel_scale),
        static_cast<float32>(coord.y * size * voxel_scale),
        static_cast<float32>(coord.z * size * voxel_scale)
    };

    auto vs = static_cast<float32>(voxel_scale);
    w.template system<transform_system>().modify(ent_)
        .set_position(world_pos)
        .set_scale({vs, vs, vs});
    w.template system<model_system>().modify(ent_).set_model(model_);
    w.template system<spatial_system>().modify(ent_).set_layer(spatial_layer::terrain);
}

template <typename WD>
chunk<WD>::~chunk() {
    if (world_ != nullptr && ent_.is_valid()) {
        world_->destroy(ent_);
    }
}

template <typename WD>
chunk<WD>::chunk(
    chunk&& other
) noexcept
    : world_(other.world_)
    , ent_(other.ent_)
    , model_(std::move(other.model_)) {
    other.world_ = nullptr;
    other.ent_   = invalid_entity;
}

template <typename WD>
auto chunk<WD>::operator=(
    chunk&& other
) noexcept -> chunk& {
    if (this != &other) {
        if (world_ != nullptr && ent_.is_valid()) {
            world_->destroy(ent_);
        }
        world_       = other.world_;
        ent_         = other.ent_;
        model_       = std::move(other.model_);
        other.world_ = nullptr;
        other.ent_   = invalid_entity;
    }
    return *this;
}

template <typename WD>
auto chunk<WD>::get_voxel(
    int32 x, int32 y, int32 z
) const -> voxel {
    return model_->get_voxel(x, y, z);
}

template <typename WD>
auto chunk<WD>::get_voxel(
    vec3i local
) const -> voxel {
    return model_->get_voxel(local);
}

template <typename WD>
void chunk<WD>::set_voxel(
    int32 x, int32 y, int32 z, const voxel& v
) const {
    model_->set_voxel(x, y, z, v);
}

template <typename WD>
void chunk<WD>::set_voxel(
    vec3i local, const voxel& v
) const {
    model_->set_voxel(local, v);
}

template <typename WD>
auto chunk<WD>::is_empty(
    int32 x, int32 y, int32 z
) const -> bool {
    return model_->is_empty(x, y, z);
}

template <typename WD>
auto chunk<WD>::get_model() const -> std::shared_ptr<vw::asset::model> {
    return model_;
}

template <typename WD>
auto chunk<WD>::get_entity() const -> entity {
    return ent_;
}

}  // namespace vw::gfx

#endif  // VW_GFX_WORLD_GRID_CHUNK_INL_H
