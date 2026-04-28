#pragma once

#ifndef VW_ECS_SYSTEMS_SOCKET_SYSTEM_INL_H
#define VW_ECS_SYSTEMS_SOCKET_SYSTEM_INL_H

#include <algorithm>

#include "vw/ecs/systems/hierarchy_system.h"
#include "vw/ecs/systems/socket_system.h"
#include "vw/ecs/systems/transform_system.h"
#include "vw/ecs/world.h"

namespace vw::ecs {

template <typename WD>
socket_system<WD>::socket_system(world_type& w)
    : world_(&w) {}

template <typename WD>
void socket_system<WD>::update(float32 /*dt*/) {}

template <typename WD>
void socket_system<WD>::cleanup(
    entity ent
) {
    auto& reg = world_->registry();
    if (!reg.template has<socket_component>(ent)) {
        return;
    }
    auto& comp = reg.template get<socket_component>(ent);
    for (auto& slot : comp.sockets_) {
        if (slot.attached.is_valid()) {
            world_->template system<hierarchy_system>().modify(slot.attached).remove_parent();
            slot.attached = invalid_entity;
        }
    }
}

template <typename WD>
socket_system<WD>::socket_modifier::socket_modifier(
    socket_system* system, entity ent
)
    : system_(system), entity_(ent) {}

template <typename WD>
auto socket_system<WD>::modify(
    entity ent
) -> socket_modifier {
    return socket_modifier(this, ent);
}

template <typename WD>
auto socket_system<WD>::socket_modifier::attach(
    const std::string& socket_name, entity child
) -> socket_modifier& {
    auto& reg = system_->world_->registry();
    if (!reg.template has<socket_component>(entity_)) {
        return *this;
    }
    auto& comp = reg.template get<socket_component>(entity_);
    auto it    = std::ranges::find_if(comp.sockets_, [&](const socket_point& sp) {
        return sp.name == socket_name;
    });
    if (it == comp.sockets_.end() || it->attached.is_valid()) {
        return *this;
    }
    it->attached = child;
    system_->world_->template system<hierarchy_system>().modify(child).set_parent(entity_);
    system_->world_->template system<transform_system>().modify(child)
        .set_position(it->position)
        .set_rotation(it->rotation)
        .set_scale(it->scale);
    return *this;
}

template <typename WD>
auto socket_system<WD>::socket_modifier::detach(
    const std::string& socket_name
) -> socket_modifier& {
    auto& reg = system_->world_->registry();
    if (!reg.template has<socket_component>(entity_)) {
        return *this;
    }
    auto& comp = reg.template get<socket_component>(entity_);
    auto it    = std::ranges::find_if(comp.sockets_, [&](const socket_point& sp) {
        return sp.name == socket_name;
    });
    if (it == comp.sockets_.end() || !it->attached.is_valid()) {
        return *this;
    }
    auto detached = it->attached;
    it->attached  = invalid_entity;
    system_->world_->template system<hierarchy_system>().modify(detached).remove_parent();
    return *this;
}

template <typename WD>
auto socket_system<WD>::socket_modifier::add_socket(
    const std::string& name, const vec3f& position, const quat& rotation, const vec3f& scale
) -> socket_modifier& {
    auto& reg = system_->world_->registry();
    if (!reg.template has<socket_component>(entity_)) {
        return *this;
    }
    auto& comp = reg.template get<socket_component>(entity_);
    comp.sockets_.push_back(socket_point{name, position, rotation, scale, invalid_entity});
    return *this;
}

template <typename WD>
auto socket_system<WD>::socket_modifier::remove_socket(
    const std::string& name
) -> socket_modifier& {
    auto& reg = system_->world_->registry();
    if (!reg.template has<socket_component>(entity_)) {
        return *this;
    }
    auto& comp = reg.template get<socket_component>(entity_);
    auto it    = std::ranges::find_if(comp.sockets_, [&](const socket_point& sp) -> auto {
        return sp.name == name;
    });
    if (it == comp.sockets_.end()) {
        return *this;
    }
    if (it->attached.is_valid()) {
        auto detached = it->attached;
        it->attached  = invalid_entity;
        system_->world_->template system<hierarchy_system>().modify(detached).remove_parent();
    }
    comp.sockets_.erase(it);
    return *this;
}

}  // namespace vw::ecs

#endif  // VW_ECS_SYSTEMS_SOCKET_SYSTEM_INL_H
