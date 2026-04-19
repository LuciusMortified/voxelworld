#include <cmath>
#include <vector>

#include <vw/core.h>
#include <vw/gfx.h>

#include "vw/gfx/world/entity_guard.h"
#include "vw/gfx/world/entity_guard_group.h"

using namespace vw;

struct point_light_info {
    vec3f color;
    float32 radius;
    float32 height;
    float32 phase;
};

class lighting_app final : public gfx::app<> {
public:
    explicit lighting_app(gfx::engine<>& eng)
        : app{eng} {
        auto& window = get_engine().get_window();
        auto& camera = get_engine().get_camera();

        camera_controller_ = std::make_unique<gfx::free_camera_controller>(0.1f, 15.0f);
        camera_controller_->setup(window, camera);

        window.sub<gfx::key_press_event>([this](const gfx::key_press_event& event) {
            handle_key_press(event.key);
            return true;
        });

        window.sub<gfx::window_close_event>([this](gfx::window_close_event&) {
            get_engine().shutdown();
            return true;
        });

        camera.set_position({0.0f, 30.0f, -60.0f});
        camera.set_rotation(0.0f, math::radians(-20.0f));
        get_engine().get_renderer().set_clear_color(0.02f, 0.02f, 0.05f, 1.0f);
        get_engine().get_debug_tool().set_visible(true);

        setup_scene();
    }

    ~lighting_app() override = default;

    void render(float delta_time) override {
        camera_controller_->update(delta_time);
        elapsed_time_ += delta_time;
        orbit_angle_ += delta_time * orbit_speed_;
        day_night_phase_ += delta_time * day_night_speed_;

        sync_light_count();
        update_point_lights();
        update_directional_light();
        render_ui();
    }

private:
    void setup_scene() {
        auto& world = get_engine().get_world();
        auto& model_reg = world.template get_resource<asset::model_registry>();
        auto& transform_sys = world.template get_system<gfx::transform_system>();
        auto& model_sys = world.template get_system<gfx::model_system>();

        floor_model_ = model_reg.create("floor", 80, 1, 80);
        for (uint32 x = 0; x < 80; ++x) {
            for (uint32 z = 0; z < 80; ++z) {
                auto v = ((x + z) % 2 == 0) ? voxel{blocks::gray_3} : voxel{blocks::gray_5};
                floor_model_->set_voxel(x, 0, z, v);
            }
        }

        floor_entity_ = std::make_unique<gfx::entity_guard<gfx::base_world_def>>(world.get_context());
        floor_entity_->with<gfx::transform_component>();
        floor_entity_->with<gfx::model_component>();
        floor_entity_->with<gfx::hierarchy_component>();
        floor_entity_->with<gfx::spatial_component>();

        transform_sys.modify(floor_entity_->get_entity())
            .set_origin({-40.0f, -0.5f, -40.0f})
            .set_position({0.0f, 0.0f, 0.0f});

        model_sys.modify(floor_entity_->get_entity()).set_model(floor_model_);

        pillar_model_ = model_reg.create("pillar", 6, 20, 6);
        for (uint32 x = 0; x < 6; ++x) {
            for (uint32 y = 0; y < 20; ++y) {
                for (uint32 z = 0; z < 6; ++z) {
                    auto v = (y < 2 || y > 17) ? voxel{blocks::gray_3} : voxel{blocks::gray_5};
                    pillar_model_->set_voxel(x, y, z, v);
                }
            }
        }

        pillar_entity_ = std::make_unique<gfx::entity_guard<gfx::base_world_def>>(world.get_context());
        pillar_entity_->with<gfx::transform_component>();
        pillar_entity_->with<gfx::model_component>();
        pillar_entity_->with<gfx::hierarchy_component>();
        pillar_entity_->with<gfx::spatial_component>();

        transform_sys.modify(pillar_entity_->get_entity())
            .set_origin({-3.0f, 0.0f, -3.0f})
            .set_position({0.0f, 0.0f, 0.0f});

        model_sys.modify(pillar_entity_->get_entity()).set_model(pillar_model_);

        sync_light_count();
    }

    static auto generate_light_info(uint32 index, uint32 total) -> point_light_info {
        static constexpr vec3f palette[] = {
            {1.0f, 0.2f, 0.2f}, {0.2f, 1.0f, 0.2f}, {0.2f, 0.2f, 1.0f},
            {1.0f, 1.0f, 0.2f}, {1.0f, 0.2f, 1.0f}, {0.2f, 1.0f, 1.0f},
            {1.0f, 0.6f, 0.2f}, {1.0f, 1.0f, 1.0f},
        };

        constexpr float32 two_pi = 2.0f * 3.14159265f;
        float32 phase = static_cast<float32>(index) / static_cast<float32>(total) * two_pi;
        float32 radius = 16.0f + 10.0f * std::sin(phase * 3.0f + 0.5f);
        float32 height = 16.0f + 8.0f * std::cos(phase * 2.0f + 1.0f);
        const auto& base_color = palette[index % std::size(palette)];

        return {base_color, radius, height, phase};
    }

    void sync_light_count() {
        auto target = static_cast<uint32>(target_light_count_);
        auto current = static_cast<uint32>(light_entities_.size());
        if (target == current) {
            return;
        }

        auto& world = get_engine().get_world();
        auto& transform_sys = world.template get_system<gfx::transform_system>();
        auto& light_sys = world.template get_system<gfx::light_system>();

        if (target > current) {
            light_infos_.resize(target);
            for (uint32 i = current; i < target; ++i) {
                light_infos_[i] = generate_light_info(i, target);
            }
            for (uint32 i = 0; i < current; ++i) {
                light_infos_[i] = generate_light_info(i, target);
            }

            light_entities_.reserve(target);
            for (uint32 i = current; i < target; ++i) {
                auto& guard = light_entities_.emplace_back(
                    std::make_unique<gfx::entity_guard<gfx::base_world_def>>(world.get_context()));
                guard->with<gfx::transform_component>();
                guard->with<gfx::light_component>();
                guard->with<gfx::hierarchy_component>();

                auto ent = guard->get_entity();
                const auto& info = light_infos_[i];

                float32 angle = info.phase + orbit_angle_;
                float32 x = info.radius * std::cos(angle);
                float32 z = info.radius * std::sin(angle);
                transform_sys.modify(ent).set_position({x, info.height, z});

                light_sys.modify(ent)
                    .set_color(info.color)
                    .set_range(20.0f);
            }
        } else {
            light_entities_.resize(target);
            light_infos_.resize(target);
            for (uint32 i = 0; i < target; ++i) {
                light_infos_[i] = generate_light_info(i, target);
            }
        }
    }

    void update_point_lights() {
        auto& world = get_engine().get_world();
        auto& transform_sys = world.template get_system<gfx::transform_system>();
        auto& renderer = get_engine().get_renderer();

        for (uint32 i = 0; i < light_entities_.size(); ++i) {
            auto ent = light_entities_[i]->get_entity();
            const auto& info = light_infos_[i];

            float32 angle = info.phase + orbit_angle_;
            float32 x = info.radius * std::cos(angle);
            float32 z = info.radius * std::sin(angle);
            float32 y = info.height + 2.0f * std::sin(elapsed_time_ * 0.5f + info.phase);

            transform_sys.modify(ent).set_position({x, y, z});

            auto line_color = color(
                static_cast<uint8>(info.color.x * 255.0f),
                static_cast<uint8>(info.color.y * 255.0f),
                static_cast<uint8>(info.color.z * 255.0f));
            renderer.draw_line(vec3f{x, y, z}, vec3f{x, 0.0f, z}, line_color);
        }
    }

    void update_directional_light() {
        auto& renderer = get_engine().get_renderer();
        auto& settings = renderer.get_directional_light_settings();

        float32 cycle = std::sin(day_night_phase_);
        float32 sun_y = -cycle;
        float32 sun_x = std::cos(day_night_phase_) * 0.3f;

        settings.direction = math::normalize(vec3f{sun_x, sun_y, 0.3f});

        float32 t = (cycle + 1.0f) * 0.5f;
        vec3f warm{1.0f, 0.85f, 0.6f};
        vec3f cold{0.4f, 0.5f, 0.8f};
        settings.color = warm * t + cold * (1.0f - t);
        settings.intensity = 0.3f + 0.5f * std::max(0.0f, -sun_y);
    }

    void render_ui() {
        ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImVec2 window_pos = ImVec2(viewport->WorkPos.x + 10, viewport->WorkPos.y + 10);
        ImGui::SetNextWindowPos(window_pos, ImGuiCond_Always, ImVec2(0.0f, 0.0f));

        ImGuiWindowFlags window_flags =
            ImGuiWindowFlags_NoCollapse |
            ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_AlwaysAutoResize |
            ImGuiWindowFlags_NoSavedSettings |
            ImGuiWindowFlags_NoNav |
            ImGuiWindowFlags_NoFocusOnAppearing;

        ImGui::Begin("Lighting Test", nullptr, window_flags);

        ImGui::Text("Controls: WASD + Mouse, F1 toggle cursor, ESC exit");
        ImGui::Separator();

        ImGui::SliderInt("Light count", &target_light_count_, 0, 256);
        ImGui::Text("Active lights: %u", static_cast<uint32>(light_entities_.size()));
        ImGui::SliderFloat("Orbit speed", &orbit_speed_, 0.0f, 5.0f);
        ImGui::SliderFloat("Day/Night speed", &day_night_speed_, 0.0f, 2.0f);

        ImGui::End();
    }

    void handle_key_press(gfx::keyboard::keys key) const {
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

    std::unique_ptr<gfx::free_camera_controller> camera_controller_;

    std::shared_ptr<asset::model> floor_model_;
    std::shared_ptr<asset::model> pillar_model_;
    std::unique_ptr<gfx::entity_guard<gfx::base_world_def>> floor_entity_;
    std::unique_ptr<gfx::entity_guard<gfx::base_world_def>> pillar_entity_;

    std::vector<std::unique_ptr<gfx::entity_guard<gfx::base_world_def>>> light_entities_;
    std::vector<point_light_info> light_infos_;

    float32 elapsed_time_{0.0f};
    float32 orbit_angle_{3.0f};
    float32 day_night_phase_{0.3f};
    float32 orbit_speed_{1.0f};
    float32 day_night_speed_{0.1f};
    int32 target_light_count_{8};
};

int main() {
    try {
        std::make_unique<gfx::engine<>>(1280, 720, "Voxel World - Lighting Test")
            ->run<lighting_app>();
    } catch (const std::exception& e) {
        log::error("Error: {}", e.what());
        return 1;
    }

    return 0;
}
