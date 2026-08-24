export module vw.gfx:debug.window;

import std;

import vw.core;

export namespace vw::gfx {

class engine;

// Отладочная панель поверх ImGui: FPS, статистика систем, рендера и буферов.
class debug_window final {
public:
    using engine_type = engine;

    explicit debug_window(engine_type& engine);
    ~debug_window() = default;

    debug_window(const debug_window&)                    = delete;
    auto operator=(const debug_window&) -> debug_window& = delete;

    debug_window(debug_window&&)                    = default;
    auto operator=(debug_window&&) -> debug_window& = default;

    auto render(float32 delta_time) -> void;

    auto toggle_visibility() -> void;
    auto set_visible(bool visible) -> void;

    [[nodiscard]] auto is_visible() const -> bool;

private:
    auto render_fps_window() -> void;
    auto render_render_mode_controls() const -> void;
    auto render_combined_buffers_detail() -> void;
    auto render_systems_detail() -> void;
    auto render_render_detail() -> void;

    engine_type* engine_;

    bool visible_                      = false;
    bool show_combined_buffers_detail_ = false;
    bool show_systems_detail_          = false;
    bool show_render_detail_           = false;

    std::unordered_map<std::string, float32> metric_max_;

    // График FPS строится по бакетам фиксированной длительности: кадры быстрее
    // бакета сливаются в один столбец, поэтому график не зависит от частоты.
    static constexpr uint64 fps_bucket_count_        = 200;
    static constexpr float32 fps_bucket_duration_ms_ = 50.0f;

    std::array<float32, fps_bucket_count_> fps_bucket_max_{};
    std::array<float32, fps_bucket_count_> fps_bucket_min_{};
    uint64 fps_bucket_index_        = 0;
    uint64 fps_bucket_filled_count_ = 0;
    float32 fps_bucket_accum_ms_    = 0.0f;
    float32 fps_bucket_current_max_ = 0.0f;
    float32 fps_bucket_current_min_ = std::numeric_limits<float32>::max();
};

}  // namespace vw::gfx
