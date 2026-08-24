export module vw.gfx:engine.app;

import vw.core;

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

protected:
    [[nodiscard]] auto get_engine() const -> engine_type& {
        return *engine_;
    }

private:
    engine_type* engine_ = nullptr;
};

}  // namespace vw::gfx
