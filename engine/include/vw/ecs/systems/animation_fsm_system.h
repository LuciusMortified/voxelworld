#pragma once

#ifndef VW_ECS_SYSTEMS_ANIMATION_FSM_SYSTEM_H
#define VW_ECS_SYSTEMS_ANIMATION_FSM_SYSTEM_H

#include "vw/asset/animation/animation_fsm.h"
#include "vw/ecs/components/animation_fsm_component.h"
#include "vw/ecs/components/animation_player_component.h"
#include "vw/ecs/entity_registry.h"
#include "vw/ecs/system_trait.h"

namespace vw::ecs {


template <typename WD>
class animation_system;

template <typename>
class world;

template <typename WD>
class animation_fsm_system final {
public:
    using components    = typename WD::components;
    using registry_type = typename entity_registry_from_tuple<components>::type;
    using world_type    = world<WD>;

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

template <>
struct vw::ecs::system_trait<vw::ecs::animation_fsm_system> {
    using components = std::tuple<vw::ecs::animation_fsm_component>;
    using resources  = std::tuple<>;
};

#include "vw/ecs/systems/animation_fsm_system.inl.h"

#endif  // VW_ECS_SYSTEMS_ANIMATION_FSM_SYSTEM_H
