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

    // Standing still and taking the world apart. Streaming has finished before
    // the first voxel goes, so every mesh built from here on was paid for by an
    // edit -- which is the number a destructible world lives or dies by and the
    // one nothing measured until now.
    dig,

    // Standing still and lighting the place up. Streaming has finished before
    // the first block goes down, so everything after that was paid for by an
    // edit -- the same shape as dig, asking a different question.
    //
    // dig cannot ask it. Removing rock underground moves the sky channel and
    // leaves the block channel at the zero it already was, so nothing dig
    // measures says what a light costs. What this is really here to price is
    // the quad count: a lamp lays a gradient over every surface within
    // fourteen voxels, and a gradient costs quads because the level is part of
    // the merge key. Run it twice, once with --bench-inert, and the difference
    // is the price of the light rather than of the block.
    light,

    // The world as it looks once somebody has lived in it: emitters scattered
    // over the surface all round the camera, more moving lights than the cull
    // will admit, and the camera turning so they come in and out of view.
    //
    // A different question from `light`, which prices one edit. Everything here
    // is standing before the first measured frame, so what is measured is the
    // steady state: the fragment loop over whatever survived the cull, on
    // geometry whose merge key is carrying block-light gradients everywhere.
    torches,

    // The same steady state as torches, and the same camera, with the sources
    // standing in two dozen dense groups instead of spread evenly over the
    // disc. Spread out, a froxel holds one source and a flat tile holds one
    // source, and the depth slices buy nothing; packed into a hamlet, a tile
    // covers the whole hamlet at every depth behind it and a froxel covers only
    // the part of it at that depth. This scene is where slices=24 and slices=1
    // are allowed to differ.
    village,

    // A ring of bodies round the camera, each one bobbing to a different
    // height, for looking at the patch under them. One of them never leaves the
    // ground on purpose: judging whether a shadow pales as its owner rises is
    // guesswork from memory unless the grounded case is in the same frame.
    blobs,

    flythrough,
    crowd,
};

// What the left button does while the cursor is captured. Off by default: this
// is a benchmark app first, and a stray click that rearranges the terrain would
// quietly invalidate a run.
enum class edit_tool : int32 {
    none = 0,
    place,
    remove,
};

struct block_choice {
    const char* name;
    block_id id;
};

// A short menu rather than all forty-eight of Apollo. The two that emit come
// first because they are what this exists for; the rest are enough to build
// something for the light to fall on.
constexpr std::array<block_choice, 8> block_menu{{
    {"lamp (emits 14)", blocks::lamp},
    {"lava (emits 15)", blocks::lava},
    {"stone", blocks::gray_5},
    {"dark stone", blocks::gray_2},
    {"grass", blocks::green_5},
    {"dirt", blocks::brown_2},
    {"sand", blocks::orange_5},
    {"white", blocks::white},
}};

// A carried source is round where a placed block's pool is a diamond, so the
// two cannot share a reach and cover the same floor. The diamond's cut through
// the light is a square of area 2r^2 and the ball's a circle of pi*r^2, which
// meet at r * sqrt(2/pi). Emission fourteen therefore carries eleven voxels
// rather than fourteen, and the pool that lands on the ground is the size the
// block would have made.
constexpr float32 round_reach = 0.8f;

// The voxel the crosshair is on and the empty one just before it, in voxel
// coordinates -- not world units. Which of the two a tool writes to is the
// whole difference between placing and removing.
struct voxel_pick {
    vec3i solid;
    vec3i empty;
};

// Everything the command line can say about a run, in one place. A struct
// rather than a row of positional parameters: the fourteenth of those was
// already impossible to read at the call site, and F3 wanted six more.
struct scene_options {
    bench_scene scene     = bench_scene::off;
    uint32 crowd_size     = 0;
    bool chunk_cull       = false;
    bool sun_in_bench     = false;
    int32 dig_per_frame   = 1;
    int32 lamps_per_frame = 1;
    bool light_inert      = false;
    int32 static_lights   = 400;
    int32 dynamic_lights  = 64;
    bool free_storm       = false;
    bool free_blobs       = false;
    int32 blob_bodies     = 8;
    int32 village_groups  = 24;
    float32 light_speed   = 1.0F;

    // 0 leaves the engine's own default in place, for all four of these.
    uint32 visible_lights = 0;
    uint32 cluster_tile   = 0;
    uint32 cluster_slices = 0;
    uint32 cluster_cap    = 0;

    bool clustered_lights = true;

    // Reading the grid back. Stats is the cheap half; verify is the whole
    // thing plus the reference, every verify_every frames.
    bool cluster_stats  = false;
    uint32 verify_every = 0;
};

class world_grid_app final : public gfx::app {
public:
    explicit world_grid_app(
        gfx::engine& eng, scene_options opts = {}
    )
        // Порядок повторяет объявление полей: иначе список врёт о том, что
        // произойдёт на самом деле, и -Wreorder-ctor это ловит.
        : app{eng},
          sun_in_bench_{opts.sun_in_bench},
          crowd_size_{opts.crowd_size},
          dig_per_frame_{opts.dig_per_frame},
          lamps_per_frame_{opts.lamps_per_frame},
          light_inert_{opts.light_inert},
          blob_bodies_{opts.blob_bodies},
          free_blobs_{opts.free_blobs},
          static_lights_{opts.static_lights},
          dynamic_lights_{opts.dynamic_lights},
          light_speed_{opts.light_speed},
          free_storm_{opts.free_storm},
          village_groups_{std::max(opts.village_groups, 1)},
          verify_every_{opts.verify_every},
          bench_scene_{opts.scene} {
        auto& renderer = get_engine().get_renderer();

        renderer.set_chunk_cull_enabled(opts.chunk_cull);
        renderer.get_cluster_settings().enabled = opts.clustered_lights;

        if (opts.visible_lights > 0) {
            renderer.get_max_visible_lights() = opts.visible_lights;
        }
        if (opts.cluster_tile > 0) {
            renderer.get_cluster_settings().tile_size = opts.cluster_tile;
        }
        if (opts.cluster_slices > 0) {
            renderer.get_cluster_settings().slices = opts.cluster_slices;
        }
        if (opts.cluster_cap > 0) {
            renderer.get_cluster_settings().cap = opts.cluster_cap;
        }

        // Verifying implies the lists; asking for stats alone spares the
        // megabytes the lists cost every frame.
        if (verify_every_ > 0) {
            renderer.set_cluster_readback(gfx::cluster_readback_level::full);
        } else if (opts.cluster_stats) {
            renderer.set_cluster_readback(gfx::cluster_readback_level::counts);
        }
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

        // Only while the cursor is captured. Uncaptured, the left button
        // belongs to the panel, and a click on a slider must not also dig a
        // hole in whatever is behind it.
        window.sub<plat::mouse_press_event>([this](const plat::mouse_press_event& event) -> bool {
            if (event.button == plat::mouse::buttons::LEFT &&
                camera_controller_->is_mouse_captured()) {
                apply_tool_();
            }
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
        set_torch_(false);
        report_dig_();
        report_light_();
        report_torches_();
        report_clusters_();
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

        // Three queues deep now: generated, lit, meshed. A column still waiting
        // on its light is not on screen yet.
        const bool streamed = streaming_settled_();

        // The storm has to be standing and quiet before anything is measured.
        // Four hundred edits worth of relighting inside the measured window
        // would drown the thing the scene is there to price.
        if (is_torch_scene_()) {
            bench_ready_ = streamed && storm_standing_ && wgs.get_stats().relight_backlog == 0;
            return bench_ready_;
        }

        // The ring goes up column by column now, so the last body can stand
        // after the last column does. Measuring a ring that is still filling
        // in counts patches that are not all there yet.
        if (bench_scene_ == bench_scene::blobs) {
            bench_ready_ = streamed && blob_seeded_ && blob_pending_.empty();
            return bench_ready_;
        }

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

    auto render(
        float delta_time
    ) -> void override{
        if (bench_scene_ == bench_scene::off) {
            camera_controller_->update(delta_time);
        } else {
            drive_bench_camera();
        }
        try_place_camera();
        tick_dig_();
        tick_light_();
        tick_torches_();
        drive_storm_lights_(delta_time);
        tick_cluster_readback_();
        tick_blob_bodies_();
        tick_crowd_settle_();
        tick_day_night_(delta_time);

        const auto& camera = get_engine().get_camera();
        const auto cam_pos = camera.get_position();

        auto& world            = get_engine().get_world();
        auto& transform_sys = world.system<ecs::transform_system>();
        transform_sys.modify(viewer_).set_position(cam_pos);

        tick_torch_(cam_pos);

        auto& renderer = get_engine().get_renderer();
        renderer.draw_line(vec3f{0, 0, 0}, vec3f{100, 0, 0}, colors::red);
        renderer.draw_line(vec3f{0, 0, 0}, vec3f{0, 100, 0}, colors::green);
        renderer.draw_line(vec3f{0, 0, 0}, vec3f{0, 0, 100}, colors::blue);

        // A captured cursor still moves ImGui's pointer around, so swinging the
        // camera hard lands it on a button and the panel lights up under a
        // crosshair that is not there. NoMouse takes the pointer away from
        // ImGui entirely for as long as the camera owns it.
        ImGuiIO& io = ImGui::GetIO();
        if (camera_controller_->is_mouse_captured()) {
            io.ConfigFlags |= ImGuiConfigFlags_NoMouse;
        } else {
            io.ConfigFlags &= ~ImGuiConfigFlags_NoMouse;
        }

        update_hovered_();
        draw_hover_();

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

        if (bench_scene_ == bench_scene::parked || bench_scene_ == bench_scene::dig ||
            bench_scene_ == bench_scene::light) {
            camera.set_position({0.0f, bench_altitude_, 0.0f});
            camera.set_rotation(-10.0f, 0.0f);
            return;
        }

        if (bench_scene_ == bench_scene::crowd) {
            camera.set_position({0.0f, bench_altitude_ + 60.0f, 260.0f});
            camera.set_rotation(-15.0f, 180.0f);
            return;
        }

        // Slowly round, so all of the ring passes through the view.
        if (bench_scene_ == bench_scene::blobs) {
            camera.set_position({0.0f, bench_altitude_, 0.0f});
            camera.set_rotation(
                -25.0f, static_cast<float32>(bench_frame_++) * bench_degrees_per_frame_ * 0.25f
            );
            return;
        }

        // Turning, and not parked, because a cull that is never asked to drop
        // anything is a cull that is not being measured.
        if (is_torch_scene_()) {
            camera.set_position({0.0f, bench_altitude_, 0.0f});
            camera.set_rotation(
                -20.0f, static_cast<float32>(bench_frame_++) * bench_degrees_per_frame_
            );
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
            // Only once the world around the start is whole. Setting off from
            // a cold start measured the first eight hundred frames of catching
            // up, which is a different thing from walking through a world that
            // is already there -- and looks, watching it, like the loader is
            // broken.
            if (!bench_ready_) {
                return;
            }

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
    auto try_place_camera() -> void {
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

    // The sun rides an arc and the world dims with it. Off in the bench scenes
    // by default: a light that turns invalidates every cascade, so a run left
    // with it would measure the sun rather than the change under test.
    //
    // --sun puts it back on for a bench run. Off by default meant no scene ever
    // exercised a turning light, and a bug that froze the shadows under one
    // lived through every run of the suite. Numbers from a --sun run are not
    // comparable with the rest.
    auto tick_day_night_(float delta_time) -> void {
        if ((bench_scene_ != bench_scene::off && !sun_in_bench_) || !day_night_running_) {
            return;
        }

        time_of_day_ += delta_time / std::max(1.0f, day_length_seconds_);
        time_of_day_ -= std::floor(time_of_day_);

        apply_time_of_day_();
    }

    auto apply_time_of_day_() -> void {
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

        // The ambient hemisphere follows the same palette: what a face pointing
        // up receives is the sky it is looking at. Damped, because the sky is a
        // saturated blue and taking it neat tints the whole world; and floored,
        // because a night with no ambient at all is a black screen rather than
        // a dark one.
        auto& ambient = renderer.get_ambient_settings();
        ambient.sky = vec3f{
            (sky.x * 0.6f) + 0.020f,
            (sky.y * 0.6f) + 0.025f,
            (sky.z * 0.6f) + 0.040f,
        };

        // Bounce off the ground: warm and dim, and it goes out with the sun,
        // since there is nothing left to bounce.
        ambient.ground = vec3f{
            std::lerp(0.030f, 0.20f, day),
            std::lerp(0.035f, 0.17f, day),
            std::lerp(0.055f, 0.14f, day),
        };
    }

    // Takes a box of ground apart one voxel at a time, in raster order, top
    // layer first. Driven by the frame index like every other bench scene: a
    // dig steered by wall clock removes a different number of voxels on every
    // machine and the per-edit numbers stop meaning anything.
    //
    // The box straddles the origin, which is where four columns meet, so chunk
    // seams get crossed constantly rather than by luck. Air is stepped over
    // rather than dug: model::set_voxel bumps the generation whatever it wrote,
    // so writing air onto air would order a remesh for nothing and flatter the
    // per-edit average.
    // A lamp block's own numbers on something that moves: emission fourteen
    // over fifteen, and fourteen voxels of reach at this world's scale. Walk it
    // up to a lamp that has been placed and the two should be one light -- that
    // comparison is the whole acceptance test for the stitch between the baked
    // half of this stage and the dynamic one.
    auto set_torch_(bool on) -> void {
        auto& world = get_engine().get_world();

        if (!on) {
            if (torch_.is_valid()) {
                world.destroy(torch_);
                torch_ = ecs::invalid_entity;
            }
            return;
        }

        if (torch_.is_valid()) {
            return;
        }

        torch_ = world.create()
                     .with<ecs::transform_component>()
                     .with<ecs::light_component>()
                     .get_entity();
    }

    auto tick_torch_(const vec3f& at) -> void {
        if (!torch_.is_valid()) {
            return;
        }

        auto& world = get_engine().get_world();
        world.system<ecs::transform_system>().modify(torch_).set_position(at);

        // Colour and strength re-read every frame rather than set once, so the
        // lamp sliders move the carried torch and the placed block together.
        // Two lights that are meant to be the same light must not have two
        // places to be set from.
        const auto& lamp = get_engine().get_renderer().get_block_light_settings();
        const auto scale = static_cast<float32>(generator_params_.voxel_scale);

        world.system<ecs::light_system>()
            .modify(torch_)
            .set_color(vec3f{
                lamp.color.x * lamp.intensity,
                lamp.color.y * lamp.intensity,
                lamp.color.z * lamp.intensity,
            })
            .set_intensity(14.0f / 15.0f)
            .set_range(14.0f * round_reach * scale);
    }

    // Amanatides and Woo, in voxel space so the walk is the plain one: step to
    // whichever axis boundary is nearest, one voxel a step, and stop at the
    // first that is not air. Reach is in voxels, so at a scale of eight a reach
    // of twelve is ninety-six world units.
    //
    // Not spatial_system::voxel_ray_cast, which is what the sculptor tools use:
    // that walks entity models through their transforms, and here the answer
    // wanted is a world voxel that world_grid::set_voxel will accept. Going
    // through the grid directly also means no dependence on whether chunk
    // entities happen to be in the spatial tree.
    [[nodiscard]] auto pick_voxel_() const -> std::optional<voxel_pick> {
        if (world_grid_ == nullptr) {
            return std::nullopt;
        }

        const auto scale   = static_cast<float32>(generator_params_.voxel_scale);
        const auto& camera = get_engine().get_camera();

        const vec3f eye = camera.get_position();
        const vec3f dir = camera.get_forward();
        const vec3f origin{eye.x / scale, eye.y / scale, eye.z / scale};

        vec3i at{
            static_cast<int32>(std::floor(origin.x)),
            static_cast<int32>(std::floor(origin.y)),
            static_cast<int32>(std::floor(origin.z)),
        };

        constexpr float32 far_away = std::numeric_limits<float32>::max();

        vec3i step{};
        vec3f next{};
        vec3f span{};

        for (std::size_t axis = 0; axis < 3; ++axis) {
            const float32 d = dir[axis];

            // A ray exactly parallel to an axis never crosses a boundary on it.
            // Left at the largest float rather than infinity so the three-way
            // comparison below stays a comparison and never sees a NaN.
            if (std::abs(d) < 1e-6f) {
                step[axis] = 0;
                next[axis] = far_away;
                span[axis] = far_away;
                continue;
            }

            step[axis] = d > 0.0f ? 1 : -1;
            span[axis] = std::abs(1.0f / d);

            const auto edge = static_cast<float32>(at[axis]) + (d > 0.0f ? 1.0f : 0.0f);
            next[axis]      = (edge - origin[axis]) / d;
        }

        vec3i empty = at;

        for (int32 i = 0; i < reach_voxels_; ++i) {
            const vec3i world{
                at.x * generator_params_.voxel_scale,
                at.y * generator_params_.voxel_scale,
                at.z * generator_params_.voxel_scale,
            };

            if (!world_grid_->get_voxel(world).is_empty()) {
                return voxel_pick{.solid = at, .empty = empty};
            }

            empty = at;

            if (next.x < next.y && next.x < next.z) {
                at.x += step.x;
                next.x += span.x;
            } else if (next.y < next.z) {
                at.y += step.y;
                next.y += span.y;
            } else {
                at.z += step.z;
                next.z += span.z;
            }
        }

        return std::nullopt;
    }

    auto update_hovered_() -> void {
        hovered_.reset();

        if (tool_ == edit_tool::none || !camera_controller_->is_mouse_captured()) {
            return;
        }

        hovered_ = pick_voxel_();
    }

    auto draw_hover_() -> void {
        if (!hovered_) {
            return;
        }

        const auto scale = static_cast<float32>(generator_params_.voxel_scale);
        auto& renderer   = get_engine().get_renderer();

        // A hair proud of the voxel on every side, or the wireframe z-fights
        // the face it is sitting on and comes out dashed.
        const auto outline = [&](vec3i cell, color clr) {
            const vec3f at{
                (static_cast<float32>(cell.x) * scale) - 0.05f,
                (static_cast<float32>(cell.y) * scale) - 0.05f,
                (static_cast<float32>(cell.z) * scale) - 0.05f,
            };
            renderer.draw_box(at, vec3f{scale + 0.1f, scale + 0.1f, scale + 0.1f}, clr);
        };

        // What is under the crosshair, always -- the same thing Minecraft
        // outlines whichever button is about to be pressed.
        outline(hovered_->solid, colors::white);

        // And where a placed block would land, which is the side of it the ray
        // came in through. Worth showing: at a scale of eight, guessing wrong
        // about which face is a whole block out of place.
        if (tool_ == edit_tool::place && hovered_->empty != hovered_->solid) {
            outline(hovered_->empty, colors::green);
        }
    }

    auto apply_tool_() -> void {
        if (tool_ == edit_tool::none || world_grid_ == nullptr) {
            return;
        }

        update_hovered_();
        if (!hovered_) {
            return;
        }

        const int32 scale  = generator_params_.voxel_scale;
        const bool placing = tool_ == edit_tool::place;
        const vec3i cell   = placing ? hovered_->empty : hovered_->solid;

        world_grid_->set_voxel(
            {cell.x * scale, cell.y * scale, cell.z * scale},
            placing ? voxel{block_menu[static_cast<std::size_t>(place_choice_)].id} : voxel{}
        );

        ++edit_clicks_;
    }

    // A cube of emitting blocks sunk into the ground under the camera. Half
    // buried on purpose: a lamp hanging in the air lights nothing a face can
    // show, and the interesting picture is the one on the ground around it.
    //
    // Two coordinate spaces meet here and they are not the same one.
    // get_surface_y is asked in voxels and answers in voxels;
    // world_grid::set_voxel is told world units and divides by the scale
    // itself. At the scale of eight this world runs at, handing either of them
    // the other's number misses by a factor of eight -- and misses in silence,
    // because a column that far out is simply not loaded. tick_dig_ has a
    // comment about the same trap, which is where this should have been read
    // first.
    auto drop_emitter_(block_id id, int32 radius) -> void {
        const int32 scale = generator_params_.voxel_scale;

        const auto floor_div = [](int32 a, int32 b) -> int32 {
            return a >= 0 ? a / b : (a - b + 1) / b;
        };

        const auto pos = get_engine().get_camera().get_position();
        const int32 vx = floor_div(static_cast<int32>(std::floor(pos.x)), scale);
        const int32 vz = floor_div(static_cast<int32>(std::floor(pos.z)), scale);

        const auto surface = world_grid_->get_surface_y(vx, vz);
        if (!surface) {
            drop_status_ = std::format("no ground under voxel {},{}", vx, vz);
            return;
        }

        for (int32 dy = -radius; dy <= radius; ++dy) {
            for (int32 dz = -radius; dz <= radius; ++dz) {
                for (int32 dx = -radius; dx <= radius; ++dx) {
                    world_grid_->set_voxel(
                        {(vx + dx) * scale, (*surface + dy) * scale, (vz + dz) * scale},
                        voxel{id}
                    );
                }
            }
        }

        const int32 side = (2 * radius) + 1;
        drop_status_ =
            std::format("{} blocks at voxel {},{},{}", side * side * side, vx, *surface, vz);
    }

    auto tick_dig_() -> void {
        if (bench_scene_ != bench_scene::dig || !bench_ready_ || world_grid_ == nullptr) {
            return;
        }

        if (!dig_started_) {
            const auto surface = world_grid_->get_surface_y(0, 0);
            if (!surface) {
                return;
            }

            // get_surface_y answers in voxels, and so does dig_top_voxel_. It
            // used to be divided by the voxel scale here, which put the spade
            // an eighth of the way up the world -- deep in rock, where nothing
            // is lit and nothing is drawn. The scene measured a dig that never
            // broke the surface.
            dig_top_voxel_ = *surface;
            dig_started_   = true;
            dig_mesh_base_ =
                get_engine().get_renderer().get_mesh_pool().get_gen_stats().chunks;

            const auto& wgs       = get_engine().get_world().system<ecs::world_grid_system>();
            dig_relight_base_     = wgs.get_stats().relit_columns;
            dig_relit_chunk_base_ = wgs.get_stats().relit_chunks;
            dig_light_base_       = wgs.get_light_stats().columns;
        }

        for (int32 done = 0; done < dig_per_frame_ && dig_cursor_ < dig_cells; ++dig_cursor_) {
            const int32 x = (dig_cursor_ % dig_side) - (dig_side / 2);
            const int32 z = ((dig_cursor_ / dig_side) % dig_side) - (dig_side / 2);
            const int32 y = dig_top_voxel_ - (dig_cursor_ / (dig_side * dig_side));

            const vec3i at{
                x * generator_params_.voxel_scale,
                y * generator_params_.voxel_scale,
                z * generator_params_.voxel_scale,
            };

            if (world_grid_->get_voxel(at).is_empty()) {
                continue;
            }

            world_grid_->set_voxel(at, voxel{});
            ++dig_edits_;
            ++done;
        }
    }

    // One emitter a step on a grid over the surface, spaced so the pools of
    // light overlap: a lamp reaches fourteen voxels and they stand four apart,
    // so every surface in the square ends up inside somebody's gradient. A
    // lamp with clear ground all round it would price the best case, and the
    // best case is not what a lit dungeon looks like.
    auto tick_light_() -> void {
        if (bench_scene_ != bench_scene::light || !bench_ready_ || world_grid_ == nullptr) {
            return;
        }

        const auto& wgs      = get_engine().get_world().system<ecs::world_grid_system>();
        const auto& mesh_gen = get_engine().get_renderer().get_mesh_pool().get_gen_stats();

        if (!light_started_) {
            light_started_    = true;
            light_mesh_base_  = mesh_gen.chunks;
            light_quads_base_ = mesh_gen.quads;

            // What streaming built, before an emitter existed anywhere. The
            // run below is compared against this, and the comparison is honest
            // only up to a point: these are not the same chunks. Two runs, one
            // of them --bench-inert, is the comparison that is.
            light_quads_per_chunk_base_ = mesh_gen.chunks == 0
                ? 0.0
                : static_cast<float64>(mesh_gen.quads) / static_cast<float64>(mesh_gen.chunks);

            light_relight_base_     = wgs.get_stats().relit_columns;
            light_relit_chunk_base_ = wgs.get_stats().relit_chunks;

            const auto light_stats = wgs.get_light_stats();
            light_columns_base_    = light_stats.columns;
            light_flood_base_ms_   = light_stats.flood_ms;
            light_bake_base_ms_    = light_stats.bake_ms;
        }

        const int32 scale = generator_params_.voxel_scale;

        for (int32 done = 0; done < lamps_per_frame_ && light_cursor_ < lamp_cells;
             ++light_cursor_) {
            const int32 ix = light_cursor_ % lamp_side;
            const int32 iz = light_cursor_ / lamp_side;

            const int32 vx = (ix - (lamp_side / 2)) * lamp_spacing;
            const int32 vz = (iz - (lamp_side / 2)) * lamp_spacing;

            const auto surface = world_grid_->get_surface_y(vx, vz);
            if (!surface) {
                continue;
            }

            // One voxel clear of the ground, so the block always lands in air
            // and both runs do the same amount of geometry work whatever the
            // block turns out to be.
            world_grid_->set_voxel(
                {vx * scale, (*surface + 1) * scale, vz * scale},
                voxel{light_inert_ ? blocks::gray_5 : blocks::lamp}
            );

            ++lamps_placed_;
            ++done;
        }
    }

    // Golden-angle spiral rather than a grid or a random scatter: even density
    // over the disc, deterministic to the voxel, and no two emitters landing on
    // the same column by accident. A benchmark that lays its lights out
    // differently on every run is not one.
    // A ring of plain pillars, because what is being looked at is the ground
    // under them and not the body. Eight by default, which is what the frame
    // uniform holds: at that count every one of them has a patch, and none is
    // missing one because the nearest-eight rule dropped it.
    auto spawn_blob_bodies_() -> void {
        if (world_grid_ == nullptr || (blob_seeded_ && blob_pending_.empty())) {
            return;
        }

        auto& world     = get_engine().get_world();
        auto& registry  = world.resource<asset::model_registry>();
        auto& transform_sys = world.system<ecs::transform_system>();
        auto& model_sys = world.system<ecs::model_system>();

        const auto scale = static_cast<float32>(generator_params_.voxel_scale);
        const auto count = std::max(blob_bodies_, 1);

        if (!blob_seeded_) {
            // One model for all of them. A model voxel is one world unit and a
            // terrain voxel is eight, so a body the size of a person is sixteen
            // across and forty tall -- built to the terrain's numbers it stands
            // one voxel high and reads as a speck, which is how this scene
            // started.
            blob_model_ = registry.create("blob_body", 16, 40, 16);
            blob_model_->fill(voxel{blocks::red_4});

            blob_pending_.reserve(static_cast<std::size_t>(count));
            for (int32 i = 0; i < count; ++i) {
                blob_pending_.push_back(i);
            }

            blob_seeded_ = true;
        }

        // A body whose columns have not arrived waits for them instead of being
        // dropped. Skipping it is what made this wait for the whole world: the
        // ring went up in one pass, and the first run of it put up five of
        // eight. Every number below reads the ring index, not the order the
        // ground turned up in, so the ring is the same one either way.
        std::size_t keep = 0;

        for (std::size_t pending = 0; pending < blob_pending_.size(); ++pending) {
            const int32 i = blob_pending_[pending];

            const float32 angle =
                (static_cast<float32>(i) / static_cast<float32>(count)) * 2.0f * math::pi;

            const auto vx = static_cast<int32>(std::lround(
                static_cast<float32>(blob_ring) * std::cos(angle)
            ));
            const auto vz = static_cast<int32>(std::lround(
                static_cast<float32>(blob_ring) * std::sin(angle)
            ));

            // The body is sixteen units across and a terrain voxel is eight,
            // so it stands on four columns and not on one. Settled on the one
            // it was measured over, the other three can be a voxel higher and
            // leave it a quarter buried -- a poor thing to ask a shadow bench
            // to judge shadows on.
            std::optional<int32> surface;
            for (int32 dz = 0; dz <= 1; ++dz) {
                for (int32 dx = 0; dx <= 1; ++dx) {
                    const auto column = world_grid_->get_surface_y(vx + dx, vz + dz);
                    if (column && (!surface || *column > *surface)) {
                        surface = column;
                    }
                }
            }

            if (!surface) {
                blob_pending_[keep++] = i;
                continue;
            }

            const auto ent = world.create()
                                 .with<ecs::transform_component>()
                                 .with<ecs::spatial_component>()
                                 .with<ecs::model_component>()
                                 // A disc a little broader than the body, which is sixteen
                                 // wide and so eight from the middle.
                                 .with(ecs::blob_shadow_component{20.0f, 48.0f, 0.6f})
                                 .get_entity();

            model_sys.modify(ent).set_model(blob_model_);

            const float32 ground = static_cast<float32>(*surface + 1) * scale;

            transform_sys.modify(ent).set_position({
                static_cast<float32>(vx) * scale, ground, static_cast<float32>(vz) * scale
            });

            bob_bodies_.push_back(bob{
                .ent    = ent,
                .x      = static_cast<float32>(vx) * scale,
                .z      = static_cast<float32>(vz) * scale,
                .ground = ground,

                // Nought to a little over the fall height, spread across the
                // ring. The first never leaves the ground and is the control,
                // the last goes a good way past it, so the widest patch and the
                // tightest are in the frame together.
                .amplitude = (static_cast<float32>(i) / static_cast<float32>(count)) * 1.3f *
                    48.0f,

                // Different speeds, or they rise and fall as one and the frame
                // only ever shows a single height.
                .speed = 0.012f + (0.004f * static_cast<float32>(i % 4)),
                .phase = static_cast<float32>(i) * 0.9f,
            });
        }

        blob_pending_.resize(keep);

        if (blob_pending_.empty()) {
            log::info(
                "blobs: {} bodies of {} asked on a ring of {} voxels", bob_bodies_.size(),
                blob_bodies_, blob_ring
            );
        }
    }

    // A raised cosine off the frame index: starts on the ground, comes back to
    // it, and never dips below. Off the frame index and not the clock, for the
    // reason every other motion here is.
    auto tick_blob_bodies_() -> void {
        if (bench_scene_ == bench_scene::blobs || free_blobs_) {
            spawn_blob_bodies_();
        }

        if (bob_bodies_.empty()) {
            return;
        }

        auto& transform_sys = get_engine().get_world().system<ecs::transform_system>();
        const auto frame    = static_cast<float32>(bob_frame_++);

        for (const bob& b : bob_bodies_) {
            const float32 rise =
                b.amplitude * 0.5f * (1.0f - std::cos((frame * b.speed) + b.phase));

            transform_sys.modify(b.ent).set_position({b.x, b.ground + rise, b.z});
        }
    }

    [[nodiscard]] auto streaming_settled_() const -> bool {
        const auto& wgs = get_engine().get_world().system<ecs::world_grid_system>();
        return wgs.get_stats().pending_count == 0 && wgs.get_stats().lighting_count == 0 &&
            get_engine().get_renderer().get_mesh_pool().get_pending_count() == 0;
    }

    [[nodiscard]] auto is_torch_scene_() const -> bool {
        return bench_scene_ == bench_scene::torches || bench_scene_ == bench_scene::village;
    }

    // Golden angle over a disc: even coverage, deterministic to the voxel, and
    // no two sites landing on each other.
    [[nodiscard]] static auto spiral_point_(int32 i, int32 count, float32 radius) -> vec2f {
        constexpr float32 golden = 2.39996323f;

        const auto n    = static_cast<float32>(std::max(count, 1));
        const float32 t = static_cast<float32>(i) / n;
        const float32 r = radius * std::sqrt(t);
        const float32 a = static_cast<float32>(i) * golden;

        return vec2f{r * std::cos(a), r * std::sin(a)};
    }

    [[nodiscard]] auto village_centre_(int32 group) const -> vec2f {
        return spiral_point_(group, village_groups_, static_cast<float32>(storm_radius));
    }

    [[nodiscard]] auto storm_site_(int32 i) const -> vec2i {
        const auto radius = static_cast<float32>(storm_radius);

        if (bench_scene_ != bench_scene::village) {
            const vec2f at = spiral_point_(i, std::max(static_lights_, 1), radius);

            return vec2i{
                static_cast<int32>(std::lround(at.x)),
                static_cast<int32>(std::lround(at.y)),
            };
        }

        // Group first and member second, so a run that places only half the
        // emitters has every hamlet started rather than the first few finished.
        const int32 group  = i % village_groups_;
        const int32 member = i / village_groups_;
        const int32 per =
            (std::max(static_lights_, 1) + village_groups_ - 1) / village_groups_;

        const vec2f centre = village_centre_(group);
        const vec2f offset =
            spiral_point_(member, per, static_cast<float32>(village_spread));

        return vec2i{
            static_cast<int32>(std::lround(centre.x + offset.x)),
            static_cast<int32>(std::lround(centre.y + offset.y)),
        };
    }

    auto tick_torches_() -> void {
        // Also without a bench, because the scene is worth walking around in
        // and a camera on rails that exits after nine hundred frames is not a
        // way to look at anything.
        const bool wanted = is_torch_scene_() || free_storm_;
        if (!wanted || world_grid_ == nullptr || storm_standing_) {
            return;
        }

        // Before the first emitter and before the world has finished arriving.
        // A moving light needs no ground under it, and a scene that only has
        // lights once the last column has landed measures a world built in the
        // dark and lit afterwards -- which is not the world it claims to price.
        spawn_storm_lights_();

        if (!storm_seeded_) {
            storm_pending_.reserve(static_cast<std::size_t>(std::max(static_lights_, 0)));
            for (int32 i = 0; i < static_lights_; ++i) {
                storm_pending_.push_back(i);
            }
            storm_seeded_ = true;
        }

        const int32 scale = generator_params_.voxel_scale;

        // A site whose column has not arrived yet is kept for a later frame,
        // not skipped. A cursor walking past it lost that emitter for the rest
        // of the run, and hiding that behind a wait for the whole world is what
        // kept the lights out of every loading frame.
        int32 done       = 0;
        std::size_t keep = 0;

        for (std::size_t i = 0; i < storm_pending_.size(); ++i) {
            const int32 site = storm_pending_[i];

            if (done < lamps_per_frame_) {
                const vec2i at = storm_site_(site);
                if (const auto surface = world_grid_->get_surface_y(at.x, at.y)) {
                    world_grid_->set_voxel(
                        {at.x * scale, (*surface + 1) * scale, at.y * scale}, voxel{blocks::lamp}
                    );

                    ++storm_placed_;
                    ++done;
                    continue;
                }
            }

            storm_pending_[keep++] = site;
        }

        storm_pending_.resize(keep);

        if (!storm_pending_.empty()) {
            return;
        }

        storm_standing_ = true;

        log::info(
            "{}: {} emitters standing, {} moving lights",
            bench_scene_ == bench_scene::village ? "village" : "torches", storm_placed_,
            storm_lights_.size()
        );
    }

    auto spawn_storm_lights_() -> void {
        auto& world = get_engine().get_world();

        while (static_cast<int32>(storm_lights_.size()) < dynamic_lights_) {
            storm_lights_.push_back(world.create()
                                        .with<ecs::transform_component>()
                                        .with<ecs::light_component>()
                                        .get_entity());
        }
    }

    // Orbits are a function of the frame index and never of elapsed time, for
    // the same reason the camera path is: a benchmark steered by wall clock
    // measures a different scene on every machine.
    auto drive_storm_lights_(
        float32 delta_time
    ) -> void {
        if (storm_lights_.empty() || world_grid_ == nullptr) {
            return;
        }

        // Over the measured window alone. The lights orbit from the frame they
        // are created, and the frames spent waiting for columns are not the
        // steady state the report describes.
        if (bench_ready_) {
            const uint32 visible = get_engine().get_renderer().get_visible_light_count();
            visible_peak_        = std::max(visible_peak_, visible);
            visible_sum_ += visible;
            ++visible_frames_;
            // Asked of the renderer rather than kept alongside it: how often the
        // scene is pressed against the cap says nothing unless it is the cap
        // the frame actually used.
        capped_frames_ += (visible >= get_engine().get_renderer().get_max_visible_lights()) ? 1 : 0;
        }

        auto& world          = get_engine().get_world();
        auto& transform_sys  = world.system<ecs::transform_system>();
        auto& light_sys      = world.system<ecs::light_system>();

        const auto& lamp = get_engine().get_renderer().get_block_light_settings();
        const auto scale = static_cast<float32>(generator_params_.voxel_scale);
        const auto phase = static_cast<float32>(storm_phase_);

        storm_phase_ += (bench_scene_ == bench_scene::off)
            ? static_cast<float64>(delta_time * storm_steps_per_second * light_speed_)
            : static_cast<float64>(light_speed_);

        for (std::size_t i = 0; i < storm_lights_.size(); ++i) {
            const auto k = static_cast<float32>(i);

            // Spread over radius, phase and speed so they do not travel as one
            // ring: a ring either fits in the frustum or does not, and then the
            // cull is only ever asked the same question.
            const float32 radius = static_cast<float32>(storm_radius) *
                (0.2f + (0.8f * (k / static_cast<float32>(storm_lights_.size()))));
            const float32 speed = 0.004f + (0.002f * std::fmod(k, 5.0f));
            const float32 angle = (k * 1.7f) + (phase * speed);

            // In the village the moving sources belong to a hamlet and orbit
            // inside it. Sent round the whole disc they would spend most of the
            // run over empty ground, and the scene would stop being the dense
            // one it exists to be.
            const bool in_village = bench_scene_ == bench_scene::village;

            const vec2f home = in_village
                ? village_centre_(static_cast<int32>(i) % village_groups_)
                : vec2f{0.0f, 0.0f};

            const float32 orbit = in_village
                ? static_cast<float32>(village_spread) *
                      (0.3f + (0.7f * (k / static_cast<float32>(storm_lights_.size()))))
                : radius;

            const float32 at_x = home.x + (orbit * std::cos(angle));
            const float32 at_z = home.y + (orbit * std::sin(angle));

            // Rounded to ask the grid how high the ground is, and nowhere else.
            // The source itself stands where the orbit put it: a light on whole
            // voxels hops a whole voxel at a time, and its falloff lands on the
            // voxel boundaries every frame, so the ground under it reads as
            // rings of flat-lit blocks rather than as a pool of light. Nothing
            // downstream wants an integer -- the buffer carries floats and the
            // shader works per pixel.
            const auto vx = static_cast<int32>(std::lround(at_x));
            const auto vz = static_cast<int32>(std::lround(at_z));

            const auto surface = world_grid_->get_surface_y(vx, vz);
            const float32 y =
                surface ? (static_cast<float32>(*surface + 3) * scale) : bench_altitude_;

            transform_sys.modify(storm_lights_[i])
                .set_position({at_x * scale, y, at_z * scale});

            light_sys.modify(storm_lights_[i])
                .set_color(vec3f{
                    lamp.color.x * lamp.intensity,
                    lamp.color.y * lamp.intensity,
                    lamp.color.z * lamp.intensity,
                })
                .set_intensity(14.0f / 15.0f)
                .set_range(14.0f * round_reach * scale);
        }
    }

    // A frame's grid comes back a full ring later, so this is always a frame
    // that has certainly finished and never the one being recorded. Both lists
    // separately: a mean taken over sources and bodies together describes
    // neither.
    auto tick_cluster_readback_() -> void {
        for (const gfx::cull_list kind : {gfx::cull_list::sources, gfx::cull_list::blobs}) {
            auto frame = get_engine().get_renderer().take_cluster_readback(kind);

            if (frame) {
                account_(kind, *frame);
            }
        }
    }

    auto account_(gfx::cull_list kind, const gfx::cluster_readback& frame) -> void {
        // Streaming frames light a world that is still arriving, and averaging
        // those into the steady state is how a scene reports a grid it never
        // actually ran.
        if (bench_scene_ != bench_scene::off && !bench_ready_) {
            return;
        }

        const uint32 clusters = frame.grid.cluster_count();

        if (frame.counts.size() < static_cast<std::size_t>(clusters) + 1) {
            return;
        }

        cluster_tally& tally = tally_[static_cast<std::size_t>(kind)];

        ++tally.frames;
        tally.grid = frame.grid;
        tally.cap  = frame.cap;

        uint64 assignments = 0;
        uint64 lit         = 0;
        uint32 peak        = 0;

        for (uint32 cluster = 0; cluster < clusters; ++cluster) {
            const uint32 listed = frame.counts[cluster];

            assignments += listed;
            lit += (listed > 0) ? 1 : 0;
            peak = std::max(peak, listed);
        }

        tally.assignments += assignments;
        tally.lit += lit;
        tally.peak = std::max(tally.peak, peak);

        const uint32 overflow = frame.counts[clusters];

        tally.overflow += overflow;
        tally.overflow_frames += (overflow > 0) ? 1 : 0;

        if (verify_every_ == 0 || frame.indices.empty() ||
            (tally.frames % verify_every_) != 0) {
            return;
        }

        verify_frame_(kind, frame);
    }

    // The reference is the same code the unit tests pin down, fed the same
    // view-space spheres the compute pass built for itself. What it costs is a
    // frame of stutter every verify_every frames, which is why nothing does
    // this unless asked.
    auto verify_frame_(gfx::cull_list kind, const gfx::cluster_readback& frame) -> void {
        const auto slot = static_cast<std::size_t>(kind);

        auto& reference = verify_reference_[slot];

        if (!reference || reference->get_grid() != frame.grid ||
            reference->get_cap() != frame.cap) {
            reference = std::make_unique<spatial::cluster_lights>(frame.grid, frame.cap);
        } else {
            reference->clear();
        }

        for (std::size_t i = 0; i < frame.columns.size(); ++i) {
            reference->add(static_cast<uint32>(i), frame.columns[i]);
        }

        const auto check = spatial::check_clusters(*reference, frame.counts, frame.indices);

        cluster_tally& tally = tally_[slot];

        ++tally.verified;
        tally.clusters += check.clusters_compared;
        tally.bad_counts += check.count_mismatches;
        tally.bad_sets += check.set_mismatches;

        if (check.ok()) {
            return;
        }

        ++tally.bad_frames;

        if (tally.bad_frames > 1) {
            return;
        }

        tally.worst = check;

        log::warn(
            "verify-lights ({}): {} counts and {} lists disagree with the reference, "
            "overflow tally {}; first at cluster {}, {} on the GPU and {} expected",
            kind == gfx::cull_list::sources ? "sources" : "bodies",
            check.count_mismatches, check.set_mismatches,
            check.overflow_matches ? "matches" : "does not match", check.first_bad,
            check.actual_count, check.reference_count
        );
    }

    auto report_clusters_() const -> void {
        report_list_(gfx::cull_list::sources, "sources");
        report_list_(gfx::cull_list::blobs, "bodies");
    }

    auto report_list_(gfx::cull_list kind, std::string_view what) const -> void {
        const cluster_tally& tally = tally_[static_cast<std::size_t>(kind)];

        if (tally.frames == 0) {
            return;
        }

        const auto frames = static_cast<float64>(tally.frames);
        const auto lit    = static_cast<float64>(tally.lit);
        const auto total  = static_cast<float64>(tally.grid.cluster_count());

        std::print(
            "\nclusters ({}): {}x{} tiles of {} px over {} slices, cap {}, {} clusters\n"
            "  per frame: {:.0f} assignments, {:.1f}% of clusters lit, "
            "{:.1f} mean list, {} peak\n"
            "  overflow: {:.1f} dropped a frame, {} frames of {} touched the cap\n",
            what,
            tally.grid.tiles_x(),
            tally.grid.tiles_y(),
            tally.grid.tile_size,
            tally.grid.slices,
            tally.cap,
            tally.grid.cluster_count(),
            static_cast<float64>(tally.assignments) / frames,
            total > 0.0 ? 100.0 * lit / (frames * total) : 0.0,
            lit > 0.0 ? static_cast<float64>(tally.assignments) / lit : 0.0,
            tally.peak,
            static_cast<float64>(tally.overflow) / frames,
            tally.overflow_frames,
            tally.frames
        );

        if (verify_every_ == 0) {
            return;
        }

        std::print(
            "  verify: {} frames checked against vw::spatial, {} disagreed;\n"
            "    {} counts and {} lists off out of {} clusters compared\n",
            tally.verified,
            tally.bad_frames,
            tally.bad_counts,
            tally.bad_sets,
            tally.clusters
        );

        if (tally.bad_frames > 0) {
            std::print(
                "    first at cluster {}: {} on the GPU, {} in the reference; "
                "{} counts and {} lists off, overflow tally {}\n",
                tally.worst.first_bad,
                tally.worst.actual_count,
                tally.worst.reference_count,
                tally.worst.count_mismatches,
                tally.worst.set_mismatches,
                tally.worst.overflow_matches ? "matches" : "does not match"
            );
        }
    }

    auto report_torches_() const -> void {
        if (!is_torch_scene_() || visible_frames_ == 0) {
            return;
        }

        const bool village = bench_scene_ == bench_scene::village;
        const uint32 cap   = get_engine().get_renderer().get_max_visible_lights();

        const std::string layout =
            village ? std::format(
                          "{} hamlets {} voxels across, spread over a spiral",
                          village_groups_, village_spread * 2
                      )
                    : std::string{"a spiral"};

        std::print(
            "\n{}: {} emitters placed of {} asked, {} moving lights\n"
            "  {} of radius {} voxels round the origin, camera turning\n"
            "  visible after the cull: {:.1f} mean, {} peak\n",
            village ? "village" : "torches",
            storm_placed_,
            static_lights_,
            storm_lights_.size(),
            layout,
            storm_radius,
            static_cast<float64>(visible_sum_) / static_cast<float64>(visible_frames_),
            visible_peak_
        );

        // Only when there is a cap to be at: with none, "0 frames at the cap
        // of 4294967295" is a line that says nothing.
        if (cap != gfx::light_buffer::no_cap) {
            std::print(
                "  {} frames of {} at the cap of {}\n", capped_frames_, visible_frames_, cap
            );
        }
    }

    auto report_light_() const -> void {
        if (!light_started_ || lamps_placed_ == 0) {
            return;
        }

        const auto& mesh_gen = get_engine().get_renderer().get_mesh_pool().get_gen_stats();
        const auto meshed    = mesh_gen.chunks - light_mesh_base_;
        const auto quads     = mesh_gen.quads - light_quads_base_;

        const auto& wgs     = get_engine().get_world().system<ecs::world_grid_system>();
        const auto& stats   = wgs.get_stats();
        const auto relit    = stats.relit_columns - light_relight_base_;
        const auto relit_ch = stats.relit_chunks - light_relit_chunk_base_;

        const auto light_stats = wgs.get_light_stats();
        const auto columns     = light_stats.columns - light_columns_base_;
        const auto flood_ms    = light_stats.flood_ms - light_flood_base_ms_;
        const auto bake_ms     = light_stats.bake_ms - light_bake_base_ms_;

        const auto per = [this](uint64 n) -> float64 {
            return static_cast<float64>(n) / static_cast<float64>(lamps_placed_);
        };

        const auto us_a_column = [columns](float32 ms) -> float64 {
            return columns == 0 ? 0.0
                                : (static_cast<float64>(ms) * 1000.0) /
                    static_cast<float64>(columns);
        };

        const float64 quads_a_chunk =
            meshed == 0 ? 0.0 : static_cast<float64>(quads) / static_cast<float64>(meshed);

        std::print(
            "\nlight: {} {} placed, {} chunk meshes, {:.2f} meshes an edit\n"
            "  {} x {} at spacing {}, {} an edit, cursor {} of {}\n"
            "  relight: {} columns asked ({:.3f} an edit), {} flooded, {} chunks changed\n"
            "  quads: {} built, {:.0f} a chunk -- streaming built {:.0f} a chunk\n"
            "  column: flood {:.0f} us, bake {:.0f} us\n"
            "  backlog {} columns left over\n",
            lamps_placed_,
            light_inert_ ? "inert blocks" : "lamps",
            meshed,
            per(meshed),
            lamp_side,
            lamp_side,
            lamp_spacing,
            lamps_per_frame_,
            light_cursor_,
            lamp_cells,
            relit,
            per(relit),
            columns,
            relit_ch,
            quads,
            quads_a_chunk,
            light_quads_per_chunk_base_,
            us_a_column(flood_ms),
            us_a_column(bake_ms),
            stats.relight_backlog
        );
    }

    auto report_dig_() const -> void {
        if (!dig_started_ || dig_edits_ == 0) {
            return;
        }

        const auto meshed =
            get_engine().get_renderer().get_mesh_pool().get_gen_stats().chunks - dig_mesh_base_;

        const auto& wgs     = get_engine().get_world().system<ecs::world_grid_system>();
        const auto& stats   = wgs.get_stats();
        const auto relit    = stats.relit_columns - dig_relight_base_;
        const auto relit_ch = stats.relit_chunks - dig_relit_chunk_base_;
        const auto lit      = wgs.get_light_stats().columns - dig_light_base_;

        const auto per = [this](uint64 n) -> float64 {
            return static_cast<float64>(n) / static_cast<float64>(dig_edits_);
        };

        std::print(
            "\ndig: {} voxels removed, {} chunk meshes, {:.2f} meshes an edit\n"
            "  box {} cubed at the origin, {} voxels a frame, cursor {} of {}\n"
            "  relight: {} columns asked ({:.3f} an edit), {} flooded, {} chunks changed\n"
            "  backlog {} columns left over\n",
            dig_edits_,
            meshed,
            per(meshed),
            dig_side,
            dig_per_frame_,
            dig_cursor_,
            dig_cells,
            relit,
            per(relit),
            lit,
            relit_ch,
            stats.relight_backlog
        );
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
    auto spawn_crowd(
        float32 ground_y
    ) -> void {
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
                .with(ecs::blob_shadow_component{8.0f, 48.0f, 0.55f})
                // Radius off the body's own girth: the collider is twelve wide,
                // so a disc a little broader than the feet. Fall height is one
                // body: a jump that clears it halves the patch.
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

            for (std::size_t part = 0; part < parts.size(); ++part) {
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

        for (std::size_t part = 0; part < crowd_target_names_.size(); ++part) {
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

    auto setup_world_grid() -> void {
        auto& world = get_engine().get_world();

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

    auto render_ui() -> void {
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
        ImGui::Text("LMB - use the tool, cursor captured");
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

        if (ImGui::CollapsingHeader("Editing")) {
            static constexpr std::array<const char*, 3> tool_names{"none", "place", "remove"};

            auto tool = static_cast<int32>(tool_);
            if (ImGui::Combo(
                    "Tool", &tool, tool_names.data(), static_cast<int32>(tool_names.size())
                )) {
                tool_ = static_cast<edit_tool>(tool);
            }

            if (tool_ == edit_tool::place) {
                std::array<const char*, block_menu.size()> names{};
                for (std::size_t i = 0; i < block_menu.size(); ++i) {
                    names[i] = block_menu[i].name;
                }
                ImGui::Combo("Block", &place_choice_, names.data(), static_cast<int32>(names.size()));
            }

            ImGui::SliderInt("Reach (voxels)", &reach_voxels_, 2, 32);

            if (tool_ == edit_tool::none) {
                ImGui::TextUnformatted("pick a tool, capture the cursor with F1, left click");
            } else if (!camera_controller_->is_mouse_captured()) {
                ImGui::TextUnformatted("F1 to capture the cursor");
            } else if (hovered_) {
                ImGui::Text(
                    "voxel %d,%d,%d", hovered_->solid.x, hovered_->solid.y, hovered_->solid.z
                );
            } else {
                ImGui::TextUnformatted("nothing in reach");
            }

            ImGui::Text("edits: %d", edit_clicks_);

            if (free_storm_ || bench_scene_ == bench_scene::torches) {
                ImGui::Text(
                    "storm: %llu of %d emitters, %zu moving, %u visible",
                    static_cast<unsigned long long>(storm_placed_), static_lights_,
                    storm_lights_.size(),
                    get_engine().get_renderer().get_visible_light_count()
                );
            }
        }

        ImGui::Separator();

        // No shadow panel: the cascades are parked, and sliders that move
        // nothing are worse than none. Turning them back on is two edits --
        // shadow_settings::enabled and SHADOW_ENABLED in voxel.frag.
        if (ImGui::CollapsingHeader("Lighting")) {
            auto& renderer = get_engine().get_renderer();
            auto& ambient  = renderer.get_ambient_settings();

            ImGui::SliderFloat("Ambient", &ambient.strength, 0.0f, 2.0f, "%.2f");
            ImGui::SliderFloat("AO strength", &ambient.ao_strength, 0.0f, 1.0f, "%.2f");
            ImGui::SliderFloat("AO curve", &ambient.ao_curve, 0.25f, 4.0f, "%.2f");

            // Drag this to zero and look at a hillside from the downhill side:
            // the steps flatten into one plane, because occlusion has nothing
            // to say about a corner with nothing above it.
            ImGui::SliderFloat("Convex strength", &ambient.convex_strength, 0.0f, 1.0f, "%.2f");
            ImGui::SliderFloat("Convex curve", &ambient.convex_curve, 0.25f, 4.0f, "%.2f");

            ImGui::ColorEdit3("Cave ambient", &ambient.cave.x);

            // Exposure moves the whole picture; the white point decides where
            // the roll-off lands. Drop the white point under what the brightest
            // face reaches and that face clips again, which is the state this
            // replaced.
            auto& tonemap = renderer.get_tonemap_settings();
            ImGui::SliderFloat("Exposure", &tonemap.exposure, 0.1f, 4.0f, "%.2f");
            ImGui::SliderFloat("White point", &tonemap.white_point, 0.25f, 4.0f, "%.2f");

            // Zero is plain Lambert. Set the time to noon and drag it there:
            // three of the four walls of every voxel collapse onto one colour,
            // because the sun gives all three exactly nothing.
            auto& sun = renderer.get_directional_light_settings();
            ImGui::SliderFloat("Sun wrap", &sun.wrap, 0.0f, 1.0f, "%.2f");

            ImGui::SliderFloat("Sky curve", &ambient.sky_curve, 0.25f, 4.0f, "%.2f");

            // Drag this to one and stand in a cave mouth: daylight walks
            // fifteen voxels in, because that is how far the flood carries it.
            // Sky light is the sun's only occluder now, so this is the whole
            // say over how sharply the sun stops at an opening.
            ImGui::SliderFloat("Sun curve", &ambient.sun_curve, 0.25f, 8.0f, "%.2f");

            // One colour for every light block there is. Set the time to
            // midnight and it is the only thing still lighting anything --
            // which is the point of keeping this channel out of the day.
            auto& lamp = renderer.get_block_light_settings();
            ImGui::ColorEdit3("Lamp colour", &lamp.color.x);
            ImGui::SliderFloat("Lamp strength", &lamp.intensity, 0.0f, 4.0f, "%.2f");

            // At one the fifteen baked steps come out even and read as a ramp
            // painted on the wall. Two is near the curve Minecraft's lightmap
            // uses and reads as falloff.
            ImGui::SliderFloat("Lamp curve", &lamp.curve, 0.25f, 4.0f, "%.2f");

            // At one a lava face comes out as exactly the colour lava was
            // drawn where nothing else reaches it. That is the anchor; above
            // it the tone curve starts taking the difference back.
            ImGui::SliderFloat("Glow", &lamp.glow, 0.0f, 3.0f, "%.2f");

            // The dynamic half of the same light. Light it, place a lamp, and
            // walk one onto the other: if the falloffs have drifted apart this
            // is where it shows.
            bool torch = torch_.is_valid();
            if (ImGui::Checkbox("Carry a torch", &torch)) {
                set_torch_(torch);
            }

            // Zero is the honest comparison: the crowd with nothing under it,
            // which is what the patch is meant to fix.
            ImGui::SliderFloat(
                "Blob shadow", &get_engine().get_renderer().get_blob_strength(), 0.0f, 1.0f,
                "%.2f"
            );

            // Nothing in the terrain emits, so without these there is nothing
            // to look at. Both write through world_grid::set_voxel, which is
            // the same path an edit takes -- the column goes dirty, the baker
            // floods it again and the chunks whose light moved are meshed
            // again. Watching the relight counters below tick is half the
            // point of the buttons.
            if (world_grid_ != nullptr) {
                if (ImGui::Button("Drop lamp")) {
                    drop_emitter_(blocks::lamp, 1);
                }
                ImGui::SameLine();
                if (ImGui::Button("Pour lava")) {
                    drop_emitter_(blocks::lava, 3);
                }

                // A button that does nothing and says nothing is the worst of
                // both: the first version of this missed the ground by a factor
                // of the voxel scale and looked exactly like a broken shader.
                if (!drop_status_.empty()) {
                    ImGui::TextUnformatted(drop_status_.c_str());
                }
            }

            // Judging occlusion off the finished frame means judging a product
            // of the block's colour and everything falling on it. These show
            // one factor with the others taken away.
            static constexpr std::array<const char*, 9> view_names{
                "off",         "ambient occlusion", "normals",
                "sky light",   "convexity",         "block light",
                "blob shadow", "light complexity",  "blob complexity"
            };
            auto view = static_cast<int32>(renderer.get_debug_view());
            if (ImGui::Combo(
                    "Debug view", &view, view_names.data(), static_cast<int32>(view_names.size())
                )) {
                renderer.set_debug_view(static_cast<gfx::debug_view>(view));
            }

            // The switch the acceptance of the froxel pass rests on: the same
            // frame lit both ways, with nothing else moving between the two.
            auto& clusters = renderer.get_cluster_settings();
            ImGui::Checkbox("Clustered lights", &clusters.enabled);
            ImGui::SameLine();
            ImGui::Text("%u x %u x %u, cap %u",
                renderer.get_cluster_grid(get_engine().get_camera()).tiles_x(),
                renderer.get_cluster_grid(get_engine().get_camera()).tiles_y(),
                clusters.slices, clusters.cap);

            ImGui::Text(
                "sky   %.3f %.3f %.3f", ambient.sky.x, ambient.sky.y, ambient.sky.z
            );
            ImGui::Text(
                "grnd  %.3f %.3f %.3f", ambient.ground.x, ambient.ground.y, ambient.ground.z
            );
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

    auto step_time_of_day_(float32 delta) -> void {
        time_of_day_ += delta;
        time_of_day_ -= std::floor(time_of_day_);
        apply_time_of_day_();
    }

    auto handle_key_press(
        plat::keyboard::keys key
    ) -> void {
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
    bool sun_in_bench_          = false;

    static constexpr std::array<std::string_view, 4> crowd_target_names_{
        "body", "head", "hand_left", "hand_right",
    };

    std::vector<ecs::entity> crowd_;
    uint32 crowd_size_ = 0;

    // Thirty-two voxels a side is two chunks across at worst and one at best,
    // which is the range where a seam is crossed often enough to show up.
    static constexpr int32 dig_side  = 32;
    static constexpr int32 dig_cells = dig_side * dig_side * dig_side;

    int32 dig_per_frame_  = 1;
    int32 dig_cursor_     = 0;
    int32 dig_top_voxel_  = 0;
    uint64 dig_edits_     = 0;
    uint64 dig_mesh_base_        = 0;
    uint64 dig_relight_base_     = 0;
    uint64 dig_relit_chunk_base_ = 0;
    uint64 dig_light_base_       = 0;
    bool dig_started_     = false;

    // Sixteen a side at four voxels apart covers sixty-four voxels, which is
    // one column across: enough for the relight to cross seams and still small
    // enough that the same handful of columns is asked over and over, which is
    // what an edit-driven relight actually looks like.
    static constexpr int32 lamp_side    = 16;
    static constexpr int32 lamp_spacing = 4;
    static constexpr int32 lamp_cells   = lamp_side * lamp_side;

    int32 lamps_per_frame_ = 1;
    int32 light_cursor_    = 0;
    uint64 lamps_placed_   = 0;
    bool light_inert_      = false;
    bool light_started_    = false;

    uint64 light_mesh_base_          = 0;
    uint64 light_quads_base_         = 0;
    uint64 light_relight_base_       = 0;
    uint64 light_relit_chunk_base_   = 0;
    uint64 light_columns_base_       = 0;
    float32 light_flood_base_ms_     = 0.0f;
    float32 light_bake_base_ms_      = 0.0f;
    float64 light_quads_per_chunk_base_ = 0.0;

    std::string drop_status_;

    ecs::entity torch_ = ecs::invalid_entity;

    struct bob {
        ecs::entity ent;
        float32 x         = 0.0f;
        float32 z         = 0.0f;
        float32 ground    = 0.0f;
        float32 amplitude = 0.0f;
        float32 speed     = 0.0f;
        float32 phase     = 0.0f;
    };

    // Twenty-four voxels out, which is a couple of hundred world units: near
    // enough that the patch on the ground is worth looking at, far enough that
    // the whole ring is on screen from the middle of it.
    static constexpr int32 blob_ring = 24;

    int32 blob_bodies_ = 8;
    bool free_blobs_   = false;
    uint32 bob_frame_  = 0;
    std::vector<bob> bob_bodies_;

    // Ring indices still waiting for the ground under them, and the one model
    // they all share. The ring goes up column by column, so both outlive the
    // frame that started it.
    bool blob_seeded_ = false;
    std::vector<int32> blob_pending_;
    std::shared_ptr<asset::model> blob_model_;

    // Ninety-six voxels is a little under two columns out, which at this scale
    // covers most of what the camera can see before the fog takes over.
    static constexpr int32 storm_radius = 96;

    int32 static_lights_  = 400;
    int32 dynamic_lights_ = 64;
    uint64 storm_placed_  = 0;
    // Steps, not frames, and float64 because it only ever grows: at sixty a
    // second a long look at the scene would start losing the step itself in
    // float32 somewhere in the tenth hour.
    float64 storm_phase_ = 0.0;

    float32 light_speed_ = 1.0F;
    bool storm_seeded_    = false;
    bool storm_standing_  = false;
    bool free_storm_      = false;
    std::vector<int32> storm_pending_;
    std::vector<ecs::entity> storm_lights_;

    uint32 visible_peak_   = 0;
    uint64 visible_sum_    = 0;
    uint64 visible_frames_ = 0;
    uint32 capped_frames_  = 0;

    int32 village_groups_ = 24;

    // A hamlet is a handful of voxels across, not a hundred: the point of the
    // scene is a knot of sources close enough that one tile holds all of them.
    static constexpr int32 village_spread = 10;

    // What a step is worth outside a bench. The orbits below step with the
    // frame, which is the only thing a measured scene may do, and exactly the
    // wrong thing for a scene being looked at: at six hundred frames a second
    // the sources crossed the whole disc in under a second and there was
    // nothing to make out. Sixty a second is the same scene at the speed a
    // monitor would have shown it.
    static constexpr float32 storm_steps_per_second = 60.0F;

    uint32 verify_every_ = 0;

    // One list's worth of what the grid did. Two of them, because a mean taken
    // over the sources and the bodies together describes neither.
    struct cluster_tally {
        spatial::cluster_grid grid{};
        uint32 cap = 0;

        uint64 frames          = 0;
        uint64 assignments     = 0;
        uint64 lit             = 0;
        uint32 peak            = 0;
        uint64 overflow        = 0;
        uint64 overflow_frames = 0;

        uint64 verified   = 0;
        uint64 bad_frames = 0;
        uint64 clusters   = 0;
        uint64 bad_counts = 0;
        uint64 bad_sets   = 0;
        spatial::cluster_check worst{};
    };

    std::array<cluster_tally, gfx::cull_list_count> tally_{};
    std::array<std::unique_ptr<spatial::cluster_lights>, gfx::cull_list_count>
        verify_reference_;

    edit_tool tool_             = edit_tool::none;
    int32 place_choice_         = 0;
    int32 reach_voxels_         = 12;
    int32 edit_clicks_          = 0;
    std::optional<voxel_pick> hovered_;
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

auto parse_float(std::string_view text, float32 fallback) -> float32 {
    float32 value = 0.0F;
    const auto* end = text.data() + text.size();
    if (std::from_chars(text.data(), end, value).ec != std::errc{}) {
        return fallback;
    }
    return value;
}

}  // namespace

auto main(int argc, char** argv) -> int {
    gfx::bench_config bench;
    scene_options opts;

    for (int32 i = 1; i < argc; ++i) {
        if (const auto value = option_value(std::string_view{argv[i]}, "--bench")) {
            if (*value == "crowd" && opts.crowd_size == 0) {
                opts.crowd_size = 50;
            }
            if (*value == "parked") {
                opts.scene = bench_scene::parked;
            } else if (*value == "spin") {
                opts.scene = bench_scene::spin;
            } else if (*value == "advance") {
                opts.scene = bench_scene::advance;
            } else if (*value == "crowd") {
                opts.scene = bench_scene::crowd;
            } else if (*value == "dig") {
                opts.scene = bench_scene::dig;
            } else if (*value == "light") {
                opts.scene = bench_scene::light;
            } else if (*value == "torches") {
                opts.scene = bench_scene::torches;
            } else if (*value == "village") {
                opts.scene = bench_scene::village;
            } else if (*value == "blobs") {
                opts.scene = bench_scene::blobs;
            } else {
                opts.scene = bench_scene::flythrough;
            }
            bench.measure_frames      = 2000;
            bench.warmup_frames       = 200;
            bench.fixed_delta_seconds = 1.0f / 60.0f;
        }
    }

    // Объявлено разом на всю цепочку: в каждой ветке своё `value` скрывало бы
    // предыдущее, а это ровно то, на что ругаются и /W4, и -Wshadow.
    std::optional<std::string_view> value;

    for (int32 i = 1; i < argc; ++i) {
        const std::string_view arg{argv[i]};

        if ((value = option_value(arg, "--bench-frames"))) {
            bench.measure_frames = parse_uint(*value, 2000);
        } else if ((value = option_value(arg, "--bench-warmup"))) {
            bench.warmup_frames = parse_uint(*value, 200);
        } else if ((value = option_value(arg, "--bench-out"))) {
            bench.report_path = std::string{*value};
        } else if ((value = option_value(arg, "--bench-crowd"))) {
            opts.crowd_size = parse_uint(*value, 50);
        } else if ((value = option_value(arg, "--bench-dig"))) {
            opts.dig_per_frame = static_cast<int32>(parse_uint(*value, 1));
        } else if ((value = option_value(arg, "--bench-lamps"))) {
            opts.lamps_per_frame = static_cast<int32>(parse_uint(*value, 1));
        } else if ((value = option_value(arg, "--bench-static"))) {
            opts.static_lights = static_cast<int32>(parse_uint(*value, 400));
        } else if ((value = option_value(arg, "--bench-dynamic"))) {
            opts.dynamic_lights = static_cast<int32>(parse_uint(*value, 64));
        } else if ((value = option_value(arg, "--bench-visible"))) {
            opts.visible_lights = parse_uint(*value, 0);
        } else if ((value = option_value(arg, "--bench-groups"))) {
            opts.village_groups = static_cast<int32>(parse_uint(*value, 24));
        } else if ((value = option_value(arg, "--cluster-tile"))) {
            opts.cluster_tile = parse_uint(*value, 0);
        } else if ((value = option_value(arg, "--cluster-slices"))) {
            // One slice is exactly flat tiles, which is the only honest way to
            // price what cutting by depth buys.
            opts.cluster_slices = parse_uint(*value, 0);
        } else if ((value = option_value(arg, "--cluster-cap"))) {
            opts.cluster_cap = parse_uint(*value, 0);
        } else if (arg == "--cluster-stats") {
            // The cheap half of the readback: how full the grid runs and how
            // much of it overflowed, without the lists themselves.
            opts.cluster_stats = true;
        } else if ((value = option_value(arg, "--verify-lights"))) {
            opts.verify_every = parse_uint(*value, 60);
        } else if (arg == "--verify-lights") {
            opts.verify_every = 60;
        } else if (arg == "--no-clusters") {
            // The other path: every source in the frame walked per pixel. The
            // froxel list is only worth a number next to this one.
            opts.clustered_lights = false;
        } else if (arg == "--blobs") {
            // The blob scene with nobody driving: the ring stands up, the
            // bodies bob, the camera is yours.
            opts.free_blobs = true;
        } else if ((value = option_value(arg, "--light-speed"))) {
            // How fast the moving sources go round, one being what the bench
            // uses. Below one to study a single source crossing a tile
            // boundary; a bench run that sets it is measuring another scene.
            opts.light_speed = std::max(parse_float(*value, 1.0F), 0.0F);
        } else if ((value = option_value(arg, "--bench-bodies"))) {
            opts.blob_bodies = static_cast<int32>(parse_uint(*value, 8));
        } else if (arg == "--lights") {
            // The torches scene with nobody driving: same four hundred emitters
            // and sixty-four moving lights, free camera, no exit after a frame
            // count. --bench-static and --bench-dynamic still size it.
            opts.free_storm = true;
        } else if (arg == "--bench-inert") {
            // The control run: the same edits, the same geometry, a block that
            // does not emit. What the two runs differ by is the light.
            opts.light_inert = true;
        } else if ((value = option_value(arg, "--mesh-workers"))) {
            bench.mesh_workers = parse_uint(*value, 0);
        } else if ((value = option_value(arg, "--terrain-workers"))) {
            bench.terrain_workers = parse_uint(*value, 0);
        } else if (arg == "--chunk-cull") {
            opts.chunk_cull = true;
        } else if (arg == "--sun") {
            opts.sun_in_bench = true;
        }
    }

    try {
        log::add_file_sink("testbed.log");
        std::make_unique<gfx::engine>(1280, 720, "Voxel World - Testbed", std::move(bench))
            ->run<world_grid_app>(opts);
    } catch (const std::exception& e) {
        log::error("Error: {}", e.what());
        return 1;
    }

    return 0;
}
