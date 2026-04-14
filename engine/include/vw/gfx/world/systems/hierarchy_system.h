#pragma once

#ifndef VW_GFX_HIERARCHY_SYSTEM_H
#define VW_GFX_HIERARCHY_SYSTEM_H

#include "vw/gfx/world/components/hierarchy_component.h"
#include "vw/gfx/world/components/transform_component.h"
#include "vw/gfx/world/system_trait.h"
#include "vw/gfx/world/world_context.h"

namespace vw::gfx {

template <typename WC>
class hierarchy_system final {
public:
    using context_type  = world_context<WC>;
    using registry_type = entity_registry_from_tuple<WC>::type;

    explicit hierarchy_system(context_type& context);

    void update(float32 dt);

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
};

}  // namespace vw::gfx

template <>
struct vw::gfx::system_trait<vw::gfx::hierarchy_system> {
    using components = std::tuple<vw::gfx::hierarchy_component>;
    using depends_on = vw::gfx::system_list<>;
    using resources  = std::tuple<>;
};

#include "vw/gfx/world/systems/hierarchy_system.inl.h"

#endif  // VW_GFX_HIERARCHY_SYSTEM_H
