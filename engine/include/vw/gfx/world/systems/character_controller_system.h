#pragma once

#ifndef VW_GFX_WORLD_SYSTEMS_CHARACTER_CONTROLLER_SYSTEM_H
#define VW_GFX_WORLD_SYSTEMS_CHARACTER_CONTROLLER_SYSTEM_H

#include "vw/gfx/world/components/character_controller_component.h"
#include "vw/gfx/world/components/rigid_body_component.h"
#include "vw/gfx/world/entity_registry.h"

namespace vw::gfx {

template <typename... Cs>
class character_controller_system final {
public:
    using registry_type = entity_registry<Cs...>;

    explicit character_controller_system(registry_type& registry);

    void update();

    class controller_modifier {
    public:
        auto set_move_input(const vec3f& input) -> controller_modifier&;
        auto set_move_speed(float32 speed) -> controller_modifier&;
        auto set_jump_impulse(float32 impulse) -> controller_modifier&;
        auto request_jump() -> controller_modifier&;

    private:
        friend class character_controller_system;
        controller_modifier(character_controller_system* system, entity ent);

        character_controller_system* system_;
        entity entity_;
    };

    auto modify(entity ent) -> controller_modifier;

private:
    registry_type* registry_;
};

template <typename... Cs>
struct character_controller_system_from_tuple;

template <typename... Cs>
struct character_controller_system_from_tuple<std::tuple<Cs...>> {
    using type = character_controller_system<Cs...>;
};

}  // namespace vw::gfx

#include "vw/gfx/world/systems/character_controller_system.inl.h"

#endif  // VW_GFX_WORLD_SYSTEMS_CHARACTER_CONTROLLER_SYSTEM_H
