#pragma once

#ifndef VW_SCULPTOR_COLOR_PICKER_TOOL_INL_H
#define VW_SCULPTOR_COLOR_PICKER_TOOL_INL_H

namespace vw::sculptor {

inline color_picker_tool::color_picker_tool(
    engine_type& eng, app_state& st, operation_manager& op_manager
)
    : engine_(&eng), state_(&st), op_manager_(&op_manager) {}

inline void color_picker_tool::render(
    float delta_time
) {
    const bool is_hovered = hovered_voxel_ != vec3i{-1, -1, -1};
    const bool has_selected_entity =
        state_->scene.name_to_entity.contains(state_->scene.selected_name);
    if (!is_hovered || !has_selected_entity) {
        return;
    }

    auto& world    = engine_->get_world();
    const auto ent = state_->scene.name_to_entity[state_->scene.selected_name];

    const bool is_renderable =  //
        world.has_component<gfx::transform_component>(ent) &&
        world.has_component<gfx::model_component>(ent);
    if (!is_renderable) {
        return;
    }

    const vec3f voxel_local_pos{
        static_cast<float>(hovered_voxel_.x),
        static_cast<float>(hovered_voxel_.y),
        static_cast<float>(hovered_voxel_.z)
    };

    const auto voxel_world_pos =  //
        world.get_component<gfx::transform_component>(ent).get_world_matrix() *
        math::translation_matrix(voxel_local_pos) *       //
        math::scale_matrix(vec3f{1.01f, 1.01f, 1.01f}) *  //
        math::translation_matrix(vec3f{-0.005f, -0.005f, -0.005f});

    auto& renderer = engine_->get_renderer();
    renderer.draw_box(voxel_world_pos, vec3f{1.f, 1.f, 1.f}, colors::black);
}

inline void color_picker_tool::on_key_press(
    const gfx::key_press_event& ev
) {}

inline void color_picker_tool::on_mouse_move(
    const gfx::mouse_move_event& ev
) {
    update_hovered_voxel_();
}

inline void color_picker_tool::on_mouse_press(
    const gfx::mouse_press_event& ev
) {
    if (ev.button != gfx::mouse::buttons::LEFT) {
        return;
    }

    const bool is_hovered = hovered_voxel_ != vec3i{-1, -1, -1};
    const bool has_selected_entity =
        state_->scene.name_to_entity.contains(state_->scene.selected_name);
    if (!is_hovered || !has_selected_entity) {
        return;
    }

    auto& world    = engine_->get_world();
    const auto ent = state_->scene.name_to_entity[state_->scene.selected_name];

    const bool is_renderable =  //
        world.has_component<gfx::transform_component>(ent) &&
        world.has_component<gfx::model_component>(ent);
    if (!is_renderable) {
        return;
    }

    const auto& model_comp = world.get_component<gfx::model_component>(ent);
    if (!model_comp.has_model()) {
        return;
    }

    state_->tool.selected_block = model_comp.get_voxel(hovered_voxel_).id;
}

inline void color_picker_tool::on_mouse_release(
    const gfx::mouse_release_event& ev
) {}

inline void color_picker_tool::on_activate() {
    update_hovered_voxel_();
}

inline void color_picker_tool::update_hovered_voxel_() {
    const auto& world  = engine_->get_world();
    const auto& window = engine_->get_window();
    const auto& camera = engine_->get_camera();

    const auto ray = camera.screen_to_world_ray(window.get_cursor_pos(), window.get_size());
    const auto hit = world.voxel_ray_cast(ray, ray_cast_entities_);
    if (!hit) {
        hovered_voxel_ = vec3i{-1, -1, -1};
        return;
    }

    const bool is_selected_entity =  //
        state_->scene.name_to_entity.contains(state_->scene.selected_name) &&
        hit->ent == state_->scene.name_to_entity[state_->scene.selected_name];
    if (is_selected_entity) {
        hovered_voxel_ = hit->voxel_pos;
    }
}

}  // namespace vw::sculptor

#endif  // VW_SCULPTOR_COLOR_PICKER_TOOL_INL_H
