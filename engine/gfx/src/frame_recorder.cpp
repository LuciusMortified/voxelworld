module vw.gfx;

import std;
import vw.core;
import :engine;

namespace vw::gfx {

namespace {

struct stage_desc {
    std::string_view name;
    auto (*get)(const frame_sample&) -> float32;
};

constexpr std::array stages{
    stage_desc{"frame", [](const frame_sample& s) -> float32 { return s.engine.frame_ms; }},
    stage_desc{"world_update", [](const frame_sample& s) -> float32 { return s.engine.world_update_ms; }},
    stage_desc{"world_render", [](const frame_sample& s) -> float32 { return s.engine.world_render_ms; }},
    stage_desc{"begin_frame", [](const frame_sample& s) -> float32 { return s.engine.begin_frame_ms; }},
    stage_desc{"app_render", [](const frame_sample& s) -> float32 { return s.engine.app_render_ms; }},
    stage_desc{"renderer", [](const frame_sample& s) -> float32 { return s.engine.renderer_ms; }},
    stage_desc{"end_frame", [](const frame_sample& s) -> float32 { return s.engine.end_frame_ms; }},
    stage_desc{"shadow_map_update", [](const frame_sample& s) -> float32 { return s.render.shadow_map_update_ms; }},
    stage_desc{"buffer_pool_update", [](const frame_sample& s) -> float32 { return s.render.buffer_pool_update_ms; }},
    stage_desc{"compute_cull", [](const frame_sample& s) -> float32 { return s.render.compute_cull_ms; }},
    stage_desc{"shadow_pass", [](const frame_sample& s) -> float32 { return s.render.shadow_pass_ms; }},
    stage_desc{"world_pass", [](const frame_sample& s) -> float32 { return s.render.world_pass_ms; }},
    stage_desc{"world_pass_uniform", [](const frame_sample& s) -> float32 { return s.render.world_pass_uniform_ms; }},
    stage_desc{"world_pass_geometry", [](const frame_sample& s) -> float32 { return s.render.world_pass_geometry_ms; }},
    stage_desc{"world_pass_debug", [](const frame_sample& s) -> float32 { return s.render.world_pass_debug_ms; }},
    stage_desc{"world_pass_imgui", [](const frame_sample& s) -> float32 { return s.render.world_pass_imgui_ms; }},
};

}  // namespace

frame_recorder::frame_recorder(
    uint32 capacity
) {
    samples_.reserve(capacity);
    scratch_.reserve(capacity);
}

auto frame_recorder::record(const frame_sample& sample) -> void {
    samples_.push_back(sample);
}

auto frame_recorder::sample_count() const -> uint32 {
    return static_cast<uint32>(samples_.size());
}

auto frame_recorder::percentile_of(stage_getter get, float32 quantile) const -> float32 {
    if (samples_.empty()) {
        return 0.0f;
    }

    scratch_.clear();
    for (const auto& sample : samples_) {
        scratch_.push_back(get(sample));
    }
    std::ranges::sort(scratch_);

    const auto count = static_cast<float32>(scratch_.size());
    const auto rank  = static_cast<uint64>(std::ceil(quantile * count));
    const auto index = std::clamp<uint64>(rank, 1, scratch_.size()) - 1;
    return scratch_[index];
}

auto frame_recorder::report() const -> std::string {
    std::string out;
    auto sink = std::back_inserter(out);

    if (samples_.empty()) {
        return "no samples recorded\n";
    }

    std::format_to(sink, "samples: {}\n", samples_.size());

    const auto frame_p50 = percentile_of(stages.front().get, 0.50f);
    if (frame_p50 > 0.0f) {
        std::format_to(sink, "fps at median frame: {:.1f}\n", 1000.0f / frame_p50);
    }

    std::format_to(
        sink, "\n{:<21}{:>9}{:>9}{:>9}{:>9}\n", "stage (ms)", "p50", "p95", "p99", "max"
    );

    for (const auto& stage : stages) {
        std::format_to(
            sink,
            "{:<21}{:9.3f}{:9.3f}{:9.3f}{:9.3f}\n",
            stage.name,
            percentile_of(stage.get, 0.50f),
            percentile_of(stage.get, 0.95f),
            percentile_of(stage.get, 0.99f),
            percentile_of(stage.get, 1.00f)
        );
    }

    return out;
}

}  // namespace vw::gfx
