#pragma once

#ifndef VW_GFX_HIERARCHY_SYSTEM_H
#define VW_GFX_HIERARCHY_SYSTEM_H

#include "vw/gfx/world/components/hierarchy_component.h"
#include "vw/gfx/world/components/transform_component.h"
#include "vw/gfx/world/world_context.h"

namespace vw::gfx {

template <typename>
class transform_system;

template <typename WC>
class hierarchy_system final {
public:
    using context_type          = world_context<WC>;
    using registry_type         = entity_registry_from_tuple<WC>::type;
    using transform_system_type = transform_system<WC>;

    hierarchy_system(context_type& context, transform_system_type& transform_sys);

    class hierarchy_modifier {
    public:
        auto set_parent(entity parent) -> hierarchy_modifier&;
        auto remove_parent() -> hierarchy_modifier&;

    private:
        friend class hierarchy_system;
        hierarchy_modifier(hierarchy_system* system, entity ent);

        hierarchy_system* system_;
        entity entity_;
    };

    void cleanup(entity ent);

    [[nodiscard]] auto modify(entity ent) -> hierarchy_modifier;

    [[nodiscard]] auto get_hierarchy_depth(entity ent) const -> size_t;

private:
    [[nodiscard]] auto check_hierarchy_cycle(entity parent, entity child) const -> bool;

    context_type* context_;
    transform_system_type* transform_system_;
};

}  // namespace vw::gfx

#include "vw/gfx/world/systems/hierarchy_system.inl.h"

#endif  // VW_GFX_HIERARCHY_SYSTEM_H
