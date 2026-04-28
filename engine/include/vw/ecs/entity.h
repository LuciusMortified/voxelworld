#pragma once

#ifndef VW_ECS_ENTITY_H
#define VW_ECS_ENTITY_H

#include <limits>

#include "vw/core.h"

namespace vw::ecs {

struct entity final {
    static constexpr uint32 invalid_index = std::numeric_limits<uint32>::max();

    uint32 index      = invalid_index;
    uint32 generation = 0;

    [[nodiscard]] auto operator==(
        const entity& rhs
    ) const -> bool {
        return index == rhs.index && generation == rhs.generation;
    }

    [[nodiscard]] auto operator!=(
        const entity& rhs
    ) const -> bool {
        return !(*this == rhs);
    }

    [[nodiscard]] auto operator<(
        const entity& rhs
    ) const -> bool {
        return index != rhs.index ? index < rhs.index : generation < rhs.generation;
    }

    [[nodiscard]] auto is_valid() const -> bool {
        return index != invalid_index;
    }
};

static constexpr entity invalid_entity = entity{};

}  // namespace vw::ecs

template <>
struct std::hash<vw::ecs::entity> {
    auto operator()(
        const vw::ecs::entity& ent
    ) const noexcept -> std::size_t {
        return std::hash<std::size_t>()(ent.index) ^
            (std::hash<std::size_t>()(ent.generation) << 1);
    }
};  // namespace std

#endif  // VW_ECS_ENTITY_H
