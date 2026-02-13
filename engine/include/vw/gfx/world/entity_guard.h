#pragma once

#ifndef VW_GFX_ENTITY_GUARD_H
#define VW_GFX_ENTITY_GUARD_H

#include "entity_archetype.h"
#include "world_components.h"

namespace vw::gfx {

template <typename WC>
class world;

template <typename WC = base_world_components>
class entity_guard_group;

template <typename WC = base_world_components>
class entity_guard final {
public:
    using world_type            = world<WC>;
    using entity_archetype_type = typename entity_archetype_from_tuple<WC>::type;

    explicit entity_guard(world_type& world);
    entity_guard(world_type& world, entity ent, entity_archetype_type archetype);
    ~entity_guard();

    entity_guard(const entity_guard&)            = delete;
    auto operator=(const entity_guard&) -> entity_guard& = delete;

    entity_guard(entity_guard&& other) noexcept;
    auto operator=(entity_guard&& other) noexcept -> entity_guard&;

    template <typename C>
    auto with(C&& value = {}) -> entity_guard&;

    template <typename C>
    auto without() -> entity_guard&;

    [[nodiscard]] auto get_entity() const -> entity;
    [[nodiscard]] auto get_archetype() const -> entity_archetype_type;
    [[nodiscard]] auto is_valid() const -> bool;
    [[nodiscard]] auto release() -> entity;
    void update_archetype();

private:
    void cleanup_() noexcept;

    world_type* world_ = nullptr;
    entity ent_        = invalid_entity;
    entity_archetype_type archetype_;
};

}  // namespace vw::gfx

#include "entity_guard.inl.h"

#endif  // VW_GFX_ENTITY_GUARD_H
