#pragma once

#ifndef VW_ECS_MODEL_COMPONENT_INL_H
#define VW_ECS_MODEL_COMPONENT_INL_H

namespace vw::ecs {

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

inline auto model_component::get_model() const -> std::shared_ptr<vw::asset::model> {
    return model_;
}

inline auto model_component::get_identity() const -> vw::asset::model_identity {
    return model_->get_identity();
}

inline auto model_component::top_brightness() const -> bool {
    return top_brightness_;
}

}  // namespace vw::ecs

#endif  // VW_ECS_MODEL_COMPONENT_INL_H
