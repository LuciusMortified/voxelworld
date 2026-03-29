#pragma once

#ifndef VW_GFX_WORLD_SYSTEMS_PHYSICS_SYSTEM_INL_H
#define VW_GFX_WORLD_SYSTEMS_PHYSICS_SYSTEM_INL_H

#include <cmath>

namespace vw::gfx {

template <typename WC>
physics_system<WC>::physics_system(
    context_type& context, transform_system_type& transform_system
)
    : context_(&context), transform_system_(&transform_system) {}

template <typename WC>
void physics_system<WC>::set_gravity(float32 g) {
    gravity_ = g;
}

template <typename WC>
auto physics_system<WC>::get_gravity() const -> float32 {
    return gravity_;
}

template <typename WC>
void physics_system<WC>::update(
    float32 delta_time
) {
    if (!context_->world_grid) {
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

template <typename WC>
void physics_system<WC>::step(
    float32 dt
) {
    constexpr float32 impulse_epsilon = 0.01f;

    for (auto [ent, rb, tc] :
         context_->registry.template view<rigid_body_component, transform_component>()) {
        auto position = tc.get_position();

        rb.velocity_.y += gravity_ * rb.gravity_scale_ * dt;

        if (context_->registry.template has<movement_intent_component>(ent)) {
            const auto& mi = context_->registry.template get<movement_intent_component>(ent);
            auto axes = mi.wish_axes_;

            if (axes & axis_flag::x) { rb.velocity_.x = mi.wish_velocity_.x; }
            if (axes & axis_flag::y) { rb.velocity_.y = mi.wish_velocity_.y; }
            if (axes & axis_flag::z) { rb.velocity_.z = mi.wish_velocity_.z; }
        }

        rb.velocity_ = rb.velocity_ + rb.impulse_;

        auto decay = 1.0f - rb.drag_ * dt;
        if (decay < 0.0f) {
            decay = 0.0f;
        }
        rb.impulse_ = rb.impulse_ * decay;
        if (math::dot(rb.impulse_, rb.impulse_) < impulse_epsilon * impulse_epsilon) {
            rb.impulse_ = {0.0f, 0.0f, 0.0f};
        }

        auto new_position = position + rb.velocity_ * dt;

        if (context_->registry.template has<box_collider_component>(ent)) {
            const auto& col = context_->registry.template get<box_collider_component>(ent);
            auto half = col.extents_ * 0.5f;
            auto box_center = new_position + col.offset_;

            if (!are_chunks_loaded(box_center, col.extents_)) {
                rb.frozen_ = true;
                rb.velocity_ = {0.0f, 0.0f, 0.0f};
                rb.impulse_ = {0.0f, 0.0f, 0.0f};
                continue;
            }

            rb.frozen_ = false;
            auto result = resolve_box_voxel(box_center, half, rb.velocity_);
            new_position = result.resolved_position - col.offset_;
            rb.grounded_ = result.grounded;
        }

        transform_system_->modify(ent).set_position(new_position);
    }
}

template <typename WC>
auto physics_system<WC>::are_chunks_loaded(
    const vec3f& position, const vec3f& extents
) const -> bool {
    const auto vs = static_cast<float32>(context_->world_grid->voxel_scale());
    auto half = extents * 0.5f;

    auto min_world = vec3i{
        static_cast<int32>(std::floor((position.x - half.x) / vs) * vs),
        0,
        static_cast<int32>(std::floor((position.z - half.z) / vs) * vs)
    };
    auto max_world = vec3i{
        static_cast<int32>(std::floor((position.x + half.x) / vs) * vs),
        0,
        static_cast<int32>(std::floor((position.z + half.z) / vs) * vs)
    };

    auto min_chunk = context_->world_grid->world_to_chunk_coord(min_world);
    auto max_chunk = context_->world_grid->world_to_chunk_coord(max_world);

    for (int32 cx = min_chunk.x; cx <= max_chunk.x; ++cx) {
        for (int32 cz = min_chunk.z; cz <= max_chunk.z; ++cz) {
            if (!context_->world_grid->has_column({cx, cz})) {
                return false;
            }
        }
    }

    return true;
}

template <typename WC>
auto physics_system<WC>::resolve_box_voxel(
    vec3f center, const vec3f& half_extents, vec3f& velocity
) const -> collision_result {
    auto vs = static_cast<float32>(context_->world_grid->voxel_scale());
    auto vs_i = context_->world_grid->voxel_scale();
    bool grounded = false;

    for (int32 iter = 0; iter < max_collision_iterations; ++iter) {
        auto entity_min = center - half_extents;
        auto entity_max = center + half_extents;

        auto min_vx = static_cast<int32>(std::floor(entity_min.x / vs));
        auto min_vy = static_cast<int32>(std::floor(entity_min.y / vs));
        auto min_vz = static_cast<int32>(std::floor(entity_min.z / vs));
        auto max_vx = static_cast<int32>(std::floor(entity_max.x / vs));
        auto max_vy = static_cast<int32>(std::floor(entity_max.y / vs));
        auto max_vz = static_cast<int32>(std::floor(entity_max.z / vs));

        float32 min_penetration = std::numeric_limits<float32>::max();
        vec3f push_direction{0.0f, 0.0f, 0.0f};
        bool found_collision = false;

        for (int32 vx = min_vx; vx <= max_vx; ++vx) {
            for (int32 vy = min_vy; vy <= max_vy; ++vy) {
                for (int32 vz = min_vz; vz <= max_vz; ++vz) {
                    auto world_pos = vec3i{vx * vs_i, vy * vs_i, vz * vs_i};
                    auto v = context_->world_grid->get_voxel(world_pos);
                    if (v.is_empty()) {
                        continue;
                    }

                    auto voxel_min = vec3f{
                        static_cast<float32>(vx) * vs,
                        static_cast<float32>(vy) * vs,
                        static_cast<float32>(vz) * vs
                    };
                    auto voxel_max = vec3f{
                        voxel_min.x + vs,
                        voxel_min.y + vs,
                        voxel_min.z + vs
                    };

                    auto overlap_x = std::min(entity_max.x, voxel_max.x) - std::max(entity_min.x, voxel_min.x);
                    auto overlap_y = std::min(entity_max.y, voxel_max.y) - std::max(entity_min.y, voxel_min.y);
                    auto overlap_z = std::min(entity_max.z, voxel_max.z) - std::max(entity_min.z, voxel_min.z);

                    if (overlap_x <= 0.0f || overlap_y <= 0.0f || overlap_z <= 0.0f) {
                        continue;
                    }

                    auto voxel_center = (voxel_min + voxel_max) * 0.5f;
                    auto dir = center - voxel_center;

                    vec3f push{0.0f, 0.0f, 0.0f};
                    float32 penetration = 0.0f;

                    if (overlap_x <= overlap_y && overlap_x <= overlap_z) {
                        penetration = overlap_x;
                        push.x = dir.x >= 0.0f ? 1.0f : -1.0f;
                    } else if (overlap_y <= overlap_x && overlap_y <= overlap_z) {
                        penetration = overlap_y;
                        push.y = dir.y >= 0.0f ? 1.0f : -1.0f;
                    } else {
                        penetration = overlap_z;
                        push.z = dir.z >= 0.0f ? 1.0f : -1.0f;
                    }

                    if (penetration < min_penetration) {
                        min_penetration = penetration;
                        push_direction = push;
                        found_collision = true;
                    }
                }
            }
        }

        if (!found_collision) {
            break;
        }

        center = center + push_direction * min_penetration;

        auto vel_along_normal = math::dot(velocity, push_direction);
        if (vel_along_normal < 0.0f) {
            velocity = velocity - push_direction * vel_along_normal;
        }

        if (push_direction.y > 0.7f && velocity.y <= 0.0f) {
            grounded = true;
        }
    }

    return {center, grounded};
}

template <typename WC>
physics_system<WC>::rigid_body_modifier::rigid_body_modifier(
    physics_system* system, entity ent
)
    : system_(system), entity_(ent) {}

template <typename WC>
auto physics_system<WC>::modify(
    entity ent
) -> rigid_body_modifier {
    return rigid_body_modifier(this, ent);
}

template <typename WC>
auto physics_system<WC>::rigid_body_modifier::set_velocity(
    const vec3f& vel
) -> rigid_body_modifier& {
    if (!system_->context_->registry.template has<rigid_body_component>(entity_)) {
        return *this;
    }
    auto& comp = system_->context_->registry.template get<rigid_body_component>(entity_);
    comp.velocity_ = vel;
    return *this;
}

template <typename WC>
auto physics_system<WC>::rigid_body_modifier::set_gravity_scale(
    float32 scale
) -> rigid_body_modifier& {
    if (!system_->context_->registry.template has<rigid_body_component>(entity_)) {
        return *this;
    }
    auto& comp = system_->context_->registry.template get<rigid_body_component>(entity_);
    comp.gravity_scale_ = scale;
    return *this;
}

template <typename WC>
auto physics_system<WC>::rigid_body_modifier::add_impulse(
    const vec3f& impulse
) -> rigid_body_modifier& {
    if (!system_->context_->registry.template has<rigid_body_component>(entity_)) {
        return *this;
    }
    auto& comp = system_->context_->registry.template get<rigid_body_component>(entity_);
    comp.velocity_ = comp.velocity_ + impulse;
    return *this;
}

template <typename WC>
auto physics_system<WC>::rigid_body_modifier::add_external_impulse(
    const vec3f& impulse
) -> rigid_body_modifier& {
    if (!system_->context_->registry.template has<rigid_body_component>(entity_)) {
        return *this;
    }
    auto& comp = system_->context_->registry.template get<rigid_body_component>(entity_);
    comp.impulse_ = comp.impulse_ + impulse;
    return *this;
}

template <typename WC>
auto physics_system<WC>::rigid_body_modifier::set_drag(
    float32 drag
) -> rigid_body_modifier& {
    if (!system_->context_->registry.template has<rigid_body_component>(entity_)) {
        return *this;
    }
    auto& comp = system_->context_->registry.template get<rigid_body_component>(entity_);
    comp.drag_ = drag;
    return *this;
}

template <typename WC>
physics_system<WC>::collider_modifier::collider_modifier(
    physics_system* system, entity ent
)
    : system_(system), entity_(ent) {}

template <typename WC>
auto physics_system<WC>::modify_collider(
    entity ent
) -> collider_modifier {
    return collider_modifier(this, ent);
}

template <typename WC>
auto physics_system<WC>::collider_modifier::set_extents(
    const vec3f& ext
) -> collider_modifier& {
    if (!system_->context_->registry.template has<box_collider_component>(entity_)) {
        return *this;
    }
    auto& comp = system_->context_->registry.template get<box_collider_component>(entity_);
    comp.extents_ = ext;
    return *this;
}

template <typename WC>
auto physics_system<WC>::collider_modifier::set_offset(
    const vec3f& offset
) -> collider_modifier& {
    if (!system_->context_->registry.template has<box_collider_component>(entity_)) {
        return *this;
    }
    auto& comp = system_->context_->registry.template get<box_collider_component>(entity_);
    comp.offset_ = offset;
    return *this;
}

}  // namespace vw::gfx

#endif  // VW_GFX_WORLD_SYSTEMS_PHYSICS_SYSTEM_INL_H
