#pragma once

#ifndef VW_ECS_SYSTEMS_CHARACTER_CONTROLLER_SYSTEM_H
#define VW_ECS_SYSTEMS_CHARACTER_CONTROLLER_SYSTEM_H

#include "vw/ecs/components/character_controller_component.h"
#include "vw/ecs/components/movement_intent_component.h"
#include "vw/ecs/components/rigid_body_component.h"
#include "vw/ecs/entity_registry.h"
#include "vw/ecs/system_trait.h"

namespace vw::ecs {

class transform_system;

class world;

class character_controller_system final {
public:
    using world_type    = world;
    using registry_type = registry;

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

}  // namespace vw::ecs

#endif  // VW_ECS_SYSTEMS_CHARACTER_CONTROLLER_SYSTEM_H
