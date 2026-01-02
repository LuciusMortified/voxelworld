#pragma once

#ifndef VW_GFX_WORLD_SYSTEMS_SPATIAL_SYSTEM_H
#define VW_GFX_WORLD_SYSTEMS_SPATIAL_SYSTEM_H

#include <unordered_set>
#include <optional>

#include "vw/gfx/spatial/dynamic_aabb_tree.h"
#include "vw/gfx/spatial/frustum.h"
#include "vw/gfx/spatial/ray.h"
#include "vw/gfx/spatial/aabb.h"
#include "vw/gfx/world/components/spatial_component.h"
#include "vw/gfx/world/registry.h"

namespace vw::gfx {

struct model_component;
struct transform_component;

template <typename... Cs>
class spatial_system {
public:
    using registry_type = registry<Cs...>;
    
    explicit spatial_system(
        registry_type& registry
    );
    
    void update();
    
    // Методы запросов - возвращают все попавшие сущности
    void query_all(
        const frustum& f,
        std::unordered_set<entity>& result_out
    ) const;
    
    void query_all(
        const ray& r,
        std::unordered_set<entity>& result_out
    ) const;
    
    void query_all(
        const aabb& bounds,
        std::unordered_set<entity>& result_out
    ) const;
    
    [[nodiscard]] auto voxel_ray_cast(
        const ray& r,
        std::unordered_set<entity>& candidates
    ) const -> std::optional<voxel_ray_hit>;
    
    void mark_dirty(entity ent);

    void cleanup(entity ent);
    
private:
    void update_entity(entity ent);
    aabb calculate_aabb_from_model(
        entity ent,
        const model_component& model_comp,
        const transform_component& transform_comp
    ) const;
    aabb expand_aabb_for_fat(const aabb& bounds) const;
    
    registry_type* registry_;
    dynamic_aabb_tree tree_;
    std::unordered_set<entity> dirty_entities_;
};

template <typename... Cs>
struct spatial_system_from_tuple;

template <typename... Cs>
struct spatial_system_from_tuple<std::tuple<Cs...>> {
    using type = spatial_system<Cs...>;
};

}  // namespace vw::gfx

#include "vw/gfx/world/systems/spatial_system.inl.h"

#endif  // VW_GFX_WORLD_SYSTEMS_SPATIAL_SYSTEM_H

