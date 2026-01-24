#pragma once

#ifndef VW_GFX_ENTITY_ARCHETYPE_INL_H
#define VW_GFX_ENTITY_ARCHETYPE_INL_H

namespace vw::gfx {

template <typename... Cs>
template <typename C>
size_t entity_archetype<Cs...>::type_index_() const noexcept {
    size_t index = 0;
    ((std::is_same_v<C, Cs> ? void() : void(++index)), ...);
    return index;
}

template <typename... Cs>
template <typename C>
void entity_archetype<Cs...>::add() {
    components_.set(type_index_<C>(), true);
}

template <typename... Cs>
template <typename C>
void entity_archetype<Cs...>::remove() {
    components_.set(type_index_<C>(), false);
}

template <typename... Cs>
template <typename C>
auto entity_archetype<Cs...>::has() const noexcept -> bool {
    return components_.test(type_index_<C>());
}

}  // namespace vw::gfx

#endif  // VW_GFX_ENTITY_ARCHETYPE_INL_H
