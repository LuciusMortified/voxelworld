#pragma once

#ifndef VW_GFX_TRANSFORM_SYSTEM_H
#define VW_GFX_TRANSFORM_SYSTEM_H

#include <vector>

#include "vw/gfx/world/entity_registry.h"
#include "vw/gfx/world/system_trait.h"

namespace vw {
struct transform;
}  // namespace vw

namespace vw::gfx {

struct transform_component;
struct spatial_component;

template <typename>
class hierarchy_system;

template <typename>
class world;

template <typename WD>
class transform_system final {
public:
    using world_type    = world<WD>;
    using components    = typename WD::components;
    using registry_type = typename entity_registry_from_tuple<components>::type;

    explicit transform_system(world_type& w);

    void update(float32 dt);

    class transform_modifier {
    public:
        auto set_transform(const transform& transform) -> transform_modifier&;
        auto set_transform_with_matrix(const transform& transform, const mat4f& local_matrix)
            -> transform_modifier&;
        auto set_position(const vec3f& position) -> transform_modifier&;
        auto set_rotation(const quat& rotation) -> transform_modifier&;
        auto set_rotation_euler(const vec3f& euler) -> transform_modifier&;
        auto set_scale(const vec3f& scale) -> transform_modifier&;
        auto set_origin(const vec3f& origin) -> transform_modifier&;
        auto translate(const vec3f& offset) -> transform_modifier&;
        auto rotate(const vec3f& angles) -> transform_modifier&;
        auto scale(const vec3f& factor) -> transform_modifier&;
        auto mark_world_dirty() -> transform_modifier&;

    private:
        friend class transform_system;
        transform_modifier(transform_system* system, entity ent);

        transform_system* system_;
        entity entity_;
    };

    [[nodiscard]] auto modify(entity ent) -> transform_modifier;

    template <typename C>
        requires (std::same_as<C, transform_component> || std::same_as<C, spatial_component>)
    void on_add(entity e);

private:
    void mark_children_world_dirty(entity ent);

    void update_entity_world_matrix(entity ent, const transform_component& transform_comp);

    world_type* world_;
    std::vector<entity> sorted_entities_;
};

}  // namespace vw::gfx

template <>
struct vw::gfx::system_trait<vw::gfx::transform_system> {
    using components = std::tuple<vw::gfx::transform_component>;
    using resources  = std::tuple<>;
};

#include "vw/gfx/world/systems/transform_system.inl.h"

#endif  // VW_GFX_TRANSFORM_SYSTEM_H
