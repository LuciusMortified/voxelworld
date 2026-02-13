#include <vw/core.h>
#include <vw/gfx.h>

#include "vw/gfx/world/entity_guard.h"
#include "vw/gfx/world/entity_guard_group.h"

using namespace vw;

class simple_model_app final : public gfx::app<> {
public:
    explicit simple_model_app(
        gfx::engine<>& eng
    )
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

    ~simple_model_app() override = default;

    void render(
        float delta_time
    ) override {
        camera_controller_->update(delta_time);
        update_object_rotation(delta_time);

        auto& renderer = get_engine().get_renderer();

        // Отрисовка осей координат
        renderer.draw_line(vec3f{0, 0, 0}, vec3f{100, 0, 0}, colors::blue);
        renderer.draw_line(vec3f{0, 0, 0}, vec3f{0, 100, 0}, colors::green);
        renderer.draw_line(vec3f{0, 0, 0}, vec3f{0, 0, 100}, colors::red);

        render_selected_voxel();

        ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImVec2 window_pos       = ImVec2(viewport->WorkPos.x + 10, viewport->WorkPos.y + 10);
        ImGui::SetNextWindowPos(window_pos, ImGuiCond_Always, ImVec2(0.0f, 0.0f));

        ImGuiWindowFlags window_flags =          //
            ImGuiWindowFlags_NoCollapse |        //
            ImGuiWindowFlags_NoResize |          //
            ImGuiWindowFlags_NoScrollbar |       //
            ImGuiWindowFlags_AlwaysAutoResize |  //
            ImGuiWindowFlags_NoSavedSettings |   //
            ImGuiWindowFlags_NoNav |             //
            ImGuiWindowFlags_NoFocusOnAppearing;

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
        ImGui::Text("Current angle: %.2f degrees", math::degrees(object_rotation_));
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

        flower_ = std::make_unique<gfx::entity_guard<>>(world);
        flower_->with<gfx::transform_component>();
        flower_->with<gfx::model_component>();
        flower_->with<gfx::hierarchy_component>();
        flower_->with<gfx::spatial_component>();

        // Настраиваем transform
        transform_system.modify(flower_->get_entity())
            .set_origin({-1.5f, 0.f, -1.5f})
            .set_position({0.f, 0.0f, 0.f})
            .set_scale({0.5f, 0.5f, 0.5f});

        // Настраиваем модель через model_system
        model_system.modify(flower_->get_entity()).set_model(model_);

        object_rotation_       = 0.0f;
        object_rotation_speed_ = math::radians(5.0f);

// Стресс-тест инстансами одной модели
#if 0
        stress_group_ = std::make_unique<gfx::entity_guard_group<>>(world, 9999);
        stress_group_->with<gfx::transform_component>();
        stress_group_->with<gfx::model_component>();
        stress_group_->with<gfx::spatial_component>();

        for (auto ent : *stress_group_) {
            transform_system.modify(ent)
                .set_position({
                    static_cast<float>(std::rand() % 100),
                    static_cast<float>(std::rand() % 100),
                    static_cast<float>(std::rand() % 100),
                })
                .set_origin({1.5f, 3.0f, 1.5f});

            model_system.modify(ent).set_model(model_);
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
        transform_system.modify(flower_->get_entity()).set_rotation({0.0f, object_rotation_, 0.0f});

        if (world.has_component<gfx::transform_component>(flower_->get_entity())) {
            const auto matrix =
                world.get_component<gfx::transform_component>(flower_->get_entity())
                    .get_world_matrix();
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
        gfx::keyboard::keys key
    ) const {
        switch (key) {
            case gfx::keyboard::keys::ESCAPE:
                get_engine().shutdown();
                break;
            case gfx::keyboard::keys::F1:
                camera_controller_->toggle_mouse_captured();
                camera_controller_->toggle_keyboard_control_enabled();
                break;
            default:
                break;
        }
    }

    void handle_mouse_press(
        const gfx::mouse_press_event& event
    ) {
        if (event.button == gfx::mouse::buttons::LEFT) {
            const auto& world  = get_engine().get_world();
            const auto& window = get_engine().get_window();
            const auto& camera = get_engine().get_camera();

            auto ray    = camera.screen_to_world_ray(window.get_cursor_pos(), window.get_size());
            select_ray_ = ray;

            auto voxel_hit = world.voxel_ray_cast(ray, selected_entities_);
            if (voxel_hit.has_value()) {
                selected_entity_    = voxel_hit->ent;
                selected_voxel_     = voxel_hit->voxel_pos;
                selected_empty_pos_ = voxel_hit->empty_pos;
                log::info(
                    "Selected entity: {}.{}, voxel: ({}, {}, {}), empty: ({}, {}, {})",
                    selected_entity_.index,
                    selected_entity_.generation,
                    selected_voxel_.x,
                    selected_voxel_.y,
                    selected_voxel_.z,
                    selected_empty_pos_.x,
                    selected_empty_pos_.y,
                    selected_empty_pos_.z
                );
            } else {
                selected_entity_    = gfx::invalid_entity;
                selected_voxel_     = vec3i{-1, -1, -1};
                selected_empty_pos_ = vec3i{-1, -1, -1};
                log::info("No voxel hit");
            }
        }
    }

    void render_selected_voxel() {
        auto& world = get_engine().get_world();

        const bool can_be_rendered =  //
            selected_entity_.is_valid() &&
            world.has_component<gfx::transform_component>(selected_entity_);
        if (!can_be_rendered) {
            return;
        }

        auto& renderer = get_engine().get_renderer();
        const auto& transform_matrix =
            world.get_component<gfx::transform_component>(selected_entity_).get_world_matrix();

        auto voxel_local_pos = vec3f{
            static_cast<float>(selected_voxel_.x),
            static_cast<float>(selected_voxel_.y),
            static_cast<float>(selected_voxel_.z)
        };
        auto voxel_world_pos =  //
            transform_matrix * math::translation_matrix(voxel_local_pos) *
            math::scale_matrix(vec3f{1.1f, 1.1f, 1.1f}) *
            math::translation_matrix(vec3f{-0.05f, -0.05f, -0.05f});
        renderer.draw_box(voxel_world_pos, vec3f{1.0f, 1.0f, 1.0f}, colors::black);

        if (selected_empty_pos_.x >= -1 && selected_empty_pos_.y >= -1 &&
            selected_empty_pos_.z >= -1) {
            auto empty_local_pos = vec3f{
                static_cast<float>(selected_empty_pos_.x),
                static_cast<float>(selected_empty_pos_.y),
                static_cast<float>(selected_empty_pos_.z)
            };
            auto empty_world_pos =  //
                transform_matrix * math::translation_matrix(empty_local_pos) *
                math::scale_matrix(vec3f{1.1f, 1.1f, 1.1f}) *
                math::translation_matrix(vec3f{-0.05f, -0.05f, -0.05f});
            renderer.draw_box(empty_world_pos, vec3f{1.0f, 1.0f, 1.0f}, colors::yellow);
        }
    }

    std::unique_ptr<gfx::fps_camera_controller> camera_controller_;

    std::shared_ptr<gfx::model> model_;
    float object_rotation_{};
    float object_rotation_speed_{};

    std::unique_ptr<gfx::entity_guard<>> flower_;
    std::unique_ptr<gfx::entity_guard_group<>> stress_group_;

    std::unordered_set<gfx::entity> selected_entities_;
    gfx::entity selected_entity_ = gfx::invalid_entity;
    vec3i selected_voxel_{-1, -1, -1};
    vec3i selected_empty_pos_{-1, -1, -1};
    gfx::ray select_ray_{vec3f{}, vec3f{}};
};

int main() {
    try {
        std::make_unique<gfx::engine<>>(1280, 720, "Voxel World - Test Simple Model")
            ->run<simple_model_app>();
    } catch (const std::exception& e) {
        log::error("Ошибка исполнения: {}", e.what());
        return 1;
    }

    return 0;
}