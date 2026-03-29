#pragma once

#ifndef VW_GFX_WORLD_SYSTEMS_LIGHT_SYSTEM_H
#define VW_GFX_WORLD_SYSTEMS_LIGHT_SYSTEM_H

#include <unordered_set>

#include "vw/gfx/world/components/light_component.h"
#include "vw/gfx/world/world_context.h"

namespace vw::gfx {

template <typename WC>
class light_system final {
public:
    using registry_type = entity_registry_from_tuple<WC>::type;
    using context_type = world_context<WC>;

    explicit light_system(context_type& context);

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
    context_type* context_;
};

}  // namespace vw::gfx

#include "vw/gfx/world/systems/light_system.inl.h"

#endif  // VW_GFX_WORLD_SYSTEMS_LIGHT_SYSTEM_H
