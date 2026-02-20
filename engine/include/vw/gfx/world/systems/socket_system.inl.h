#pragma once

#ifndef VW_GFX_WORLD_SYSTEMS_SOCKET_SYSTEM_INL_H
#define VW_GFX_WORLD_SYSTEMS_SOCKET_SYSTEM_INL_H

#include <algorithm>

#include "vw/gfx/world/systems/hierarchy_system.h"
#include "vw/gfx/world/systems/socket_system.h"
#include "vw/gfx/world/systems/transform_system.h"

namespace vw::gfx {

template <typename... Cs>
socket_system<Cs...>::socket_system(
    registry_type& registry,
    hierarchy_system_type& hierarchy_system,
    transform_system_type& transform_system
)
    : registry_(&registry)
    , hierarchy_system_(&hierarchy_system)
    , transform_system_(&transform_system) {}

template <typename... Cs>
void socket_system<Cs...>::update() {}

template <typename... Cs>
void socket_system<Cs...>::cleanup(
    entity ent
) {
    if (!registry_->template has<socket_component>(ent)) {
        return;
    }
    auto& comp = registry_->template get<socket_component>(ent);
    for (auto& slot : comp.sockets_) {
        if (slot.attached.is_valid()) {
            hierarchy_system_->modify(slot.attached).remove_parent();
            slot.attached = invalid_entity;
        }
    }
}

template <typename... Cs>
socket_system<Cs...>::socket_modifier::socket_modifier(
    socket_system* system, entity ent
)
    : system_(system), entity_(ent) {}

template <typename... Cs>
auto socket_system<Cs...>::modify(
    entity ent
) -> socket_modifier {
    return socket_modifier(this, ent);
}

template <typename... Cs>
auto socket_system<Cs...>::socket_modifier::attach(
    const std::string& socket_name, entity child
) -> socket_modifier& {
    if (!system_->registry_->template has<socket_component>(entity_)) {
        return *this;
    }
    auto& comp = system_->registry_->template get<socket_component>(entity_);
    auto it    = std::ranges::find_if(comp.sockets_, [&](const socket_point& sp) {
        return sp.name == socket_name;
    });
    if (it == comp.sockets_.end() || it->attached.is_valid()) {
        return *this;
    }
    it->attached = child;
    system_->hierarchy_system_->modify(child).set_parent(entity_);
    system_->transform_system_->modify(child)
        .set_position(it->position)
        .set_rotation(it->rotation)
        .set_scale(it->scale);
    return *this;
}

template <typename... Cs>
auto socket_system<Cs...>::socket_modifier::detach(
    const std::string& socket_name
) -> socket_modifier& {
    if (!system_->registry_->template has<socket_component>(entity_)) {
        return *this;
    }
    auto& comp = system_->registry_->template get<socket_component>(entity_);
    auto it    = std::ranges::find_if(comp.sockets_, [&](const socket_point& sp) {
        return sp.name == socket_name;
    });
    if (it == comp.sockets_.end() || !it->attached.is_valid()) {
        return *this;
    }
    auto detached = it->attached;
    it->attached  = invalid_entity;
    system_->hierarchy_system_->modify(detached).remove_parent();
    return *this;
}

template <typename... Cs>
auto socket_system<Cs...>::socket_modifier::add_socket(
    const std::string& name, const vec3f& position, const vec3f& rotation, const vec3f& scale
) -> socket_modifier& {
    if (!system_->registry_->template has<socket_component>(entity_)) {
        return *this;
    }
    auto& comp = system_->registry_->template get<socket_component>(entity_);
    comp.sockets_.push_back(socket_point{name, position, rotation, scale, invalid_entity});
    return *this;
}

template <typename... Cs>
auto socket_system<Cs...>::socket_modifier::remove_socket(
    const std::string& name
) -> socket_modifier& {
    if (!system_->registry_->template has<socket_component>(entity_)) {
        return *this;
    }
    auto& comp = system_->registry_->template get<socket_component>(entity_);
    auto it    = std::ranges::find_if(comp.sockets_, [&](const socket_point& sp) -> auto {
        return sp.name == name;
    });
    if (it == comp.sockets_.end()) {
        return *this;
    }
    if (it->attached.is_valid()) {
        auto detached = it->attached;
        it->attached  = invalid_entity;
        system_->hierarchy_system_->modify(detached).remove_parent();
    }
    comp.sockets_.erase(it);
    return *this;
}

}  // namespace vw::gfx

#endif  // VW_GFX_WORLD_SYSTEMS_SOCKET_SYSTEM_INL_H
