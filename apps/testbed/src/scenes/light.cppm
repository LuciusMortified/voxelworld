export module vw.testbed:scenes.light;

import std;

import vw.core;
import :app;
import :options;
import :scene;

export namespace vw::testbed {

// Стоит и зажигает мир. Стриминг закончился до того, как лёг первый блок,
// поэтому всё дальнейшее оплачено правкой — та же форма, что у dig, но вопрос
// другой.
//
// dig этот вопрос задать не может: снятие породы под землёй двигает небесный
// канал и оставляет канал блоков на том же нуле, где он и был. Что эта сцена
// на самом деле оценивает — это число квадов: лампа кладёт градиент на каждую
// поверхность в четырнадцати вокселях, а градиент стоит квадов, потому что
// уровень входит в ключ слияния. Прогнать дважды, второй раз с --bench-inert, и
// разница будет ценой света, а не блока.
class light_scene final : public scene {
public:
    light_scene(testbed_app& stand, light_options opts);

    [[nodiscard]] auto name() const -> std::string_view override {
        return "light";
    }

    auto drive_camera() -> void override;
    auto tick(float32 delta_time) -> void override;
    auto collect_report(gfx::report& out) const -> void override;
    auto ui() -> void override;

private:
    // Шестнадцать в стороне через четыре вокселя покрывают шестьдесят четыре
    // вокселя, то есть одну колонку поперёк: достаточно, чтобы перезаливка
    // пересекала швы, и достаточно мало, чтобы одни и те же колонки спрашивали
    // снова и снова — а именно так перезаливка от правок и выглядит.
    static constexpr int32 side    = 16;
    static constexpr int32 spacing = 4;
    static constexpr int32 cells   = side * side;

    auto start_() -> void;

    light_options opts_;

    int32 cursor_  = 0;
    uint64 placed_ = 0;
    bool started_  = false;

    uint64 mesh_base_        = 0;
    uint64 quads_base_       = 0;
    uint64 relight_base_     = 0;
    uint64 relit_chunk_base_ = 0;
    uint64 columns_base_     = 0;
    float32 flood_base_ms_   = 0.0f;
    float32 bake_base_ms_    = 0.0f;

    // Что построил стриминг, пока эмиттеров не было нигде. Прогон ниже
    // сравнивают с этим, и сравнение честно лишь до известного предела: это не
    // те же чанки. Честное сравнение — два прогона, один из них --bench-inert.
    float64 quads_per_chunk_base_ = 0.0;
};

}  // namespace vw::testbed
