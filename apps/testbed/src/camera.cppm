export module vw.testbed:camera;

import std;

import vw.core;

export namespace vw::testbed {

class testbed_app;

// Откуда сцену смотреть. Сцена называет режим и его параметры, а командная
// строка вправе назвать другой режим: параметры при этом остаются сценины —
// она одна знает, на какой высоте и под каким наклоном в ней есть что видеть.
struct camera_hint {
    std::string_view rig = "parked";

    // Смещение от точки съёмки, то есть от земли над началом координат.
    vec3f offset{0.0f, 0.0f, 0.0f};

    float32 pitch = -10.0f;

    // Куда смотрит неподвижная камера. Идущие и вращающиеся ведут курс сами:
    // он у них следует из пути.
    float32 yaw = 0.0f;

    float32 degrees_per_frame = 0.25f;
};

// Путь, которым камера идёт по сцене. Путь отделён от того, что в сцене стоит:
// раньше каждый из них был отдельным классом сцены, и посмотреть на свет
// облётом было нельзя — облёт умел только пустой рельеф.
class camera_rig {
public:
    explicit camera_rig(testbed_app& stand) : stand_{&stand} {}

    virtual ~camera_rig() = default;

    camera_rig(const camera_rig&)                    = delete;
    auto operator=(const camera_rig&) -> camera_rig& = delete;
    camera_rig(camera_rig&&)                         = delete;
    auto operator=(camera_rig&&) -> camera_rig&      = delete;

    [[nodiscard]] virtual auto name() const -> std::string_view = 0;

    // Рельсовый путь отмеряется от земли над началом координат, и до того, как
    // колонка под камерой загрузилась, вести его некуда. Свободному это не
    // нужно: он ходит с первого кадра.
    [[nodiscard]] virtual auto needs_ground() const -> bool {
        return true;
    }

    // Камера ведётся по номеру кадра, а не по часам: путь, который ведут по
    // стенным часам, на каждой машине разный, и мерить его нельзя.
    //
    // Кадры считает сам риг: тем путям, что трогаются с места только после
    // загрузки мира, счётчик должен стоять всё то время, пока они ждут. Общий
    // счётчик уносил такую камеру на тысячу кадров вперёд — за край
    // загруженного мира, где кадр пустой и дешёвый, а замер бессмысленный.
    virtual auto drive(const camera_hint& hint, float32 delta_time) -> void = 0;

protected:
    [[nodiscard]] auto stand() const -> testbed_app& {
        return *stand_;
    }

private:
    testbed_app* stand_;
};

// Риг строит стенд, когда сцена уже стоит: умолчание режима спрашивают у неё.
using camera_factory = std::function<auto(testbed_app&)->std::unique_ptr<camera_rig>>;

}  // namespace vw::testbed
