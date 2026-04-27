#pragma once

#ifndef VW_GFX_WORLD_SYSTEM_TRAIT_H
#define VW_GFX_WORLD_SYSTEM_TRAIT_H

#include <concepts>
#include <tuple>

#include "vw/gfx/world/entity.h"

namespace vw::gfx {

template <template <typename> class S>
struct system_trait {
    using components = std::tuple<>;
    using resources  = std::tuple<>;
};

template <typename S, typename C>
concept has_on_add = requires(S& s, entity e) {
    s.template on_add<C>(e);
};

template <typename S, typename C>
concept has_on_remove = requires(S& s, entity e) {
    s.template on_remove<C>(e);
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

}  // namespace detail

}  // namespace vw::gfx

#endif  // VW_GFX_WORLD_SYSTEM_TRAIT_H
