#pragma once

#ifndef VW_GFX_ENTITY_ARCHETYPE_H
#define VW_GFX_ENTITY_ARCHETYPE_H

#include <bitset>

namespace vw::gfx {

template <typename... Cs>
struct entity_archetype final {
private:
    static constexpr size_t component_count = sizeof...(Cs);
    std::bitset<component_count> components_;

    template <typename C>
    [[nodiscard]] size_t type_index_() const noexcept;

public:
    template <typename C>
    void add();

    template <typename C>
    void remove();

    template <typename C>
    [[nodiscard]] auto has() const noexcept -> bool;
};

template <typename... Cs>
struct entity_archetype_from_tuple;

template <typename... Cs>
struct entity_archetype_from_tuple<std::tuple<Cs...>> {
    using type = entity_archetype<Cs...>;
};

}  // namespace vw::gfx

#include "vw/gfx/world/entity_archetype.inl.h"

#endif  // VW_GFX_ENTITY_ARCHETYPE_H
