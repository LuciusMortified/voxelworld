#pragma once

#ifndef VW_GFX_WORLD_H
#define VW_GFX_WORLD_H

#include <unordered_set>

#include "vw/asset/animation/animation_clip_registry.h"
#include "vw/asset/model/model_registry.h"
#include "vw/gfx/world/base_world_def.h"
#include "vw/gfx/world/entity_registry.h"

namespace vw::gfx {

template <typename WD = base_world_def>
class world final {
public:
    using components      = WD::components;
    using registry_type   = entity_registry_from_tuple<components>::type;
    using systems_tuple   = WD::systems_tuple;
    using resources_tuple = WD::resources;

    class modifier {
    public:
        modifier(world& w, entity ent);

        template <typename C>
        auto with(C&& value = {}) -> modifier&;

        template <typename C>
        auto without() -> modifier&;

        [[nodiscard]] auto get_entity() const -> entity;

    private:
        world* world_;
        entity ent_;
    };

    world();
    ~world() = default;

    world(const world&)                    = delete;
    auto operator=(const world&) -> world& = delete;
    world(world&&)                         = delete;
    auto operator=(world&&) -> world&      = delete;

    void update(float32 delta_time);

    [[nodiscard]] auto create_entity() -> entity;
    void destroy_entity(entity ent) noexcept;

    [[nodiscard]] auto batch_create_entities(uint32 count) -> std::vector<entity>;
    void batch_destroy_entities(const std::vector<entity>& entities) noexcept;

    [[nodiscard]] auto create() -> modifier;
    [[nodiscard]] auto modify(entity ent) -> modifier;

    template <typename T>
    void add_component(entity ent, T&& value = {});

    template <typename T>
    void remove_component(entity ent) noexcept;

    template <typename T>
    [[nodiscard]] auto has_component(entity ent) const -> bool;

    template <typename T>
    [[nodiscard]] auto get_component(entity ent) -> T&;

    template <typename T>
    [[nodiscard]] auto get_component(entity ent) const -> const T&;

    template <typename... Cs>
    [[nodiscard]] auto view_components() -> component_view<registry_type, Cs...>;

    template <template <typename> class S>
    [[nodiscard]] auto get_system() -> S<WD>& {
        return std::get<S<WD>>(systems_);
    }

    template <template <typename> class S>
    [[nodiscard]] auto get_system() const -> const S<WD>& {
        return std::get<S<WD>>(systems_);
    }

    template <typename R>
    [[nodiscard]] auto get_resource() -> R& {
        return std::get<R>(resources_);
    }

    template <typename R>
    [[nodiscard]] auto get_resource() const -> const R& {
        return std::get<R>(resources_);
    }

    [[nodiscard]] auto get_registry() -> registry_type&;

    template <typename T>
    [[nodiscard]] auto changed() -> std::unordered_set<entity>&;

    [[nodiscard]] auto destroyed() const -> const std::vector<entity>&;

    void clear_changed();

private:
    registry_type registry_;
    systems_tuple systems_;
    resources_tuple resources_;
};

}  // namespace vw::gfx

#include "vw/gfx/world/world.inl.h"

#endif  // VW_GFX_WORLD_H
