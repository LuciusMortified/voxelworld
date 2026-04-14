#pragma once

#ifndef VW_GFX_WORLD_CONTEXT_H
#define VW_GFX_WORLD_CONTEXT_H

#include <cassert>
#include <memory>
#include <typeindex>
#include <unordered_map>

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
    using mesh_pool_type  = mesh_pool;

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

    template <template <typename> class S>
    [[nodiscard]] auto get_system() -> S<WC>& {
        auto it = systems_.find(std::type_index(typeid(S<WC>)));
        assert(it != systems_.end());
        return *static_cast<S<WC>*>(it->second);
    }

    template <typename R>
    [[nodiscard]] auto get_resource() -> R& {
        auto it = resources_.find(std::type_index(typeid(R)));
        assert(it != resources_.end());
        return *static_cast<R*>(it->second);
    }

    template <template <typename> class S>
    void register_system_(S<WC>* ptr) {
        systems_[std::type_index(typeid(S<WC>))] = ptr;
    }

    template <typename R>
    void register_resource_(R* ptr) {
        resources_[std::type_index(typeid(R))] = ptr;
    }

private:
    friend class world<WC>;

    registry_type*                    registry_   = nullptr;
    mesh_pool_type*                   mesh_pool_  = nullptr;
    std::shared_ptr<world_grid_type>  world_grid_;
    std::unordered_map<std::type_index, void*> systems_;
    std::unordered_map<std::type_index, void*> resources_;
};

}  // namespace vw::gfx

#endif  // VW_GFX_WORLD_CONTEXT_H