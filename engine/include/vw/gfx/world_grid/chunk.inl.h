#pragma once

#ifndef VW_GFX_WORLD_GRID_CHUNK_INL_H
#define VW_GFX_WORLD_GRID_CHUNK_INL_H

#include "vw/gfx/model/model.h"
#include "vw/gfx/world/world.h"

namespace vw::gfx {

template <typename WC>
chunk<WC>::chunk(
    world<WC>& world, region_id region, vec3i coord, std::vector<voxel> voxels
)
    : guard_(world)
    , region_id_(region) {
    auto& model_registry = world.get_model_registry();
    model_ = model_registry.create_unnamed(size, size, size);
    model_->set_voxels(std::move(voxels));

    guard_.template with<transform_component>();
    guard_.template with<model_component>();
    guard_.template with<spatial_component>();

    auto world_pos = vec3f{
        static_cast<float32>(coord.x * size),
        static_cast<float32>(coord.y * size),
        static_cast<float32>(coord.z * size)
    };

    world.get_transform_system().modify(guard_.get_entity()).set_position(world_pos);
    world.get_model_system().modify(guard_.get_entity()).set_model(model_);
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
) {
    model_->set_voxel(x, y, z, v);
}

template <typename WC>
void chunk<WC>::set_voxel(
    vec3i local, const voxel& v
) {
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

template <typename WC>
auto chunk<WC>::get_region_id() const -> region_id {
    return region_id_;
}

}  // namespace vw::gfx

#endif  // VW_GFX_WORLD_GRID_CHUNK_INL_H
