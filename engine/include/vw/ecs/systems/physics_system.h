#pragma once

#ifndef VW_ECS_SYSTEMS_PHYSICS_SYSTEM_H
#define VW_ECS_SYSTEMS_PHYSICS_SYSTEM_H

#include <algorithm>
#include <memory>

#include "vw/ecs/components/box_collider_component.h"
#include "vw/ecs/components/movement_intent_component.h"
#include "vw/ecs/components/rigid_body_component.h"
#include "vw/ecs/components/transform_component.h"
#include "vw/ecs/entity_registry.h"
#include "vw/ecs/spatial/spatial_layer.h"
#include "vw/ecs/system_trait.h"

namespace vw::ecs {

class transform_system;

class spatial_system;

class world_grid;

class world;

class debug_primitives;

struct physics_stats {
    float32 step_ms              = 0.0f;
    float32 voxel_collision_ms   = 0.0f;
    float32 entity_collision_ms  = 0.0f;
    float32 entity_query_ms      = 0.0f;
    float32 entity_resolve_ms    = 0.0f;
    int32 step_count             = 0;
    int32 entity_query_results   = 0;
};

class physics_system final {
public:
    using world_type    = world;
    using registry_type = registry;

    static constexpr int32 max_collision_iterations = 4;
    static constexpr float32 fixed_dt = 1.0f / 60.0f;
    static constexpr int32 max_steps_per_frame = 5;

    explicit physics_system(world_type& w);

    void set_gravity(float32 g);
    [[nodiscard]] auto get_gravity() const -> float32;

    void update(float32 delta_time);
    [[nodiscard]] auto get_stats() const -> const physics_stats&;

    class rigid_body_modifier {
    public:
        auto set_velocity(const vec3f& vel) -> rigid_body_modifier&;
        auto set_gravity_scale(float32 scale) -> rigid_body_modifier&;
        auto add_impulse(const vec3f& impulse) -> rigid_body_modifier&;
        auto add_external_impulse(const vec3f& impulse) -> rigid_body_modifier&;
        auto set_drag(float32 drag) -> rigid_body_modifier&;

    private:
        friend class physics_system;
        rigid_body_modifier(physics_system* system, entity ent);

        physics_system* system_;
        entity entity_;
    };

    class collider_modifier {
    public:
        auto set_extents(const vec3f& ext) -> collider_modifier&;
        auto set_offset(const vec3f& offset) -> collider_modifier&;

    private:
        friend class physics_system;
        collider_modifier(physics_system* system, entity ent);

        physics_system* system_;
        entity entity_;
    };

    auto modify(entity ent) -> rigid_body_modifier;
    auto modify_collider(entity ent) -> collider_modifier;

private:
    void step(float32 dt);
    [[nodiscard]] auto are_chunks_loaded(const vec3f& position, const vec3f& extents) const -> bool;

    struct collision_result {
        vec3f resolved_position;
        bool grounded = false;
    };

    [[nodiscard]] auto resolve_box_voxel(
        vec3f center, const vec3f& half_extents, vec3f& velocity
    ) const -> collision_result;

    auto resolve_entity_collisions(
        entity ent, vec3f& position, vec3f& velocity,
        const vec3f& half_extents, const vec3f& offset
    ) -> void;

    world_type* world_;
    float32 gravity_ = -300.f;
    float32 accumulated_time_ = 0.0f;
    physics_stats stats_;
    std::vector<entity> entity_query_cache_;
};

}  // namespace vw::ecs

#endif  // VW_ECS_SYSTEMS_PHYSICS_SYSTEM_H
