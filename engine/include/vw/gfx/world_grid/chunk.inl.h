#pragma once

#ifndef VW_GFX_WORLD_GRID_CHUNK_INL_H
#define VW_GFX_WORLD_GRID_CHUNK_INL_H

#include "vw/gfx/model/model.h"
#include "vw/gfx/world/world.h"
#include "vw/log/logger.h"

namespace vw::gfx {

namespace {
constexpr log::log_category lc_chunk_{"chunk"};
}

template <typename WC>
chunk<WC>::chunk(
    world<WC>& world, vec3i coord, std::shared_ptr<model> model, int32 voxel_scale
)
    : guard_(world)
    , model_(std::move(model)) {
    guard_.template with<transform_component>();
    guard_.template with<model_component>();
    guard_.template with<spatial_component>();

    auto id = model_->get_identity();
    // log::debug(
    //     lc_chunk_,
    //     "CREATE chunk ({},{},{}) entity {}.{} model {}.{}",
    //     coord.x, coord.y, coord.z,
    //     guard_.get_entity().index, guard_.get_entity().generation,
    //     id.index, id.generation
    // );

    auto world_pos = vec3f{
        static_cast<float32>(coord.x * size * voxel_scale),
        static_cast<float32>(coord.y * size * voxel_scale),
        static_cast<float32>(coord.z * size * voxel_scale)
    };

    auto vs = static_cast<float32>(voxel_scale);
    world.get_transform_system().modify(guard_.get_entity())
        .set_position(world_pos)
        .set_scale({vs, vs, vs});
    world.get_model_system().modify(guard_.get_entity()).set_model(model_);
    world.get_spatial_system().modify(guard_.get_entity()).set_layer(spatial_layer::terrain);
}

template <typename WC>
auto chunk<WC>::get_voxel(
    int32 x, int32 y, int32 z
) const -> voxel {
    return model_->get_voxel(x, y, z);
}

template <typename WC>
auto chunk<WC>::get_voxel(
    vec3i local
) const -> voxel {
    return model_->get_voxel(local);
}

template <typename WC>
void chunk<WC>::set_voxel(
    int32 x, int32 y, int32 z, const voxel& v
) const {
    model_->set_voxel(x, y, z, v);
}

template <typename WC>
void chunk<WC>::set_voxel(
    vec3i local, const voxel& v
) const {
    model_->set_voxel(local, v);
}

template <typename WC>
auto chunk<WC>::is_empty(
    int32 x, int32 y, int32 z
) const -> bool {
    return model_->is_empty(x, y, z);
}

template <typename WC>
auto chunk<WC>::get_model() const -> std::shared_ptr<model> {
    return model_;
}

template <typename WC>
auto chunk<WC>::get_entity() const -> entity {
    return guard_.get_entity();
}

}  // namespace vw::gfx

#endif  // VW_GFX_WORLD_GRID_CHUNK_INL_H
