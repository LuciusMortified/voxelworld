#pragma once

#ifndef VW_GFX_WORLD_SYSTEMS_SPATIAL_SYSTEM_INL_H
#define VW_GFX_WORLD_SYSTEMS_SPATIAL_SYSTEM_INL_H

#include <algorithm>
#include <cmath>
#include <limits>

#include "vw/core/math.h"
#include "vw/gfx/world/components/model_component.h"
#include "vw/gfx/world/components/transform_component.h"
#include "vw/gfx/world/systems/spatial_system.h"
#include "vw/spatial/aabb.h"

namespace vw::gfx {

template <typename WD>
spatial_system<WD>::spatial_system(
    context_type& context
)
    : context_(&context) {}

template <typename WD>
auto spatial_system<WD>::modify(entity ent) -> spatial_modifier {
    return spatial_modifier(this, ent);
}

template <typename WD>
spatial_system<WD>::spatial_modifier::spatial_modifier(
    spatial_system* system, entity ent
)
    : system_(system)
    , entity_(ent) {}

template <typename WD>
auto spatial_system<WD>::spatial_modifier::set_layer(
    spatial_layer_mask layer
) -> spatial_modifier& {
    if (!system_->context_->registry().template has<spatial_component>(entity_)) {
        return *this;
    }
    auto& spatial = system_->context_->registry().template get<spatial_component>(entity_);
    spatial.layer_ = layer;
    return *this;
}

template <typename WD>
void spatial_system<WD>::update(float32 /*dt*/) {
    auto& requested = context_->registry().template requested<spatial_component>();
    if (requested.empty()) {
        return;
    }

    for (entity ent : requested) {
        const bool can_be_updated =  //
            context_->registry().template has<model_component>(ent) &&
            context_->registry().template has<transform_component>(ent) &&
            context_->registry().template has<spatial_component>(ent);
        if (!can_be_updated) {
            continue;
        }
        update_entity(ent);
        context_->registry().template notify_changed<spatial_component>(ent);
    }

    context_->registry().template clear_requested<spatial_component>();
}

template <typename WD>
void spatial_system<WD>::update_entity(
    entity ent
) {
    auto& spatial = context_->registry().template get<spatial_component>(ent);

    const auto& model_comp     = context_->registry().template get<model_component>(ent);
    const auto& transform_comp = context_->registry().template get<transform_component>(ent);
    aabb new_bounds            = calculate_aabb_from_model(ent, model_comp, transform_comp);

    const bool bounds_changed =  //
        spatial.bounds_.min != new_bounds.min || spatial.bounds_.max != new_bounds.max;

    if (bounds_changed) {
        const bool needs_tree_update = spatial.dirty_ || new_bounds.min.x < spatial.fat_bounds_.min.x ||
            new_bounds.min.y < spatial.fat_bounds_.min.y ||
            new_bounds.min.z < spatial.fat_bounds_.min.z ||
            new_bounds.max.x > spatial.fat_bounds_.max.x ||
            new_bounds.max.y > spatial.fat_bounds_.max.y ||
            new_bounds.max.z > spatial.fat_bounds_.max.z;

        if (needs_tree_update) {
            aabb new_fat_bounds = expand_aabb_for_fat(new_bounds);

            if (!spatial.dirty_) {
                tree_.update(ent, new_fat_bounds, spatial.layer_);
            } else {
                tree_.insert(ent, new_fat_bounds, spatial.layer_);
            }

            spatial.bounds_     = new_bounds;
            spatial.fat_bounds_ = new_fat_bounds;
            spatial.dirty_      = false;
        } else {
            spatial.bounds_ = new_bounds;
            spatial.dirty_  = false;
        }
    } else {
        spatial.dirty_ = false;
    }
}

template <typename WD>
auto spatial_system<WD>::expand_aabb_for_fat(
    const aabb& bounds
) -> aabb {
    constexpr float expansion_factor = 0.1f;
    constexpr float min_expansion    = 0.1f;

    const vec3f size = bounds.size();
    vec3f expansion{
        std::max(size.x * expansion_factor, min_expansion),
        std::max(size.y * expansion_factor, min_expansion),
        std::max(size.z * expansion_factor, min_expansion)
    };

    return aabb{
        vec3f{bounds.min.x - expansion.x, bounds.min.y - expansion.y, bounds.min.z - expansion.z},
        vec3f{bounds.max.x + expansion.x, bounds.max.y + expansion.y, bounds.max.z + expansion.z}
    };
}

template <typename WD>
auto spatial_system<WD>::calculate_aabb_from_model(
    entity ent, const model_component& model_comp, const transform_component& transform_comp
) const -> aabb {
    if (!model_comp.has_model()) {
        return aabb{.min={0.0f, 0.0f, 0.0f}, .max={0.0f, 0.0f, 0.0f}};
    }

    const auto scale = model_comp.get_model()->voxel_scale();
    const int width  = model_comp.width() * scale;
    const int height = model_comp.height() * scale;
    const int depth  = model_comp.depth() * scale;

    constexpr vec3f local_min{0.0f, 0.0f, 0.0f};
    const vec3f local_max{
        static_cast<float>(width), static_cast<float>(height), static_cast<float>(depth)
    };

    const mat4f world_matrix = transform_comp.get_world_matrix();

    const std::array vertices = {
        vec3f{local_min.x, local_min.y, local_min.z},
        vec3f{local_max.x, local_min.y, local_min.z},
        vec3f{local_max.x, local_max.y, local_min.z},
        vec3f{local_min.x, local_max.y, local_min.z},
        vec3f{local_min.x, local_min.y, local_max.z},
        vec3f{local_max.x, local_min.y, local_max.z},
        vec3f{local_max.x, local_max.y, local_max.z},
        vec3f{local_min.x, local_max.y, local_max.z}
    };

    vec3f min_point{std::numeric_limits<float>::max()};
    vec3f max_point{std::numeric_limits<float>::lowest()};

    for (auto vertice : vertices) {
        vec3f world_vertex = world_matrix * vertice;
        min_point.x        = std::min(min_point.x, world_vertex.x);
        min_point.y        = std::min(min_point.y, world_vertex.y);
        min_point.z        = std::min(min_point.z, world_vertex.z);

        max_point.x = std::max(max_point.x, world_vertex.x);
        max_point.y = std::max(max_point.y, world_vertex.y);
        max_point.z = std::max(max_point.z, world_vertex.z);
    }

    return aabb{min_point, max_point};
}

template <typename WD>
void spatial_system<WD>::query_all(
    const frustum& f, std::vector<entity>& result_out, spatial_layer_mask layer_mask
) const {
    tree_.query_all(f, result_out, layer_mask);
}

template <typename WD>
void spatial_system<WD>::query_all_any(
    std::span<const frustum> frustums, std::vector<entity>& result_out
) const {
    tree_.query_all_any(frustums, result_out);
    std::sort(result_out.begin(), result_out.end());
}

template <typename WD>
void spatial_system<WD>::query_all(
    const ray& r, std::vector<entity>& result_out, spatial_layer_mask layer_mask
) const {
    tree_.query_all(r, result_out, layer_mask);
}

template <typename WD>
void spatial_system<WD>::query_all(
    const aabb& bounds, std::vector<entity>& result_out, spatial_layer_mask layer_mask
) const {
    tree_.query_all(bounds, result_out, layer_mask);
}

template <typename WD>
void spatial_system<WD>::cleanup(
    entity ent
) {
    tree_.remove(ent);
}

template <typename WD>
auto spatial_system<WD>::voxel_ray_cast(
    const ray& r, std::vector<entity>& candidates, spatial_layer_mask layer_mask
) const -> std::optional<voxel_ray_hit> {
    query_all(r, candidates, layer_mask);

    std::optional<voxel_ray_hit> closest_hit;
    float closest_distance_sq = std::numeric_limits<float>::max();

    for (entity ent : candidates) {
        const bool can_be_processed =  //
            context_->registry().template has<model_component>(ent) &&
            context_->registry().template has<transform_component>(ent);
        if (!can_be_processed) {
            continue;
        }

        const auto& model_comp     = context_->registry().template get<model_component>(ent);
        const auto& transform_comp = context_->registry().template get<transform_component>(ent);

        if (!model_comp.has_model()) {
            continue;
        }

        const vec3i model_size = model_comp.size();

        if (model_size.x <= 0 || model_size.y <= 0 || model_size.z <= 0) {
            continue;
        }

        const int width  = model_size.x;
        const int height = model_size.y;
        const int depth  = model_size.z;

        const mat4f world_matrix    = transform_comp.get_world_matrix();
        const auto inv_result       = math::inverse_matrix(world_matrix);
        const mat4f inverse_world   = inv_result.value_or(math::identity_matrix());
        const vec3f local_start     = inverse_world * r.start;
        const vec3f local_end       = inverse_world * r.end;
        const vec3f local_direction = math::normalize(local_end - local_start);

        const aabb model_aabb{
            .min=vec3f{-1.f, -1.f, -1.f},
            .max=vec3f{
                static_cast<float>(width) + 1.f,
                static_cast<float>(height) + 1.f,
                static_cast<float>(depth) + 1.f
            }
        };

        float t_entry = 0.0f;
        ray local_ray{local_start, local_end};
        if (!local_ray.intersects_at(model_aabb, t_entry)) {
            continue;
        }

        vec3f entry_point = local_ray.point_at(t_entry);

        entry_point.x = math::clamp(entry_point.x, -1.f, static_cast<float>(width) + 1.f);
        entry_point.y = math::clamp(entry_point.y, -1.f, static_cast<float>(height) + 1.f);
        entry_point.z = math::clamp(entry_point.z, -1.f, static_cast<float>(depth) + 1.f);

        int x = static_cast<int>(std::floor(entry_point.x));
        int y = static_cast<int>(std::floor(entry_point.y));
        int z = static_cast<int>(std::floor(entry_point.z));

        x = std::max(-1, std::min(width, x));
        y = std::max(-1, std::min(height, y));
        z = std::max(-1, std::min(depth, z));

        int prev_x = x;
        int prev_y = y;
        int prev_z = z;

        const int step_x = local_direction.x > 0.0f ? 1 : -1;
        const int step_y = local_direction.y > 0.0f ? 1 : -1;
        const int step_z = local_direction.z > 0.0f ? 1 : -1;

        float t_max_x = 0.0f;
        float t_max_y = 0.0f;
        float t_max_z = 0.0f;

        if (local_direction.x != 0.0f) {
            const auto next_boundary_x =  //
                static_cast<float>(step_x > 0 ? x + 1 : x);
            t_max_x = (next_boundary_x - entry_point.x) / local_direction.x;
        } else {
            t_max_x = std::numeric_limits<float>::max();
        }

        if (local_direction.y != 0.0f) {
            const auto next_boundary_y =  //
                static_cast<float>(step_y > 0 ? y + 1 : y);
            t_max_y = (next_boundary_y - entry_point.y) / local_direction.y;
        } else {
            t_max_y = std::numeric_limits<float>::max();
        }

        if (local_direction.z != 0.0f) {
            const auto next_boundary_z =  //
                static_cast<float>(step_z > 0 ? z + 1 : z);
            t_max_z = (next_boundary_z - entry_point.z) / local_direction.z;
        } else {
            t_max_z = std::numeric_limits<float>::max();
        }

        const float t_delta_x =  //
            local_direction.x != 0.0f ?
            static_cast<float>(step_x) / local_direction.x :
            std::numeric_limits<float>::max();
        const float t_delta_y =  //
            local_direction.y != 0.0f ?
            static_cast<float>(step_y) / local_direction.y :
            std::numeric_limits<float>::max();
        const float t_delta_z =  //
            local_direction.z != 0.0f ?
            static_cast<float>(step_z) / local_direction.z :
            std::numeric_limits<float>::max();

        constexpr int max_iterations = 10000;
        int iterations               = 0;

        while (iterations < max_iterations) {
            if (x < -1 || x > width || y < -1 || y > height || z < -1 || z > depth) {
                break;
            }

            bool is_out_of_bounds =  //
                x == -1 || x == width || y == -1 || y == height || z == -1 || z == depth;

            if (!is_out_of_bounds && !model_comp.is_empty(x, y, z)) {
                vec3f hit_point_local{
                    static_cast<float>(x) + 0.5f,
                    static_cast<float>(y) + 0.5f,
                    static_cast<float>(z) + 0.5f
                };
                vec3f hit_point_world = world_matrix * hit_point_local;
                float distance_sq     = math::length_squared(hit_point_world - r.start);

                if (distance_sq < closest_distance_sq) {
                    closest_distance_sq = distance_sq;

                    closest_hit = voxel_ray_hit{
                        .ent       = ent,
                        .voxel_pos = vec3i{x, y, z},
                        .empty_pos = vec3i{prev_x, prev_y, prev_z}
                    };
                }
                break;
            }

            prev_x = x;
            prev_y = y;
            prev_z = z;

            if (t_max_x < t_max_y && t_max_x < t_max_z) {
                x += step_x;
                t_max_x += t_delta_x;
            } else if (t_max_y < t_max_z) {
                y += step_y;
                t_max_y += t_delta_y;
            } else {
                z += step_z;
                t_max_z += t_delta_z;
            }

            ++iterations;
        }
    }

    return closest_hit;
}

}  // namespace vw::gfx

#endif  // VW_GFX_WORLD_SYSTEMS_SPATIAL_SYSTEM_INL_H
