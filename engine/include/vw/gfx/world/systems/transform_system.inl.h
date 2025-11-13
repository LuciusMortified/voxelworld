#pragma once

#ifndef VW_GFX_TRANSFORM_SYSTEM_INL_H
#define VW_GFX_TRANSFORM_SYSTEM_INL_H

#include <algorithm>
#include <cmath>
#include <ranges>
#include <stdexcept>

#include "vw/gfx/world/components/hierarchy_component.h"
#include "vw/gfx/world/components/transform_component.h"
#include "vw/gfx/world/systems/transform_system.h"

namespace vw::gfx {

template <typename... Cs>
inline transform_system<Cs...>::transform_system(
    registry_type& registry
)
    : registry_(&registry) {}

template <typename... Cs>
inline transform_system<Cs...>::transform_modifier::transform_modifier(
    transform_system* system, entity ent
)
    : system_(system), entity_(ent) {}

template <typename... Cs>
inline auto transform_system<Cs...>::modify(
    entity ent
) -> transform_modifier {
    return transform_modifier(this, ent);
}

template <typename... Cs>
inline auto transform_system<Cs...>::transform_modifier::set_position(
    const vec3f& position
) -> transform_modifier& {
    if (!system_->registry_->template has<transform_component>(entity_)) {
        return *this;
    }

    auto& transform_comp = system_->registry_->template get<transform_component>(entity_);
    transform_comp.transform_.set_position(position);
    transform_comp.local_dirty_ = true;
    transform_comp.world_dirty_ = true;

    system_->dirty_entities_.push_back(entity_);
    system_->mark_children_world_dirty(entity_);

    return *this;
}

template <typename... Cs>
inline auto transform_system<Cs...>::transform_modifier::set_rotation(
    const vec3f& rotation
) -> transform_modifier& {
    if (!system_->registry_->template has<transform_component>(entity_)) {
        return *this;
    }

    auto& transform_comp = system_->registry_->template get<transform_component>(entity_);
    transform_comp.transform_.set_rotation(rotation);
    transform_comp.local_dirty_ = true;
    transform_comp.world_dirty_ = true;

    system_->dirty_entities_.push_back(entity_);
    system_->mark_children_world_dirty(entity_);

    return *this;
}

template <typename... Cs>
inline auto transform_system<Cs...>::transform_modifier::set_scale(
    const vec3f& scale
) -> transform_modifier& {
    if (!system_->registry_->template has<transform_component>(entity_)) {
        return *this;
    }

    auto& transform_comp = system_->registry_->template get<transform_component>(entity_);
    transform_comp.transform_.set_scale(scale);
    transform_comp.local_dirty_ = true;
    transform_comp.world_dirty_ = true;

    system_->dirty_entities_.push_back(entity_);
    system_->mark_children_world_dirty(entity_);

    return *this;
}

template <typename... Cs>
inline auto transform_system<Cs...>::transform_modifier::set_origin(
    const vec3f& origin
) -> transform_modifier& {
    if (!system_->registry_->template has<transform_component>(entity_)) {
        return *this;
    }

    auto& transform_comp = system_->registry_->template get<transform_component>(entity_);
    transform_comp.transform_.set_origin(origin);
    transform_comp.local_dirty_ = true;
    transform_comp.world_dirty_ = true;

    system_->dirty_entities_.push_back(entity_);
    system_->mark_children_world_dirty(entity_);

    return *this;
}

template <typename... Cs>
inline auto transform_system<Cs...>::transform_modifier::translate(
    const vec3f& offset
) -> transform_modifier& {
    if (!system_->registry_->template has<transform_component>(entity_)) {
        return *this;
    }

    auto& transform_comp = system_->registry_->template get<transform_component>(entity_);
    transform_comp.transform_.translate(offset);
    transform_comp.local_dirty_ = true;
    transform_comp.world_dirty_ = true;

    system_->dirty_entities_.push_back(entity_);
    system_->mark_children_world_dirty(entity_);

    return *this;
}

template <typename... Cs>
inline auto transform_system<Cs...>::transform_modifier::rotate(
    const vec3f& angles
) -> transform_modifier& {
    if (!system_->registry_->template has<transform_component>(entity_)) {
        return *this;
    }

    auto& transform_comp = system_->registry_->template get<transform_component>(entity_);
    transform_comp.transform_.rotate(angles);
    transform_comp.local_dirty_ = true;
    transform_comp.world_dirty_ = true;

    system_->dirty_entities_.push_back(entity_);
    system_->mark_children_world_dirty(entity_);

    return *this;
}

template <typename... Cs>
inline auto transform_system<Cs...>::transform_modifier::scale(
    const vec3f& factor
) -> transform_modifier& {
    if (!system_->registry_->template has<transform_component>(entity_)) {
        return *this;
    }

    auto& transform_comp = system_->registry_->template get<transform_component>(entity_);
    transform_comp.transform_.scale(factor);
    transform_comp.local_dirty_ = true;
    transform_comp.world_dirty_ = true;

    system_->dirty_entities_.push_back(entity_);
    system_->mark_children_world_dirty(entity_);

    return *this;
}

template <typename... Cs>
inline void transform_system<Cs...>::mark_world_dirty(
    entity ent
) {
    if (!registry_->template has<transform_component>(ent)) {
        return;
    }

    auto& transform_comp        = registry_->template get<transform_component>(ent);
    transform_comp.world_dirty_ = true;

    dirty_entities_.push_back(ent);
    mark_children_world_dirty(ent);
}

template <typename... Cs>
inline void transform_system<Cs...>::update() {
    if (dirty_entities_.empty()) {
        return;
    }

    std::ranges::sort(dirty_entities_, [this](entity lhs, entity rhs) {
        return get_hierarchy_depth(lhs) < get_hierarchy_depth(rhs);
    });

    for (entity ent : dirty_entities_) {
        if (!registry_->template has<transform_component>(ent)) {
            continue;
        }

        auto& transform_comp = registry_->template get<transform_component>(ent);

        if (transform_comp.local_dirty_) {
            transform_comp.local_matrix_ = transform_comp.transform_.calc_matrix();
            transform_comp.local_dirty_  = false;
        }

        if (transform_comp.world_dirty_) {
            mat4f local_matrix              = transform_comp.get_local_matrix();
            transform_comp.world_matrix_    = local_matrix;
            transform_comp.world_transform_ = transform_comp.get_local_transform();

            if (registry_->template has<hierarchy_component>(ent)) {
                const auto& hierarchy_comp = registry_->template get<hierarchy_component>(ent);
                if (hierarchy_comp.has_parent()) {
                    entity parent = hierarchy_comp.get_parent();
                    if (registry_->template has<transform_component>(parent)) {
                        const auto& parent_transform_comp =
                            registry_->template get<transform_component>(parent);
                        transform_comp.world_matrix_ =
                            parent_transform_comp.get_world_matrix() * local_matrix;

                        transform_comp.world_transform_ =
                            calc_world_transform(transform_comp.transform_, parent_transform_comp);
                    }
                }
            }

            transform_comp.world_dirty_ = false;
        }
    }

    dirty_entities_.clear();
}

template <typename... Cs>
inline auto transform_system<Cs...>::calc_world_transform(
    transform local_transform, const transform_component& parent_comp
) -> transform {
    // Получаем параметры родительской мировой трансформации
    const auto& parent_transform = parent_comp.get_world_transform();

    vec3f parent_pos    = parent_transform.get_position();
    vec3f parent_rot    = parent_transform.get_rotation();
    vec3f parent_scale  = parent_transform.get_scale();
    vec3f parent_origin = parent_transform.get_origin();

    // Получаем параметры локальной трансформации
    vec3f local_pos    = local_transform.get_position();
    vec3f local_rot    = local_transform.get_rotation();
    vec3f local_scale  = local_transform.get_scale();
    vec3f local_origin = local_transform.get_origin();

    // Мировой масштаб = произведение масштабов
    vec3f final_scale = {
        parent_scale.x * local_scale.x,
        parent_scale.y * local_scale.y,
        parent_scale.z * local_scale.z
    };

    // Мировая ротация = комбинирование ротаций через матрицы
    // Правильный способ - умножить матрицы поворота и извлечь углы
    mat4f parent_rot_matrix   = math::rotation_matrix(parent_rot);
    mat4f local_rot_matrix    = math::rotation_matrix(local_rot);
    mat4f combined_rot_matrix = parent_rot_matrix * local_rot_matrix;

    // Извлекаем углы из комбинированной матрицы поворота
    // Используем упрощенный подход для ZYX углов Эйлера
    float sy = std::sqrt(
        (combined_rot_matrix[0, 0] * combined_rot_matrix[0, 0]) +
        (combined_rot_matrix[0, 1] * combined_rot_matrix[0, 1])
    );
    float final_rotation_x;
    float final_rotation_y;
    float final_rotation_z;

    if (sy > 1e-6f) {
        final_rotation_x = std::atan2(combined_rot_matrix[2, 1], combined_rot_matrix[2, 2]);
        final_rotation_y = std::atan2(-combined_rot_matrix[2, 0], sy);
        final_rotation_z = std::atan2(combined_rot_matrix[1, 0], combined_rot_matrix[0, 0]);
    } else {
        // Gimbal lock case
        final_rotation_x = std::atan2(-combined_rot_matrix[1, 2], combined_rot_matrix[1, 1]);
        final_rotation_y = std::atan2(-combined_rot_matrix[2, 0], sy);
        final_rotation_z = 0.0f;
    }

    vec3f final_rotation = {final_rotation_x, final_rotation_y, final_rotation_z};

    // Вычисляем мировую позицию origin'а локальной трансформации
    // Применяем родительскую трансформацию к локальному origin
    vec3f relative_origin = local_origin - parent_origin;
    vec3f rotated_origin  = parent_rot_matrix * relative_origin;
    vec3f scaled_origin   = {
        rotated_origin.x * parent_scale.x,
        rotated_origin.y * parent_scale.y,
        rotated_origin.z * parent_scale.z
    };
    vec3f world_origin = parent_pos + scaled_origin;

    // Вычисляем мировую позицию точки position локальной трансформации
    // Порядок операций в transform_matrix: trans * orig * rot * scl
    // Для точки (0,0,0) после локальной трансформации получаем position
    // Но нам нужно правильно применить локальную трансформацию с учетом origin,
    // а затем применить родительскую трансформацию

    // Применяем локальную трансформацию к точке (0,0,0)
    // Порядок: scale(0) -> rot(0) -> -origin -> +position
    vec3f local_point_after_transform = local_pos;  // position уже результат

    // Теперь применяем родительскую трансформацию к этой точке
    // Порядок: scale -> rot -> -origin -> +position
    vec3f relative_to_parent = local_point_after_transform - parent_origin;
    vec3f rotated_relative   = parent_rot_matrix * relative_to_parent;
    vec3f scaled_relative    = {
        rotated_relative.x * parent_scale.x,
        rotated_relative.y * parent_scale.y,
        rotated_relative.z * parent_scale.z
    };
    vec3f final_position = parent_pos + scaled_relative;

    transform result;
    result.set_position(final_position);
    result.set_rotation(final_rotation);
    result.set_scale(final_scale);
    result.set_origin(world_origin);

    return result;
}

template <typename... Cs>
inline void transform_system<Cs...>::mark_children_world_dirty(
    entity ent
) {
    if (!registry_->template has<hierarchy_component>(ent)) {
        return;
    }

    const auto& hierarchy_comp = registry_->template get<hierarchy_component>(ent);
    const auto& children       = hierarchy_comp.get_children();

    for (entity child : children) {
        if (registry_->template has<transform_component>(child)) {
            auto& child_transform        = registry_->template get<transform_component>(child);
            child_transform.world_dirty_ = true;
            dirty_entities_.push_back(child);
        }
        mark_children_world_dirty(child);
    }
}

template <typename... Cs>
inline auto transform_system<Cs...>::get_hierarchy_depth(
    entity ent
) const -> size_t {
    constexpr int MAX_HIERARCHY_DEPTH = 100;

    int depth      = 0;
    entity current = ent;

    while (registry_->template has<hierarchy_component>(current)) {
        const auto& hierarchy_comp = registry_->template get<hierarchy_component>(current);
        if (!hierarchy_comp.has_parent()) {
            break;
        }
        current = hierarchy_comp.get_parent();
        depth++;

        if (depth >= MAX_HIERARCHY_DEPTH) {
            throw std::runtime_error("hierarchy depth is too deep");
        }
    }

    return depth;
}

}  // namespace vw::gfx

#endif  // VW_GFX_TRANSFORM_SYSTEM_INL_H