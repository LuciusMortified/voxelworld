#pragma once

#ifndef VW_GFX_WORLD_COMPONENTS_ANIMATION_FSM_COMPONENT_H
#define VW_GFX_WORLD_COMPONENTS_ANIMATION_FSM_COMPONENT_H

#include <vector>

#include "vw/asset/animation/animation_fsm.h"

namespace vw::gfx {


struct animation_fsm_component final {
private:
    using trigger_set = vw::asset::animation_fsm::trigger_set;

    std::vector<vw::asset::animation_fsm> machines_;
    trigger_set triggers_;

    template <typename>
    friend class animation_fsm_system;

public:
    [[nodiscard]] auto machine_count() const -> size_t;
    [[nodiscard]] auto get_machine(size_t index) -> vw::asset::animation_fsm&;
    [[nodiscard]] auto get_machine(size_t index) const -> const vw::asset::animation_fsm&;
};

}  // namespace vw::gfx

#include "animation_fsm_component.inl.h"

#endif  // VW_GFX_WORLD_COMPONENTS_ANIMATION_FSM_COMPONENT_H