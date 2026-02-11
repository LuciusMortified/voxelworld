#include <vw/core.h>
#include <vw/gfx.h>

using namespace vw;

class test_animation_app final : public gfx::app<> {
public:
    explicit test_animation_app(
        gfx::engine<>& eng
    )
        : app{eng} {
        auto& window = get_engine().get_window();
        auto& camera = get_engine().get_camera();

        camera_controller_ = std::make_unique<gfx::fps_camera_controller>(0.1f, 5.0f);
        camera_controller_->setup(window, camera);

        window.sub<gfx::key_press_event>([this](const gfx::key_press_event& event) {
            handle_key_press(event);
            return true;
        });

        window.sub<gfx::window_close_event>([this](gfx::window_close_event&) {
            get_engine().shutdown();
            return true;
        });

        camera.set_position({0.0f, 5.0f, -10.0f});
        camera.set_rotation(0.0f, 0.0f);
        get_engine().get_renderer().set_clear_color(0.1f, 0.1f, 0.15f, 1.0f);

        setup_scene();
        create_animations();
    }

    ~test_animation_app() override = default;

    void render(
        float32 delta_time
    ) override {
        camera_controller_->update(delta_time);

        auto& renderer = get_engine().get_renderer();

        renderer.draw_line(vec3f{0, 0, 0}, vec3f{5, 0, 0}, colors::red);
        renderer.draw_line(vec3f{0, 0, 0}, vec3f{0, 5, 0}, colors::green);
        renderer.draw_line(vec3f{0, 0, 0}, vec3f{0, 0, 5}, colors::blue);

        render_ui();
    }

private:
    void setup_scene() {
        auto& world            = get_engine().get_world();
        auto& model_registry   = world.get_model_registry();
        auto& transform_system = world.get_transform_system();
        auto& model_system     = world.get_model_system();
        auto& hierarchy_system = world.get_hierarchy_system();
        auto& animation_system = world.get_animation_system();

        auto red_cube_model = model_registry.create("red_cube", 3, 3, 3);
        red_cube_model->fill(voxel{colors::red});

        auto green_cube_model = model_registry.create("green_cube", 3, 3, 3);
        green_cube_model->fill(voxel{colors::green});

        auto blue_cube_model = model_registry.create("blue_cube", 3, 3, 3);
        blue_cube_model->fill(voxel{colors::blue});

        root_ =
            gfx::entity_builder<>{world}
                .with<gfx::transform_component>()
                .with<gfx::hierarchy_component>()
                .with<gfx::animation_player_component>()
                .with<gfx::animation_target_component>()
                .release_guard();
        auto root_ent = root_->get_entity();

        animation_system  //
            .modify_target(root_ent)
            .set_target_name("root");

        red_ =
            gfx::entity_builder<>{world}
                .with<gfx::transform_component>()
                .with<gfx::hierarchy_component>()
                .with<gfx::spatial_component>()
                .with<gfx::model_component>()
                .with<gfx::animation_target_component>()
                .release_guard();

        auto red_ent = red_->get_entity();
        transform_system  //
            .modify(red_ent)
            .set_position({-4.0f, 0.0f, 0.0f});
        model_system  //
            .modify(red_ent)
            .set_model(red_cube_model);
        animation_system  //
            .modify_target(red_ent)
            .set_target_name("red");
        hierarchy_system  //
            .modify(red_ent)
            .set_parent(root_ent);

        green_ =
            gfx::entity_builder<>{world}
                .with<gfx::transform_component>()
                .with<gfx::hierarchy_component>()
                .with<gfx::spatial_component>()
                .with<gfx::model_component>()
                .with<gfx::animation_target_component>()
                .release_guard();

        auto green_ent = green_->get_entity();
        transform_system  //
            .modify(green_ent)
            .set_position({0.0f, 0.0f, 0.0f});
        model_system  //
            .modify(green_ent)
            .set_model(green_cube_model);
        animation_system  //
            .modify_target(green_ent)
            .set_target_name("green");
        hierarchy_system  //
            .modify(green_ent)
            .set_parent(root_ent);

        blue_ =
            gfx::entity_builder<>{world}
                .with<gfx::transform_component>()
                .with<gfx::hierarchy_component>()
                .with<gfx::spatial_component>()
                .with<gfx::model_component>()
                .with<gfx::animation_target_component>()
                .release_guard();

        auto blue_ent = blue_->get_entity();
        transform_system  //
            .modify(blue_ent)
            .set_position({4.0f, 0.0f, 0.0f});
        model_system  //
            .modify(blue_ent)
            .set_model(blue_cube_model);
        animation_system  //
            .modify_target(blue_ent)
            .set_target_name("blue");
        hierarchy_system  //
            .modify(blue_ent)
            .set_parent(root_ent);
    }

    void create_animations() {
        create_bounce_animation();
        create_rotation_animation();
        create_wave_animation();
        create_scale_animation();
    }

    void create_bounce_animation() {
        auto& world         = get_engine().get_world();
        auto& clip_registry = world.get_animation_clip_registry();
        auto clip           = clip_registry.create("bounce");

        {
            auto track       = gfx::animation_track("red", 120.0f);

            auto pos_ch = gfx::make_animation_channel<gfx::animation_property::position>();
            pos_ch.add({0.0f, vec3f{-4.0f, 0.0f, 0.0f}});
            pos_ch.add({0.5f, vec3f{-4.0f, 3.0f, 0.0f}});
            pos_ch.add({1.0f, vec3f{-4.0f, 0.0f, 0.0f}});
            track.add<gfx::animation_property::position>(pos_ch);

            auto org_ch = gfx::make_animation_channel<gfx::animation_property::origin>();
            org_ch.add({0.0f, vec3f{-1.5f, -1.5f, -1.5f}});
            track.add<gfx::animation_property::origin>(org_ch);

            clip->add_track(track);
        }

        {
            auto track       = gfx::animation_track("green", 120.0f);

            auto pos_ch = gfx::make_animation_channel<gfx::animation_property::position>();
            pos_ch.add({0.0f, vec3f{0.0f, 3.0f, 0.0f}});
            pos_ch.add({0.5f, vec3f{0.0f, 0.0f, 0.0f}});
            pos_ch.add({1.0f, vec3f{0.0f, 3.0f, 0.0f}});
            track.add<gfx::animation_property::position>(pos_ch);

            auto org_ch = gfx::make_animation_channel<gfx::animation_property::origin>();
            org_ch.add({0.0f, vec3f{-1.5f, -1.5f, -1.5f}});
            track.add<gfx::animation_property::origin>(org_ch);

            clip->add_track(track);
        }

        {
            auto track       = gfx::animation_track("blue", 120.0f);

            auto pos_ch = gfx::make_animation_channel<gfx::animation_property::position>();
            pos_ch.add({0.0f, vec3f{4.0f, 0.0f, 0.0f}});
            pos_ch.add({0.5f, vec3f{4.0f, 3.0f, 0.0f}});
            pos_ch.add({1.0f, vec3f{4.0f, 0.0f, 0.0f}});
            track.add<gfx::animation_property::position>(pos_ch);

            auto org_ch = gfx::make_animation_channel<gfx::animation_property::origin>();
            org_ch.add({0.0f, vec3f{-1.5f, -1.5f, -1.5f}});
            track.add<gfx::animation_property::origin>(org_ch);

            clip->add_track(track);
        }

        auto& animation_system = world.get_animation_system();
        auto player_modifier   = animation_system.modify_player(root_->get_entity());
        player_modifier.set_clip_by_name("bounce");
        player_modifier.set_loop_mode(gfx::animation_loop_mode::loop);
        player_modifier.play();
    }

    void create_rotation_animation() {
        auto& world         = get_engine().get_world();
        auto& clip_registry = world.get_animation_clip_registry();
        auto clip           = clip_registry.create("rotation");

        {
            auto track       = gfx::animation_track("red", 120.0f);

            auto pos_ch = gfx::make_animation_channel<gfx::animation_property::position>();
            pos_ch.add({0.0f, vec3f{-4.0f, 0.0f, 0.0f}});
            track.add<gfx::animation_property::position>(pos_ch);

            auto org_ch = gfx::make_animation_channel<gfx::animation_property::origin>();
            org_ch.add({0.0f, vec3f{-1.5f, -1.5f, -1.5f}});
            track.add<gfx::animation_property::origin>(org_ch);

            auto rot_ch = gfx::make_animation_channel<gfx::animation_property::rotation>();
            rot_ch.add({0.0f, vec3f{0.0f, 0.0f, 0.0f}});
            rot_ch.add({1.0f, vec3f{math::radians(180.0f), 0.0f, 0.0f}});
            track.add<gfx::animation_property::rotation>(rot_ch);

            clip->add_track(track);
        }

        {
            auto track       = gfx::animation_track("green", 120.0f);

            auto pos_ch = gfx::make_animation_channel<gfx::animation_property::position>();
            pos_ch.add({0.0f, vec3f{0.0f, 0.0f, 0.0f}});
            track.add<gfx::animation_property::position>(pos_ch);

            auto org_ch = gfx::make_animation_channel<gfx::animation_property::origin>();
            org_ch.add({0.0f, vec3f{-1.5f, -1.5f, -1.5f}});
            track.add<gfx::animation_property::origin>(org_ch);

            auto rot_ch = gfx::make_animation_channel<gfx::animation_property::rotation>();
            rot_ch.add({0.0f, vec3f{0.0f, 0.0f, 0.0f}});
            rot_ch.add({1.0f, vec3f{0.0f, math::radians(180.0f), 0.0f}});
            track.add<gfx::animation_property::rotation>(rot_ch);

            clip->add_track(track);
        }

        {
            auto track       = gfx::animation_track("blue", 120.0f);

            auto pos_ch = gfx::make_animation_channel<gfx::animation_property::position>();
            pos_ch.add({0.0f, vec3f{4.0f, 0.0f, 0.0f}});
            track.add<gfx::animation_property::position>(pos_ch);

            auto org_ch = gfx::make_animation_channel<gfx::animation_property::origin>();
            org_ch.add({0.0f, vec3f{-1.5f, -1.5f, -1.5f}});
            track.add<gfx::animation_property::origin>(org_ch);

            auto rot_ch = gfx::make_animation_channel<gfx::animation_property::rotation>();
            rot_ch.add({0.0f, vec3f{0.0f, 0.0f, 0.0f}});
            rot_ch.add({1.0f, vec3f{0.0f, 0.0f, math::radians(180.0f)}});
            track.add<gfx::animation_property::rotation>(rot_ch);

            clip->add_track(track);
        }
    }

    void create_wave_animation() {
        auto& world         = get_engine().get_world();
        auto& clip_registry = world.get_animation_clip_registry();
        auto clip           = clip_registry.create("wave");

        {
            auto track       = gfx::animation_track("red", 120.0f);

            auto pos_ch = gfx::make_animation_channel<gfx::animation_property::position>();
            pos_ch.add({0.0f, vec3f{-4.0f, 0.0f, 0.0f}});
            pos_ch.add({0.33f, vec3f{-4.0f, 2.0f, 0.0f}});
            pos_ch.add({0.66f, vec3f{-4.0f, 0.0f, 0.0f}});
            pos_ch.add({1.0f, vec3f{-4.0f, 0.0f, 0.0f}});
            track.add<gfx::animation_property::position>(pos_ch);

            auto org_ch = gfx::make_animation_channel<gfx::animation_property::origin>();
            org_ch.add({0.0f, vec3f{-1.5f, -1.5f, -1.5f}});
            track.add<gfx::animation_property::origin>(org_ch);

            clip->add_track(track);
        }

        {
            auto track       = gfx::animation_track("green", 120.0f);

            auto pos_ch = gfx::make_animation_channel<gfx::animation_property::position>();
            pos_ch.add({0.0f, vec3f{0.0f, 0.0f, 0.0f}});
            pos_ch.add({0.33f, vec3f{0.0f, 0.0f, 0.0f}});
            pos_ch.add({0.66f, vec3f{0.0f, 2.0f, 0.0f}});
            pos_ch.add({1.0f, vec3f{0.0f, 0.0f, 0.0f}});
            track.add<gfx::animation_property::position>(pos_ch);

            auto org_ch = gfx::make_animation_channel<gfx::animation_property::origin>();
            org_ch.add({0.0f, vec3f{-1.5f, -1.5f, -1.5f}});
            track.add<gfx::animation_property::origin>(org_ch);

            clip->add_track(track);
        }

        {
            auto track       = gfx::animation_track("blue", 120.0f);

            auto pos_ch = gfx::make_animation_channel<gfx::animation_property::position>();
            pos_ch.add({0.0f, vec3f{4.0f, 0.0f, 0.0f}});
            pos_ch.add({0.33f, vec3f{4.0f, 0.0f, 0.0f}});
            pos_ch.add({0.66f, vec3f{4.0f, 0.0f, 0.0f}});
            pos_ch.add({1.0f, vec3f{4.0f, 2.0f, 0.0f}});
            pos_ch.add({1.33f, vec3f{4.0f, 0.0f, 0.0f}});
            track.add<gfx::animation_property::position>(pos_ch);

            auto org_ch = gfx::make_animation_channel<gfx::animation_property::origin>();
            org_ch.add({0.0f, vec3f{-1.5f, -1.5f, -1.5f}});
            track.add<gfx::animation_property::origin>(org_ch);

            clip->add_track(track);
        }
    }

    void create_scale_animation() {
        auto& world         = get_engine().get_world();
        auto& clip_registry = world.get_animation_clip_registry();
        auto clip           = clip_registry.create("scale");

        {
            auto track         = gfx::animation_track("red", 120.0f);

            auto scl_ch = gfx::make_animation_channel<gfx::animation_property::scale>();
            scl_ch.add({0.0f, vec3f{1.0f, 1.0f, 1.0f}});
            scl_ch.add({0.5f, vec3f{1.5f, 1.5f, 1.5f}});
            scl_ch.add({1.0f, vec3f{1.0f, 1.0f, 1.0f}});
            track.add<gfx::animation_property::scale>(scl_ch);

            auto pos_ch = gfx::make_animation_channel<gfx::animation_property::position>();
            pos_ch.add({0.0f, vec3f{-4.0f, 0.0f, 0.0f}});
            track.add<gfx::animation_property::position>(pos_ch);

            auto org_ch = gfx::make_animation_channel<gfx::animation_property::origin>();
            org_ch.add({0.0f, vec3f{-1.5f, -1.5f, -1.5f}});
            track.add<gfx::animation_property::origin>(org_ch);

            clip->add_track(track);
        }

        {
            auto track         = gfx::animation_track("green", 120.0f);

            auto scl_ch = gfx::make_animation_channel<gfx::animation_property::scale>();
            scl_ch.add({0.0f, vec3f{1.0f, 1.0f, 1.0f}});
            scl_ch.add({0.5f, vec3f{0.5f, 0.5f, 0.5f}});
            scl_ch.add({1.0f, vec3f{1.0f, 1.0f, 1.0f}});
            track.add<gfx::animation_property::scale>(scl_ch);

            auto pos_ch   = gfx::make_animation_channel<gfx::animation_property::position>();
            pos_ch.add({0.0f, vec3f{0.0f, 0.0f, 0.0f}});
            track.add<gfx::animation_property::position>(pos_ch);

            auto org_ch = gfx::make_animation_channel<gfx::animation_property::origin>();
            org_ch.add({0.0f, vec3f{-1.5f, -1.5f, -1.5f}});
            track.add<gfx::animation_property::origin>(org_ch);

            clip->add_track(track);
        }

        {
            auto track         = gfx::animation_track("blue", 120.0f);

            auto scl_ch = gfx::make_animation_channel<gfx::animation_property::scale>();
            scl_ch.add({0.0f, vec3f{1.0f, 1.0f, 1.0f}});
            scl_ch.add({0.5f, vec3f{1.2f, 0.8f, 1.2f}});
            scl_ch.add({1.0f, vec3f{1.0f, 1.0f, 1.0f}});
            track.add<gfx::animation_property::scale>(scl_ch);

            auto pos_ch   = gfx::make_animation_channel<gfx::animation_property::position>();
            pos_ch.add({0.0f, vec3f{4.0f, 0.0f, 0.0f}});
            track.add<gfx::animation_property::position>(pos_ch);

            auto org_ch = gfx::make_animation_channel<gfx::animation_property::origin>();
            org_ch.add({0.0f, vec3f{-1.5f, -1.5f, -1.5f}});
            track.add<gfx::animation_property::origin>(org_ch);

            clip->add_track(track);
        }
    }

    void handle_key_press(
        const gfx::key_press_event& ev
    ) {
        using keys = gfx::keyboard::keys;

        if (ev.key == keys::ESCAPE) {
            get_engine().shutdown();
        } else if (ev.key == keys::F1) {
            camera_controller_->toggle_mouse_captured();
            camera_controller_->toggle_keyboard_control_enabled();
        }
    }

    void render_ui() {
        ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImVec2 window_pos       = ImVec2(viewport->WorkPos.x + 10, viewport->WorkPos.y + 10);
        ImGui::SetNextWindowPos(window_pos, ImGuiCond_Always);

        ImGuiWindowFlags window_flags =          //
            ImGuiWindowFlags_NoCollapse |        //
            ImGuiWindowFlags_NoResize |          //
            ImGuiWindowFlags_AlwaysAutoResize |  //
            ImGuiWindowFlags_NoSavedSettings;

        ImGui::Begin("Animation Test", nullptr, window_flags);

        ImGui::Text("Controls:");
        ImGui::Text("WASD - move camera");
        ImGui::Text("Mouse - rotate camera");
        ImGui::Text("F1 - toggle cursor mode");
        ImGui::Text("ESC - exit");

        ImGui::Separator();

        auto& world            = get_engine().get_world();
        auto& animation_system = world.get_animation_system();
        auto modifier          = animation_system.modify_player(root_->get_entity());
        auto& animation_comp =
            world.get_component<gfx::animation_player_component>(root_->get_entity());

        ImGui::Text("Animation: ");

        if (ImGui::Button("Bounce")) {
            modifier.blend_to_by_name("bounce", 0.5f);
            current_animation_ = "bounce";
        }
        ImGui::SameLine();

        if (ImGui::Button("Rotation")) {
            modifier.blend_to_by_name("rotation", 0.5f);
            current_animation_ = "rotation";
        }
        ImGui::SameLine();

        if (ImGui::Button("Wave")) {
            modifier.blend_to_by_name("wave", 0.5f);
            current_animation_ = "wave";
        }
        ImGui::SameLine();

        if (ImGui::Button("Scale")) {
            modifier.blend_to_by_name("scale", 0.5f);
            current_animation_ = "scale";
        }

        ImGui::Separator();

        ImGui::Text("Loop Mode:");
        if (ImGui::Button("Once")) {
            modifier.set_loop_mode(gfx::animation_loop_mode::once);
        }
        ImGui::SameLine();
        if (ImGui::Button("Loop")) {
            modifier.set_loop_mode(gfx::animation_loop_mode::loop);
        }
        ImGui::SameLine();
        if (ImGui::Button("Ping Pong")) {
            modifier.set_loop_mode(gfx::animation_loop_mode::ping_pong);
        }

        ImGui::Separator();

        ImGui::Text("Control:");
        if (ImGui::Button("Play")) {
            modifier.play();
        }
        ImGui::SameLine();
        if (ImGui::Button("Pause")) {
            modifier.pause();
        }
        ImGui::SameLine();
        if (ImGui::Button("Resume")) {
            modifier.resume();
        }
        ImGui::SameLine();
        if (ImGui::Button("Stop")) {
            modifier.stop();
        }

        ImGui::Separator();

        float32 speed = animation_comp.get_playback_speed();

        if (ImGui::SliderFloat("Speed", &speed, 0.1f, 3.0f)) {
            modifier.set_playback_speed(speed);
        }

        float32 fps = animation_system.get_target_fps();
        if (ImGui::SliderFloat("Target FPS", &fps, 10.0f, 120.0f)) {
            animation_system.set_target_fps(fps);
        }

        ImGui::Separator();
        ImGui::Text("Current: %s", current_animation_.c_str());

        if (world.has_component<gfx::animation_player_component>(root_->get_entity())) {
            auto& comp = world.get_component<gfx::animation_player_component>(root_->get_entity());

            ImGui::Separator();
            ImGui::Text("Animation State:");

            const char* state_str = "Unknown";
            if (comp.is_playing()) {
                state_str = "Playing";
            } else if (comp.is_paused()) {
                state_str = "Paused";
            } else if (comp.is_stopped()) {
                state_str = "Stopped";
            }
            ImGui::Text("State: %s", state_str);

            const char* loop_str = "Unknown";
            auto loop_mode       = comp.get_loop_mode();
            if (loop_mode == gfx::animation_loop_mode::once) {
                loop_str = "Once";
            } else if (loop_mode == gfx::animation_loop_mode::loop) {
                loop_str = "Loop";
            } else if (loop_mode == gfx::animation_loop_mode::ping_pong) {
                loop_str = "Ping Pong";
            }
            ImGui::Text("Loop Mode: %s", loop_str);

            float32 current_time = comp.get_current_time();
            float32 duration     = comp.get_duration();
            ImGui::Text("Time: %.2f / %.2f s", current_time, duration);
            ImGui::ProgressBar(duration > 0.0f ? current_time / duration : 0.0f);
        }

        ImGui::End();
    }

    std::unique_ptr<gfx::fps_camera_controller> camera_controller_;
    std::unique_ptr<gfx::entity_guard<>> root_;
    std::unique_ptr<gfx::entity_guard<>> red_;
    std::unique_ptr<gfx::entity_guard<>> green_;
    std::unique_ptr<gfx::entity_guard<>> blue_;
    std::string current_animation_ = "bounce";
};

auto main() -> int32 {
    try {
        gfx::engine<> engine{1280, 720, "Test Animation"};
        engine.run<test_animation_app>();
    } catch (const std::exception& e) {
        log::error("Fatal error: {}", e.what());
        return 1;
    }
    return 0;
}
