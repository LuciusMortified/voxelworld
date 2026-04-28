#pragma once

#ifndef VW_ECS_SYSTEMS_SOCKET_SYSTEM_H
#define VW_ECS_SYSTEMS_SOCKET_SYSTEM_H

#include "vw/ecs/components/socket_component.h"
#include "vw/ecs/entity_registry.h"
#include "vw/ecs/system_trait.h"

namespace vw::ecs {

template <typename>
class hierarchy_system;

template <typename>
class transform_system;

template <typename>
class world;

template <typename WD>
class socket_system final {
public:
    using components    = typename WD::components;
    using registry_type = typename entity_registry_from_tuple<components>::type;
    using world_type    = world<WD>;

    explicit socket_system(world_type& w);

    class socket_modifier {
    public:
        auto attach(const std::string& socket_name, entity child) -> socket_modifier&;
        auto detach(const std::string& socket_name) -> socket_modifier&;
        auto add_socket(
            const std::string& name,
            const vec3f& position = {},
            const quat& rotation  = {},
            const vec3f& scale    = vec3f{1, 1, 1}
        ) -> socket_modifier&;
        auto remove_socket(const std::string& name) -> socket_modifier&;

    private:
        friend class socket_system;
        socket_modifier(socket_system* system, entity ent);

        socket_system* system_;
        entity entity_;
    };

    auto modify(entity ent) -> socket_modifier;
    void cleanup(entity ent);
    void update(float32 dt);

    template <typename C>
        requires std::same_as<C, socket_component>
    void on_remove(entity e) {
        cleanup(e);
    }

private:
    world_type* world_;
};

}  // namespace vw::ecs

template <>
struct vw::ecs::system_trait<vw::ecs::socket_system> {
    using components = std::tuple<vw::ecs::socket_component>;
    using resources  = std::tuple<>;
};

#include "vw/ecs/systems/socket_system.inl.h"

#endif  // VW_ECS_SYSTEMS_SOCKET_SYSTEM_H
