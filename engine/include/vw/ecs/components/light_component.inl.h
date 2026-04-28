#pragma once

#ifndef VW_ECS_COMPONENTS_LIGHT_COMPONENT_INL_H
#define VW_ECS_COMPONENTS_LIGHT_COMPONENT_INL_H

namespace vw::ecs {

inline auto light_component::get_color() const -> const vec3f& {
    return color_;
}

inline auto light_component::get_intensity() const -> float32 {
    return intensity_;
}

inline auto light_component::get_range() const -> float32 {
    return range_;
}

inline auto light_component::get_attenuation_constant() const -> float32 {
    return attenuation_constant_;
}

inline auto light_component::get_attenuation_linear() const -> float32 {
    return attenuation_linear_;
}

inline auto light_component::get_attenuation_quadratic() const -> float32 {
    return attenuation_quadratic_;
}

}  // namespace vw::ecs

#endif  // VW_ECS_COMPONENTS_LIGHT_COMPONENT_INL_H
