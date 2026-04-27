#pragma once

#ifndef VW_GFX_WORLD_SYSTEMS_LIGHT_SYSTEM_H
#define VW_GFX_WORLD_SYSTEMS_LIGHT_SYSTEM_H

#include <unordered_set>

#include "vw/gfx/world/components/light_component.h"
#include "vw/gfx/world/entity_registry.h"
#include "vw/gfx/world/system_trait.h"

namespace vw::gfx {

template <typename>
class world;

template <typename WD>
class light_system final {
public:
    using world_type    = world<WD>;
    using components    = typename WD::components;
    using registry_type = typename entity_registry_from_tuple<components>::type;

    explicit light_system(world_type& w);

    void update(float32 dt);

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

    template <typename C>
        requires std::same_as<C, light_component>
    void on_add(entity e);

private:
    world_type* world_;
};

}  // namespace vw::gfx

template <>
struct vw::gfx::system_trait<vw::gfx::light_system> {
    using components = std::tuple<vw::gfx::light_component>;
    using resources  = std::tuple<>;
};

#include "vw/gfx/world/systems/light_system.inl.h"

#endif  // VW_GFX_WORLD_SYSTEMS_LIGHT_SYSTEM_H
