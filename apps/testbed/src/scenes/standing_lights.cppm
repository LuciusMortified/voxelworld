export module vw.testbed:scenes.standing_lights;

import std;

import vw.core;
import vw.ecs;
import :app;
import :args;
import :camera;
import :scene;

export namespace vw::testbed {

// Мир, каким он выглядит, когда в нём кто-то пожил: эмиттеры рассыпаны по
// поверхности кругом от камеры, движущихся источников больше, чем пропустит
// отсев, и камера поворачивается, так что они входят в вид и выходят из него.
//
// Вопрос другой, чем у lamp-edits, которая оценивает одну правку. Здесь всё уже
// стоит до первого замерного кадра, и меряется установившееся состояние:
// фрагментный цикл по тому, что пережило отсев, на геометрии, чей ключ слияния
// всюду несёт градиенты света от блоков.
class standing_lights_scene : public scene {
public:
    standing_lights_scene(testbed_app& stand, const arg_reader& args);

    [[nodiscard]] auto name() const -> std::string_view override {
        return "standing-lights";
    }

    auto tick(float32 delta_time) -> void override;

    // Поворачивается, а не стоит: отсев, которого ни разу не попросили ничего
    // отбросить, — это отсев, который не измеряют. Наклон круче, чем у пустого
    // рельефа: смотреть тут стоит на землю, где лужи света.
    [[nodiscard]] auto default_camera() const -> camera_hint override {
        return {.rig = "spin", .pitch = -20.0f};
    }

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

    // Хутор — это горстка вокселей поперёк, а не сотня: смысл сцены со сбитыми
    // источниками в узле, достаточно тесном, чтобы один тайл держал их все.
    static constexpr int32 hamlet_spread = 10;

    // Сколько стоит шаг вне бенча. Орбиты шагают с кадром — единственное, что
    // замерной сцене позволено, — и ровно то, что не нужно сцене, на которую
    // смотрят: при шести сотнях кадров в секунду источники обходили весь диск
    // меньше чем за секунду, и разглядеть было нечего.
    static constexpr float32 steps_per_second = 60.0F;

    // Золотой угол по диску: ровная плотность, воспроизводимость до вокселя и
    // никаких двух точек, попавших друг на друга.
    [[nodiscard]] static auto spiral_point(int32 i, int32 count, float32 span) -> vec2f;

    // Ключи сцены: --emitters (эмиттеров), --moving-lights (движущихся
    // источников), --hamlets (узлов у сбитой в кучки сцены),
    // --emitters-per-frame (эмиттеров за кадр, пока сцена расставляется) и
    // --light-speed.
    [[nodiscard]] auto static_lights() const -> int32 {
        return static_lights_;
    }

    [[nodiscard]] auto hamlets() const -> int32 {
        return hamlets_;
    }

    // Где стоит эмиттер номер i и вокруг чего ходит движущийся источник.
    // Сбитая в кучки сцена переопределяет обе: в ней и то, и другое привязано к
    // хутору.
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

    int32 static_lights_  = 400;
    int32 dynamic_lights_ = 64;
    int32 hamlets_ = 24;
    int32 per_frame_      = 1;

    // Во сколько раз быстрее ходят движущиеся источники. Замерный прогон, где
    // это не единица, меряет другую сцену.
    float32 light_speed_ = 1.0F;

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
};

// То же установившееся состояние и та же камера, но источники стоят в двух
// десятках плотных групп, а не размазаны по диску ровно. В разлёте фроксель
// держит один источник и плоский тайл держит один источник, и срезы по глубине
// не покупают ничего; сбитые в хутор, тайл накрывает весь хутор на всякой
// глубине за ним, а фроксель — только ту его часть, что на этой глубине. Эта
// сцена и есть то место, где slices=24 и slices=1 вправе разойтись.
class clustered_lights_scene final : public standing_lights_scene {
public:
    using standing_lights_scene::standing_lights_scene;

    [[nodiscard]] auto name() const -> std::string_view override {
        return "clustered-lights";
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
