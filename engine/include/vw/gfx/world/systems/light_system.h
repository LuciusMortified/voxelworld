#pragma once

#ifndef VW_GFX_WORLD_SYSTEMS_LIGHT_SYSTEM_H
#define VW_GFX_WORLD_SYSTEMS_LIGHT_SYSTEM_H

#include <unordered_set>

#include "vw/gfx/world/registry.h"
#include "vw/gfx/world/components/light_component.h"

namespace vw::gfx {

template <typename... Cs>
class light_system final {
public:
    using registry_type = registry<Cs...>;
    
    explicit light_system(registry_type& registry);
    
    void update();  // Пока пустой, можно использовать для других целей
    
    void mark_dirty(entity ent);  // Опционально
    
    [[nodiscard]] auto get_render_dirty_entities() -> std::unordered_set<entity>&;
    void mark_render_dirty(entity ent);
    
private:
    registry_type* registry_;
    std::unordered_set<entity> dirty_entities_;
    std::unordered_set<entity> render_dirty_entities_;
};

template <typename... Cs>
struct light_system_from_tuple;

template <typename... Cs>
struct light_system_from_tuple<std::tuple<Cs...>> {
    using type = light_system<Cs...>;
};

}  // namespace vw::gfx

#include "vw/gfx/world/systems/light_system.inl.h"

#endif  // VW_GFX_WORLD_SYSTEMS_LIGHT_SYSTEM_H
