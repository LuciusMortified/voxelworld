#pragma once

#ifndef VW_SCULPTOR_APP_INL_H
#define VW_SCULPTOR_APP_INL_H

namespace vw::sculptor {

inline app::app(
    gfx::engine<>& eng
)
    : gfx::app<>(eng)

    , camera_controller_(0.1f, 5.0f)

    , entity_creation_tool_(eng, state_)

    , tool_panel_(state_)
    , color_palette_panel_(state_)
    , entity_properties_panel_(eng, state_)
    , entity_tree_panel_(eng, state_, entity_creation_tool_) {
    auto& window = eng.get_window();
    auto& camera = eng.get_camera();

    camera_controller_.setup(window, camera);

    window.sub<gfx::key_press_event>([this](const gfx::key_press_event& event) {
        handle_key_press(event.key);
        return true;
    });

    window.sub<gfx::window_close_event>([this](gfx::window_close_event&) {
        get_engine().shutdown();
        return true;
    });

    window.sub<gfx::mouse_press_event>([this](gfx::mouse_press_event& ev) {
        handle_mouse_press(ev);
        return true;
    });

    camera.set_position({0.0f, 0.0f, -15.0f});
    camera.set_rotation(0.0f, 0.0f);
}

inline void app::render(
    float delta_time
) {
    camera_controller_.update(delta_time);

    auto& renderer = get_engine().get_renderer();
    renderer.draw_line(vec3f{0, 0, 0}, vec3f{100, 0, 0}, colors::blue);
    renderer.draw_line(vec3f{0, 0, 0}, vec3f{0, 100, 0}, colors::green);
    renderer.draw_line(vec3f{0, 0, 0}, vec3f{0, 0, 100}, colors::red);

    tool_panel_.render(delta_time);
    color_palette_panel_.render(delta_time);
    entity_properties_panel_.render(delta_time);
    entity_tree_panel_.render(delta_time);
}

inline void app::handle_key_press(
    gfx::keyboard::key key
) {
    if (key == gfx::keyboard::key::F1) {
        camera_movement_enabled_ = !camera_movement_enabled_;
        camera_controller_.set_mouse_captured(camera_movement_enabled_);
        camera_controller_.set_keyboard_control_enabled(camera_movement_enabled_);

        auto& io = ImGui::GetIO();
        if (camera_movement_enabled_) {
            io.ConfigFlags |= ImGuiConfigFlags_NoMouse | ImGuiConfigFlags_NoKeyboard;
            io.WantCaptureMouse    = false;
            io.WantCaptureKeyboard = false;

            ImGui::CloseCurrentPopup();
            ImGui::SetWindowFocus(nullptr);

            io.MousePos = ImVec2(-FLT_MAX, -FLT_MAX);
        } else {
            io.ConfigFlags &= ~(ImGuiConfigFlags_NoMouse | ImGuiConfigFlags_NoKeyboard);
        }
    }
}

inline void app::handle_mouse_press(
    const gfx::mouse_press_event& event
) {}

}  // namespace vw::sculptor

#endif  // VW_SCULPTOR_APP_INL_H
