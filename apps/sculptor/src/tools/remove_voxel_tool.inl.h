#pragma once

#ifndef VW_SCULPTOR_REMOVE_VOXEL_TOOL_INL_H
#define VW_SCULPTOR_REMOVE_VOXEL_TOOL_INL_H
#include "operations/remove_voxel_operation.h"

namespace vw::sculptor {

template <typename WC>
remove_voxel_tool<WC>::remove_voxel_tool(
    engine_type& eng, app_state& st, operation_manager& op_manager
)
    : engine_(&eng), state_(&st), op_manager_(&op_manager) {}

template <typename WC>
void remove_voxel_tool<WC>::render(
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
        world.template has_component<gfx::transform_component>(ent) &&
        world.template has_component<gfx::model_component>(ent);
    if (!is_renderable) {
        return;
    }

    auto& renderer = engine_->get_renderer();
    auto voxel_local_pos = vec3f{
        static_cast<float>(hovered_voxel_.x),
        static_cast<float>(hovered_voxel_.y),
        static_cast<float>(hovered_voxel_.z)
    };

    auto voxel_world_pos =  //
        world.template get_component<gfx::transform_component>(ent).get_world_matrix() *
        math::translation_matrix(voxel_local_pos) *         //
        math::scale_matrix(vec3f{1.1f, 1.1f, 1.1f}) *  //
        math::translation_matrix(vec3f{-0.05f, -0.05f, -0.05f});
    renderer.draw_box(voxel_world_pos, vec3f{1.f, 1.f, 1.f}, colors::black);
}

template <typename WC>
void remove_voxel_tool<WC>::on_key_press(
    const gfx::key_press_event& ev
) {}

template <typename WC>
void remove_voxel_tool<WC>::on_mouse_move(
    const gfx::mouse_move_event& ev
) {
    const auto& world  = engine_->get_world();
    const auto& window = engine_->get_window();
    const auto& camera = engine_->get_camera();

    auto ray = camera.screen_to_world_ray(window.get_cursor_pos(), window.get_size());

    auto voxel_hit = world.voxel_ray_cast(ray, ray_cast_entities_);
    if (voxel_hit.has_value()) {
        hovered_voxel_ = voxel_hit->position;
    } else {
        hovered_voxel_ = vec3i{-1, -1, -1};
    }
}

template <typename WC>
void remove_voxel_tool<WC>::on_mouse_press(
    const gfx::mouse_press_event& ev
) {
    if (ev.button == gfx::mouse::buttons::LEFT) {
        if (hovered_voxel_ == vec3i{-1, -1, -1}) {
            return;
        }

        if (!state_->name_to_entity.contains(state_->selected_name)) {
            return;
        }

        remove_voxel_params params = {
            .name = state_->selected_name,
            .position = hovered_voxel_,
        };

        auto op = std::make_unique<remove_voxel_operation<WC>>(
            *engine_, *state_, params
        );
        op_manager_->execute(std::move(op));
    }
}

template <typename WC>
void remove_voxel_tool<WC>::on_mouse_release(
    const gfx::mouse_release_event& ev
) {}

}  // namespace vw::sculptor

#endif  // VW_SCULPTOR_REMOVE_VOXEL_TOOL_INL_H
