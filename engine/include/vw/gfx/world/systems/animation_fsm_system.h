#pragma once

#ifndef VW_GFX_WORLD_SYSTEMS_ANIMATION_FSM_SYSTEM_H
#define VW_GFX_WORLD_SYSTEMS_ANIMATION_FSM_SYSTEM_H

#include "vw/gfx/animation/animation_fsm.h"
#include "vw/gfx/world/components/animation_player_component.h"
#include "vw/gfx/world/components/animation_fsm_component.h"
#include "vw/gfx/world/entity_registry.h"
#include "vw/gfx/world/world_context.h"

namespace vw::gfx {

template <typename WC>
class animation_system;

template <typename WC>
class animation_fsm_system final {
public:
    using registry_type         = entity_registry_from_tuple<WC>::type;
    using context_type          = world_context<WC>;
    using animation_system_type = animation_system<WC>;

    explicit animation_fsm_system(
        context_type& context,
        animation_system_type& anim_system
    );

    void update();

    class modifier {
    public:
        void add_machine(size_t index, animation_fsm machine) const;
        void fire_trigger(std::string_view name) const;

    private:
        friend class animation_fsm_system;
        explicit modifier(animation_fsm_component* component);

        animation_fsm_component* component_;
    };

    auto modify(entity ent) -> modifier;

private:
    context_type* context_;
    animation_system_type* anim_system_;
};

}  // namespace vw::gfx

#include "vw/gfx/world/systems/animation_fsm_system.inl.h"

#endif  // VW_GFX_WORLD_SYSTEMS_ANIMATION_FSM_SYSTEM_H