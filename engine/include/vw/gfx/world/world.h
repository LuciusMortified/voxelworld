#pragma once

#ifndef VW_GFX_WORLD_H
#define VW_GFX_WORLD_H

#include <memory>
#include <unordered_set>

#include "vw/asset/animation/animation_clip_registry.h"
#include "vw/asset/model/model_registry.h"
#include "vw/gfx/world/base_world_def.h"
#include "vw/gfx/world/entity_registry.h"
#include "vw/gfx/world/world_context.h"

namespace vw::gfx {

template <typename WD>
class world_grid;

template <typename WD = base_world_def>
class world final {

public:
    using components       = typename WD::components;
    using registry_type    = typename entity_registry_from_tuple<components>::type;
    using context_type     = world_context<WD>;
    using systems_tuple    = typename WD::systems_tuple;
    using resources_tuple  = typename WD::resources;

    world();
    ~world();

    world(const world&)                    = delete;
    auto operator=(const world&) -> world& = delete;
    world(world&&)                         = delete;
    auto operator=(world&&) -> world&      = delete;

    void update(float32 delta_time);

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

    void set_grid(std::unique_ptr<world_grid<WD>> grid);
    [[nodiscard]] auto get_grid() -> world_grid<WD>*;
    [[nodiscard]] auto get_grid() const -> const world_grid<WD>*;
    [[nodiscard]] auto has_grid() const -> bool;

    [[nodiscard]] auto get_context() -> context_type& { return context_; }
    [[nodiscard]] auto get_context() const -> const context_type& { return context_; }

    [[nodiscard]] auto get_registry() -> registry_type&;

    template <typename T>
    [[nodiscard]] auto changed() -> std::unordered_set<entity>&;

    [[nodiscard]] auto destroyed() const -> const std::vector<entity>&;

    void clear_changed();

private:
    template <typename T>
    void add_component(entity ent, T&& value = {});

    template <typename T>
    void remove_component(entity ent) noexcept;

    [[nodiscard]] auto create_entity() -> entity;
    void destroy_entity(entity ent) noexcept;

    [[nodiscard]] auto batch_create_entities(uint32 count) -> std::vector<entity>;
    void batch_destroy_entities(const std::vector<entity>& entities) noexcept;

    registry_type                    registry_;
    context_type                     context_;
    systems_tuple                    systems_;
    resources_tuple                  resources_;
    std::unique_ptr<world_grid<WD>>  grid_;
};

}  // namespace vw::gfx

#include "vw/gfx/world/world.inl.h"

#endif  // VW_GFX_WORLD_H
