#pragma once

#ifndef VW_GFX_ENTITY_GUARD_H
#define VW_GFX_ENTITY_GUARD_H

#include "entity_archetype.h"
#include "world_context.h"

namespace vw::gfx {

template <typename WD>
class entity_guard_group;

template <typename WD>
class entity_guard final {
public:
    using components            = typename WD::components;
    using context_type          = world_context<WD>;
    using entity_archetype_type = typename entity_archetype_from_tuple<components>::type;

    explicit entity_guard(context_type& ctx);
    entity_guard(context_type& ctx, entity ent, entity_archetype_type archetype);
    ~entity_guard();

    entity_guard(const entity_guard&)                    = delete;
    auto operator=(const entity_guard&) -> entity_guard& = delete;

    entity_guard(entity_guard&& other) noexcept;
    auto operator=(entity_guard&& other) noexcept -> entity_guard&;

    template <typename C>
    auto with(C&& value = {}) -> entity_guard&;

    template <typename C>
    auto without() -> entity_guard&;

    template <typename C>
    [[nodiscard]] auto has() const -> bool;

    template <typename C>
    [[nodiscard]] auto get() const -> const C&;

    [[nodiscard]] auto get_entity() const -> entity;
    [[nodiscard]] auto get_archetype() const -> entity_archetype_type;
    [[nodiscard]] auto is_valid() const -> bool;
    [[nodiscard]] auto release() -> entity;
    void update_archetype();

    [[nodiscard]] auto operator*() const -> entity;

private:
    void cleanup_() noexcept;

    context_type* context_ = nullptr;
    entity ent_            = invalid_entity;
    entity_archetype_type archetype_;
};

}  // namespace vw::gfx

#include "entity_guard.inl.h"

#endif  // VW_GFX_ENTITY_GUARD_H
