#pragma once

#ifndef VW_GFX_TRANSFORM_SYSTEM_H
#define VW_GFX_TRANSFORM_SYSTEM_H

#include <set>

#include "vw/gfx/world/registry.h"
#include "vw/gfx/world/world.h"

namespace vw {
struct transform;
}  // namespace vw

namespace vw::gfx {

struct transform_component;

template <typename... Cs>
class spatial_system;

template <typename WC>
class world;

template <typename... Cs>
class transform_system final {
public:
    using world_type = world<std::tuple<Cs...>>;
    using registry_type = registry<Cs...>;

    explicit transform_system(
        world_type& world,
        registry_type& registry
    );

    void update();

    void mark_world_dirty(entity ent);

    void mark_dirty(entity ent);

    class transform_modifier {
    public:
        transform_modifier& set_transform(const transform& transform);
        transform_modifier& set_position(const vec3f& position);
        transform_modifier& set_rotation(const vec3f& rotation);
        transform_modifier& set_scale(const vec3f& scale);
        transform_modifier& set_origin(const vec3f& origin);
        transform_modifier& translate(const vec3f& offset);
        transform_modifier& rotate(const vec3f& angles);
        transform_modifier& scale(const vec3f& factor);

    private:
        friend class transform_system;
        transform_modifier(transform_system* system, entity ent);

        transform_system* system_;
        entity entity_;
    };

    transform_modifier modify(entity ent);

    [[nodiscard]] auto get_render_dirty_entities() -> std::unordered_set<entity>&;

    void mark_render_dirty(entity ent);

private:
    void mark_children_world_dirty(entity ent);

    void update_entity_world_matrix(entity ent, const transform_component& transform_comp);

    world_type* world_;
    registry_type* registry_;

    std::unordered_set<entity> dirty_entities_;
    std::vector<entity> sorted_dirty_entities_;

    std::unordered_set<entity> render_dirty_entities_;
};

template <typename... Cs>
struct transform_system_from_tuple;

template <typename... Cs>
struct transform_system_from_tuple<std::tuple<Cs...>> {
    using type = transform_system<Cs...>;
};

}  // namespace vw::gfx

#include "vw/gfx/world/systems/transform_system.inl.h"

#endif  // VW_GFX_TRANSFORM_SYSTEM_H
