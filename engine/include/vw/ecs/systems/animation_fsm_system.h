#pragma once

#ifndef VW_ECS_SYSTEMS_ANIMATION_FSM_SYSTEM_H
#define VW_ECS_SYSTEMS_ANIMATION_FSM_SYSTEM_H

#include "vw/asset/animation/animation_fsm.h"
#include "vw/ecs/components/animation_fsm_component.h"
#include "vw/ecs/components/animation_player_component.h"
#include "vw/ecs/entity_registry.h"
#include "vw/ecs/system_trait.h"

namespace vw::ecs {

class animation_system;

class world;

class animation_fsm_system final {
public:
    using registry_type = registry;
    using world_type    = world;

    explicit animation_fsm_system(world_type& w);

    void update(float32 dt);

    class modifier {
    public:
        void add_machine(size_t index, vw::asset::animation_fsm machine) const;
        void fire_trigger(std::string_view name) const;

    private:
        friend class animation_fsm_system;
        explicit modifier(animation_fsm_component* component);

        animation_fsm_component* component_;
    };

    auto modify(entity ent) -> modifier;

private:
    world_type* world_;
};

}  // namespace vw::ecs

#endif  // VW_ECS_SYSTEMS_ANIMATION_FSM_SYSTEM_H
