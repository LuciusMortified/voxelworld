#pragma once

#ifndef VW_ECS_ENTITY_ARCHETYPE_H
#define VW_ECS_ENTITY_ARCHETYPE_H

#include <bitset>

#include "vw/core.h"

namespace vw::ecs {

template <typename T, typename... Ts>
consteval auto type_index_in() -> size_t;

template <typename... Cs>
class entity_archetype final {
public:
    static constexpr size_t component_count = sizeof...(Cs);

    template <typename C>
    void set() noexcept {
        bits_.set(type_index_in<C, Cs...>());
    }

    template <typename C>
    void unset() noexcept {
        bits_.reset(type_index_in<C, Cs...>());
    }

    template <typename C>
    [[nodiscard]] auto has() const noexcept -> bool {
        return bits_.test(type_index_in<C, Cs...>());
    }

    void clear() noexcept {
        bits_.reset();
    }

    [[nodiscard]] auto empty() const noexcept -> bool {
        return bits_.none();
    }

    [[nodiscard]] auto bits() const noexcept -> const std::bitset<component_count>& {
        return bits_;
    }

private:
    std::bitset<component_count> bits_;
};

template <typename... Cs>
struct entity_archetype_from_tuple;

template <typename... Cs>
struct entity_archetype_from_tuple<std::tuple<Cs...>> {
    using type = entity_archetype<Cs...>;
};

}  // namespace vw::ecs

#endif  // VW_ECS_ENTITY_ARCHETYPE_H
