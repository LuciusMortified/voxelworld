#pragma once

#ifndef VW_GFX_WORLD_COMPONENTS_SOCKET_COMPONENT_INL_H
#define VW_GFX_WORLD_COMPONENTS_SOCKET_COMPONENT_INL_H

#include <algorithm>
#include <utility>

namespace vw::gfx {

inline socket_component::socket_component(std::vector<socket_point> sockets)
    : sockets_(std::move(sockets)) {}

inline auto socket_component::get_sockets() const -> const std::vector<socket_point>& {
    return sockets_;
}

inline auto socket_component::find(const std::string& name) const -> const socket_point* {
    auto it = std::ranges::find_if(sockets_, [&](const socket_point& sp) {
        return sp.name == name;
    });
    return it != sockets_.end() ? &(*it) : nullptr;
}

inline auto socket_component::is_occupied(const std::string& name) const -> bool {
    auto* sp = find(name);
    return sp != nullptr && sp->attached.is_valid();
}

}  // namespace vw::gfx

#endif  // VW_GFX_WORLD_COMPONENTS_SOCKET_COMPONENT_INL_H
