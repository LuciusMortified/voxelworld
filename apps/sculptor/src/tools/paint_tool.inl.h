#pragma once

#ifndef VW_SCULPTOR_PAINT_TOOL_INL_H
#define VW_SCULPTOR_PAINT_TOOL_INL_H
#include "operations/paint_voxel_operation.h"

namespace vw::sculptor {

inline paint_tool::paint_tool(
    engine_type& eng, app_state& st, operation_manager& op_manager
)
    : engine_(&eng), state_(&st), op_manager_(&op_manager) {}

inline void paint_tool::render(
    float delta_time
) {
    if (hovered_voxel_ == vec3i{-1, -1, -1}) {
        return;
    }

    if (!state_->name_to_entity.contains(state_->selected_name)) {
        return;
    }

    auto ent = state_->name_to_entity[state_->selected_name];

    auto& world        = engine_->get_world();
    bool is_renderable =  //
        world.has_component<gfx::transform_component>(ent) &&
        world.has_component<gfx::model_component>(ent);
    if (!is_renderable) {
        return;
    }

    auto& renderer       = engine_->get_renderer();
    auto voxel_local_pos = vec3f{
        static_cast<float>(hovered_voxel_.x),
        static_cast<float>(hovered_voxel_.y),
        static_cast<float>(hovered_voxel_.z)
    };

    auto voxel_world_pos =  //
        world.get_component<gfx::transform_component>(ent).get_world_matrix() *
        math::translation_matrix(voxel_local_pos) *    //
        math::scale_matrix(vec3f{1.01f, 1.01f, 1.01f}) *  //
        math::translation_matrix(vec3f{-0.005f, -0.005f, -0.005f});

    renderer.draw_box(voxel_world_pos, vec3f{1.f, 1.f, 1.f}, colors::black);
}

inline void paint_tool::on_key_press(
    const gfx::key_press_event& ev
) {}

inline void paint_tool::on_mouse_move(
    const gfx::mouse_move_event& ev
) {
    update_hovered_voxel_();
}

inline void paint_tool::on_mouse_press(
    const gfx::mouse_press_event& ev
) {
    using buttons = gfx::mouse::buttons;

    if (ev.button == buttons::LEFT) {
        if (hovered_voxel_ == vec3i{-1, -1, -1}) {
            return;
        }

        if (!state_->name_to_entity.contains(state_->selected_name)) {
            return;
        }

        auto ent = state_->name_to_entity[state_->selected_name];

        auto& world        = engine_->get_world();
        bool is_renderable =  //
            world.has_component<gfx::transform_component>(ent) &&
            world.has_component<gfx::model_component>(ent);
        if (!is_renderable) {
            return;
        }

        paint_voxel_params params;
        params.name     = state_->selected_name;
        params.position = hovered_voxel_;
        params.color    = state_->selected_color;

        auto op = std::make_unique<paint_voxel_operation>(
            *engine_, *state_, params
        );
        op_manager_->execute(std::move(op));
    }
}

inline void paint_tool::on_mouse_release(
    const gfx::mouse_release_event& ev
) {}

inline void paint_tool::on_activate() {
    update_hovered_voxel_();
}

inline void paint_tool::update_hovered_voxel_() {
    const auto& world  = engine_->get_world();
    const auto& window = engine_->get_window();
    const auto& camera = engine_->get_camera();

    auto ray = camera.screen_to_world_ray(window.get_cursor_pos(), window.get_size());

    auto voxel_hit = world.voxel_ray_cast(ray, ray_cast_entities_);
    if (voxel_hit.has_value()) {
        hovered_voxel_ = voxel_hit->voxel_pos;
        state_->selected_name = state_->entity_to_name[voxel_hit->ent];
    } else {
        hovered_voxel_ = vec3i{-1, -1, -1};
    }
}

}  // namespace vw::sculptor

#endif  // VW_SCULPTOR_PAINT_TOOL_INL_H
