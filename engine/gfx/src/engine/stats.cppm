export module vw.gfx:engine.stats;

import std;

import vw.core;

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

    // Из-за стриминга память — это кривая, а не число: значение имеет пик за
    // весь полёт, а не то, что занято на последнем кадре.
    uint64 ram_peak_bytes  = 0;
    uint64 vram_peak_bytes = 0;

    // Рабочий набор — то, что резидентно; commit charge — то, что процесс
    // запросил. Расхождение означает, что аллокатор держит уже отданную память.
    uint64 commit_bytes      = 0;
    uint64 commit_peak_bytes = 0;
};

// Превращает движок в офлайновый бенчмарк: прогнать фиксированное число кадров,
// записать отчёт, выйти. Без measure_frames замеры выключены.
struct bench_config {
    uint32 warmup_frames  = 0;
    uint32 measure_frames = 0;
    std::string report_path;

    // Ноль оставляет каждой очереди её умолчание. Настройки здесь ради того,
    // чтобы снимать кривую без пересборки; её сняли, и колено у неё — четыре.
    uint32 mesh_workers    = 0;
    uint32 terrain_workers = 0;

    // Подаёт миру фиксированный шаг вместо измеренного, чтобы работа симуляции
    // не плыла вслед за частотой кадров, которую этот шаг и меряет.
    float32 fixed_delta_seconds = 0.0f;

    [[nodiscard]] auto enabled() const -> bool {
        return measure_frames > 0;
    }
};

}  // namespace vw::gfx
