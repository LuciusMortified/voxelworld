#pragma once

#ifndef VW_ECS_H
#define VW_ECS_H

#include <unordered_set>

#include "vw/asset/animation/animation_clip_registry.h"
#include "vw/asset/model/model_registry.h"
#include "vw/ecs/base_world_def.h"
#include "vw/ecs/entity_registry.h"

namespace vw::ecs {

template <typename WD = base_world_def>
class world final {
public:
    using components    = WD::components;
    using registry_type = entity_registry_from_tuple<components>::type;
    using systems       = WD::systems;
    using resources     = WD::resources;

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

    class batch_modifier {
    public:
        batch_modifier(world& w, std::vector<entity> entities);

        template <typename C>
        auto with(const C& value = {}) -> batch_modifier&;

        template <typename C>
        auto without() -> batch_modifier&;

        [[nodiscard]] auto get_entities() const -> const std::vector<entity>&;
        [[nodiscard]] auto release_entities() -> std::vector<entity>;

    private:
        world* world_;
        std::vector<entity> entities_;
    };

    world();
    ~world() = default;

    world(const world&)                    = delete;
    auto operator=(const world&) -> world& = delete;
    world(world&&)                         = delete;
    auto operator=(world&&) -> world&      = delete;

    void update(float32 delta_time);
    void clear_changed();

    [[nodiscard]] auto create() -> modifier;
    [[nodiscard]] auto modify(entity ent) -> modifier;
    void destroy(entity ent) noexcept;

    [[nodiscard]] auto batch_create(uint32 count) -> batch_modifier;
    [[nodiscard]] auto batch_modify(std::vector<entity> entities) -> batch_modifier;
    void batch_destroy(const std::vector<entity>& entities) noexcept;

    template <typename T>
    [[nodiscard]] auto has(entity ent) const -> bool;

    template <typename T>
    [[nodiscard]] auto get(entity ent) -> T&;

    template <typename T>
    [[nodiscard]] auto get(entity ent) const -> const T&;

    template <typename... Cs>
    [[nodiscard]] auto view() -> component_view<registry_type, Cs...>;

    template <template <typename> class S>
    [[nodiscard]] auto system() -> S<WD>& {
        return std::get<S<WD>>(systems_);
    }

    template <template <typename> class S>
    [[nodiscard]] auto system() const -> const S<WD>& {
        return std::get<S<WD>>(systems_);
    }

    template <typename R>
    [[nodiscard]] auto resource() -> R& {
        return std::get<R>(resources_);
    }

    template <typename R>
    [[nodiscard]] auto resource() const -> const R& {
        return std::get<R>(resources_);
    }

    [[nodiscard]] auto registry() -> registry_type&;

    template <typename T>
    [[nodiscard]] auto changed() -> std::unordered_set<entity>&;

    [[nodiscard]] auto destroyed() const -> const std::vector<entity>&;

private:
    [[nodiscard]] auto create_entity_() -> entity;
    [[nodiscard]] auto batch_create_entities_(uint32 count) -> std::vector<entity>;

    template <typename T>
    void add_component_(entity ent, T&& value = {});

    template <typename T>
    void remove_component_(entity ent) noexcept;

    template <typename T>
    void batch_add_component_(const std::vector<entity>& entities, const T& value = {});

    template <typename T>
    void batch_remove_component_(const std::vector<entity>& entities) noexcept;

    registry_type registry_;
    systems systems_;
    resources resources_;
};

}  // namespace vw::ecs

#include "vw/ecs/world.inl.h"

#endif  // VW_ECS_H
