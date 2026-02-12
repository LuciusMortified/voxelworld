#pragma once

#ifndef VW_GFX_WORLD_SYSTEMS_SPATIAL_SYSTEM_INL_H
#define VW_GFX_WORLD_SYSTEMS_SPATIAL_SYSTEM_INL_H

#include <algorithm>
#include <cmath>
#include <limits>

#include "vw/core/math.h"
#include "vw/gfx/spatial/aabb.h"
#include "vw/gfx/world/components/model_component.h"
#include "vw/gfx/world/components/transform_component.h"
#include "vw/gfx/world/systems/spatial_system.h"

namespace vw::gfx {

template <typename... Cs>
spatial_system<Cs...>::spatial_system(
    registry_type& registry
)
    : registry_(&registry) {
    dirty_entities_.reserve(128);
}

template <typename... Cs>
void spatial_system<Cs...>::update() {
    // Обработать только dirty entities (оптимизация - не проходим по всем сущностям)
    if (dirty_entities_.empty()) {
        return;
    }

    // Обновить spatial index для dirty entities
    for (entity ent : dirty_entities_) {
        const bool can_be_updated =  //
            registry_->template has<model_component>(ent) &&
            registry_->template has<transform_component>(ent) &&
            registry_->template has<spatial_component>(ent);
        if (!can_be_updated) {
            continue;
        }
        update_entity(ent);
    }

    dirty_entities_.clear();
}

template <typename... Cs>
void spatial_system<Cs...>::update_entity(
    entity ent
) {
    auto& spatial = registry_->template get<spatial_component>(ent);

    const auto& model_comp     = registry_->template get<model_component>(ent);
    const auto& transform_comp = registry_->template get<transform_component>(ent);
    aabb new_bounds            = calculate_aabb_from_model(ent, model_comp, transform_comp);

    bool bounds_changed =  //
        spatial.bounds_.min != new_bounds.min || spatial.bounds_.max != new_bounds.max;

    if (bounds_changed) {
        bool needs_tree_update = spatial.dirty_ || new_bounds.min.x < spatial.fat_bounds_.min.x ||
            new_bounds.min.y < spatial.fat_bounds_.min.y ||
            new_bounds.min.z < spatial.fat_bounds_.min.z ||
            new_bounds.max.x > spatial.fat_bounds_.max.x ||
            new_bounds.max.y > spatial.fat_bounds_.max.y ||
            new_bounds.max.z > spatial.fat_bounds_.max.z;

        if (needs_tree_update) {
            aabb new_fat_bounds = expand_aabb_for_fat(new_bounds);

            if (!spatial.dirty_) {
                tree_.update(ent, new_fat_bounds);
            } else {
                tree_.insert(ent, new_fat_bounds);
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

    mark_render_dirty(ent);
}

template <typename... Cs>
aabb spatial_system<Cs...>::expand_aabb_for_fat(
    const aabb& bounds
) const {
    // Коэффициент расширения: 10% от размера или минимум 0.1 единицы
    constexpr float expansion_factor = 0.1f;
    constexpr float min_expansion    = 0.1f;

    vec3f size = bounds.size();
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

template <typename... Cs>
aabb spatial_system<Cs...>::calculate_aabb_from_model(
    entity ent, const model_component& model_comp, const transform_component& transform_comp
) const {
    if (!model_comp.has_model()) {
        // Если нет модели, вернуть пустой AABB
        return aabb{{0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}};
    }

    // Получить размеры модели
    int width  = model_comp.width();
    int height = model_comp.height();
    int depth  = model_comp.depth();

    // Создать локальный AABB модели (от 0,0,0 до width,height,depth)
    vec3f local_min{0.0f, 0.0f, 0.0f};
    vec3f local_max{
        static_cast<float>(width), static_cast<float>(height), static_cast<float>(depth)
    };

    // Получить world matrix для трансформации
    mat4f world_matrix = transform_comp.get_world_matrix();

    // Трансформировать 8 вершин локального AABB в мировые координаты
    vec3f vertices[8] = {
        {local_min.x, local_min.y, local_min.z},
        {local_max.x, local_min.y, local_min.z},
        {local_max.x, local_max.y, local_min.z},
        {local_min.x, local_max.y, local_min.z},
        {local_min.x, local_min.y, local_max.z},
        {local_max.x, local_min.y, local_max.z},
        {local_max.x, local_max.y, local_max.z},
        {local_min.x, local_max.y, local_max.z}
    };

    // Трансформировать все вершины и найти min/max
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

template <typename... Cs>
void spatial_system<Cs...>::query_all(
    const frustum& f, std::unordered_set<entity>& result_out
) const {
    tree_.query_all(f, result_out);

    for (auto it = result_out.begin(), ite = result_out.end(); it != ite;) {
        if (!registry_->template has<spatial_component>(*it)) {
            it = result_out.erase(it);
            continue;
        }

        const auto& spatial = registry_->template get<spatial_component>(*it);
        if (!f.intersects(spatial.get_bounds())) {
            it = result_out.erase(it);
        } else {
            ++it;
        }
    }
}

template <typename... Cs>
void spatial_system<Cs...>::query_all(
    const ray& r, std::unordered_set<entity>& result_out
) const {
    tree_.query_all(r, result_out);

    for (auto it = result_out.begin(), ite = result_out.end(); it != ite;) {
        if (!registry_->template has<spatial_component>(*it)) {
            it = result_out.erase(it);
            continue;
        }

        const auto& spatial = registry_->template get<spatial_component>(*it);
        if (!spatial.get_bounds().intersects(r)) {
            it = result_out.erase(it);
        } else {
            ++it;
        }
    }
}

template <typename... Cs>
void spatial_system<Cs...>::query_all(
    const aabb& bounds, std::unordered_set<entity>& result_out
) const {
    tree_.query_all(bounds, result_out);

    for (auto it = result_out.begin(), ite = result_out.end(); it != ite;) {
        if (!registry_->template has<spatial_component>(*it)) {
            it = result_out.erase(it);
            continue;
        }

        const auto& spatial = registry_->template get<spatial_component>(*it);
        if (!spatial.get_bounds().intersects(bounds)) {
            it = result_out.erase(it);
        } else {
            ++it;
        }
    }
}

template <typename... Cs>
void spatial_system<Cs...>::mark_dirty(
    entity ent
) {
    dirty_entities_.insert(ent);
}

template <typename... Cs>
void spatial_system<Cs...>::cleanup(
    entity ent
) {
    // Удалить entity из spatial tree перед удалением компонента
    tree_.remove(ent);
    // Также удалить из dirty_entities_ если там есть
    dirty_entities_.erase(ent);
}

template <typename... Cs>
auto spatial_system<Cs...>::get_render_dirty_entities() -> std::unordered_set<entity>& {
    return render_dirty_entities_;
}

template <typename... Cs>
void spatial_system<Cs...>::mark_render_dirty(
    entity ent
) {
    render_dirty_entities_.insert(ent);
}

template <typename... Cs>
auto spatial_system<Cs...>::voxel_ray_cast(
    const ray& r, std::unordered_set<entity>& candidates
) const -> std::optional<voxel_ray_hit> {
    query_all(r, candidates);

    std::optional<voxel_ray_hit> closest_hit;
    float closest_distance_sq = std::numeric_limits<float>::max();

    for (entity ent : candidates) {
        const bool can_be_processed =  //
            registry_->template has<model_component>(ent) &&
            registry_->template has<transform_component>(ent);
        if (!can_be_processed) {
            continue;
        }

        const auto& model_comp     = registry_->template get<model_component>(ent);
        const auto& transform_comp = registry_->template get<transform_component>(ent);

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

        // AABB модели в локальных координатах (от (0,0,0) до (width, height, depth))
        const aabb model_aabb{
            vec3f{-1.f, -1.f, -1.f},
            vec3f{
                static_cast<float>(width) + 1.f,
                static_cast<float>(height) + 1.f,
                static_cast<float>(depth) + 1.f
            }
        };

        // Проверить пересечение луча с AABB модели
        float t_entry = 0.0f;
        ray local_ray{local_start, local_end};
        if (!local_ray.intersects_at(model_aabb, t_entry)) {
            continue;
        }

        // Найти точку входа в AABB
        vec3f entry_point = local_ray.point_at(t_entry);

        // Ограничить начальную точку границами модели
        entry_point.x = math::clamp(entry_point.x, -1.f, static_cast<float>(width) + 1.f);
        entry_point.y = math::clamp(entry_point.y, -1.f, static_cast<float>(height) + 1.f);
        entry_point.z = math::clamp(entry_point.z, -1.f, static_cast<float>(depth) + 1.f);

        // Начальная позиция вокселя (до ограничения границами)
        int x = static_cast<int>(std::floor(entry_point.x));
        int y = static_cast<int>(std::floor(entry_point.y));
        int z = static_cast<int>(std::floor(entry_point.z));

        // Ограничить начальную позицию границами
        x = std::max(-1, std::min(width, x));
        y = std::max(-1, std::min(height, y));
        z = std::max(-1, std::min(depth, z));

        int prev_x = x;
        int prev_y = y;
        int prev_z = z;

        // Шаги по осям
        const int step_x = local_direction.x > 0.0f ? 1 : -1;
        const int step_y = local_direction.y > 0.0f ? 1 : -1;
        const int step_z = local_direction.z > 0.0f ? 1 : -1;

        // Расстояния до следующей границы вокселя (tMax)
        float t_max_x = 0.0f;
        float t_max_y = 0.0f;
        float t_max_z = 0.0f;

        // Вычислить начальные tMax
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

        // Расстояние между границами вокселей (tDelta)
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

        // DDA traversal
        constexpr int max_iterations = 10000;  // Защита от бесконечного цикла
        int iterations               = 0;

        while (iterations < max_iterations) {
            // Проверить, что воксель в пределах границ
            if (x < -1 || x > width || y < -1 || y > height || z < -1 || z > depth) {
                break;
            }

            bool is_out_of_bounds =  //
                x == -1 || x == width || y == -1 || y == height || z == -1 || z == depth;

            // Проверить воксель
            if (!is_out_of_bounds && !model_comp.is_empty(x, y, z)) {
                // Найдено попадание!
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
                break;  // Найдено первое попадание в этой модели, переходим к следующему кандидату
            }

            prev_x = x;
            prev_y = y;
            prev_z = z;

            // Переместиться к следующему вокселю
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
