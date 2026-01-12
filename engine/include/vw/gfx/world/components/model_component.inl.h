#pragma once

#ifndef VW_GFX_MODEL_COMPONENT_INL_H
#define VW_GFX_MODEL_COMPONENT_INL_H

namespace vw::gfx {

inline auto model_component::get_voxel(
    int x, int y, int z
) const -> voxel {
    return model_->get_voxel(x, y, z);
}

inline auto model_component::get_voxel(
    vec3i pos
) const -> voxel {
    return model_->get_voxel(pos);
}

inline auto model_component::get_voxels() const -> const std::vector<voxel>& {
    return model_->get_voxels();
}

inline auto model_component::is_empty(
    int x, int y, int z
) const -> bool {
    return model_->is_empty(x, y, z);
}

inline auto model_component::is_empty(
    vec3i pos
) const -> bool {
    return model_->is_empty(pos);
}

inline auto model_component::width() const -> int {
    return model_->width();
}

inline auto model_component::height() const -> int {
    return model_->height();
}

inline auto model_component::depth() const -> int {
    return model_->depth();
}

inline auto model_component::size() const -> vec3i {
    return model_->size();
}

inline auto model_component::has_model() const -> bool {
    return model_ != nullptr;
}

inline auto model_component::get_identity() const -> model_identity {
    return model_->get_identity();
}

}  // namespace vw::gfx

#endif  // VW_GFX_MODEL_COMPONENT_INL_H
