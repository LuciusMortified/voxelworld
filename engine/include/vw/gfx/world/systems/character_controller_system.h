#pragma once

#ifndef VW_GFX_WORLD_SYSTEMS_CHARACTER_CONTROLLER_SYSTEM_H
#define VW_GFX_WORLD_SYSTEMS_CHARACTER_CONTROLLER_SYSTEM_H

#include "vw/gfx/world/components/character_controller_component.h"
#include "vw/gfx/world/components/movement_intent_component.h"
#include "vw/gfx/world/components/rigid_body_component.h"
#include "vw/gfx/world/entity_registry.h"
#include "vw/gfx/world/system_trait.h"

namespace vw::gfx {

template <typename>
class transform_system;

template <typename>
class world;

template <typename WD>
class character_controller_system final {
public:
    using world_type    = world<WD>;
    using components    = typename WD::components;
    using registry_type = typename entity_registry_from_tuple<components>::type;

    explicit character_controller_system(world_type& w);

    void update(float32 delta_time);

    class controller_modifier {
    public:
        auto set_move_input(const vec3f& input) -> controller_modifier&;
        auto set_facing_direction(const vec3f& direction) -> controller_modifier&;
        auto set_move_speed(float32 speed) -> controller_modifier&;
        auto set_jump_impulse(float32 impulse) -> controller_modifier&;
        auto set_rotation_speed(float32 speed) -> controller_modifier&;
        auto request_jump() -> controller_modifier&;

    private:
        friend class character_controller_system;
        controller_modifier(character_controller_system* system, entity ent);

        character_controller_system* system_;
        entity entity_;
    };

    auto modify(entity ent) -> controller_modifier;

private:
    world_type* world_;
};

}  // namespace vw::gfx

template <>
struct vw::gfx::system_trait<vw::gfx::character_controller_system> {
    using components = std::tuple<
        vw::gfx::character_controller_component,
        vw::gfx::movement_intent_component>;
    using resources  = std::tuple<>;
};

#include "vw/gfx/world/systems/character_controller_system.inl.h"

#endif  // VW_GFX_WORLD_SYSTEMS_CHARACTER_CONTROLLER_SYSTEM_H
