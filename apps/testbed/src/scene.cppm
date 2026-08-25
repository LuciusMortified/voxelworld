export module vw.testbed:scene;

import std;

import vw.core;
import vw.gfx;
import :camera;

export namespace vw::testbed {

class testbed_app;

// Одна сцена стенда: что стоит в мире, откуда это смотрят и что сцена про себя
// рассказывает в конце прогона.
//
// Стенд отдаёт сцене себя в конструкторе и дальше только зовёт: он не знает,
// какая сцена внутри, и не разводит их условиями у себя. Раньше девять сцен
// жили одним enum и ветвились в четырёх местах сразу — в проводке камеры, в
// готовности к замеру, в покадровом обходе и в деструкторе, — и добавление
// десятой означало правку всех четырёх.
class scene {
public:
    explicit scene(testbed_app& stand) : stand_{&stand} {}

    virtual ~scene() = default;

    scene(const scene&)                    = delete;
    auto operator=(const scene&) -> scene& = delete;
    scene(scene&&)                         = delete;
    auto operator=(scene&&) -> scene&      = delete;

    [[nodiscard]] virtual auto name() const -> std::string_view = 0;

    // Кадр сцены. Зовётся всегда, в том числе пока мир ещё грузится.
    virtual auto tick([[maybe_unused]] float32 delta_time) -> void {}

    // Откуда сцену смотреть, пока не сказано иное. Сам путь ведёт риг, и
    // --camera вправе назвать любой; наклон, смещение и скорость поворота
    // остаются сценины — она одна знает, что в ней есть смотреть.
    [[nodiscard]] virtual auto default_camera() const -> camera_hint {
        return {};
    }

    // Сцена устоялась и её можно мерить. Стриминг стенд проверяет сам, здесь —
    // только то, что знает про себя сцена: встали ли тела, замолчал ли свет.
    [[nodiscard]] virtual auto is_ready() const -> bool {
        return true;
    }

    // Мир вокруг догрузился. Отсюда сцене можно расставлять своё содержимое:
    // до этого момента под ним может не быть земли.
    virtual auto on_world_ready() -> void {}

    // Блок сцены в общий отчёт. Зовётся до того, как отчёт записан.
    virtual auto collect_report([[maybe_unused]] gfx::report& out) const -> void {}

    // Своя панель в интерфейсе стенда.
    virtual auto ui() -> void {}

protected:
    [[nodiscard]] auto stand() const -> testbed_app& {
        return *stand_;
    }

private:
    testbed_app* stand_;
};

// Сцену строит стенд, когда сам уже готов, поэтому командная строка выбирает не
// сцену, а способ её построить.
using scene_factory = std::function<auto(testbed_app&)->std::unique_ptr<scene>>;

}  // namespace vw::testbed
