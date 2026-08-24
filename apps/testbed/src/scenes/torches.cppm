export module vw.testbed:scenes.torches;

import std;

import vw.core;
import vw.ecs;
import :app;
import :options;
import :scene;

export namespace vw::testbed {

// Мир, каким он выглядит, когда в нём кто-то пожил: эмиттеры рассыпаны по
// поверхности кругом от камеры, движущихся источников больше, чем пропустит
// отсев, и камера поворачивается, так что они входят в вид и выходят из него.
//
// Вопрос другой, чем у light, которая оценивает одну правку. Здесь всё уже
// стоит до первого замерного кадра, и меряется установившееся состояние:
// фрагментный цикл по тому, что пережило отсев, на геометрии, чей ключ слияния
// всюду несёт градиенты света от блоков.
class torches_scene : public scene {
public:
    torches_scene(testbed_app& stand, torches_options opts);

    [[nodiscard]] auto name() const -> std::string_view override {
        return "torches";
    }

    auto drive_camera() -> void override;
    auto tick(float32 delta_time) -> void override;

    // Гроза должна стоять и молчать до того, как что-то замеряют: четыреста
    // правок перезаливки внутри замерного окна утопили бы то, ради чего сцена
    // существует.
    [[nodiscard]] auto is_ready() const -> bool override;

    auto collect_report(gfx::report& out) const -> void override;
    auto ui() -> void override;

protected:
    // Девяносто шесть вокселей — чуть меньше двух колонок, что при этом
    // масштабе покрывает почти всё, что камера успевает увидеть до тумана.
    static constexpr int32 radius = 96;

    // Хутор — это горстка вокселей поперёк, а не сотня: смысл деревенской сцены
    // в узле источников, достаточно тесном, чтобы один тайл держал их все.
    static constexpr int32 village_spread = 10;

    // Сколько стоит шаг вне бенча. Орбиты шагают с кадром — единственное, что
    // замерной сцене позволено, — и ровно то, что не нужно сцене, на которую
    // смотрят: при шести сотнях кадров в секунду источники обходили весь диск
    // меньше чем за секунду, и разглядеть было нечего.
    static constexpr float32 steps_per_second = 60.0F;

    // Золотой угол по диску: ровная плотность, воспроизводимость до вокселя и
    // никаких двух точек, попавших друг на друга.
    [[nodiscard]] static auto spiral_point(int32 i, int32 count, float32 span) -> vec2f;

    [[nodiscard]] auto options() const -> const torches_options& {
        return opts_;
    }

    // Где стоит эмиттер номер i и вокруг чего ходит движущийся источник.
    // Деревня переопределяет обе: в ней и то, и другое привязано к хутору.
    [[nodiscard]] virtual auto site(int32 i) const -> vec2i;
    [[nodiscard]] virtual auto orbit_home(std::size_t i) const -> vec2f;
    [[nodiscard]] virtual auto orbit_radius(std::size_t i, float32 spread) const -> float32;

    [[nodiscard]] virtual auto layout_text() const -> std::string {
        return "a spiral";
    }

private:
    auto spawn_lights_() -> void;
    auto place_emitters_() -> void;
    auto drive_lights_(float32 delta_time) -> void;

    torches_options opts_;

    uint64 placed_  = 0;
    bool seeded_    = false;
    bool standing_  = false;
    // Шаги, а не кадры, и float64, потому что оно только растёт: при шестидесяти
    // в секунду долгий взгляд на сцену начал бы терять сам шаг в float32
    // где-то на десятом часу.
    float64 phase_  = 0.0;

    std::vector<int32> pending_;
    std::vector<ecs::entity> lights_;

    uint32 visible_peak_   = 0;
    uint64 visible_sum_    = 0;
    uint64 visible_frames_ = 0;
    uint32 capped_frames_  = 0;
    uint64 camera_frame_   = 0;
};

// То же установившееся состояние и та же камера, но источники стоят в двух
// десятках плотных групп, а не размазаны по диску ровно. В разлёте фроксель
// держит один источник и плоский тайл держит один источник, и срезы по глубине
// не покупают ничего; сбитые в хутор, тайл накрывает весь хутор на всякой
// глубине за ним, а фроксель — только ту его часть, что на этой глубине. Эта
// сцена и есть то место, где slices=24 и slices=1 вправе разойтись.
class village_scene final : public torches_scene {
public:
    using torches_scene::torches_scene;

    [[nodiscard]] auto name() const -> std::string_view override {
        return "village";
    }

protected:
    [[nodiscard]] auto site(int32 i) const -> vec2i override;
    [[nodiscard]] auto orbit_home(std::size_t i) const -> vec2f override;
    [[nodiscard]] auto orbit_radius(std::size_t i, float32 spread) const -> float32 override;
    [[nodiscard]] auto layout_text() const -> std::string override;

private:
    [[nodiscard]] auto centre_(int32 group) const -> vec2f;
};

}  // namespace vw::testbed
