#include <vw/core.h>
#include <vw/gfx.h>

using namespace vw;

class simple_model_app final : public gfx::app<> {
public:
    void setup() override {
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

        window.sub<gfx::mouse_press_event>([this](gfx::mouse_press_event& ev) {
            handle_mouse_press(ev);
            return true;
        });

        camera.set_position({0.0f, 0.0f, -5.0f});
        camera.set_rotation(0.0f, 0.0f);
        get_engine().get_renderer().set_clear_color(0.1f, 0.2f, 0.3f, 1.0f);
        get_engine().get_debug_tool().set_visible(true);

        create_flower_model();
    }

    void cleanup() override {
        auto& world = get_engine().get_world();
        world.destroy_entity(some_entity_);
    }

    void render(
        float delta_time
    ) override {
        camera_controller_->update(delta_time);
        update_object_rotation(delta_time);

        get_engine().get_renderer().draw_line(vec3f{0, 0, 0}, vec3f{100, 0, 0}, colors::blue);
        get_engine().get_renderer().draw_line(vec3f{0, 0, 0}, vec3f{0, 100, 0}, colors::green);
        get_engine().get_renderer().draw_line(vec3f{0, 0, 0}, vec3f{0, 0, 100}, colors::red);

        ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImVec2 window_pos       = ImVec2(viewport->WorkPos.x + 10, viewport->WorkPos.y + 10);
        ImGui::SetNextWindowPos(window_pos, ImGuiCond_Always, ImVec2(0.0f, 0.0f));

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
        ImGui::Text("Current angle: %.2f degrees", vw::math::degrees(object_rotation_));

        ImGui::End();
    }

private:
    void create_flower_model() {
        auto& world            = get_engine().get_world();
        auto& model_registry   = world.get_model_registry();
        auto& transform_system = world.get_transform_system();
        auto& model_system     = world.get_model_system();

        // Создаем модель и регистрируем ее
        model_ = model_registry.create("flower", 3, 6, 3);
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

        // Создаем сущность с моделью
        some_entity_ = world.create_entity();
        world.add_component<gfx::transform_component>(some_entity_);
        world.add_component<gfx::model_component>(some_entity_);
        world.add_component<gfx::hierarchy_component>(some_entity_);
        world.add_component<gfx::spatial_component>(some_entity_);

        // Настраиваем transform
        transform_system.modify(some_entity_)
            .set_origin({1.5f, 3.0f, 1.5f})
            .set_position({-1.5f, 0.0f, -1.5f});

        // Настраиваем модель через model_system
        model_system.modify(some_entity_).set_model(model_);

        object_rotation_       = 0.0f;
        object_rotation_speed_ = math::radians(5.0f);

// Стресс-тест инстансами одной модели
#if 1
        for (int i = 0; i < 9999; i++) {
            auto another_entity_ = world.create_entity();
            world.add_component<gfx::transform_component>(another_entity_);
            world.add_component<gfx::model_component>(another_entity_);
            world.add_component<gfx::spatial_component>(another_entity_);

            transform_system.modify(another_entity_)
                .set_position({
                    static_cast<float>(std::rand() % 100),
                    static_cast<float>(std::rand() % 100),
                    static_cast<float>(std::rand() % 100),
                })
                .set_origin({1.5f, 3.0f, 1.5f});

            model_system.modify(another_entity_).set_model(model_);
        }
#endif
    }

    void update_object_rotation(
        float delta_time
    ) {
        object_rotation_ += object_rotation_speed_ * delta_time;

        if (object_rotation_ > math::radians(360.0f)) {
            object_rotation_ = math::radians(0.0f);
        }

        auto& world            = get_engine().get_world();
        auto& transform_system = world.get_transform_system();
        transform_system.modify(some_entity_).set_rotation({0.0f, object_rotation_, 0.0f});

        if (world.has_component<gfx::transform_component>(some_entity_)) {
            const auto matrix =
                world.get_component<gfx::transform_component>(some_entity_).get_world_matrix();
            get_engine().get_renderer().draw_grid(matrix, 1.f, 3, 3, colors::white);
        }

#if 0
        auto transform_view = world.view_components<gfx::transform_component>();
        for (const auto& [entity, transform_comp] : transform_view) {
            transform_system.modify(entity).set_rotation({0.0f, object_rotation_, 0.0f});
        }
#endif
    }

    void handle_key_press(
        gfx::keyboard::key key
    ) const {
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

    void handle_mouse_press(
        const gfx::mouse_press_event& event
    ) const {
        if (event.button == gfx::mouse::button::LEFT) {
            const auto& world = get_engine().get_world();
        }
    }

    std::unique_ptr<gfx::fps_camera_controller> camera_controller_;

    std::shared_ptr<gfx::model> model_;
    float object_rotation_{};
    float object_rotation_speed_{};

    gfx::entity some_entity_;

    std::unordered_set<gfx::entity> selected_entities_;
};

int main() {
    try {
        std::make_unique<gfx::engine<>>(1280, 720, "Voxel World - Test Simple Model")
            ->run(std::make_unique<simple_model_app>());
    } catch (const std::exception& e) {
        vw::log::error("Ошибка: {}", e.what());
        return 1;
    }

    return 0;
}