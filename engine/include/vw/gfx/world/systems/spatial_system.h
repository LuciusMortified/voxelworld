#pragma once

#ifndef VW_GFX_WORLD_SYSTEMS_SPATIAL_SYSTEM_H
#define VW_GFX_WORLD_SYSTEMS_SPATIAL_SYSTEM_H

#include <algorithm>
#include <optional>
#include <span>
#include <unordered_set>
#include <vector>

#include "vw/gfx/spatial/aabb.h"
#include "vw/gfx/spatial/dynamic_aabb_tree.h"
#include "vw/gfx/spatial/frustum.h"
#include "vw/gfx/spatial/ray.h"
#include "vw/gfx/world/components/spatial_component.h"
#include "vw/gfx/world/entity_registry.h"
#include "vw/gfx/world/spatial_layer.h"

namespace vw::gfx {

struct model_component;
struct transform_component;

template <typename... Cs>
class spatial_system {
public:
    using registry_type = entity_registry<Cs...>;

    explicit spatial_system(
        registry_type& registry
    );

    void update();

    void query_all(
        const frustum& f,
        std::unordered_set<entity>& result_out,
        spatial_layer_mask layer_mask = spatial_layer::all
    ) const;

    void query_all(
        const ray& r,
        std::unordered_set<entity>& result_out,
        spatial_layer_mask layer_mask = spatial_layer::all
    ) const;

    void query_all(
        const aabb& bounds,
        std::unordered_set<entity>& result_out,
        spatial_layer_mask layer_mask = spatial_layer::all
    ) const;

    void query_all_any(
        std::span<const frustum> frustums,
        std::vector<entity>& result_out
    ) const;

    [[nodiscard]] auto voxel_ray_cast(
        const ray& r,
        std::unordered_set<entity>& candidates,
        spatial_layer_mask layer_mask = spatial_layer::all
    ) const -> std::optional<voxel_ray_hit>;

    void cleanup(entity ent);

    class spatial_modifier {
    public:
        auto set_layer(spatial_layer_mask layer) -> spatial_modifier&;

    private:
        friend class spatial_system;
        spatial_modifier(spatial_system* system, entity ent);

        spatial_system* system_;
        entity entity_;
    };

    auto modify(entity ent) -> spatial_modifier;

private:
    void update_entity(entity ent);
    auto calculate_aabb_from_model(
        entity ent,
        const model_component& model_comp,
        const transform_component& transform_comp
    ) const -> aabb;
    auto expand_aabb_for_fat(const aabb& bounds) const -> aabb;

    registry_type* registry_;
    dynamic_aabb_tree tree_;
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
