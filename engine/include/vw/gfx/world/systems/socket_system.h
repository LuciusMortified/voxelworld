#pragma once

#ifndef VW_GFX_WORLD_SYSTEMS_SOCKET_SYSTEM_H
#define VW_GFX_WORLD_SYSTEMS_SOCKET_SYSTEM_H

#include "vw/gfx/world/components/socket_component.h"
#include "vw/gfx/world/registry.h"

namespace vw::gfx {

template <typename... Cs>
class hierarchy_system;

template <typename... Cs>
class transform_system;

template <typename... Cs>
class socket_system final {
public:
    using registry_type         = registry<Cs...>;
    using hierarchy_system_type = hierarchy_system<Cs...>;
    using transform_system_type = transform_system<Cs...>;

    explicit socket_system(
        registry_type& registry,
        hierarchy_system_type& hierarchy_system,
        transform_system_type& transform_system
    );

    class socket_modifier {
    public:
        auto attach(const std::string& socket_name, entity child) -> socket_modifier&;
        auto detach(const std::string& socket_name) -> socket_modifier&;
        auto add_socket(
            const std::string& name,
            const vec3f& position = {},
            const vec3f& rotation = {},
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
    void update();

private:
    registry_type* registry_;
    hierarchy_system_type* hierarchy_system_;
    transform_system_type* transform_system_;
};

template <typename... Cs>
struct socket_system_from_tuple;

template <typename... Cs>
struct socket_system_from_tuple<std::tuple<Cs...>> {
    using type = socket_system<Cs...>;
};

}  // namespace vw::gfx

#include "vw/gfx/world/systems/socket_system.inl.h"

#endif  // VW_GFX_WORLD_SYSTEMS_SOCKET_SYSTEM_H
