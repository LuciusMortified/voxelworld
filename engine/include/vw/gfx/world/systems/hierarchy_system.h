#pragma once

#ifndef VW_GFX_HIERARCHY_SYSTEM_H
#define VW_GFX_HIERARCHY_SYSTEM_H

#include "vw/gfx/world/components/hierarchy_component.h"
#include "vw/gfx/world/components/transform_component.h"
#include "vw/gfx/world/entity_registry.h"
#include "vw/gfx/world/system_trait.h"

namespace vw::gfx {

template <typename>
class world;

template <typename WD>
class hierarchy_system final {
public:
    using world_type    = world<WD>;
    using components    = typename WD::components;
    using registry_type = typename entity_registry_from_tuple<components>::type;

    explicit hierarchy_system(world_type& w);

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

    template <typename C>
        requires std::same_as<C, hierarchy_component>
    void on_remove(entity e) {
        cleanup(e);
    }

    [[nodiscard]] auto modify(entity ent) -> hierarchy_modifier;

    [[nodiscard]] auto get_hierarchy_depth(entity ent) const -> size_t;

private:
    [[nodiscard]] auto check_hierarchy_cycle(entity parent, entity child) const -> bool;

    world_type* world_;
};

}  // namespace vw::gfx

template <>
struct vw::gfx::system_trait<vw::gfx::hierarchy_system> {
    using components = std::tuple<vw::gfx::hierarchy_component>;
    using resources  = std::tuple<>;
};

#include "vw/gfx/world/systems/hierarchy_system.inl.h"

#endif  // VW_GFX_HIERARCHY_SYSTEM_H
