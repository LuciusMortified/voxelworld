export module vw.gfx:engine;

import std;

import vw.core;
import vw.ecs;
import vw.world;
import vw.platform;
import :camera;
import :render;
import :renderer;

namespace vw::gfx {
using namespace ::vw::ecs;
using namespace ::vw::plat;
}

// ---- from vw/gfx/engine/app.h
export namespace vw::gfx {


class engine;

class app {
public:
    using engine_type = engine;

    explicit app(
        engine_type& eng
    )
        : engine_(&eng) {}

    virtual ~app() = default;

    app(const app&)                    = delete;
    auto operator=(const app&) -> app& = delete;

    virtual void render(
        [[maybe_unused]] float delta_time
    ) {}

    // A benchmark run holds off warmup until the scene reports itself settled,
    // so streaming that has not finished cannot leak into the samples.
    [[nodiscard]] virtual auto is_bench_ready() const -> bool {
        return true;
    }

protected:
    [[nodiscard]] auto get_engine() const -> engine_type& {
        return *engine_;
    }

private:
    engine_type* engine_ = nullptr;
};
}  // namespace vw::gfx

// ---- from vw/gfx/debug/debug_window.h
export namespace vw::gfx {


class engine;

class debug_window final {
public:
    using engine_type = engine;

    explicit debug_window(engine_type& engine);
    ~debug_window() = default;

    debug_window(const debug_window&)            = delete;
    auto operator=(const debug_window&) -> debug_window& = delete;

    debug_window(debug_window&&)            = default;
    debug_window& operator=(debug_window&&) = default;

    void render(float delta_time);

    void toggle_visibility();
    void set_visible(bool visible);

    [[nodiscard]] bool is_visible() const;

private:
    void render_fps_window();
    void render_render_mode_controls() const;
    void render_combined_buffers_detail();
    void render_systems_detail();
    void render_render_detail();

    engine_type* engine_;

    bool visible_                      = false;
    bool show_combined_buffers_detail_ = false;
    bool show_systems_detail_          = false;
    bool show_render_detail_           = false;

    std::unordered_map<std::string, float32> metric_max_;

    static constexpr size_t fps_bucket_count_      = 200;
    static constexpr float32 fps_bucket_duration_ms_ = 50.0f;

    std::array<float32, fps_bucket_count_> fps_bucket_max_{};
    std::array<float32, fps_bucket_count_> fps_bucket_min_{};
    size_t  fps_bucket_index_         = 0;
    size_t  fps_bucket_filled_count_  = 0;
    float32 fps_bucket_accum_ms_      = 0.0f;
    float32 fps_bucket_current_max_   = 0.0f;
    float32 fps_bucket_current_min_   = std::numeric_limits<float32>::max();
};

}  // namespace vw::gfx

// ---- from vw/gfx/engine/engine.h
export namespace vw::gfx {


struct engine_stats {
    float32 fps             = 0.0f;
    float32 frame_ms        = 0.0f;
    float32 world_update_ms = 0.0f;
    float32 world_render_ms = 0.0f;
    float32 begin_frame_ms  = 0.0f;
    float32 app_render_ms   = 0.0f;
    float32 renderer_ms     = 0.0f;
    float32 end_frame_ms    = 0.0f;
    uint64 ram_usage_bytes  = 0;
    uint64 vram_usage_bytes = 0;

    // Streaming makes memory a curve, not a number: what matters is the high
    // water mark over a flight, not what is held at the moment the run ends.
    uint64 ram_peak_bytes  = 0;
    uint64 vram_peak_bytes = 0;

    // The working set is what is resident; the commit charge is what the
    // process has actually asked for. When the two diverge, the allocator is
    // sitting on memory it has already been given back.
    uint64 commit_bytes      = 0;
    uint64 commit_peak_bytes = 0;
};

// Turns the engine into an offline benchmark: run a fixed number of frames,
// write a timing report, exit. Measurement is off unless measure_frames is set.
struct bench_config {
    uint32 warmup_frames  = 0;
    uint32 measure_frames = 0;
    std::string report_path;

    // Zero leaves each queue on its own default. They are here so the curve can
    // be swept without a rebuild; it was, and four is its knee.
    uint32 mesh_workers    = 0;
    uint32 terrain_workers = 0;

    // Feeds the world a fixed step instead of the measured one, so simulation
    // work does not drift with the frame rate it is meant to measure.
    float32 fixed_delta_seconds = 0.0f;

    [[nodiscard]] auto enabled() const -> bool {
        return measure_frames > 0;
    }
};

}  // namespace vw::gfx

namespace vw::gfx {

struct frame_sample {
    engine_stats engine{};
    render_timing_stats render{};
    world_grid_system_stats grid{};
};

// Accumulates whole frame samples over a benchmark run and reduces them to
// percentiles. Samples are kept intact on purpose: a spike in the total frame
// time only means something next to the stage that produced it.
class frame_recorder final {
public:
    explicit frame_recorder(uint32 capacity);

    auto record(const frame_sample& sample) -> void;

    [[nodiscard]] auto sample_count() const -> uint32;
    [[nodiscard]] auto report() const -> std::string;

private:
    using stage_getter = auto (*)(const frame_sample&) -> float32;

    [[nodiscard]] auto percentile_of(stage_getter get, float32 quantile) const -> float32;

    std::vector<frame_sample> samples_;
    mutable std::vector<float32> scratch_;
};

}  // namespace vw::gfx

export namespace vw::gfx {

class engine final {
public:
    using renderer_type     = renderer;
    using world_type        = world;
    using debug_window_type = debug_window;
    using app_type          = app;

    engine(int32 width, int32 height, std::string_view title, bench_config bench = {});
    ~engine();

    engine(const engine&)                    = delete;
    auto operator=(const engine&) -> engine& = delete;

    template <typename TApp, typename... TArgs>
    void run(TArgs&&... args) {
        app_ = std::make_unique<TApp>(*this, std::forward<TArgs>(args)...);
        main_loop();
    }

    void shutdown();

    [[nodiscard]] auto get_window() const -> window&;
    [[nodiscard]] auto get_vulkan_context() const -> vulkan_context&;
    [[nodiscard]] auto get_renderer() const -> renderer_type&;
    [[nodiscard]] auto get_camera() const -> camera&;
    [[nodiscard]] auto get_world() const -> world_type&;
    [[nodiscard]] auto get_block_registry() const -> const block_registry&;
    [[nodiscard]] auto get_debug_tool() const -> debug_window_type&;

    // Zero means the loader picks its own default.
    [[nodiscard]] auto get_terrain_workers() const -> uint32 {
        return bench_.terrain_workers;
    }
    [[nodiscard]] const engine_stats& get_stats() const;

private:
    void main_loop();

    void render(float delta_time);

    auto bench_tick_() -> void;
    auto write_bench_report_() const -> void;

    std::unique_ptr<window> window_;
    std::unique_ptr<vulkan_context> vulkan_context_;
    std::unique_ptr<renderer_type> renderer_;
    std::unique_ptr<camera> camera_;
    block_registry block_registry_;
    std::unique_ptr<world_type> world_;
    std::unique_ptr<debug_window_type> debug_tool_;

    std::unique_ptr<app_type> app_;

    bool running_ = false;
    std::chrono::high_resolution_clock::time_point start_time_ =
        std::chrono::high_resolution_clock::now();
    std::chrono::high_resolution_clock::time_point last_frame_time_;

    mutable engine_stats stats_;
    std::chrono::high_resolution_clock::time_point frame_start_time_;
    std::chrono::high_resolution_clock::time_point last_memory_update_time_;

    bench_config bench_;
    std::unique_ptr<frame_recorder> recorder_;
    uint64 frame_index_       = 0;
    uint64 bench_start_frame_ = 0;

    // How long the scene took to stream in: everything generated, meshed and
    // uploaded. The one number the worker counts move.
    float32 ready_ms_        = 0.0f;
    uint64 ready_frames_     = 0;
    bool ready_recorded_     = false;

    static constexpr float MEMORY_UPDATE_INTERVAL_SEC = 1.0f;

    void update_stats();
    [[nodiscard]] static uint64 calculate_ram_usage();
    [[nodiscard]] static uint64 calculate_commit_usage();
    [[nodiscard]] uint64 calculate_vram_usage() const;

    event_sub<window_resize_event> window_resize_sub_;
    event_sub<key_press_event> key_press_sub_;
};

}  // namespace vw::gfx

// ---- from vw/gfx/animation.h
// Агрегирующий заголовок для системы анимаций
// Включает все необходимые файлы для работы с анимациями

// Базовые типы и перечисления

// Структуры данных анимации

// Реестр анимационных клипов

// ECS компоненты

// Система анимаций
