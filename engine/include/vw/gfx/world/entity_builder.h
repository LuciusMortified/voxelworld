#pragma once

#ifndef VW_GFX_ENTITY_BUILDER_H
#define VW_GFX_ENTITY_BUILDER_H

#include "entity_archetype.h"
#include "entity_guard.h"
#include "world.h"
#include "world_components.h"

namespace vw::gfx {

template <typename WC = base_world_components>
class entity_builder final {
public:
    using world_type            = world<WC>;
    using entity_archetype_type = entity_archetype_from_tuple<WC>::type;
    using entity_guard_type     = entity_guard<WC>;

    explicit entity_builder(world_type& world);

    template <typename C>
    auto with() -> entity_builder&;

    [[nodiscard]] auto release() -> entity;

    [[nodiscard]] auto release_guard() -> std::unique_ptr<entity_guard_type>;

private:
    world_type* world_;
    entity ent_;
    entity_archetype_type archetype_;
};

}  // namespace vw::gfx

#include "entity_builder.inl.h"

#endif  // VW_GFX_ENTITY_BUILDER_H
