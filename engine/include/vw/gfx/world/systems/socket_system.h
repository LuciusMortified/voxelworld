#pragma once

#ifndef VW_GFX_WORLD_SYSTEMS_SOCKET_SYSTEM_H
#define VW_GFX_WORLD_SYSTEMS_SOCKET_SYSTEM_H

#include "vw/gfx/world/components/socket_component.h"
#include "vw/gfx/world/system_trait.h"
#include "vw/gfx/world/world_context.h"

namespace vw::gfx {

template <typename>
class hierarchy_system;

template <typename>
class transform_system;

template <typename WC>
class socket_system final {
public:
    using registry_type = entity_registry_from_tuple<WC>::type;
    using context_type  = world_context<WC>;

    explicit socket_system(context_type& context);

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

private:
    context_type* context_;
};

}  // namespace vw::gfx

template <>
struct vw::gfx::system_trait<vw::gfx::socket_system> {
    using components = std::tuple<vw::gfx::socket_component>;
    using depends_on = vw::gfx::system_list<vw::gfx::hierarchy_system, vw::gfx::transform_system>;
    using resources  = std::tuple<>;
};

#include "vw/gfx/world/systems/socket_system.inl.h"

#endif  // VW_GFX_WORLD_SYSTEMS_SOCKET_SYSTEM_H
