#pragma once

#ifndef VW_GFX_WORLD_SYSTEMS_SOCKET_SYSTEM_H
#define VW_GFX_WORLD_SYSTEMS_SOCKET_SYSTEM_H

#include "vw/gfx/world/components/socket_component.h"
#include "vw/gfx/world/entity_registry.h"
#include "vw/gfx/world/system_trait.h"

namespace vw::gfx {

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

}  // namespace vw::gfx

template <>
struct vw::gfx::system_trait<vw::gfx::socket_system> {
    using components = std::tuple<vw::gfx::socket_component>;
    using resources  = std::tuple<>;
};

#include "vw/gfx/world/systems/socket_system.inl.h"

#endif  // VW_GFX_WORLD_SYSTEMS_SOCKET_SYSTEM_H
