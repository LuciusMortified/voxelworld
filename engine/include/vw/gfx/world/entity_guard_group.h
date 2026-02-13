#pragma once

#ifndef VW_GFX_ENTITY_GUARD_GROUP_H
#define VW_GFX_ENTITY_GUARD_GROUP_H

#include <vector>

#include "entity_archetype.h"
#include "world_components.h"

namespace vw::gfx {

template <typename WC>
class world;

template <typename WC>
class entity_guard_group final {
public:
    using world_type            = world<WC>;
    using entity_archetype_type = typename entity_archetype_from_tuple<WC>::type;

    entity_guard_group(world_type& world, uint32 count);
    ~entity_guard_group();

    entity_guard_group(const entity_guard_group&)            = delete;
    auto operator=(const entity_guard_group&) -> entity_guard_group& = delete;

    entity_guard_group(entity_guard_group&& other) noexcept;
    auto operator=(entity_guard_group&& other) noexcept -> entity_guard_group&;

    template <typename C>
    auto with(C&& value = {}) -> entity_guard_group&;

    template <typename C>
    auto without() -> entity_guard_group&;

    [[nodiscard]] auto size() const -> uint32;
    [[nodiscard]] auto operator[](uint32 index) const -> entity;
    [[nodiscard]] auto entities() const -> const std::vector<entity>&;
    [[nodiscard]] auto get_archetype() const -> entity_archetype_type;

    [[nodiscard]] auto begin() const -> std::vector<entity>::const_iterator;
    [[nodiscard]] auto end() const -> std::vector<entity>::const_iterator;

private:
    void cleanup_() noexcept;

    world_type* world_ = nullptr;
    std::vector<entity> entities_;
    entity_archetype_type archetype_;
};

}  // namespace vw::gfx

#include "entity_guard_group.inl.h"

#endif  // VW_GFX_ENTITY_GUARD_GROUP_H
