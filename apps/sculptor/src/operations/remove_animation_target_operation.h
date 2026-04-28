#pragma once

#ifndef VW_SCULPTOR_REMOVE_ANIMATION_TARGET_OPERATION_H
#define VW_SCULPTOR_REMOVE_ANIMATION_TARGET_OPERATION_H

#include <vw/gfx/engine/engine.h>
#include <vw/ecs/world_components.h>

#include "app/app_state.h"
#include "base_operation.h"

namespace vw::sculptor {

struct remove_animation_target_params {
    std::string entity_name;
};

class remove_animation_target_operation final : public base_operation {
public:
    using engine_type = gfx::engine<>;

    remove_animation_target_operation(
        engine_type& engine, app_state& state, const remove_animation_target_params& params
    );

    void execute() override;
    void undo() override;

private:
    engine_type* engine_;
    app_state* state_;
    remove_animation_target_params params_;

    std::string saved_target_name_;

    [[nodiscard]] auto find_animation_root_(ecs::entity ent) const -> ecs::entity;
};

}  // namespace vw::sculptor

#include "remove_animation_target_operation.inl.h"

#endif  // VW_SCULPTOR_REMOVE_ANIMATION_TARGET_OPERATION_H
