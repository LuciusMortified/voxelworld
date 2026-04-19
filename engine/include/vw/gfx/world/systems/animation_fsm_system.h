#pragma once

#ifndef VW_GFX_WORLD_SYSTEMS_ANIMATION_FSM_SYSTEM_H
#define VW_GFX_WORLD_SYSTEMS_ANIMATION_FSM_SYSTEM_H

#include "vw/asset/animation/animation_fsm.h"
#include "vw/gfx/world/components/animation_player_component.h"
#include "vw/gfx/world/components/animation_fsm_component.h"
#include "vw/gfx/world/system_trait.h"
#include "vw/gfx/world/entity_registry.h"
#include "vw/gfx/world/world_context.h"

namespace vw::gfx {


template <typename WD>
class animation_system;

template <typename WD>
class animation_fsm_system final {
public:
    using components = typename WD::components;
    using registry_type = typename entity_registry_from_tuple<components>::type;
    using context_type  = world_context<WD>;

    explicit animation_fsm_system(context_type& context);

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
    context_type* context_;
};

}  // namespace vw::gfx

template <>
struct vw::gfx::system_trait<vw::gfx::animation_fsm_system> {
    using components = std::tuple<vw::gfx::animation_fsm_component>;
    using resources  = std::tuple<>;
};

#include "vw/gfx/world/systems/animation_fsm_system.inl.h"

#endif  // VW_GFX_WORLD_SYSTEMS_ANIMATION_FSM_SYSTEM_H