#pragma once

#ifndef VW_GFX_WORLD_COMPONENTS_ANIMATION_STATE_MACHINE_COMPONENT_H
#define VW_GFX_WORLD_COMPONENTS_ANIMATION_STATE_MACHINE_COMPONENT_H

#include <vector>

#include "vw/gfx/animation/animation_state_machine.h"

namespace vw::gfx {

struct animation_state_machine_component final {
private:
    using trigger_set = animation_state_machine::trigger_set;

    std::vector<animation_state_machine> machines_;
    trigger_set triggers_;

    template <typename>
    friend class animation_state_machine_system;

public:
    [[nodiscard]] auto machine_count() const -> size_t;
    [[nodiscard]] auto get_machine(size_t index) -> animation_state_machine&;
    [[nodiscard]] auto get_machine(size_t index) const -> const animation_state_machine&;
};

}  // namespace vw::gfx

#include "animation_state_machine_component.inl.h"

#endif  // VW_GFX_WORLD_COMPONENTS_ANIMATION_STATE_MACHINE_COMPONENT_H
