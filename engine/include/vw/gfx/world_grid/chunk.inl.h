#pragma once

#ifndef VW_GFX_WORLD_GRID_CHUNK_INL_H
#define VW_GFX_WORLD_GRID_CHUNK_INL_H

#include "vw/asset/model/model.h"

namespace vw::gfx {


template <typename WD>
chunk<WD>::chunk(
    context_type& ctx, vec3i coord, std::shared_ptr<vw::asset::model> mdl, int32 voxel_scale
)
    : guard_(ctx)
    , model_(std::move(mdl)) {
    guard_.template with<transform_component>();
    guard_.template with<model_component>();
    guard_.template with<spatial_component>();

    auto world_pos = vec3f{
        static_cast<float32>(coord.x * size * voxel_scale),
        static_cast<float32>(coord.y * size * voxel_scale),
        static_cast<float32>(coord.z * size * voxel_scale)
    };

    auto vs = static_cast<float32>(voxel_scale);
    ctx.template get_system<transform_system>().modify(guard_.get_entity())
        .set_position(world_pos)
        .set_scale({vs, vs, vs});
    ctx.template get_system<model_system>().modify(guard_.get_entity()).set_model(model_);
    ctx.template get_system<spatial_system>().modify(guard_.get_entity()).set_layer(spatial_layer::terrain);
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
    return guard_.get_entity();
}

}  // namespace vw::gfx

#endif  // VW_GFX_WORLD_GRID_CHUNK_INL_H
