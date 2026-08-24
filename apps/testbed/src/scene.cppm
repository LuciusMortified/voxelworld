export module vw.testbed:scene;

import std;

import vw.core;
import vw.gfx;
import :options;

export namespace vw::testbed {

class testbed_app;

// Одна сцена стенда: что стоит в мире, куда смотрит камера и что она про себя
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

    // Камера ведётся по номеру кадра, а не по часам: путь, который ведут по
    // стенным часам, на каждой машине разный, и мерить его нельзя.
    //
    // Кадры считает сама сцена, а не стенд: тем, кто трогается с места только
    // после загрузки мира, счётчик должен стоять всё то время, пока они ждут.
    // Общий счётчик уносил такую камеру на тысячу кадров вперёд — за край
    // загруженного мира, где кадр пустой и дешёвый, а замер бессмысленный.
    virtual auto drive_camera() -> void {}

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
