export module vw.gfx:engine.app;

import vw.core;
import :engine.report;

export namespace vw::gfx {

class engine;

// База приложения: движок владеет циклом кадра и зовёт render каждый кадр.
class app {
public:
    using engine_type = engine;

    explicit app(engine_type& eng) : engine_(&eng) {}

    virtual ~app() = default;

    app(const app&)                    = delete;
    auto operator=(const app&) -> app& = delete;

    virtual auto render([[maybe_unused]] float32 delta_time) -> void {}

    // Бенчмарк откладывает прогрев, пока сцена не объявит себя устоявшейся:
    // недогруженный стриминг не должен просочиться в замеры.
    [[nodiscard]] virtual auto is_bench_ready() const -> bool {
        return true;
    }

    // Что приложение хочет сказать в отчёте о прогоне. Зовётся до того, как
    // отчёт записан, и потому попадает и в текст, и в JSON.
    //
    // Раньше приложению оставалось печатать своё в stdout из деструктора, то
    // есть уже после записи файла: числа сцены — то, ради чего сцена и
    // существует, — не попадали в отчёт вовсе.
    virtual auto collect_report([[maybe_unused]] report& out) const -> void {}

protected:
    [[nodiscard]] auto get_engine() const -> engine_type& {
        return *engine_;
    }

private:
    engine_type* engine_ = nullptr;
};

}  // namespace vw::gfx
