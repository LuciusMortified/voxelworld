#pragma once

#ifndef VW_SCULPTOR_REMOVE_TRACK_OPERATION_H
#define VW_SCULPTOR_REMOVE_TRACK_OPERATION_H

#include <vw/gfx/animation/animation_track.h>
#include <vw/gfx/engine/engine.h>

#include "app/app_state.h"
#include "base_operation.h"

namespace vw::sculptor {

struct remove_track_params {
    std::string clip_name;
    std::string track_name;
};

class remove_track_operation final : public base_operation {
public:
    using engine_type = gfx::engine<>;

    remove_track_operation(engine_type& eng, app_state& state, remove_track_params params)
        : engine_(&eng), state_(&state), params_(std::move(params)) {}

    void execute() override {
        auto& registry = engine_->get_world().get_animation_clip_registry();
        auto clip      = registry.get(params_.clip_name);
        if (!clip) {
            return;
        }

        auto* track = clip->get_track(params_.track_name);
        if (track) {
            saved_track_ = *track;
        }

        clip->remove_track(params_.track_name);

        if (state_->selected_track_name == params_.track_name) {
            state_->selected_track_name.clear();
            state_->selected_keyframe_time = -1.f;
        }
        state_->expanded_tracks.erase(params_.track_name);
    }

    void undo() override {
        auto& registry = engine_->get_world().get_animation_clip_registry();
        auto clip      = registry.get(params_.clip_name);
        if (!clip || !saved_track_) {
            return;
        }
        clip->add_track(*saved_track_);
    }

private:
    engine_type* engine_;
    app_state* state_;
    remove_track_params params_;
    std::optional<gfx::animation_track> saved_track_;
};

}  // namespace vw::sculptor

#endif  // VW_SCULPTOR_REMOVE_TRACK_OPERATION_H
