#pragma once

#ifndef VW_GFX_TRANSFORM_SYSTEM_H
#define VW_GFX_TRANSFORM_SYSTEM_H

#include <vector>

#include "vw/gfx/world/world_context.h"

namespace vw {
struct transform;
}  // namespace vw

namespace vw::gfx {

struct transform_component;

template <typename>
class hierarchy_system;

template <typename WC>
class transform_system final {
public:
    using context_type  = world_context<WC>;
    using registry_type = entity_registry_from_tuple<WC>::type;
    using hierarchy_system_type = hierarchy_system<WC>;

    explicit transform_system(context_type& context, hierarchy_system_type& hierarchy_sys);

    void update();

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

    transform_modifier modify(entity ent);

private:
    void mark_children_world_dirty(entity ent);

    void update_entity_world_matrix(entity ent, const transform_component& transform_comp);

    context_type* context_;
    hierarchy_system_type* hierarchy_system_;

    std::vector<entity> sorted_entities_;
};

}  // namespace vw::gfx

#include "vw/gfx/world/systems/transform_system.inl.h"

#endif  // VW_GFX_TRANSFORM_SYSTEM_H
