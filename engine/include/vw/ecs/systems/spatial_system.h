#pragma once

#ifndef VW_ECS_SYSTEMS_SPATIAL_SYSTEM_H
#define VW_ECS_SYSTEMS_SPATIAL_SYSTEM_H

#include <algorithm>
#include <optional>
#include <span>
#include <vector>

#include "vw/ecs/spatial/dynamic_aabb_tree.h"
#include "vw/ecs/components/spatial_component.h"
#include "vw/ecs/entity.h"
#include "vw/ecs/entity_registry.h"
#include "vw/ecs/spatial/spatial_layer.h"
#include "vw/ecs/system_trait.h"
#include "vw/spatial/aabb.h"
#include "vw/spatial/frustum.h"
#include "vw/spatial/ray.h"

namespace vw::ecs {


struct model_component;
struct transform_component;

template <typename>
class world;

struct voxel_ray_hit {
    entity ent;
    vec3i voxel_pos;
    vec3i empty_pos;
};

template <typename WD>
class spatial_system {
public:
    using components    = typename WD::components;
    using registry_type = typename entity_registry_from_tuple<components>::type;
    using world_type    = world<WD>;

    explicit spatial_system(
        world_type& w
    );

    void update(float32 dt);

    void query_all(
        const vw::spatial::frustum& f,
        std::vector<entity>& result_out,
        spatial_layer_mask layer_mask = spatial_layer::all
    ) const;

    void query_all(
        const vw::spatial::ray& r,
        std::vector<entity>& result_out,
        spatial_layer_mask layer_mask = spatial_layer::all
    ) const;

    void query_all(
        const vw::spatial::aabb& bounds,
        std::vector<entity>& result_out,
        spatial_layer_mask layer_mask = spatial_layer::all
    ) const;

    void query_all_any(
        std::span<const vw::spatial::frustum> frustums,
        std::vector<entity>& result_out
    ) const;

    [[nodiscard]] auto voxel_ray_cast(
        const vw::spatial::ray& r,
        std::vector<entity>& candidates,
        spatial_layer_mask layer_mask = spatial_layer::all
    ) const -> std::optional<voxel_ray_hit>;

    void cleanup(entity ent);

    template <typename C>
        requires std::same_as<C, spatial_component>
    void on_remove(entity e) {
        cleanup(e);
    }

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
    ) const -> vw::spatial::aabb;
    static auto expand_aabb_for_fat(const vw::spatial::aabb& bounds) -> vw::spatial::aabb;

    world_type* world_;
    dynamic_aabb_tree tree_;
};

}  // namespace vw::ecs

template <>
struct vw::ecs::system_trait<vw::ecs::spatial_system> {
    using components = std::tuple<vw::ecs::spatial_component>;
    using resources  = std::tuple<>;
};

#include "vw/ecs/systems/spatial_system.inl.h"

#endif  // VW_ECS_SYSTEMS_SPATIAL_SYSTEM_H
