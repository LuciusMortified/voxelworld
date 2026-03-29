#include <vw/core.h>
#include <vw/gfx.h>

#include "vw/gfx/camera/third_person_camera_controller.h"
#include "vw/gfx/player/player_input_controller.h"
#include "vw/gfx/player/third_person_player_controller.h"
#include "vw/gfx/world/entity_guard.h"
#include "vw/gfx/world_grid/generators/perlin_terrain_generator.h"
#include "vw/gfx/world_grid/world_grid.h"

using namespace vw;

class player_controller_app final : public gfx::app<> {
public:
    explicit player_controller_app(
        gfx::engine<>& eng
    )
        : app{eng}
        , input_controller_(get_engine().get_window())
        , camera_controller_(
              get_engine().get_camera(),
              get_engine().get_world(),
              gfx::third_person_camera_params{
                  .arm_length     = 80.0f,
                  .arm_length_min = 10.0f,
                  .arm_length_max = 200.0f,
                  .target_offset  = {0.0f, 24.0f, 0.0f},
                  .pitch_min      = -30.0f,
                  .pitch_max      = 80.0f,
                  .zoom_speed     = 5.0f,
                  .collision_skin = 2.0f
              }
          )
        , player_controller_(
              get_engine().get_camera(),
              get_engine().get_world()
          ) {
        auto& window = get_engine().get_window();

        window.sub<gfx::key_press_event>([this](const gfx::key_press_event& event) -> bool {
            handle_key_press(event.key);
            return true;
        });

        window.sub<gfx::window_close_event>([this](gfx::window_close_event&) -> bool {
            get_engine().shutdown();
            return true;
        });

        get_engine().get_renderer().set_clear_color(0.4f, 0.6f, 0.9f, 1.0f);
        get_engine().get_debug_tool().set_visible(true);

        auto& fog         = get_engine().get_renderer().get_fog_settings();
        fog.color         = {0.4f, 0.6f, 0.9f};
        fog.near_distance = 6 * 64 * 8;
        fog.far_distance  = 9 * 64 * 8;

        input_controller_.set_mouse_captured(true);

        setup_world_grid();
        setup_player();
    }

    ~player_controller_app() override = default;

    void render(
        float delta_time
    ) override {
        try_place_player();

        const auto input = input_controller_.get_input_state();

        if (player_) {
            const auto player_ent = player_->get_entity();

            player_controller_.update(input, player_ent);
            camera_controller_.update(input, player_ent);
        }

        if (show_colliders_) {
            get_engine().get_renderer().draw_colliders(get_engine().get_world());
        }

        render_ui();
    }

private:
    void setup_world_grid() {
        auto& world    = get_engine().get_world();
        auto& registry = world.get_model_registry();

        generator_params_ = {
            .voxel_scale = 8,
        };
        auto generator = std::make_unique<gfx::perlin_terrain_generator>(
            registry.get_identity_pool(), registry.get_page_pool(), generator_params_
        );
        world.set_world_grid(std::make_shared<gfx::world_grid<>>(
            world, std::move(generator), generator_params_.voxel_scale
        ));
    }

    void setup_player() {
        auto& world            = get_engine().get_world();
        auto& model_registry   = world.get_model_registry();
        auto& transform_system = world.get_transform_system();
        auto& model_system     = world.get_model_system();
        auto& physics_system   = world.get_physics_system();

        player_model_ = model_registry.create("player_cube", 16, 16, 16);
        player_model_->fill(voxel{blocks::red_0});

        player_ = std::make_unique<gfx::entity_guard<>>(world);
        player_->with<gfx::transform_component>();
        player_->with<gfx::model_component>();
        player_->with<gfx::spatial_component>();
        player_->with<gfx::rigid_body_component>();
        player_->with<gfx::box_collider_component>();
        player_->with<gfx::character_controller_component>();
        player_->with<gfx::movement_intent_component>();
        player_->with<gfx::world_view_component>();

        const auto player_ent = player_->get_entity();

        transform_system
            .modify(player_ent)
            .set_position({0.0f, 500.0f, 0.0f})
            .set_origin({-8.0f, 0.0f, -8.0f});

        model_system
            .modify(player_ent)
            .set_model(player_model_);

        physics_system
            .modify_collider(player_ent)
            .set_extents({16.0f, 16.0f, 16.0f})
            .set_offset({0.0f, 8.0f, 0.0f});

        world.get_spatial_system()
            .modify(player_ent)
            .set_layer(gfx::spatial_layer::character);
    }

    void try_place_player() {
        if (player_placed_) {
            return;
        }

        const auto grid = get_engine().get_world().get_world_grid();
        const auto surface = grid->get_surface_y(0, 0);
        if (!surface) {
            return;
        }

        auto scale      = static_cast<float32>(generator_params_.voxel_scale);
        float32 spawn_y = (static_cast<float32>(*surface) + 3.0f) * scale;

        auto& world = get_engine().get_world();
        world.get_transform_system()
            .modify(player_->get_entity())
            .set_position({0.0f, spawn_y, 0.0f});

        player_placed_ = true;
    }

    void render_ui() const {
        const ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImVec2 window_pos             = ImVec2(viewport->WorkPos.x + 10, viewport->WorkPos.y + 10);
        ImGui::SetNextWindowPos(window_pos, ImGuiCond_Always, ImVec2(0.0f, 0.0f));

        ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_AlwaysAutoResize |
            ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoNav |
            ImGuiWindowFlags_NoFocusOnAppearing;

        ImGui::Begin("Player Controller Test", nullptr, window_flags);
        ImGui::Text("Controls:");
        ImGui::Text("WASD - move");
        ImGui::Text("SPACE - jump");
        ImGui::Text("F - test impulse");
        ImGui::Text("Mouse - rotate camera");
        ImGui::Text("Scroll - zoom");
        ImGui::Text("F1 - toggle cursor");
        ImGui::Text("F2 - toggle colliders");
        ImGui::Text("ESC - exit");
        ImGui::Separator();

        if (player_) {
            auto& world     = get_engine().get_world();
            auto player_ent = player_->get_entity();
            auto& tc        = world.get_component<gfx::transform_component>(player_ent);
            auto pos        = tc.get_position();
            ImGui::Text("Position: (%.1f, %.1f, %.1f)", pos.x, pos.y, pos.z);

            auto& rb = world.get_component<gfx::rigid_body_component>(player_ent);
            auto vel = rb.get_velocity();
            ImGui::Text("Velocity: (%.1f, %.1f, %.1f)", vel.x, vel.y, vel.z);
            auto imp = rb.get_impulse();
            ImGui::Text("Impulse: (%.1f, %.1f, %.1f)", imp.x, imp.y, imp.z);
            ImGui::Text("Grounded: %s", rb.is_grounded() ? "yes" : "no");
            ImGui::Text("Frozen: %s", rb.is_frozen() ? "yes" : "no");

            ImGui::Text("Arm length: %.1f", camera_controller_.get_actual_arm_length());
        }

        const auto grid = get_engine().get_world().get_world_grid();
        if (grid) {
            ImGui::Separator();
            ImGui::Text("Loaded chunks: %u", grid->get_loaded_chunk_count());
            ImGui::Text("Pending columns: %u", grid->get_pending_column_count());
        }

        const auto& stats = get_engine().get_world().get_update_stats();
        ImGui::Separator();
        ImGui::Text("Character Controller: %.2f ms", stats.character_controller_ms);
        ImGui::Text("Physics: %.2f ms", stats.physics_ms);

        ImGui::End();
    }

    void handle_key_press(
        gfx::keyboard::keys key
    ) {
        switch (key) {
            case gfx::keyboard::keys::ESCAPE:
                get_engine().shutdown();
                break;
            case gfx::keyboard::keys::F:
                if (player_) {
                    get_engine().get_world().get_physics_system()
                        .modify(player_->get_entity())
                        .add_external_impulse({0.0f, 30.0f, 50.0f});
                }
                break;
            case gfx::keyboard::keys::F1:
                input_controller_.set_mouse_captured(!input_controller_.is_mouse_captured());
                break;
            case gfx::keyboard::keys::F2:
                show_colliders_ = !show_colliders_;
                break;
            default:
                break;
        }
    }

    gfx::player_input_controller input_controller_;
    gfx::third_person_camera_controller<> camera_controller_;
    gfx::third_person_player_controller<> player_controller_;

    std::unique_ptr<gfx::entity_guard<>> player_;
    std::shared_ptr<gfx::model> player_model_;
    gfx::perlin_terrain_generator::params generator_params_;
    bool player_placed_ = false;
    bool show_colliders_ = true;
};

auto main() -> int {
    try {
        log::logger::get().add_file_sink("test_player_controller.log");
        std::make_unique<gfx::engine<>>(1280, 720, "Voxel World - Player Controller Test")
            ->run<player_controller_app>();
    } catch (const std::exception& e) {
        log::error("Error: {}", e.what());
        return 1;
    }

    return 0;
}
