module vw.testbed;

import std;
import vw.core;
import vw.ecs;
import vw.world;
import vw.platform;
import vw.gfx;

namespace vw::testbed {

testbed_app::testbed_app(
    gfx::engine& eng, testbed_options opts, const scene_factory& make_scene
)
    // Порядок повторяет объявление полей: иначе список врёт о том, что
    // произойдёт на самом деле, и -Wreorder-ctor это ловит.
    : app{eng}
    , sun_in_bench_{opts.stand.sun_in_bench}
    , drives_camera_{opts.stand.bench_running}
    , clusters_{opts.stand.cluster_stats, opts.stand.verify_every} {
    auto& renderer = get_engine().get_renderer();

    renderer.set_chunk_cull_enabled(opts.stand.chunk_cull);
    renderer.get_cluster_settings().enabled = opts.stand.clustered_lights;

    if (opts.stand.visible_lights > 0) {
        renderer.get_max_visible_lights() = opts.stand.visible_lights;
    }
    if (opts.stand.cluster_tile > 0) {
        renderer.get_cluster_settings().tile_size = opts.stand.cluster_tile;
    }
    if (opts.stand.cluster_slices > 0) {
        renderer.get_cluster_settings().slices = opts.stand.cluster_slices;
    }
    if (opts.stand.cluster_cap > 0) {
        renderer.get_cluster_settings().cap = opts.stand.cluster_cap;
    }

    // Проверка подразумевает списки; одни счётчики избавляют от мегабайтов,
    // которых списки стоят каждый кадр.
    if (clusters_.wanted()) {
        renderer.set_cluster_readback(clusters_.readback_level());
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

    // Только пока курсор захвачен. Без захвата левая кнопка принадлежит панели,
    // и клик по ползунку не должен заодно копать яму в том, что за ним.
    window.sub<plat::mouse_press_event>([this](const plat::mouse_press_event& event) -> bool {
        if (event.button == plat::mouse::buttons::LEFT &&
            camera_controller_->is_mouse_captured()) {
            apply_tool_();
        }
        return true;
    });

    renderer.set_clear_color(0.4f, 0.6f, 0.9f, 1.0f);
    get_engine().get_debug_tool().set_visible(true);

    auto& fog         = renderer.get_fog_settings();
    fog.color         = {0.4f, 0.6f, 0.9f};
    fog.near_distance = 6 * 64 * 8;
    fog.far_distance  = 9 * 64 * 8;

    // Всё за туманом — сплошной цвет тумана, поэтому рисовать его чистая трата;
    // дальняя плоскость и есть то, чем отсев по пирамиде это отбрасывает.
    camera.set_far(fog.far_distance);

    setup_world_grid();
    camera.set_rotation(0.0f, 0.0f);

    viewer_ = get_engine().get_world().create().with<ecs::world_view_component>().get_entity();

    // Сцена строится последней: её конструктор вправе спрашивать стенд про мир,
    // камеру и рельеф, а к этому моменту всё это уже стоит.
    scene_ = make_scene(*this);
}

testbed_app::~testbed_app() {
    set_torch_(false);

    if (viewer_.is_valid()) {
        get_engine().get_world().destroy(viewer_);
    }
}

auto testbed_app::world() const -> ecs::world& {
    return get_engine().get_world();
}

auto testbed_app::renderer() const -> gfx::renderer& {
    return get_engine().get_renderer();
}

auto testbed_app::camera() const -> gfx::camera& {
    return get_engine().get_camera();
}

// Защёлка: она сторожит только начальную загрузку. Стоит облёту начаться, как
// стриминг больше не успокаивается никогда, а его цена — ровно то, что эта
// сцена и меряет.
auto testbed_app::is_bench_ready() const -> bool {
    if (bench_ready_) {
        return true;
    }
    if (!camera_placed_) {
        return false;
    }

    bench_ready_ = streaming_settled() && (scene_ == nullptr || scene_->is_ready());
    return bench_ready_;
}

auto testbed_app::render(
    float32 delta_time
) -> void {
    if (drives_camera_) {
        if (camera_placed_) {
            scene_->drive_camera();
        }
    } else {
        camera_controller_->update(delta_time);
    }

    try_place_camera();

    // Мир вокруг догрузился — сцене можно расставлять своё содержимое: до этого
    // момента под ним может не быть земли.
    if (!world_ready_ && camera_placed_ && streaming_settled()) {
        world_ready_ = true;
        scene_->on_world_ready();
    }

    scene_->tick(delta_time);
    clusters_.collect(get_engine().get_renderer(), !drives_camera_ || bench_ready_);
    tick_day_night_(delta_time);

    const auto cam_pos = get_engine().get_camera().get_position();

    auto& world_ref     = get_engine().get_world();
    auto& transform_sys = world_ref.system<ecs::transform_system>();
    transform_sys.modify(viewer_).set_position(cam_pos);

    tick_torch_(cam_pos);

    auto& renderer_ref = get_engine().get_renderer();
    renderer_ref.draw_line(vec3f{0, 0, 0}, vec3f{100, 0, 0}, colors::red);
    renderer_ref.draw_line(vec3f{0, 0, 0}, vec3f{0, 100, 0}, colors::green);
    renderer_ref.draw_line(vec3f{0, 0, 0}, vec3f{0, 0, 100}, colors::blue);

    update_hovered_();
    draw_hover_();

    render_ui();
}

auto testbed_app::collect_report(gfx::report& out) const -> void {
    if (scene_ != nullptr) {
        scene_->collect_report(out);
    }
    clusters_.collect_report(out);
}

}  // namespace vw::testbed
