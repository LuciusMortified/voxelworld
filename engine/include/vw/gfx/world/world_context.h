#pragma once

#ifndef VW_GFX_WORLD_CONTEXT_H
#define VW_GFX_WORLD_CONTEXT_H

#include <cassert>
#include <memory>
#include <typeindex>
#include <unordered_map>

#include "vw/gfx/world/entity_registry.h"
#include "vw/gfx/world/system_trait.h"

namespace vw::gfx {

template <typename WD>
class world_grid;

template <typename WD>
class world;

template <typename WD>
class world_context {
public:
    using components      = typename WD::components;
    using registry_type   = typename entity_registry_from_tuple<components>::type;
    using systems_tuple   = typename WD::systems_tuple;
    using world_grid_type = world_grid<WD>;

    explicit world_context(registry_type& reg)
        : registry_(&reg) {}

    [[nodiscard]] auto registry() -> registry_type& {
        return *registry_;
    }
    [[nodiscard]] auto registry() const -> const registry_type& {
        return *registry_;
    }

    [[nodiscard]] auto get_world_grid() -> std::shared_ptr<world_grid_type> {
        return world_grid_;
    }
    [[nodiscard]] auto get_world_grid() const -> std::shared_ptr<world_grid_type> {
        return world_grid_;
    }

    template <template <typename> class S>
    [[nodiscard]] auto get_system() -> S<WD>& {
        return std::get<S<WD>>(*systems_);
    }

    template <template <typename> class S>
    [[nodiscard]] auto get_system() const -> const S<WD>& {
        return std::get<S<WD>>(*systems_);
    }

    template <typename R>
    [[nodiscard]] auto get_resource() -> R& {
        auto it = resources_.find(std::type_index(typeid(R)));
        assert(it != resources_.end());
        return *static_cast<R*>(it->second);
    }

    template <typename R>
    void register_resource_(R* ptr) {
        resources_[std::type_index(typeid(R))] = ptr;
    }

    auto create_entity() -> entity {
        return registry_->create();
    }

    void destroy_entity(entity ent) noexcept {
        registry_->destroy(ent);
    }

    auto batch_create_entities(uint32 count) -> std::vector<entity> {
        return registry_->batch_create(count);
    }

    void batch_destroy_entities(const std::vector<entity>& entities) noexcept {
        registry_->batch_destroy(entities);
    }

    template <typename T>
    void add_component(entity ent, T&& value) {
        registry_->add(ent, std::forward<T>(value));
        using C = std::remove_cvref_t<T>;
        std::apply([ent](auto&... systems) {
            (detail::invoke_on_add<C>(systems, ent), ...);
        }, *systems_);
    }

    template <typename T>
    void remove_component(entity ent) noexcept {
        std::apply([ent](auto&... systems) {
            (detail::invoke_on_remove<T>(systems, ent), ...);
        }, *systems_);
        registry_->template remove<T>(ent);
    }

    template <typename T>
    [[nodiscard]] auto has_component(entity ent) const -> bool {
        return registry_->template has<T>(ent);
    }

    template <typename T>
    [[nodiscard]] auto get_component(entity ent) -> T& {
        return registry_->template get<T>(ent);
    }

    template <typename T>
    [[nodiscard]] auto get_component(entity ent) const -> const T& {
        return registry_->template get<T>(ent);
    }

    void set_systems_ptr_(systems_tuple* ptr) { systems_ = ptr; }

private:
    template <typename> friend class world;

    registry_type*                    registry_    = nullptr;
    systems_tuple*                    systems_     = nullptr;
    std::shared_ptr<world_grid_type>  world_grid_;
    std::unordered_map<std::type_index, void*> resources_;
};

}  // namespace vw::gfx

#endif  // VW_GFX_WORLD_CONTEXT_H
