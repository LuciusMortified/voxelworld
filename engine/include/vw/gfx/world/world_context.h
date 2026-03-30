#pragma once

#ifndef VW_GFX_WORLD_CONTEXT_H
#define VW_GFX_WORLD_CONTEXT_H

#include <memory>

#include "vw/gfx/world/entity_registry.h"

namespace vw::gfx {

class mesh_pool;

template <typename WC>
class world_grid;

template <typename WC>
class world;

template <typename WC>
class world_context {
public:
    using registry_type   = entity_registry_from_tuple<WC>::type;
    using world_grid_type = world_grid<WC>;
    using mesh_pool_type = mesh_pool;

    explicit world_context(
        registry_type& reg, mesh_pool* mp = nullptr, std::shared_ptr<world_grid_type> wg = nullptr
    )
        : registry_(&reg), mesh_pool_(mp), world_grid_(std::move(wg)) {}

    [[nodiscard]] auto registry() -> registry_type& {
        return *registry_;
    }
    [[nodiscard]] auto registry() const -> const registry_type& {
        return *registry_;
    }
    [[nodiscard]] auto get_mesh_pool() -> mesh_pool_type* {
        return mesh_pool_;
    }
    [[nodiscard]] auto get_mesh_pool() const -> const mesh_pool_type* {
        return mesh_pool_;
    }
    [[nodiscard]] auto get_world_grid() -> std::shared_ptr<world_grid_type> {
        return world_grid_;
    }
    [[nodiscard]] auto get_world_grid() const -> std::shared_ptr<world_grid_type> {
        return world_grid_;
    }

private:
    friend class world<WC>;

    registry_type* registry_    = nullptr;
    mesh_pool_type* mesh_pool_ = nullptr;
    std::shared_ptr<world_grid_type> world_grid_;
};

}  // namespace vw::gfx

#endif  // VW_GFX_WORLD_CONTEXT_H