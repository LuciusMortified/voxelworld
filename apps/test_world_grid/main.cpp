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

    // Standing still and turning on the spot. Nothing streams and nothing
    // moves, so whatever the frame does here is the cost of looking somewhere
    // else -- which is where the stutter people actually notice comes from.
    spin,

    // Walking forward in a straight line for as long as the run lasts, so the
    // world in front is generated, meshed and uploaded the whole time. The
    // flythrough circles and comes back over its own tracks; this one never
    // does.
    advance,

    flythrough,
    crowd,
};

class world_grid_app final : public gfx::app {
public:
    explicit world_grid_app(
        gfx::engine& eng, bench_scene scene = bench_scene::off, uint32 crowd_size = 0,
        bool chunk_cull = true
    )
        : app{eng}, bench_scene_{scene}, crowd_size_{crowd_size} {
        get_engine().get_renderer().set_chunk_cull_enabled(chunk_cull);
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
        const bool streamed = wgs.get_stats().pending_count == 0 &&
            get_engine().get_renderer().get_mesh_pool().get_pending_count() == 0;

        // The crowd is spawned in the air and has to land before it is worth
        // measuring: bodies caught mid-fall put a different amount of work in
        // every run, and the spread swamps what the scene is there to measure.
        if (crowd_size_ > 0) {
            bench_ready_ = streamed && crowd_settle_frames_ >= crowd_settle_target_;
            return bench_ready_;
        }

        bench_ready_ = streamed;
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
        tick_crowd_settle_();
        tick_day_night_(delta_time);

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

        if (bench_scene_ == bench_scene::crowd) {
            camera.set_position({0.0f, bench_altitude_ + 60.0f, 260.0f});
            camera.set_rotation(-15.0f, 180.0f);
            return;
        }

        if (bench_scene_ == bench_scene::spin) {
            camera.set_position({0.0f, bench_altitude_, 0.0f});
            camera.set_rotation(
                -10.0f, static_cast<float32>(bench_frame_++) * bench_degrees_per_frame_
            );
            return;
        }

        if (bench_scene_ == bench_scene::advance) {
            const auto frame = static_cast<float32>(bench_frame_++);
            camera.set_position({
                frame * bench_advance_per_frame_,
                bench_altitude_ + bench_clearance_,
                0.0f,
            });
            camera.set_rotation(-10.0f, 90.0f);
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
            bench_altitude_ + bench_clearance_,
            std::cos(rad) * bench_radius_,
        });
        camera.set_rotation(-10.0f, angle + 90.0f);
    }

    // The origin is the corner where four columns meet, and the ground inside
    // one column alone spans twenty voxels on this terrain. Asking the surface
    // at the origin voxel and adding three put the eye at the bottom of a
    // hillside, walled in by ground seven voxels over it. The eye goes above
    // the highest ground within about a chunk instead.
    void try_place_camera() {
        if (camera_placed_) {
            return;
        }

        for (auto column : {vec2i{0, 0}, vec2i{-1, 0}, vec2i{0, -1}, vec2i{-1, -1}}) {
            if (!world_grid_->has_column(column)) {
                return;
            }
        }

        constexpr int32 probe_radius = 48;
        constexpr int32 probe_step   = 4;
        constexpr int32 eye_height   = 6;

        std::optional<int32> highest;
        for (int32 x = -probe_radius; x <= probe_radius; x += probe_step) {
            for (int32 z = -probe_radius; z <= probe_radius; z += probe_step) {
                if (const auto h = world_grid_->get_surface_y(x, z)) {
                    highest = highest ? std::max(*highest, *h) : *h;
                }
            }
        }

        if (!highest) {
            return;
        }

        auto scale    = static_cast<float32>(generator_params_.voxel_scale);
        float32 cam_y = static_cast<float32>(*highest + eye_height) * scale;
        get_engine().get_camera().set_position({0.0f, cam_y, 0.0f});
        bench_altitude_ = cam_y;
        camera_placed_  = true;

        log::info(
            "camera at y {} -- {} voxels over the highest ground near the origin, which is {}",
            cam_y, eye_height, *highest
        );

        if (crowd_size_ > 0) {
            spawn_crowd(cam_y);
        }
    }

    // The sun rides an arc and the world dims with it. Off in the bench
    // scenes: a light that turns invalidates every cascade, so a benchmark left
    // running with it would measure the sun rather than the change under test.
    auto tick_day_night_(float delta_time) -> void {
        if (bench_scene_ != bench_scene::off || !day_night_running_) {
            return;
        }

        time_of_day_ += delta_time / std::max(1.0f, day_length_seconds_);
        time_of_day_ -= std::floor(time_of_day_);

        apply_time_of_day_();
    }

    void apply_time_of_day_() {
        // Midnight at zero, sunrise at a quarter, noon at a half. The sun comes
        // up over +X and goes down over -X, tilted so shadows are never cast
        // straight along an axis -- axis-aligned voxels under an axis-aligned
        // sun give a flat, unreadable picture.
        const float32 angle = (time_of_day_ - 0.25f) * 2.0f * math::pi;

        const vec3f sun = math::normalize(
            vec3f{std::cos(angle), std::sin(angle), 0.42f}
        );

        // How high the sun is, eased so dusk lasts longer than the geometry
        // alone would give.
        const float32 height = std::clamp(sun.y, -1.0f, 1.0f);
        const float32 day    = std::clamp((height + 0.12f) / 0.35f, 0.0f, 1.0f);
        const float32 low    = 1.0f - std::clamp(std::abs(height) / 0.30f, 0.0f, 1.0f);

        auto& light = get_engine().get_renderer().get_directional_light_settings();

        // Below the horizon the light keeps coming from the sun's direction and
        // simply goes out; swinging it to the moon would flip every shadow in
        // one frame.
        light.direction = -sun;
        light.intensity = std::lerp(night_intensity_, 1.0f, day);

        const vec3f noon{1.0f, 0.97f, 0.92f};
        const vec3f dusk{1.0f, 0.62f, 0.34f};
        const vec3f night{0.42f, 0.52f, 0.78f};

        const vec3f warm{
            std::lerp(noon.x, dusk.x, low),
            std::lerp(noon.y, dusk.y, low),
            std::lerp(noon.z, dusk.z, low),
        };
        light.color = vec3f{
            std::lerp(night.x, warm.x, day),
            std::lerp(night.y, warm.y, day),
            std::lerp(night.z, warm.z, day),
        };

        const vec3f sky_day{0.40f, 0.60f, 0.90f};
        const vec3f sky_dusk{0.55f, 0.36f, 0.30f};
        const vec3f sky_night{0.03f, 0.04f, 0.09f};

        const vec3f sky_warm{
            std::lerp(sky_day.x, sky_dusk.x, low),
            std::lerp(sky_day.y, sky_dusk.y, low),
            std::lerp(sky_day.z, sky_dusk.z, low),
        };
        const vec3f sky{
            std::lerp(sky_night.x, sky_warm.x, day),
            std::lerp(sky_night.y, sky_warm.y, day),
            std::lerp(sky_night.z, sky_warm.z, day),
        };

        auto& renderer = get_engine().get_renderer();
        renderer.set_clear_color(sky.x, sky.y, sky.z, 1.0f);
        renderer.get_fog_settings().color = sky;
    }

    auto tick_crowd_settle_() -> void {
        if (crowd_size_ == 0 || crowd_.empty()) {
            return;
        }
        if (crowd_settle_frames_ < crowd_settle_target_) {
            ++crowd_settle_frames_;
        }
    }

    // A crowd of animated, physical bodies: the scene the PRD actually cares
    // about, and the only one where per-entity CPU work is visible at all.
    void spawn_crowd(
        float32 ground_y
    ) {
        auto& world    = get_engine().get_world();
        auto& registry = world.resource<asset::model_registry>();

        auto body = registry.create("crowd_body", 6, 12, 4);
        body->fill(voxel{blocks::blue_3});
        auto head = registry.create("crowd_head", 6, 6, 6);
        head->fill(voxel{blocks::brown_2});
        auto hand = registry.create("crowd_hand", 3, 8, 3);
        hand->fill(voxel{blocks::green_4});

        const std::array<std::pair<std::shared_ptr<asset::model>, vec3f>, 4> parts{{
            {body, vec3f{0.0f, 8.0f, 0.0f}},
            {head, vec3f{0.0f, 20.0f, 0.0f}},
            {hand, vec3f{-5.0f, 8.0f, 0.0f}},
            {hand, vec3f{5.0f, 8.0f, 0.0f}},
        }};

        const auto clip = make_crowd_clip(world);

        const auto side = static_cast<int32>(std::ceil(std::sqrt(static_cast<float32>(crowd_size_))));
        constexpr float32 spacing = 40.0f;
        const float32 origin = -0.5f * static_cast<float32>(side - 1) * spacing;

        auto& transform_sys = world.system<ecs::transform_system>();
        auto& hierarchy_sys = world.system<ecs::hierarchy_system>();
        auto& physics_sys   = world.system<ecs::physics_system>();
        auto& model_sys     = world.system<ecs::model_system>();
        auto& anim_sys      = world.system<ecs::animation_system>();

        for (uint32 i = 0; i < crowd_size_; ++i) {
            const auto col = static_cast<int32>(i) % side;
            const auto row = static_cast<int32>(i) / side;

            const auto root = world.create()
                .with<ecs::hierarchy_component>()
                .with<ecs::transform_component>()
                .with<ecs::spatial_component>()
                .with<ecs::rigid_body_component>()
                .with<ecs::box_collider_component>()
                .with<ecs::animation_player_component>()
                .get_entity();

            // Dropped from just above the ground and given time to land before
            // the run starts: settling them by hand onto uneven terrain leaves
            // bodies part-buried, and the push-out is worse than the fall.
            transform_sys.modify(root).set_position({
                origin + (static_cast<float32>(col) * spacing),
                ground_y + 40.0f,
                origin + (static_cast<float32>(row) * spacing),
            });
            physics_sys.modify_collider(root).set_extents({12.0f, 24.0f, 12.0f});
            world.system<ecs::spatial_system>().modify(root).set_layer(
                ecs::spatial_layer::character
            );

            for (size_t part = 0; part < parts.size(); ++part) {
                const auto ent = world.create()
                    .with<ecs::hierarchy_component>()
                    .with<ecs::transform_component>()
                    .with<ecs::spatial_component>()
                    .with<ecs::model_component>()
                    .with<ecs::animation_target_component>()
                    .get_entity();

                hierarchy_sys.modify(ent).set_parent(root);

                transform rest;
                rest.set_position(parts[part].second);
                transform_sys.modify(ent).set_transform(rest);
                model_sys.modify(ent).set_model(parts[part].first);

                const auto target = anim_sys.modify_target(ent);
                target.set_target_name(std::string{crowd_target_names_[part]});
                target.set_rest_transform(rest);

                crowd_.push_back(ent);
            }

            auto player = anim_sys.modify_player(root);
            player.add_layer(0);
            player.layer(0).blend_to(clip);
            player.layer(0).set_loop_mode(asset::animation_loop_mode::loop);
            player.layer(0).play();

            crowd_.push_back(root);
        }

        log::info("crowd: {} bodies, {} entities", crowd_size_, crowd_.size());
    }

    [[nodiscard]] static auto make_crowd_clip(
        ecs::world& world
    ) -> std::shared_ptr<asset::animation_clip> {
        auto& clips = world.resource<asset::animation_clip_registry>();
        auto clip   = clips.create("crowd_wave");

        for (size_t part = 0; part < crowd_target_names_.size(); ++part) {
            asset::animation_track track{std::string{crowd_target_names_[part]}, 60.0f};

            asset::animation_channel<vec3f> channel{asset::animation_property::position};
            const float32 phase = static_cast<float32>(part) * 0.25f;
            channel.add(asset::keyframe_vec3f{0.0f, vec3f{0.0f, phase, 0.0f}});
            channel.add(asset::keyframe_vec3f{0.5f, vec3f{0.0f, phase + 2.0f, 0.0f}});
            channel.add(asset::keyframe_vec3f{1.0f, vec3f{0.0f, phase, 0.0f}});
            track.add<asset::animation_property::position>(std::move(channel));

            clip->add_track(std::move(track));
        }

        return clip;
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
        auto loader  = std::make_unique<ecs::chunk_loader>(
            std::move(generator), get_engine().get_terrain_workers());
        auto& gs     = world.system<ecs::world_grid_system>();
        gs.set_grid(std::move(grid));
        gs.set_loader(std::move(loader));

        viewer_ = world.create()
            .with<ecs::transform_component>()
            .with<ecs::world_view_component>()
            .get_entity();

        gs.modify_view(viewer_).set_view_distance(10);
    }

    void render_ui() {
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
        ImGui::Text("N - pause the sun, [ ] - move it");
        ImGui::Text("ESC - exit");
        ImGui::Separator();

        {
            const auto hour = static_cast<int32>(time_of_day_ * 24.0f);
            const auto minute =
                static_cast<int32>(((time_of_day_ * 24.0f) - static_cast<float32>(hour)) * 60.0f);
            ImGui::Text("%02d:%02d %s", hour, minute, day_night_running_ ? "" : "(paused)");

            float32 time = time_of_day_;
            if (ImGui::SliderFloat("Time", &time, 0.0f, 1.0f, "%.3f")) {
                time_of_day_ = time;
                apply_time_of_day_();
            }
            ImGui::SliderFloat("Day (s)", &day_length_seconds_, 8.0f, 600.0f, "%.0f");
        }

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
            ImGui::Text(
                "Loaded chunks: %u (%u drawn)", world_grid_->chunk_count(),
                world_grid_->drawn_chunk_count()
            );
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

    void step_time_of_day_(float32 delta) {
        time_of_day_ += delta;
        time_of_day_ -= std::floor(time_of_day_);
        apply_time_of_day_();
    }

    void handle_key_press(
        plat::keyboard::keys key
    ) {
        switch (key) {
            case plat::keyboard::keys::ESCAPE:
                get_engine().shutdown();
                break;
            case plat::keyboard::keys::F1:
                camera_controller_->toggle_mouse_captured();
                camera_controller_->toggle_keyboard_control_enabled();
                break;
            case plat::keyboard::keys::N:
                day_night_running_ = !day_night_running_;
                break;
            case plat::keyboard::keys::LEFT_BRACKET:
                step_time_of_day_(-0.02f);
                break;
            case plat::keyboard::keys::RIGHT_BRACKET:
                step_time_of_day_(0.02f);
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

    // Noon to start with, so the first thing seen is the light the rest of the
    // engine was tuned under.
    float32 time_of_day_        = 0.5f;
    float32 day_length_seconds_ = 120.0f;
    float32 night_intensity_    = 0.06f;
    bool day_night_running_     = true;

    static constexpr std::array<std::string_view, 4> crowd_target_names_{
        "body", "head", "hand_left", "hand_right",
    };

    std::vector<ecs::entity> crowd_;
    uint32 crowd_size_ = 0;
    uint32 crowd_settle_frames_ = 0;
    static constexpr uint32 crowd_settle_target_ = 400;

    bench_scene bench_scene_  = bench_scene::off;
    float32 bench_altitude_   = 0.0f;
    mutable bool bench_ready_ = false;
    uint64 bench_frame_       = 0;

    // The path is flown at the height of the ground over the origin, and the
    // hills a kilometre away are higher than that. Before the caves came out of
    // noise that cost nothing -- inside a hill every chunk is solid and draws
    // nothing at all. Now the rock is hollow, and the same path measured 100 ms
    // a frame of cave walls at arm's length instead of the streaming it is
    // there to measure.
    static constexpr float32 bench_clearance_         = 400.0f;
    static constexpr float32 bench_radius_            = 1500.0f;

    // A column is 512 units across, so this walks into a new one every forty
    // frames or so: enough that the loader never catches up and the scene
    // measures streaming rather than a settled world.
    static constexpr float32 bench_advance_per_frame_ = 13.0f;
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
    auto scene        = bench_scene::off;
    uint32 crowd_size = 0;
    bool chunk_cull   = false;

    for (int32 i = 1; i < argc; ++i) {
        if (const auto value = option_value(std::string_view{argv[i]}, "--bench")) {
            if (*value == "crowd" && crowd_size == 0) {
                crowd_size = 50;
            }
            if (*value == "parked") {
                scene = bench_scene::parked;
            } else if (*value == "spin") {
                scene = bench_scene::spin;
            } else if (*value == "advance") {
                scene = bench_scene::advance;
            } else if (*value == "crowd") {
                scene = bench_scene::crowd;
            } else {
                scene = bench_scene::flythrough;
            }
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
        } else if (const auto value = option_value(arg, "--bench-crowd")) {
            crowd_size = parse_uint(*value, 50);
        } else if (const auto value = option_value(arg, "--mesh-workers")) {
            bench.mesh_workers = parse_uint(*value, 0);
        } else if (const auto value = option_value(arg, "--terrain-workers")) {
            bench.terrain_workers = parse_uint(*value, 0);
        } else if (arg == "--chunk-cull") {
            chunk_cull = true;
        }
    }

    try {
        log::add_file_sink("test_world_grid.log");
        std::make_unique<gfx::engine>(1280, 720, "Voxel World - World Grid Test", std::move(bench))
            ->run<world_grid_app>(scene, crowd_size, chunk_cull);
    } catch (const std::exception& e) {
        log::error("Error: {}", e.what());
        return 1;
    }

    return 0;
}
