#pragma once

#ifndef VW_ECS_HIERARCHY_SYSTEM_H
#define VW_ECS_HIERARCHY_SYSTEM_H

#include "vw/ecs/components/hierarchy_component.h"
#include "vw/ecs/components/transform_component.h"
#include "vw/ecs/entity_registry.h"
#include "vw/ecs/system_trait.h"

namespace vw::ecs {

class world;

class hierarchy_system final {
public:
    using world_type    = world;
    using registry_type = registry;

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

}  // namespace vw::ecs

#endif  // VW_ECS_HIERARCHY_SYSTEM_H
