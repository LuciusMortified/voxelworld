#include <vw/core.h>
#include <vw/gfx.h>

#include <algorithm>

#include "vw/gfx/world/entity_guard.h"
#include "vw/gfx/world_grid/generators/perlin_terrain_generator.h"
#include "vw/gfx/world_grid/world_grid.h"

using namespace vw;

class world_grid_app final : public gfx::app<> {
public:
    explicit world_grid_app(
        gfx::engine<>& eng
    )
        : app{eng} {
        auto& window = get_engine().get_window();
        auto& camera = get_engine().get_camera();

        camera_controller_ = std::make_unique<gfx::fps_camera_controller>(0.1f, 60.0f);
        camera_controller_->setup(window, camera);

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

        auto& fog = get_engine().get_renderer().get_fog_settings();
        fog.color = {0.4f, 0.6f, 0.9f};
        fog.near_distance = 6 * 64 * 8;
        fog.far_distance = 9 * 64 * 8;

        setup_world_grid();
        camera.set_rotation(0.0f, 0.0f);
    }

    ~world_grid_app() override = default;

    void render(
        float delta_time
    ) override {
        camera_controller_->update(delta_time);
        try_place_camera();

        const auto& camera = get_engine().get_camera();
        const auto cam_pos = camera.get_position();

        auto& world            = get_engine().get_world();
        auto& transform_system = world.get_transform_system();
        transform_system.modify(viewer_->get_entity()).set_position(cam_pos);

        auto& renderer = get_engine().get_renderer();
        renderer.draw_line(vec3f{0, 0, 0}, vec3f{100, 0, 0}, colors::red);
        renderer.draw_line(vec3f{0, 0, 0}, vec3f{0, 100, 0}, colors::green);
        renderer.draw_line(vec3f{0, 0, 0}, vec3f{0, 0, 100}, colors::blue);

        render_ui();
    }

private:
    void try_place_camera() {
        if (camera_placed_) {
            return;
        }

        auto surface = world_grid_->get_surface_y(0, 0);
        if (!surface) {
            return;
        }

        auto scale = static_cast<float32>(generator_params_.voxel_scale);
        float32 cam_y = (static_cast<float32>(*surface) + 3.0f) * scale;
        get_engine().get_camera().set_position({0.0f, cam_y, 0.0f});
        camera_placed_ = true;
    }

    void setup_world_grid() {
        auto& world       = get_engine().get_world();
        auto& grid_system = world.get_world_grid_system();

        generator_params_ = {
            .voxel_scale = 8,
        };
        auto& registry = world.get_model_registry();
        auto generator = std::make_unique<gfx::perlin_terrain_generator>(
            registry.get_identity_pool(), registry.get_page_pool(), generator_params_);
        generator_     = generator.get();
        world_grid_    = std::make_shared<gfx::world_grid<>>(
            world, std::move(generator), generator_params_.voxel_scale
        );
        grid_system.set_world_grid(world_grid_);

        viewer_ = std::make_unique<gfx::entity_guard<>>(world);
        viewer_->with<gfx::transform_component>();
        viewer_->with<gfx::world_view_component>();
    }

    void render_ui() const {
        const ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImVec2 window_pos             = ImVec2(viewport->WorkPos.x + 10, viewport->WorkPos.y + 10);
        ImGui::SetNextWindowPos(window_pos, ImGuiCond_Always, ImVec2(0.0f, 0.0f));

        ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_AlwaysAutoResize |
            ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoNav |
            ImGuiWindowFlags_NoFocusOnAppearing;

        ImGui::Begin("World Grid Test", nullptr, window_flags);
        ImGui::Text("Controls:");
        ImGui::Text("WASD + Mouse - moving");
        ImGui::Text("F1 - toggle cursor");
        ImGui::Text("ESC - exit");
        ImGui::Separator();

        const auto& camera = get_engine().get_camera();
        const auto pos     = camera.get_position();
        ImGui::Text("Camera: (%.1f, %.1f, %.1f)", pos.x, pos.y, pos.z);

        if (world_grid_) {
            auto chunk_coord = world_grid_->world_to_chunk_coord(
                {static_cast<int32>(pos.x), static_cast<int32>(pos.y), static_cast<int32>(pos.z)}
            );
            ImGui::Text("Chunk: (%d, %d, %d)", chunk_coord.x, chunk_coord.y, chunk_coord.z);
            ImGui::Text("Loaded columns: %u", world_grid_->get_loaded_column_count());
            ImGui::Text("Loaded chunks: %u", world_grid_->get_loaded_chunk_count());
            ImGui::Text("Pending columns: %u", world_grid_->get_pending_column_count());
            ImGui::Text(
                "Pending meshes: %u", get_engine().get_world().get_mesh_pool().get_pending_count()
            );
        }

        ImGui::Separator();
        float speed = camera_controller_->get_camera_speed();
        if (ImGui::SliderFloat("Speed", &speed, 1.0f, 5000.0f, "%.0f")) {
            camera_controller_->set_camera_speed(speed);
        }

        ImGui::End();
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

    std::unique_ptr<gfx::fps_camera_controller> camera_controller_;
    std::shared_ptr<gfx::world_grid<>> world_grid_;
    std::unique_ptr<gfx::entity_guard<>> viewer_;
    gfx::perlin_terrain_generator* generator_ = nullptr;
    gfx::perlin_terrain_generator::params generator_params_;
    bool camera_placed_ = false;
};

auto main() -> int {
    try {
        log::logger::get().add_file_sink("test_world_grid.log");
        std::make_unique<gfx::engine<>>(1280, 720, "Voxel World - World Grid Test")
            ->run<world_grid_app>();
    } catch (const std::exception& e) {
        log::error("Error: {}", e.what());
        return 1;
    }

    return 0;
}
