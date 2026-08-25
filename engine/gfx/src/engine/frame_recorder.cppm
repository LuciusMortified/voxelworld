export module vw.gfx:engine.frame_recorder;

import std;

import vw.core;
import :engine.report;
import vw.world;
import :engine.stats;
import :renderer;

namespace vw::gfx {

struct frame_sample {
    engine_stats engine{};
    render_timing_stats render{};
    ecs::world_grid_system_stats grid{};
    ecs::world_update_stats systems{};
};

// Копит целые кадровые сэмплы за прогон бенчмарка и сводит их к перцентилям.
// Сэмплы хранятся целиком намеренно: всплеск полного кадрового времени что-то
// значит только рядом со стадией, которая его породила.
class frame_recorder final {
public:
    explicit frame_recorder(uint32 capacity);

    auto record(const frame_sample& sample) -> void;

    [[nodiscard]] auto sample_count() const -> uint32;
    [[nodiscard]] auto report() const -> std::string;

    // Те же стадии и те же перцентили, но деревом: текст читает человек, это —
    // машина. Обе формы идут от одной таблицы стадий, поэтому разойтись им
    // негде.
    auto collect(gfx::report& out) const -> void;

private:
    using stage_getter = auto (*)(const frame_sample&) -> float32;

    [[nodiscard]] auto percentile_of(stage_getter get, float32 quantile) const -> float32;

    std::vector<frame_sample> samples_;
    mutable std::vector<float32> scratch_;
};

}  // namespace vw::gfx
