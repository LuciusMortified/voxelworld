#pragma once

#ifndef VW_GFX_WORLD_SYSTEMS_PHYSICS_SYSTEM_H
#define VW_GFX_WORLD_SYSTEMS_PHYSICS_SYSTEM_H

#include <algorithm>
#include <memory>

#include "vw/gfx/world/components/rigid_body_component.h"
#include "vw/gfx/world/components/sphere_collider_component.h"
#include "vw/gfx/world/components/transform_component.h"
#include "vw/gfx/world/entity_registry.h"
#include "vw/gfx/world/systems/transform_system.h"
#include "vw/gfx/world_grid/world_grid.h"

namespace vw::gfx {

template <typename WC, typename... Cs>
class physics_system final {
public:
    using registry_type = entity_registry<Cs...>;
    using transform_system_type = transform_system<Cs...>;

    static constexpr float32 gravity = -49.6f;
    static constexpr float32 terminal_velocity = -50.0f;
    static constexpr int32 max_collision_iterations = 4;
    static constexpr float32 fixed_dt = 1.0f / 60.0f;
    static constexpr int32 max_steps_per_frame = 5;

    physics_system(registry_type& registry, transform_system_type& transform_system);

    void set_world_grid(std::shared_ptr<world_grid<WC>> grid);

    void update(float32 delta_time);

    class rigid_body_modifier {
    public:
        auto set_velocity(const vec3f& vel) -> rigid_body_modifier&;
        auto set_gravity_scale(float32 scale) -> rigid_body_modifier&;
        auto add_impulse(const vec3f& impulse) -> rigid_body_modifier&;

    private:
        friend class physics_system;
        rigid_body_modifier(physics_system* system, entity ent);

        physics_system* system_;
        entity entity_;
    };

    class collider_modifier {
    public:
        auto set_radius(float32 r) -> collider_modifier&;
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
    [[nodiscard]] auto are_chunks_loaded(const vec3f& position, float32 radius) const -> bool;

    struct collision_result {
        vec3f resolved_position;
        bool grounded = false;
    };

    [[nodiscard]] auto resolve_sphere_voxel(
        vec3f center, float32 radius, vec3f& velocity
    ) const -> collision_result;

    registry_type* registry_;
    transform_system_type* transform_system_;
    std::shared_ptr<world_grid<WC>> world_grid_;
    float32 accumulated_time_ = 0.0f;
};

template <typename WC, typename... Cs>
struct physics_system_from_tuple;

template <typename WC, typename... Cs>
struct physics_system_from_tuple<WC, std::tuple<Cs...>> {
    using type = physics_system<WC, Cs...>;
};

}  // namespace vw::gfx

#include "vw/gfx/world/systems/physics_system.inl.h"

#endif  // VW_GFX_WORLD_SYSTEMS_PHYSICS_SYSTEM_H
