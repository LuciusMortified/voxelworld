#pragma once

#ifndef VW_SCULPTOR_ADD_VOXEL_TOOL_INL_H
#define VW_SCULPTOR_ADD_VOXEL_TOOL_INL_H

namespace vw::sculptor {

template <typename WC>
add_voxel_tool<WC>::add_voxel_tool(
    engine_type& eng, app_state& st, operation_manager& op_manager
)
    : engine_(&eng), state_(&st), op_manager_(&op_manager) {}

template <typename WC>
void add_voxel_tool<WC>::render(
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
void add_voxel_tool<WC>::on_key_press(
    const gfx::key_press_event& ev
) {}

template <typename WC>
void add_voxel_tool<WC>::on_mouse_move(
    const gfx::mouse_move_event& ev
) {
    hovered_voxel_ = vec3i{-1, -1, -1};

    auto& world  = engine_->get_world();
    auto& window = engine_->get_window();
    auto& camera = engine_->get_camera();

    auto ray = camera.screen_to_world_ray(window.get_cursor_pos(), window.get_size());

    auto voxel_hit = world.voxel_ray_cast(ray, ray_cast_entities_);
    if (!voxel_hit.has_value()) {
        return;
    }

    auto ent = voxel_hit->ent;

    const auto& transform_comp = world.template get_component<gfx::transform_component>(ent);
    const auto& model_comp     = world.template get_component<gfx::model_component>(ent);

    mat4f world_matrix   = transform_comp.get_world_matrix();
    vec3i occupied_voxel = voxel_hit->position;
    vec3f occupied_voxel_center_local{
        static_cast<float>(occupied_voxel.x) + 0.5f,
        static_cast<float>(occupied_voxel.y) + 0.5f,
        static_cast<float>(occupied_voxel.z) + 0.5f
    };
    vec3f occupied_voxel_center_world = world_matrix * occupied_voxel_center_local;
    vec3f direction_to_ray_start      = math::normalize(ray.start - occupied_voxel_center_world);

    const vec3i neighbor_offsets[6] = {
        vec3i{1, 0, 0},   // +X
        vec3i{-1, 0, 0},  // -X
        vec3i{0, 1, 0},   // +Y
        vec3i{0, -1, 0},  // -Y
        vec3i{0, 0, 1},   // +Z
        vec3i{0, 0, -1}   // -Z
    };

    vec3i best_empty_voxel = vec3i{-1, -1, -1};
    float best_dot_product = -std::numeric_limits<float>::max();

    for (const auto& offset : neighbor_offsets) {
        vec3i neighbor_pos = occupied_voxel + offset;

        if (neighbor_pos.x < 0 || neighbor_pos.x >= model_comp.width() || neighbor_pos.y < 0 ||
            neighbor_pos.y >= model_comp.height() || neighbor_pos.z < 0 ||
            neighbor_pos.z >= model_comp.depth()) {
            continue;
        }

        if (model_comp.is_empty(neighbor_pos)) {
            vec3f neighbor_center_local{
                static_cast<float>(neighbor_pos.x) + 0.5f,
                static_cast<float>(neighbor_pos.y) + 0.5f,
                static_cast<float>(neighbor_pos.z) + 0.5f
            };

            vec3f neighbor_center_world = world_matrix * neighbor_center_local;
            vec3f neighbor_direction_world =
                math::normalize(neighbor_center_world - occupied_voxel_center_world);

            float dot_product = math::dot(neighbor_direction_world, direction_to_ray_start);
            if (dot_product > 0.0f && dot_product > best_dot_product) {
                best_dot_product = dot_product;
                best_empty_voxel = neighbor_pos;
            }
        }
    }

    hovered_voxel_ = best_empty_voxel;
}

template <typename WC>
void add_voxel_tool<WC>::on_mouse_press(
    const gfx::mouse_press_event& ev
) {}

template <typename WC>
void add_voxel_tool<WC>::on_mouse_release(
    const gfx::mouse_release_event& ev
) {}

}  // namespace vw::sculptor

#endif  // VW_SCULPTOR_ADD_VOXEL_TOOL_INL_H
