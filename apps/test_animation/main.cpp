#include <vw/core.h>
#include <vw/gfx.h>

using namespace vw;

class test_animation_app final : public gfx::app<> {
public:
    explicit test_animation_app(gfx::engine<>& eng)
        : app{eng} {
        auto& window = get_engine().get_window();
        auto& camera = get_engine().get_camera();

        camera_controller_ = std::make_unique<gfx::fps_camera_controller>(0.1f, 5.0f);
        camera_controller_->setup(window, camera);

        window.sub<gfx::key_press_event>([this](const gfx::key_press_event& event) {
            handle_key_press(event.key);
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

    void render(float32 delta_time) override {
        camera_controller_->update(delta_time);

        auto& renderer = get_engine().get_renderer();

        renderer.draw_line(vec3f{0, 0, 0}, vec3f{5, 0, 0}, colors::red);
        renderer.draw_line(vec3f{0, 0, 0}, vec3f{0, 5, 0}, colors::green);
        renderer.draw_line(vec3f{0, 0, 0}, vec3f{0, 0, 5}, colors::blue);

        render_ui();
    }

private:
    void setup_scene() {
        auto& world = get_engine().get_world();
        auto& model_registry = world.get_model_registry();
        auto& registry = world.get_registry();
        auto& animation_system = world.get_animation_system();

        auto red_cube = model_registry.create("red_cube", 3, 3, 3);
        red_cube->fill(voxel{colors::red});

        auto green_cube = model_registry.create("green_cube", 3, 3, 3);
        green_cube->fill(voxel{colors::green});

        auto blue_cube = model_registry.create("blue_cube", 3, 3, 3);
        blue_cube->fill(voxel{colors::blue});

        root_entity_ = registry.create();
        registry.add<gfx::transform_component>(root_entity_);
        registry.add<gfx::hierarchy_component>(root_entity_);
        registry.add<gfx::animation_player_component>(root_entity_);
        registry.add<gfx::animation_target_component>(root_entity_);
        animation_system.modify_target(root_entity_).set_target_name("root");

        auto red_ent = registry.create();
        registry.add<gfx::transform_component>(red_ent).set_position({-4.0f, 0.0f, 0.0f});
        registry.add<gfx::model_component>(red_ent).set_model_name("red_cube");
        registry.add<gfx::animation_target_component>(red_ent);
        animation_system.modify_target(red_ent).set_target_name("red");
        world.get_hierarchy_system().add_child(root_entity_, red_ent);

        auto green_ent = registry.create();
        registry.add<gfx::transform_component>(green_ent).set_position({0.0f, 0.0f, 0.0f});
        registry.add<gfx::model_component>(green_ent).set_model_name("green_cube");
        registry.add<gfx::animation_target_component>(green_ent);
        animation_system.modify_target(green_ent).set_target_name("green");
        world.get_hierarchy_system().add_child(root_entity_, green_ent);

        auto blue_ent = registry.create();
        registry.add<gfx::transform_component>(blue_ent).set_position({4.0f, 0.0f, 0.0f});
        registry.add<gfx::model_component>(blue_ent).set_model_name("blue_cube");
        registry.add<gfx::animation_target_component>(blue_ent);
        animation_system.modify_target(blue_ent).set_target_name("blue");
        world.get_hierarchy_system().add_child(root_entity_, blue_ent);
    }

    void create_animations() {
        auto& world = get_engine().get_world();
        auto& clip_registry = world.get_animation_clip_registry();

        create_bounce_animation(clip_registry);
        create_rotation_animation(clip_registry);
        create_wave_animation(clip_registry);
        create_scale_animation(clip_registry);
    }

    void create_bounce_animation(gfx::animation_clip_registry& registry) {
        auto clip = std::make_shared<gfx::animation_clip>("bounce");

        auto red_track = gfx::animation_track("red", 30.0f);
        auto red_pos_channel = gfx::make_animation_channel<gfx::animation_property::position>();
        red_pos_channel.add_keyframe({0.0f, vec3f{-4.0f, 0.0f, 0.0f}});
        red_pos_channel.add_keyframe({0.5f, vec3f{-4.0f, 3.0f, 0.0f}});
        red_pos_channel.add_keyframe({1.0f, vec3f{-4.0f, 0.0f, 0.0f}});
        red_track.add<gfx::animation_property::position>(red_pos_channel);
        clip->add_track(red_track);

        auto green_track = gfx::animation_track("green", 30.0f);
        auto green_pos_channel = gfx::make_animation_channel<gfx::animation_property::position>();
        green_pos_channel.add_keyframe({0.0f, vec3f{0.0f, 0.0f, 0.0f}});
        green_pos_channel.add_keyframe({0.5f, vec3f{0.0f, 3.0f, 0.0f}});
        green_pos_channel.add_keyframe({1.0f, vec3f{0.0f, 0.0f, 0.0f}});
        green_track.add<gfx::animation_property::position>(green_pos_channel);
        clip->add_track(green_track);

        auto blue_track = gfx::animation_track("blue", 30.0f);
        auto blue_pos_channel = gfx::make_animation_channel<gfx::animation_property::position>();
        blue_pos_channel.add_keyframe({0.0f, vec3f{4.0f, 0.0f, 0.0f}});
        blue_pos_channel.add_keyframe({0.5f, vec3f{4.0f, 3.0f, 0.0f}});
        blue_pos_channel.add_keyframe({1.0f, vec3f{4.0f, 0.0f, 0.0f}});
        blue_track.add<gfx::animation_property::position>(blue_pos_channel);
        clip->add_track(blue_track);

        registry.add("bounce", clip);
    }

    void create_rotation_animation(gfx::animation_clip_registry& registry) {
        auto clip = std::make_shared<gfx::animation_clip>("rotation");

        auto root_track = gfx::animation_track("root", 30.0f);
        auto rot_channel = gfx::make_animation_channel<gfx::animation_property::rotation>();
        rot_channel.add_keyframe({0.0f, vec3f{0.0f, 0.0f, 0.0f}});
        rot_channel.add_keyframe({1.0f, vec3f{0.0f, math::pi * 2.0f, 0.0f}});
        root_track.add<gfx::animation_property::rotation>(rot_channel);
        clip->add_track(root_track);

        registry.add("rotation", clip);
    }

    void create_wave_animation(gfx::animation_clip_registry& registry) {
        auto clip = std::make_shared<gfx::animation_clip>("wave");

        auto red_track = gfx::animation_track("red", 30.0f);
        auto red_pos_channel = gfx::make_animation_channel<gfx::animation_property::position>();
        red_pos_channel.add_keyframe({0.0f, vec3f{-4.0f, 0.0f, 0.0f}});
        red_pos_channel.add_keyframe({0.33f, vec3f{-4.0f, 2.0f, 0.0f}});
        red_pos_channel.add_keyframe({0.66f, vec3f{-4.0f, 0.0f, 0.0f}});
        red_pos_channel.add_keyframe({1.0f, vec3f{-4.0f, 0.0f, 0.0f}});
        red_track.add<gfx::animation_property::position>(red_pos_channel);
        clip->add_track(red_track);

        auto green_track = gfx::animation_track("green", 30.0f);
        auto green_pos_channel = gfx::make_animation_channel<gfx::animation_property::position>();
        green_pos_channel.add_keyframe({0.0f, vec3f{0.0f, 0.0f, 0.0f}});
        green_pos_channel.add_keyframe({0.33f, vec3f{0.0f, 0.0f, 0.0f}});
        green_pos_channel.add_keyframe({0.66f, vec3f{0.0f, 2.0f, 0.0f}});
        green_pos_channel.add_keyframe({1.0f, vec3f{0.0f, 0.0f, 0.0f}});
        green_track.add<gfx::animation_property::position>(green_pos_channel);
        clip->add_track(green_track);

        auto blue_track = gfx::animation_track("blue", 30.0f);
        auto blue_pos_channel = gfx::make_animation_channel<gfx::animation_property::position>();
        blue_pos_channel.add_keyframe({0.0f, vec3f{4.0f, 0.0f, 0.0f}});
        blue_pos_channel.add_keyframe({0.33f, vec3f{4.0f, 0.0f, 0.0f}});
        blue_pos_channel.add_keyframe({0.66f, vec3f{4.0f, 0.0f, 0.0f}});
        blue_pos_channel.add_keyframe({1.0f, vec3f{4.0f, 2.0f, 0.0f}});
        blue_track.add<gfx::animation_property::position>(blue_pos_channel);
        clip->add_track(blue_track);

        registry.add("wave", clip);
    }

    void create_scale_animation(gfx::animation_clip_registry& registry) {
        auto clip = std::make_shared<gfx::animation_clip>("scale");

        auto red_track = gfx::animation_track("red", 30.0f);
        auto red_scale_channel = gfx::make_animation_channel<gfx::animation_property::scale>();
        red_scale_channel.add_keyframe({0.0f, vec3f{1.0f, 1.0f, 1.0f}});
        red_scale_channel.add_keyframe({0.5f, vec3f{1.5f, 1.5f, 1.5f}});
        red_scale_channel.add_keyframe({1.0f, vec3f{1.0f, 1.0f, 1.0f}});
        red_track.add<gfx::animation_property::scale>(red_scale_channel);
        clip->add_track(red_track);

        auto green_track = gfx::animation_track("green", 30.0f);
        auto green_scale_channel = gfx::make_animation_channel<gfx::animation_property::scale>();
        green_scale_channel.add_keyframe({0.0f, vec3f{1.0f, 1.0f, 1.0f}});
        green_scale_channel.add_keyframe({0.5f, vec3f{0.5f, 0.5f, 0.5f}});
        green_scale_channel.add_keyframe({1.0f, vec3f{1.0f, 1.0f, 1.0f}});
        green_track.add<gfx::animation_property::scale>(green_scale_channel);
        clip->add_track(green_track);

        auto blue_track = gfx::animation_track("blue", 30.0f);
        auto blue_scale_channel = gfx::make_animation_channel<gfx::animation_property::scale>();
        blue_scale_channel.add_keyframe({0.0f, vec3f{1.0f, 1.0f, 1.0f}});
        blue_scale_channel.add_keyframe({0.5f, vec3f{1.2f, 0.8f, 1.2f}});
        blue_scale_channel.add_keyframe({1.0f, vec3f{1.0f, 1.0f, 1.0f}});
        blue_track.add<gfx::animation_property::scale>(blue_scale_channel);
        clip->add_track(blue_track);

        registry.add("scale", clip);
    }

    void handle_key_press(gfx::key key) {
        auto& window = get_engine().get_window();

        if (key == gfx::key::escape) {
            get_engine().shutdown();
        } else if (key == gfx::key::f1) {
            if (window.get_cursor_mode() == gfx::cursor_mode::normal) {
                window.set_cursor_mode(gfx::cursor_mode::disabled);
            } else {
                window.set_cursor_mode(gfx::cursor_mode::normal);
            }
        }
    }

    void render_ui() {
        ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImVec2 window_pos = ImVec2(viewport->WorkPos.x + 10, viewport->WorkPos.y + 10);
        ImGui::SetNextWindowPos(window_pos, ImGuiCond_Always, ImVec2(0.0f, 0.0f));

        ImGuiWindowFlags window_flags =
            ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings;

        ImGui::Begin("Animation Test", nullptr, window_flags);

        ImGui::Text("Controls:");
        ImGui::Text("WASD - move camera");
        ImGui::Text("Mouse - rotate camera");
        ImGui::Text("F1 - toggle cursor mode");
        ImGui::Text("ESC - exit");

        ImGui::Separator();
        ImGui::Text("Animations:");

        auto& world = get_engine().get_world();
        auto& animation_system = world.get_animation_system();
        auto modifier = animation_system.modify_player(root_entity_);

        if (ImGui::Button("Play Bounce")) {
            modifier.set_clip_by_name("bounce");
            modifier.set_loop_mode(gfx::animation_loop_mode::loop);
            modifier.play();
            current_animation_ = "bounce";
        }

        if (ImGui::Button("Play Rotation")) {
            if (current_animation_.empty()) {
                modifier.set_clip_by_name("rotation");
                modifier.set_loop_mode(gfx::animation_loop_mode::loop);
                modifier.play();
            } else {
                modifier.blend_to_by_name("rotation", 0.5f);
                modifier.set_loop_mode(gfx::animation_loop_mode::loop);
            }
            current_animation_ = "rotation";
        }

        if (ImGui::Button("Play Wave")) {
            if (current_animation_.empty()) {
                modifier.set_clip_by_name("wave");
                modifier.set_loop_mode(gfx::animation_loop_mode::loop);
                modifier.play();
            } else {
                modifier.blend_to_by_name("wave", 0.5f);
                modifier.set_loop_mode(gfx::animation_loop_mode::loop);
            }
            current_animation_ = "wave";
        }

        if (ImGui::Button("Play Scale")) {
            if (current_animation_.empty()) {
                modifier.set_clip_by_name("scale");
                modifier.set_loop_mode(gfx::animation_loop_mode::loop);
                modifier.play();
            } else {
                modifier.blend_to_by_name("scale", 0.5f);
                modifier.set_loop_mode(gfx::animation_loop_mode::loop);
            }
            current_animation_ = "scale";
        }

        ImGui::Separator();

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
            current_animation_.clear();
        }

        ImGui::Separator();

        float32 speed = 1.0f;
        auto& registry = world.get_registry();
        if (registry.has<gfx::animation_player_component>(root_entity_)) {
            auto& comp = registry.get<gfx::animation_player_component>(root_entity_);
            speed = comp.get_playback_speed();
        }

        if (ImGui::SliderFloat("Speed", &speed, 0.1f, 3.0f)) {
            modifier.set_playback_speed(speed);
        }

        float32 frame_time = animation_system.get_target_frame_time();
        float32 fps = 1.0f / frame_time;
        if (ImGui::SliderFloat("Target FPS", &fps, 10.0f, 120.0f)) {
            animation_system.set_target_frame_time(1.0f / fps);
        }

        ImGui::Separator();
        ImGui::Text("Current: %s", current_animation_.c_str());

        ImGui::End();
    }

    std::unique_ptr<gfx::fps_camera_controller> camera_controller_;
    gfx::entity root_entity_;
    std::string current_animation_;
};

auto main() -> int32 {
    try {
        gfx::engine engine{"Test Animation", 1280, 720};
        test_animation_app app{engine};
        engine.run();
    } catch (const std::exception& e) {
        log::error("Fatal error: {}", e.what());
        return 1;
    }
    return 0;
}
