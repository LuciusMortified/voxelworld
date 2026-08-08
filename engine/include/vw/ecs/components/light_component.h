#pragma once

#ifndef VW_ECS_COMPONENTS_LIGHT_COMPONENT_H
#define VW_ECS_COMPONENTS_LIGHT_COMPONENT_H

#include "vw/core.h"

namespace vw::ecs {

class light_system;

struct light_component final {
private:
    vec3f color_;
    float32 intensity_;
    float32 range_;
    float32 attenuation_constant_;
    float32 attenuation_linear_;
    float32 attenuation_quadratic_;
    
        friend class light_system;
    
public:
    // Конструктор по умолчанию с разумными значениями
    light_component()
        : color_(vec3f(1.0f, 1.0f, 1.0f))
        , intensity_(1.0f)
        , range_(10.0f)
        , attenuation_constant_(1.0f)
        , attenuation_linear_(0.09f)
        , attenuation_quadratic_(0.032f) {}
    
    [[nodiscard]] auto get_color() const -> const vec3f&;
    [[nodiscard]] auto get_intensity() const -> float32;
    [[nodiscard]] auto get_range() const -> float32;
    [[nodiscard]] auto get_attenuation_constant() const -> float32;
    [[nodiscard]] auto get_attenuation_linear() const -> float32;
    [[nodiscard]] auto get_attenuation_quadratic() const -> float32;
};

}  // namespace vw::ecs

#include "light_component.inl.h"

#endif  // VW_ECS_COMPONENTS_LIGHT_COMPONENT_H
