#pragma once

#ifndef VW_GFX_ENTITY_GUARD_H
#define VW_GFX_ENTITY_GUARD_H

#include "entity_archetype.h"
#include "world.h"
#include "world_components.h"

namespace vw::gfx {

template <typename WC = base_world_components>
class entity_guard final {
public:
    using world_type            = world<WC>;
    using entity_archetype_type = entity_archetype_from_tuple<WC>::type;

    entity_guard(world_type& world, entity ent, entity_archetype_type archetype);
    ~entity_guard();

    entity_guard(const entity_guard&)            = delete;
    entity_guard& operator=(const entity_guard&) = delete;

    entity_guard(entity_guard&&)            = default;
    entity_guard& operator=(entity_guard&&) = default;

    [[nodiscard]] auto get_entity() const -> entity;

    [[nodiscard]] auto get_archetype() const -> entity_archetype_type;

private:
    world_type* world_;
    entity ent_;
    entity_archetype_type archetype_;
};

}  // namespace vw::gfx

#include "entity_guard.inl.h"

#endif  // VW_GFX_ENTITY_GUARD_H
