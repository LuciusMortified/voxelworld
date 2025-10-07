#include <iostream>

#include <vw/core.h>
#include <vw/gfx.h>

using namespace vw;

class simple_model_app : public gfx::app {
public:
    void setup() override {
        auto& window = get_engine().get_window();
        auto& camera = get_engine().get_camera();

        camera_controller_ =
            std::make_unique<gfx::fps_camera_controller>(0.1f, 5.0f);
        camera_controller_->setup(window, camera);

        window.sub<gfx::key_press_event>(
            [this](const gfx::key_press_event& event) {
                handle_key_press(event.key);
                return true;
            }
        );

        window.sub<gfx::window_close_event>([this](gfx::window_close_event&) {
            get_engine().shutdown();
            return true;
        });

        camera.set_position({-5.0f, 1.5f, 0.0f});
        camera.set_rotation(vw::math::radians(0.0f), 0.0f);
        get_engine().get_renderer().set_clear_color(0.1f, 0.2f, 0.3f, 1.0f);
        get_engine().get_debug_tool().set_visible(true);

        create_flower_model();
    }

    void update(float delta_time) override {
        camera_controller_->update(delta_time);
        update_object_rotation(delta_time);
    }

    void render() override {
        ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImVec2 window_pos =
            ImVec2(viewport->WorkPos.x + 10, viewport->WorkPos.y + 10);
        ImGui::SetNextWindowPos(
            window_pos,
            ImGuiCond_Always,
            ImVec2(0.0f, 0.0f)
        );

        // clang-format off
        ImGuiWindowFlags window_flags =
            ImGuiWindowFlags_NoCollapse |
            ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoScrollbar |
            ImGuiWindowFlags_AlwaysAutoResize |
            ImGuiWindowFlags_NoSavedSettings |
            ImGuiWindowFlags_NoNav |
            ImGuiWindowFlags_NoFocusOnAppearing;
        // clang-format on

        ImGui::Begin("Test Simple Model", nullptr, window_flags);
        ImGui::Text("Controls:");
        ImGui::Text("WASD - move camera");
        ImGui::Text("Mouse - rotate camera");
        ImGui::Text("F1 - toggle cursor mode");
        ImGui::Text("Ctrl+F12 - toggle debug tool");
        ImGui::Text("ESC - exit");

        ImGui::Separator();

        ImGui::Text("Object:");
        ImGui::Text("Rotation speed:");
        ImGui::SliderFloat("##", &object_rotation_speed_, 0.0f, 20.0f);
        ImGui::Text(
            "Current angle: %.2f degrees",
            vw::math::degrees(object_rotation_)
        );
        ImGui::End();

        get_engine().get_renderer().draw_grid(vec3f{1, 0, 1}, 1.f, 3);

        get_engine().get_renderer().draw_box(vec3f{1, 0, 1}, vec3f{1.f});
    }

private:
    void create_flower_model() {
        model_ = std::make_shared<gfx::model>(3, 6, 3);
        model_->set_voxel(1, 0, 1, colors::green);
        model_->set_voxel(1, 1, 1, colors::green);
        model_->set_voxel(1, 2, 1, colors::green);
        model_->set_voxel(1, 3, 1, colors::green);
        model_->set_voxel(1, 4, 1, colors::green);
        model_->set_voxel(1, 5, 1, colors::yellow);
        model_->set_voxel(1, 4, 0, colors::white);
        model_->set_voxel(1, 4, 1, colors::white);
        model_->set_voxel(1, 4, 2, colors::white);
        model_->set_voxel(0, 4, 1, colors::white);
        model_->set_voxel(2, 4, 1, colors::white);

        object_id_ =
            get_engine().get_world().add_object(model_, {0.0f, 0.0f, 0.0f});

        object_rotation_       = 0.0f;
        object_rotation_speed_ = vw::math::radians(0.0f);
    }

    void update_object_rotation(float delta_time) {
        object_rotation_ += object_rotation_speed_ * delta_time;
        get_engine().get_world().set_object_rotation(
            object_id_,
            {0.0f, object_rotation_, 0.0f}
        );
    }

    void handle_key_press(gfx::keyboard::key key) const {
        switch (key) {
            case gfx::keyboard::key::ESCAPE:
                get_engine().shutdown();
                break;
            case gfx::keyboard::key::F1:
                camera_controller_->toggle_mouse_captured();
                break;
            default:
                break;
        }
    }

    std::unique_ptr<gfx::fps_camera_controller> camera_controller_;

    std::shared_ptr<gfx::model> model_;
    gfx::object_id object_id_ = 0;
    float object_rotation_{};
    float object_rotation_speed_{};
};

int main() {
    try {
        const auto instance = std::make_shared<gfx::engine>(
            1280,
            720,
            "Voxel World - Test Simple Model"
        );
        instance->run(std::make_unique<simple_model_app>());
    } catch (const std::exception& e) {
        std::cerr << "Ошибка: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}