export module vw.testbed:scenes.traverse;

import std;

import vw.core;
import :app;
import :args;
import :scene;

export namespace vw::testbed {

// Четыре сцены, которые ничего не строят и ничего не правят: они только водят
// камеру. Всё, чем они отличаются друг от друга, — путь, и потому каждая из них
// это ровно один метод.
//
// Общее у них — высота, на которой идёт съёмка, и просвет над землёй. Путь
// пролегает на высоте земли над началом координат, а холмы в километре оттуда
// выше: пока порода была сплошной, это не стоило ничего — внутри холма каждый
// чанк сплошной и не рисует ничего вовсе. С пещерами порода стала полой, и тот
// же путь мерил уже стены пещеры в упор — сто миллисекунд на кадр вместо
// стриминга, ради которого он есть.
class traverse_scene : public scene {
public:
    // Ключей у путей нет: всё их поведение — в них самих. Аргументы всё равно
    // принимаются, чтобы сцену строили одинаково, какой бы она ни была.
    traverse_scene(testbed_app& stand, const arg_reader& /*args*/) : scene{stand} {}

protected:
    static constexpr float32 clearance         = 400.0f;
    static constexpr float32 radius            = 1500.0f;
    static constexpr float32 advance_per_frame = 13.0f;
    static constexpr float32 degrees_per_frame = 0.25f;

    [[nodiscard]] auto eye_height() const -> float32 {
        return stand().altitude();
    }

    // Кадры пути, а не кадры приложения: они начинают идти, когда сцена
    // тронулась с места.
    uint64 frame_ = 0;
};

// Стоит и смотрит. Самая низкая дисперсия из всех, поэтому именно это число
// сравнивают между сборками.
class parked_scene final : public traverse_scene {
public:
    using traverse_scene::traverse_scene;

    [[nodiscard]] auto name() const -> std::string_view override {
        return "parked";
    }

    auto drive_camera() -> void override;
};

// Стоит и поворачивается на месте: цена посмотреть в другую сторону, то есть
// ровно того заикания, которое замечают.
class spin_scene final : public traverse_scene {
public:
    using traverse_scene::traverse_scene;

    [[nodiscard]] auto name() const -> std::string_view override {
        return "spin";
    }

    auto drive_camera() -> void override;
};

// Идёт по прямой всё время замера, поэтому мир впереди генерируется, мешится и
// уезжает на устройство непрерывно. Облёт кружит и возвращается на свои следы;
// эта — никогда.
class advance_scene final : public traverse_scene {
public:
    using traverse_scene::traverse_scene;

    [[nodiscard]] auto name() const -> std::string_view override {
        return "advance";
    }

    auto drive_camera() -> void override;
};

// Облёт по окружности: стриминг плюс хвосты p95 и p99.
class flythrough_scene final : public traverse_scene {
public:
    using traverse_scene::traverse_scene;

    [[nodiscard]] auto name() const -> std::string_view override {
        return "flythrough";
    }

    auto drive_camera() -> void override;
};

}  // namespace vw::testbed
