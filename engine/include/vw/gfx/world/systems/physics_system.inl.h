#pragma once

#ifndef VW_GFX_WORLD_SYSTEMS_PHYSICS_SYSTEM_INL_H
#define VW_GFX_WORLD_SYSTEMS_PHYSICS_SYSTEM_INL_H

#include <cmath>

namespace vw::gfx {

template <typename WC, typename... Cs>
physics_system<WC, Cs...>::physics_system(
    registry_type& registry, transform_system_type& transform_system
)
    : registry_(&registry), transform_system_(&transform_system) {}

template <typename WC, typename... Cs>
void physics_system<WC, Cs...>::set_world_grid(
    std::shared_ptr<world_grid<WC>> grid
) {
    world_grid_ = std::move(grid);
}

template <typename WC, typename... Cs>
void physics_system<WC, Cs...>::update(
    float32 delta_time
) {
    if (!world_grid_) {
        return;
    }

    accumulated_time_ += delta_time;
    auto max_accumulated = fixed_dt * static_cast<float32>(max_steps_per_frame);
    if (accumulated_time_ > max_accumulated) {
        accumulated_time_ = max_accumulated;
    }

    while (accumulated_time_ >= fixed_dt) {
        step(fixed_dt);
        accumulated_time_ -= fixed_dt;
    }
}

template <typename WC, typename... Cs>
void physics_system<WC, Cs...>::step(
    float32 dt
) {
    for (auto [ent, rb_const, tc_const] :
         registry_->template view<rigid_body_component, transform_component>()) {
        auto& rb = registry_->template get<rigid_body_component>(ent);
        auto position = tc_const.get_position();

        rb.velocity_.y += gravity * rb.gravity_scale_ * dt;
        if (rb.velocity_.y < terminal_velocity) {
            rb.velocity_.y = terminal_velocity;
        }

        auto new_position = position + rb.velocity_ * dt;

        if (registry_->template has<sphere_collider_component>(ent)) {
            const auto& col = registry_->template get<sphere_collider_component>(ent);
            auto sphere_center = new_position + col.offset_;

            if (!are_chunks_loaded(sphere_center, col.radius_)) {
                rb.frozen_ = true;
                rb.velocity_ = {0.0f, 0.0f, 0.0f};
                continue;
            }

            rb.frozen_ = false;
            auto result = resolve_sphere_voxel(sphere_center, col.radius_, rb.velocity_);
            new_position = result.resolved_position - col.offset_;
            rb.grounded_ = result.grounded;
        }

        transform_system_->modify(ent).set_position(new_position);
    }
}

template <typename WC, typename... Cs>
auto physics_system<WC, Cs...>::are_chunks_loaded(
    const vec3f& position, float32 radius
) const -> bool {
    auto vs = static_cast<float32>(world_grid_->voxel_scale());

    auto min_world = vec3i{
        static_cast<int32>(std::floor((position.x - radius) / vs) * vs),
        static_cast<int32>(std::floor((position.y - radius) / vs) * vs),
        static_cast<int32>(std::floor((position.z - radius) / vs) * vs)
    };
    auto max_world = vec3i{
        static_cast<int32>(std::floor((position.x + radius) / vs) * vs),
        static_cast<int32>(std::floor((position.y + radius) / vs) * vs),
        static_cast<int32>(std::floor((position.z + radius) / vs) * vs)
    };

    auto min_chunk = world_grid_->world_to_chunk_coord(min_world);
    auto max_chunk = world_grid_->world_to_chunk_coord(max_world);

    for (int32 cx = min_chunk.x; cx <= max_chunk.x; ++cx) {
        for (int32 cy = min_chunk.y; cy <= max_chunk.y; ++cy) {
            for (int32 cz = min_chunk.z; cz <= max_chunk.z; ++cz) {
                if (!world_grid_->has_chunk({cx, cy, cz})) {
                    return false;
                }
            }
        }
    }

    return true;
}

template <typename WC, typename... Cs>
auto physics_system<WC, Cs...>::resolve_sphere_voxel(
    vec3f center, float32 radius, vec3f& velocity
) const -> collision_result {
    auto vs = static_cast<float32>(world_grid_->voxel_scale());
    auto vs_i = world_grid_->voxel_scale();
    bool grounded = false;
    constexpr float32 epsilon = 0.0001f;

    for (int32 iter = 0; iter < max_collision_iterations; ++iter) {
        auto min_vx = static_cast<int32>(std::floor((center.x - radius) / vs));
        auto min_vy = static_cast<int32>(std::floor((center.y - radius) / vs));
        auto min_vz = static_cast<int32>(std::floor((center.z - radius) / vs));
        auto max_vx = static_cast<int32>(std::floor((center.x + radius) / vs));
        auto max_vy = static_cast<int32>(std::floor((center.y + radius) / vs));
        auto max_vz = static_cast<int32>(std::floor((center.z + radius) / vs));

        float32 deepest_penetration = 0.0f;
        vec3f deepest_normal{0.0f, 0.0f, 0.0f};

        for (int32 vx = min_vx; vx <= max_vx; ++vx) {
            for (int32 vy = min_vy; vy <= max_vy; ++vy) {
                for (int32 vz = min_vz; vz <= max_vz; ++vz) {
                    auto world_pos = vec3i{vx * vs_i, vy * vs_i, vz * vs_i};
                    auto v = world_grid_->get_voxel(world_pos);
                    if (v.is_empty()) {
                        continue;
                    }

                    auto box_min = vec3f{
                        static_cast<float32>(vx) * vs,
                        static_cast<float32>(vy) * vs,
                        static_cast<float32>(vz) * vs
                    };
                    auto box_max = vec3f{
                        box_min.x + vs,
                        box_min.y + vs,
                        box_min.z + vs
                    };

                    auto closest = vec3f{
                        math::clamp(center.x, box_min.x, box_max.x),
                        math::clamp(center.y, box_min.y, box_max.y),
                        math::clamp(center.z, box_min.z, box_max.z)
                    };

                    auto diff = center - closest;
                    auto dist_sq = math::dot(diff, diff);

                    if (dist_sq >= radius * radius) {
                        continue;
                    }

                    if (dist_sq < epsilon) {
                        auto cx_dist = std::min(center.x - box_min.x, box_max.x - center.x);
                        auto cy_dist = std::min(center.y - box_min.y, box_max.y - center.y);
                        auto cz_dist = std::min(center.z - box_min.z, box_max.z - center.z);

                        vec3f push_normal{0.0f, 1.0f, 0.0f};
                        auto min_dist = cy_dist;

                        if (cx_dist < min_dist) {
                            min_dist = cx_dist;
                            push_normal = {center.x - box_min.x < box_max.x - center.x ? -1.0f : 1.0f, 0.0f, 0.0f};
                        }
                        if (cz_dist < min_dist) {
                            push_normal = {0.0f, 0.0f, center.z - box_min.z < box_max.z - center.z ? -1.0f : 1.0f};
                        }

                        auto penetration = radius + min_dist;
                        if (penetration > deepest_penetration) {
                            deepest_penetration = penetration;
                            deepest_normal = push_normal;
                        }
                        continue;
                    }

                    auto dist = std::sqrt(dist_sq);
                    auto penetration = radius - dist;
                    auto normal = diff / dist;

                    if (penetration > deepest_penetration) {
                        deepest_penetration = penetration;
                        deepest_normal = normal;
                    }
                }
            }
        }

        if (deepest_penetration <= 0.0f) {
            break;
        }

        center = center + deepest_normal * deepest_penetration;

        auto vel_along_normal = math::dot(velocity, deepest_normal);
        if (vel_along_normal < 0.0f) {
            velocity = velocity - deepest_normal * vel_along_normal;
        }

        if (deepest_normal.y > 0.7f) {
            grounded = true;
        }
    }

    return {center, grounded};
}

template <typename WC, typename... Cs>
physics_system<WC, Cs...>::rigid_body_modifier::rigid_body_modifier(
    physics_system* system, entity ent
)
    : system_(system), entity_(ent) {}

template <typename WC, typename... Cs>
auto physics_system<WC, Cs...>::modify(
    entity ent
) -> rigid_body_modifier {
    return rigid_body_modifier(this, ent);
}

template <typename WC, typename... Cs>
auto physics_system<WC, Cs...>::rigid_body_modifier::set_velocity(
    const vec3f& vel
) -> rigid_body_modifier& {
    if (!system_->registry_->template has<rigid_body_component>(entity_)) {
        return *this;
    }
    auto& comp = system_->registry_->template get<rigid_body_component>(entity_);
    comp.velocity_ = vel;
    return *this;
}

template <typename WC, typename... Cs>
auto physics_system<WC, Cs...>::rigid_body_modifier::set_gravity_scale(
    float32 scale
) -> rigid_body_modifier& {
    if (!system_->registry_->template has<rigid_body_component>(entity_)) {
        return *this;
    }
    auto& comp = system_->registry_->template get<rigid_body_component>(entity_);
    comp.gravity_scale_ = scale;
    return *this;
}

template <typename WC, typename... Cs>
auto physics_system<WC, Cs...>::rigid_body_modifier::add_impulse(
    const vec3f& impulse
) -> rigid_body_modifier& {
    if (!system_->registry_->template has<rigid_body_component>(entity_)) {
        return *this;
    }
    auto& comp = system_->registry_->template get<rigid_body_component>(entity_);
    comp.velocity_ = comp.velocity_ + impulse;
    return *this;
}

template <typename WC, typename... Cs>
physics_system<WC, Cs...>::collider_modifier::collider_modifier(
    physics_system* system, entity ent
)
    : system_(system), entity_(ent) {}

template <typename WC, typename... Cs>
auto physics_system<WC, Cs...>::modify_collider(
    entity ent
) -> collider_modifier {
    return collider_modifier(this, ent);
}

template <typename WC, typename... Cs>
auto physics_system<WC, Cs...>::collider_modifier::set_radius(
    float32 r
) -> collider_modifier& {
    if (!system_->registry_->template has<sphere_collider_component>(entity_)) {
        return *this;
    }
    auto& comp = system_->registry_->template get<sphere_collider_component>(entity_);
    comp.radius_ = r;
    return *this;
}

template <typename WC, typename... Cs>
auto physics_system<WC, Cs...>::collider_modifier::set_offset(
    const vec3f& offset
) -> collider_modifier& {
    if (!system_->registry_->template has<sphere_collider_component>(entity_)) {
        return *this;
    }
    auto& comp = system_->registry_->template get<sphere_collider_component>(entity_);
    comp.offset_ = offset;
    return *this;
}

}  // namespace vw::gfx

#endif  // VW_GFX_WORLD_SYSTEMS_PHYSICS_SYSTEM_INL_H
