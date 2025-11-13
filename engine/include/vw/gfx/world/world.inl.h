#pragma once

#ifndef VW_GFX_WORLD_WORLD_INL_H
#define VW_GFX_WORLD_WORLD_INL_H

#include "vw/gfx/world/world.h"

#include "vw/gfx/render/vulkan_context.h"

namespace vw::gfx {

inline world::world(vulkan_context& context)
    : transform_system_(registry_),
      hierarchy_system_(registry_, transform_system_),
      mesh_pool_(context),
      model_system_(registry_, mesh_pool_) {}

inline void world::update() {
    transform_system_.update();
    model_system_.update();
}

// Entity management methods with system notifications
inline auto world::create_entity() -> entity {
    return registry_.create();
}

template <typename T>
inline void world::add_component(entity ent, T&& value) {
    registry_.add(ent, std::forward<T>(value));
    
    // Notify systems about new components with dirty flags
    if constexpr (std::same_as<T, transform_component>) {
        transform_system_.mark_world_dirty(ent);
    }
    if constexpr (std::same_as<T, model_component>) {
        model_system_.mark_dirty(ent);
    }
    if constexpr (std::same_as<T, hierarchy_component>) {
        // hierarchy_system doesn't need dirty tracking, but we could add logic here if needed
    }
}

template<typename T>
inline auto world::has_component(entity ent) const -> bool {
    return registry_.has<T>(ent);
}

template <typename T>
inline auto world::get_component(entity ent) -> T& {
    return registry_.get<T>(ent);
}

template <typename T>
inline void world::remove_component(entity ent) {
    registry_.remove<T>(ent);
}

inline void world::remove_all_components(entity ent) {
    registry_.remove_all(ent);
}

inline void world::destroy_entity(entity ent) {
    registry_.destroy(ent);
}

template <typename... Cs>
inline auto world::view_components() -> component_view<registry_type, Cs...> {
    return registry_.view<Cs...>();
}

// System and registry accessors
inline auto world::get_hierarchy_system() -> hierarchy_system_type& {
    return hierarchy_system_;
}

inline auto world::get_transform_system() -> transform_system_type& {
    return transform_system_;
}

inline auto world::get_model_registry() -> model_registry& {
    return model_registry_;
}

inline auto world::get_mesh_pool() -> mesh_pool& {
    return mesh_pool_;
}

inline auto world::get_model_system() -> model_system_type& {
    return model_system_;
}

}  // namespace vw::gfx

#endif  // VW_GFX_WORLD_WORLD_INL_H
