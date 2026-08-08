#pragma once

#ifndef VW_ECS_SYSTEM_TRAIT_H
#define VW_ECS_SYSTEM_TRAIT_H

#include <concepts>

#include "vw/ecs/entity.h"

namespace vw::ecs {

template <typename S, typename C>
concept has_on_add = requires(S& s, entity e) {
    s.template on_add<C>(e);
};

template <typename S, typename C>
concept has_on_remove = requires(S& s, entity e) {
    s.template on_remove<C>(e);
};

template <typename S>
concept has_shutdown = requires(S& s) {
    s.shutdown();
};

namespace detail {

template <typename C, typename S>
void invoke_on_add(S& system, entity ent) {
    if constexpr (has_on_add<S, C>) {
        system.template on_add<C>(ent);
    }
}

template <typename C, typename S>
void invoke_on_remove(S& system, entity ent) {
    if constexpr (has_on_remove<S, C>) {
        system.template on_remove<C>(ent);
    }
}

template <typename S>
void invoke_shutdown(S& system) {
    if constexpr (has_shutdown<S>) {
        system.shutdown();
    }
}

}  // namespace detail

}  // namespace vw::ecs

#endif  // VW_ECS_SYSTEM_TRAIT_H
