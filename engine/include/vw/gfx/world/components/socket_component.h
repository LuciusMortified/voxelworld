#pragma once

#ifndef VW_GFX_WORLD_COMPONENTS_SOCKET_COMPONENT_H
#define VW_GFX_WORLD_COMPONENTS_SOCKET_COMPONENT_H

#include <string>
#include <vector>

#include "vw/core.h"
#include "vw/gfx/world/entity.h"

namespace vw::gfx {

template <typename... Cs>
class socket_system;

struct socket_point {
    std::string name;
    vec3f position{0.0F, 0.0F, 0.0F};
    vec3f rotation{0.0F, 0.0F, 0.0F};
    vec3f scale{1.0F, 1.0F, 1.0F};
    entity attached = invalid_entity;
};

struct socket_component final {
public:
    socket_component() = default;
    explicit socket_component(std::vector<socket_point> sockets);

    [[nodiscard]] auto get_sockets() const -> const std::vector<socket_point>&;
    [[nodiscard]] auto find(const std::string& name) const -> const socket_point*;
    [[nodiscard]] auto is_occupied(const std::string& name) const -> bool;

private:
    std::vector<socket_point> sockets_;

    template <typename... Cs>
    friend class socket_system;
};

}  // namespace vw::gfx

#include "socket_component.inl.h"

#endif  // VW_GFX_WORLD_COMPONENTS_SOCKET_COMPONENT_H
