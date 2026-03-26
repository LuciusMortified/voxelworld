#include <vw/core.h>
#include <vw/gfx.h>

#include <cmath>

#include "vw/gfx/world/entity_guard.h"
#include "vw/gfx/world_grid/generators/perlin_terrain_generator.h"
#include "vw/gfx/world_grid/world_grid.h"

using namespace vw;

class player_controller_app final : public gfx::app<> {
public:
    explicit player_controller_app(
        gfx::engine<>& eng
    )
        : app{eng} {
        auto& window = get_engine().get_window();
        auto& camera = get_engine().get_camera();

        camera.set_rotation(0.0f, 0.0f);

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

        setup_world_grid();
        setup_player();
    }

    ~player_controller_app() override = default;

    void render(
        float delta_time
    ) override {
        update_input();
        try_place_player();

        auto& world = get_engine().get_world();

        if (player_) {
            auto player_ent = player_->get_entity();
            auto& tc        = world.get_component<gfx::transform_component>(player_ent);
            auto player_pos = tc.get_position();

            world.get_transform_system().modify(viewer_->get_entity()).set_position(player_pos);

            update_camera(player_pos);
        }

        render_ui();
    }

private:
    void setup_world_grid() {
        auto& world       = get_engine().get_world();
        auto& grid_system = world.get_world_grid_system();

        generator_params_ = {
            .voxel_scale = 8,
        };
        auto& registry = world.get_model_registry();
        auto generator = std::make_unique<gfx::perlin_terrain_generator>(
            registry.get_identity_pool(), registry.get_page_pool(), generator_params_
        );
        world_grid_ = std::make_shared<gfx::world_grid<>>(
            world, std::move(generator), generator_params_.voxel_scale
        );
        grid_system.set_world_grid(world_grid_);
        world.get_physics_system().set_world_grid(world_grid_);

        viewer_ = std::make_unique<gfx::entity_guard<>>(world);
        viewer_->with<gfx::transform_component>();
        viewer_->with<gfx::world_view_component>();
    }

    void setup_player() {
        auto& world            = get_engine().get_world();
        auto& model_registry   = world.get_model_registry();
        auto& transform_system = world.get_transform_system();
        auto& model_system     = world.get_model_system();
        auto& physics_system   = world.get_physics_system();
        auto& character_system = world.get_character_controller_system();

        player_model_ = model_registry.create("player_cube", 16, 16, 16);
        player_model_->fill(voxel{blocks::red_0});

        player_ = std::make_unique<gfx::entity_guard<>>(world);
        player_->with<gfx::transform_component>();
        player_->with<gfx::model_component>();
        player_->with<gfx::spatial_component>();
        player_->with<gfx::rigid_body_component>();
        player_->with<gfx::sphere_collider_component>();
        player_->with<gfx::character_controller_component>();

        const auto player_ent = player_->get_entity();

        transform_system  //
            .modify(player_ent)
            .set_position({0.0f, 500.0f, 0.0f})
            .set_origin({-8.0f, 0.0f, -8.0f});

        model_system  //
            .modify(player_ent)
            .set_model(player_model_);

        physics_system  //
            .modify_collider(player_ent)
            .set_radius(16 * 0.5f)
            .set_offset({0.0f, 16 * 0.5f, 0.0f});
    }

    void try_place_player() {
        if (player_placed_) {
            return;
        }

        auto surface = world_grid_->get_surface_y(0, 0);
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

    void update_input() {
        auto& window    = get_engine().get_window();
        auto& world     = get_engine().get_world();
        auto& camera    = get_engine().get_camera();
        auto& cc_system = world.get_character_controller_system();

        if (!player_) {
            return;
        }

        auto forward = camera.get_forward();
        forward.y    = 0.0f;
        auto fwd_len = math::length(forward);
        if (fwd_len > 0.001f) {
            forward = forward / fwd_len;
        }

        auto right     = camera.get_right();
        right.y        = 0.0f;
        auto right_len = math::length(right);
        if (right_len > 0.001f) {
            right = right / right_len;
        }

        vec3f move_dir{0.0f, 0.0f, 0.0f};
        if (window.is_key_pressed(gfx::keyboard::keys::W)) {
            move_dir = move_dir + forward;
        }
        if (window.is_key_pressed(gfx::keyboard::keys::S)) {
            move_dir = move_dir - forward;
        }
        if (window.is_key_pressed(gfx::keyboard::keys::A)) {
            move_dir = move_dir - right;
        }
        if (window.is_key_pressed(gfx::keyboard::keys::D)) {
            move_dir = move_dir + right;
        }

        auto move_len = math::length(move_dir);
        if (move_len > 0.001f) {
            move_dir = move_dir / move_len;
        }

        auto modifier = cc_system.modify(player_->get_entity());
        modifier.set_move_input(move_dir);

        if (window.is_key_pressed(gfx::keyboard::keys::SPACE)) {
            modifier.request_jump();
        }
    }

    void update_camera(
        const vec3f& player_pos
    ) {
        auto& camera = get_engine().get_camera();
        auto& window = get_engine().get_window();

        if (window.is_key_pressed(gfx::keyboard::keys::LEFT)) {
            camera_yaw_ -= 2.0f;
        }
        if (window.is_key_pressed(gfx::keyboard::keys::RIGHT)) {
            camera_yaw_ += 2.0f;
        }
        if (window.is_key_pressed(gfx::keyboard::keys::UP)) {
            camera_pitch_ += 1.0f;
            if (camera_pitch_ > 60.0f)
                camera_pitch_ = 60.0f;
        }
        if (window.is_key_pressed(gfx::keyboard::keys::DOWN)) {
            camera_pitch_ -= 1.0f;
            if (camera_pitch_ < -30.0f)
                camera_pitch_ = -30.0f;
        }

        float32 yaw_rad   = camera_yaw_ * 3.14159f / 180.0f;
        float32 pitch_rad = camera_pitch_ * 3.14159f / 180.0f;

        float32 dist = camera_distance_;
        vec3f offset{
            std::sin(yaw_rad) * std::cos(pitch_rad) * dist,
            std::sin(pitch_rad) * dist,
            std::cos(yaw_rad) * std::cos(pitch_rad) * dist
        };

        auto cam_pos = player_pos + offset;
        camera.set_position(cam_pos);

        auto look_dir           = player_pos - cam_pos;
        float32 horizontal_dist = std::sqrt(look_dir.x * look_dir.x + look_dir.z * look_dir.z);
        float32 look_pitch      = std::atan2(look_dir.y, horizontal_dist) * 180.0f / 3.14159f;
        float32 look_yaw        = std::atan2(look_dir.x, look_dir.z) * 180.0f / 3.14159f;
        camera.set_rotation(look_pitch, look_yaw);
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
        ImGui::Text("Arrow keys - rotate camera");
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
            ImGui::Text("Grounded: %s", rb.is_grounded() ? "yes" : "no");
            ImGui::Text("Frozen: %s", rb.is_frozen() ? "yes" : "no");
        }

        if (world_grid_) {
            ImGui::Separator();
            ImGui::Text("Loaded chunks: %u", world_grid_->get_loaded_chunk_count());
            ImGui::Text("Pending columns: %u", world_grid_->get_pending_column_count());
        }

        auto& stats = get_engine().get_world().get_update_stats();
        ImGui::Separator();
        ImGui::Text("CC: %.2f ms", stats.character_controller_ms);
        ImGui::Text("Physics: %.2f ms", stats.physics_ms);

        ImGui::End();
    }

    void handle_key_press(
        gfx::keyboard::keys key
    ) const {
        switch (key) {
            case gfx::keyboard::keys::ESCAPE:
                get_engine().shutdown();
                break;
            default:
                break;
        }
    }

    std::shared_ptr<gfx::world_grid<>> world_grid_;
    std::unique_ptr<gfx::entity_guard<>> viewer_;
    std::unique_ptr<gfx::entity_guard<>> player_;
    std::shared_ptr<gfx::model> player_model_;
    gfx::perlin_terrain_generator::params generator_params_;
    bool player_placed_      = false;
    float32 camera_yaw_      = 0.0f;
    float32 camera_pitch_    = 20.0f;
    float32 camera_distance_ = 80.0f;
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
