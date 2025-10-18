#pragma once

#ifndef VW_GFX_HIERARCHY_SYSTEM_H
#define VW_GFX_HIERARCHY_SYSTEM_H

#include "vw/gfx/world/registry.h"

#include "vw/gfx/world/components/hierarchy_component.h"
#include "vw/gfx/world/components/transform_component.h"

#include "vw/gfx/world/systems/transform_system.h"

namespace vw::gfx {


template <typename... Cs>
class hierarchy_system final {
public:
    using registry_type = registry<Cs...>;
    using transform_system_type = transform_system<Cs...>;

    hierarchy_system(registry_type& registry, transform_system_type& transform_sys);

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
    
    [[nodiscard]] auto modify(entity ent) -> hierarchy_modifier;

private:
    [[nodiscard]] auto check_hierarchy_cycle(entity parent, entity child) const -> bool;

    registry_type* registry_;
    transform_system_type* transform_system_;
};

template <typename... Cs>
struct hierarchy_system_from_tuple;

template <typename... Cs>
struct hierarchy_system_from_tuple<std::tuple<Cs...>> {
    using type = hierarchy_system<Cs...>;
};

}  // namespace vw::gfx

#include "vw/gfx/world/systems/hierarchy_system.inl.h"

#endif  // VW_GFX_HIERARCHY_SYSTEM_H
