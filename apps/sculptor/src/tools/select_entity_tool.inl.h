#pragma once

#ifndef VW_SCULPTOR_SELECT_ENTITY_TOOL_INL_H
#define VW_SCULPTOR_SELECT_ENTITY_TOOL_INL_H

namespace vw::sculptor {

inline select_entity_tool::select_entity_tool(
    engine_type& eng, app_state& st
)
    : engine_(&eng), state_(&st) {}

inline void select_entity_tool::render(
    float delta_time
) {
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

    auto& model_comp     = world.get_component<gfx::model_component>(ent);
    if (!model_comp.has_model()) {
        return;
    }
    auto model_size = model_comp.size();
    auto model_size_vec3f = vec3f{
        static_cast<float>(model_size.x),
        static_cast<float>(model_size.y),
        static_cast<float>(model_size.z)
    };

    auto& renderer = engine_->get_renderer();

    auto ent_world_pos =  //
        world.get_component<gfx::transform_component>(ent).get_world_matrix() *
        math::scale_matrix(model_size_vec3f) *            //
        math::scale_matrix(vec3f{1.01f, 1.01f, 1.01f}) *  //
        math::translation_matrix(vec3f{-0.005f, -0.005f, -0.005f});
    renderer.draw_box(
        ent_world_pos, vec3f{1.f, 1.f, 1.f}, colors::black
    );
}

inline void select_entity_tool::on_key_press(
    const gfx::key_press_event& ev
) {}

inline void select_entity_tool::on_mouse_move(
    const gfx::mouse_move_event& ev
) {

}

inline void select_entity_tool::on_mouse_press(
    const gfx::mouse_press_event& ev
) {
    if (ev.button == gfx::mouse::buttons::LEFT) {
        const auto& world  = engine_->get_world();
        const auto& window = engine_->get_window();
        const auto& camera = engine_->get_camera();

        auto ray = camera.screen_to_world_ray(
            window.get_cursor_pos(), window.get_size()
        );

        auto voxel_hit = world.voxel_ray_cast(ray, ray_cast_entities_);
        if (voxel_hit.has_value()) {
            auto ent = voxel_hit->ent;
            if (state_->entity_to_name.contains(ent)) {
                state_->selected_name = state_->entity_to_name[ent];
            }
        }
    }

}

inline void select_entity_tool::on_mouse_release(
    const gfx::mouse_release_event& ev
) {}

}  // namespace vw::sculptor

#endif  // VW_SCULPTOR_SELECT_ENTITY_TOOL_INL_H
