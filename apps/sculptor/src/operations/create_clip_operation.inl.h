#pragma once

#ifndef VW_SCULPTOR_CREATE_CLIP_OPERATION_INL_H
#define VW_SCULPTOR_CREATE_CLIP_OPERATION_INL_H

namespace vw::sculptor {

inline create_clip_operation::create_clip_operation(
    engine_type& engine, app_state& state, const create_clip_params& params
)
    : base_operation(), engine_(&engine), state_(&state), params_(params) {}

inline void create_clip_operation::execute() {
    auto& registry = engine_->get_world().get_animation_clip_registry();
    (void)registry.create(params_.name);
    state_->selected_clip_name = params_.name;
    state_->ui.show_timeline   = true;
    state_->has_unsaved_changes = true;
}

inline void create_clip_operation::undo() {
    auto& registry = engine_->get_world().get_animation_clip_registry();
    registry.remove(params_.name);
    state_->selected_clip_name.clear();
    state_->has_unsaved_changes = true;
}

}  // namespace vw::sculptor

#endif  // VW_SCULPTOR_CREATE_CLIP_OPERATION_INL_H
