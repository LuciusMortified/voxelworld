#pragma once

#ifndef VW_GFX_WORLD_SYSTEMS_LIGHT_SYSTEM_H
#define VW_GFX_WORLD_SYSTEMS_LIGHT_SYSTEM_H

#include <unordered_set>

#include "vw/gfx/world/components/light_component.h"
#include "vw/gfx/world/entity_registry.h"

namespace vw::gfx {

template <typename... Cs>
class light_system final {
public:
    using registry_type = entity_registry<Cs...>;

    explicit light_system(registry_type& registry);

    void update();

    class light_modifier {
    public:
        auto set_color(const vec3f& color) -> light_modifier&;
        auto set_intensity(float32 intensity) -> light_modifier&;
        auto set_range(float32 range) -> light_modifier&;
        auto set_attenuation(float32 constant, float32 linear, float32 quadratic) -> light_modifier&;

    private:
        friend class light_system;
        light_modifier(light_system* system, entity ent);

        light_system* system_;
        entity entity_;
    };

    auto modify(entity ent) -> light_modifier;

private:
    registry_type* registry_;
};

template <typename... Cs>
struct light_system_from_tuple;

template <typename... Cs>
struct light_system_from_tuple<std::tuple<Cs...>> {
    using type = light_system<Cs...>;
};

}  // namespace vw::gfx

#include "vw/gfx/world/systems/light_system.inl.h"

#endif  // VW_GFX_WORLD_SYSTEMS_LIGHT_SYSTEM_H
