#pragma once

#ifndef VW_GFX_WORLD_COMPONENTS_ANIMATION_FSM_COMPONENT_H
#define VW_GFX_WORLD_COMPONENTS_ANIMATION_FSM_COMPONENT_H

#include <vector>

#include "vw/gfx/animation/animation_fsm.h"

namespace vw::gfx {

struct animation_fsm_component final {
private:
    using trigger_set = animation_fsm::trigger_set;

    std::vector<animation_fsm> machines_;
    trigger_set triggers_;

    template <typename>
    friend class animation_fsm_system;

public:
    [[nodiscard]] auto machine_count() const -> size_t;
    [[nodiscard]] auto get_machine(size_t index) -> animation_fsm&;
    [[nodiscard]] auto get_machine(size_t index) const -> const animation_fsm&;
};

}  // namespace vw::gfx

#include "animation_fsm_component.inl.h"

#endif  // VW_GFX_WORLD_COMPONENTS_ANIMATION_FSM_COMPONENT_H