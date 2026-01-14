#pragma once

#ifndef VW_GFX_WORLD_SYSTEMS_LIGHT_SYSTEM_INL_H
#define VW_GFX_WORLD_SYSTEMS_LIGHT_SYSTEM_INL_H

#include "vw/gfx/world/systems/light_system.h"

namespace vw::gfx {

template <typename... Cs>
light_system<Cs...>::light_system(registry_type& registry)
    : registry_(&registry) {}

template <typename... Cs>
void light_system<Cs...>::update() {
    // Пока пустой, можно использовать для других целей
}

template <typename... Cs>
void light_system<Cs...>::mark_dirty(entity ent) {
    dirty_entities_.insert(ent);
}

template <typename... Cs>
auto light_system<Cs...>::get_render_dirty_entities() -> std::unordered_set<entity>& {
    return render_dirty_entities_;
}

template <typename... Cs>
void light_system<Cs...>::mark_render_dirty(entity ent) {
    render_dirty_entities_.insert(ent);
}

}  // namespace vw::gfx

#endif  // VW_GFX_WORLD_SYSTEMS_LIGHT_SYSTEM_INL_H
