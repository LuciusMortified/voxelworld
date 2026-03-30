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
using world_grid_ptr = std::shared_ptr<world_grid<WC>>;

template <typename WC>
class world_context {
    friend class world<WC>;

public:
    using registry_type = typename entity_registry_from_tuple<WC>::type;
    using grid_ptr = world_grid_ptr<WC>;

    explicit world_context(registry_type& reg)
        : registry_(&reg) {}

    world_context(registry_type& reg, class mesh_pool* mp, grid_ptr wg = {})
        : registry_(&reg), mesh_pool_(mp), world_grid_(std::move(wg)) {}

    [[nodiscard]] auto registry() -> registry_type& { return *registry_; }
    [[nodiscard]] auto registry() const -> const registry_type& { return *registry_; }
    [[nodiscard]] auto mesh_pool() -> class mesh_pool* { return mesh_pool_; }
    [[nodiscard]] auto mesh_pool() const -> const class mesh_pool* { return mesh_pool_; }
    [[nodiscard]] auto world_grid() -> grid_ptr& { return world_grid_; }
    [[nodiscard]] auto world_grid() const -> const grid_ptr& { return world_grid_; }

private:

    registry_type* registry_ = nullptr;
    class mesh_pool* mesh_pool_ = nullptr;
    grid_ptr world_grid_;
};

}  // namespace vw::gfx

#endif  // VW_GFX_WORLD_CONTEXT_H