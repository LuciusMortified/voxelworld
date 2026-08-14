#include <imgui.h>

import std;

import vw.core;
import vw.ecs;
import vw.world;
import vw.platform;
import vw.gfx;

using namespace vw;

enum class bench_scene {
    off,
    parked,
    flythrough,
};

class world_grid_app final : public gfx::app {
public:
    explicit world_grid_app(
        gfx::engine& eng, bench_scene scene = bench_scene::off
    )
        : app{eng}, bench_scene_{scene} {
        auto& window = get_engine().get_window();
        auto& camera = get_engine().get_camera();

        camera_controller_ = std::make_unique<gfx::free_camera_controller>(0.1f, 60.0f);
        camera_controller_->setup(window, camera);

        window.sub<plat::key_press_event>([this](const plat::key_press_event& event) -> bool {
            handle_key_press(event.key);
            return true;
        });

        window.sub<plat::window_close_event>([this](plat::window_close_event&) -> bool {
            get_engine().shutdown();
            return true;
        });

        get_engine().get_renderer().set_clear_color(0.4f, 0.6f, 0.9f, 1.0f);
        get_engine().get_debug_tool().set_visible(true);

        auto& fog = get_engine().get_renderer().get_fog_settings();
        fog.color = {0.4f, 0.6f, 0.9f};
        fog.near_distance = 6 * 64 * 8;
        fog.far_distance = 9 * 64 * 8;

        // Anything past the fog is solid fog colour, so drawing it is pure
        // waste; the far plane is what makes the frustum test drop it.
        camera.set_far(fog.far_distance);

        setup_world_grid();
        camera.set_rotation(0.0f, 0.0f);
    }

    ~world_grid_app() override {
        if (viewer_.is_valid()) {
            get_engine().get_world().destroy(viewer_);
        }
    }

    // Readiness latches: it gates the initial load only. Once the flythrough
    // starts, streaming never settles again, and the streaming cost is exactly
    // what that scene is there to measure.
    [[nodiscard]] auto is_bench_ready() const -> bool override {
        if (bench_ready_) {
            return true;
        }
        if (!camera_placed_) {
            return false;
        }

        const auto& wgs = get_engine().get_world().system<ecs::world_grid_system>();
        bench_ready_    = wgs.get_stats().pending_count == 0 &&
            get_engine().get_renderer().get_mesh_pool().get_pending_count() == 0;
        return bench_ready_;
    }

    void render(
        float delta_time
    ) override {
        if (bench_scene_ == bench_scene::off) {
            camera_controller_->update(delta_time);
        } else {
            drive_bench_camera();
        }
        try_place_camera();

        const auto& camera = get_engine().get_camera();
        const auto cam_pos = camera.get_position();

        auto& world            = get_engine().get_world();
        auto& transform_sys = world.system<ecs::transform_system>();
        transform_sys.modify(viewer_).set_position(cam_pos);

        auto& renderer = get_engine().get_renderer();
        renderer.draw_line(vec3f{0, 0, 0}, vec3f{100, 0, 0}, colors::red);
        renderer.draw_line(vec3f{0, 0, 0}, vec3f{0, 100, 0}, colors::green);
        renderer.draw_line(vec3f{0, 0, 0}, vec3f{0, 0, 100}, colors::blue);

        render_ui();
    }

private:
    // Camera path is a function of the frame index, never of elapsed time:
    // a benchmark that steers by wall clock measures a different flight on
    // every machine.
    auto drive_bench_camera() -> void {
        if (!camera_placed_) {
            return;
        }

        auto& camera = get_engine().get_camera();

        if (bench_scene_ == bench_scene::parked) {
            camera.set_position({0.0f, bench_altitude_, 0.0f});
            camera.set_rotation(-10.0f, 0.0f);
            return;
        }

        if (!bench_ready_) {
            return;
        }

        const auto frame    = static_cast<float32>(bench_frame_++);
        const float32 angle = frame * bench_degrees_per_frame_;
        const float32 rad   = math::radians(angle);

        camera.set_position({
            std::sin(rad) * bench_radius_,
            bench_altitude_,
            std::cos(rad) * bench_radius_,
        });
        camera.set_rotation(-10.0f, angle + 90.0f);
    }

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
        bench_altitude_ = cam_y;
        camera_placed_  = true;
    }

    void setup_world_grid() {
        auto& world       = get_engine().get_world();
        auto& grid_system = world.system<ecs::world_grid_system>();

        generator_params_ = {
            .voxel_scale = 8,
        };
        auto& registry = world.resource<asset::model_registry>();
        auto generator = std::make_unique<ecs::perlin_terrain_generator>(
            registry.get_identity_pool(), registry.get_page_pool(), generator_params_);
        generator_  = generator.get();
        auto grid   = std::make_unique<ecs::world_grid>(
            world, generator_params_.voxel_scale
        );
        world_grid_  = grid.get();
        auto loader  = std::make_unique<ecs::chunk_loader>(std::move(generator));
        auto& gs     = world.system<ecs::world_grid_system>();
        gs.set_grid(std::move(grid));
        gs.set_loader(std::move(loader));

        viewer_ = world.create()
            .with<ecs::transform_component>()
            .with<ecs::world_view_component>()
            .get_entity();
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
            const auto& wgs = get_engine().get_world().system<ecs::world_grid_system>();
            ImGui::Text("Loaded columns: %u", world_grid_->column_count());
            ImGui::Text("Loaded chunks: %u", world_grid_->chunk_count());
            ImGui::Text("Pending columns: %u", wgs.get_stats().pending_count);
            ImGui::Text(
                "Pending meshes: %u", get_engine().get_renderer().get_mesh_pool().get_pending_count()
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
        plat::keyboard::keys key
    ) const {
        switch (key) {
            case plat::keyboard::keys::ESCAPE:
                get_engine().shutdown();
                break;
            case plat::keyboard::keys::F1:
                camera_controller_->toggle_mouse_captured();
                camera_controller_->toggle_keyboard_control_enabled();
                break;
            default:
                break;
        }
    }

    std::unique_ptr<gfx::free_camera_controller> camera_controller_;
    ecs::world_grid* world_grid_ = nullptr;
    ecs::entity viewer_ = ecs::invalid_entity;
    ecs::perlin_terrain_generator* generator_ = nullptr;
    ecs::perlin_terrain_generator::params generator_params_;
    bool camera_placed_ = false;

    bench_scene bench_scene_  = bench_scene::off;
    float32 bench_altitude_   = 0.0f;
    mutable bool bench_ready_ = false;
    uint64 bench_frame_       = 0;

    static constexpr float32 bench_radius_            = 1500.0f;
    static constexpr float32 bench_degrees_per_frame_ = 0.25f;
};

namespace {

auto option_value(std::string_view arg, std::string_view name) -> std::optional<std::string_view> {
    if (!arg.starts_with(name) || arg.size() <= name.size() || arg[name.size()] != '=') {
        return std::nullopt;
    }
    return arg.substr(name.size() + 1);
}

auto parse_uint(std::string_view text, uint32 fallback) -> uint32 {
    uint32 value = 0;
    const auto* end = text.data() + text.size();
    if (std::from_chars(text.data(), end, value).ec != std::errc{}) {
        return fallback;
    }
    return value;
}

}  // namespace

auto main(int argc, char** argv) -> int {
    gfx::bench_config bench;
    auto scene = bench_scene::off;

    for (int32 i = 1; i < argc; ++i) {
        if (const auto value = option_value(std::string_view{argv[i]}, "--bench")) {
            scene = (*value == "parked") ? bench_scene::parked : bench_scene::flythrough;
            bench.measure_frames      = 2000;
            bench.warmup_frames       = 200;
            bench.fixed_delta_seconds = 1.0f / 60.0f;
        }
    }

    for (int32 i = 1; i < argc; ++i) {
        const std::string_view arg{argv[i]};

        if (const auto value = option_value(arg, "--bench-frames")) {
            bench.measure_frames = parse_uint(*value, 2000);
        } else if (const auto value = option_value(arg, "--bench-warmup")) {
            bench.warmup_frames = parse_uint(*value, 200);
        } else if (const auto value = option_value(arg, "--bench-out")) {
            bench.report_path = std::string{*value};
        }
    }

    try {
        log::add_file_sink("test_world_grid.log");
        std::make_unique<gfx::engine>(1280, 720, "Voxel World - World Grid Test", std::move(bench))
            ->run<world_grid_app>(scene);
    } catch (const std::exception& e) {
        log::error("Error: {}", e.what());
        return 1;
    }

    return 0;
}
