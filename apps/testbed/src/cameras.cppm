export module vw.testbed:cameras;

// Пути камеры и таблица, по которой их выбирает командная строка.

import std;

import vw.core;
import :camera;

export namespace vw::testbed {

// Общее у идущих путей — просвет над землёй. Путь пролегает на высоте земли над
// началом координат, а холмы в километре оттуда выше: пока порода была
// сплошной, это не стоило ничего — внутри холма каждый чанк сплошной и не
// рисует ничего вовсе. С пещерами порода стала полой, и тот же путь мерил уже
// стены пещеры в упор — сто миллисекунд на кадр вместо стриминга, ради которого
// он есть.
inline constexpr float32 path_clearance = 400.0f;

// Стоит и смотрит. Самая низкая дисперсия из всех, поэтому именно это число
// сравнивают между сборками.
class parked_rig final : public camera_rig {
public:
    using camera_rig::camera_rig;

    [[nodiscard]] auto name() const -> std::string_view override {
        return "parked";
    }

    auto drive(const camera_hint& hint, float32 delta_time) -> void override;
};

// Стоит и поворачивается на месте: цена посмотреть в другую сторону, то есть
// ровно того заикания, которое замечают. Заодно единственный способ спросить
// отсев о чём-то новом, не сходя с места.
class spin_rig final : public camera_rig {
public:
    using camera_rig::camera_rig;

    [[nodiscard]] auto name() const -> std::string_view override {
        return "spin";
    }

    auto drive(const camera_hint& hint, float32 delta_time) -> void override;

private:
    uint64 frame_ = 0;
};

// Идёт по прямой всё время замера, поэтому мир впереди генерируется, мешится и
// уезжает на устройство непрерывно. Облёт кружит и возвращается на свои следы;
// этот — никогда.
class walk_rig final : public camera_rig {
public:
    using camera_rig::camera_rig;

    [[nodiscard]] auto name() const -> std::string_view override {
        return "walk";
    }

    auto drive(const camera_hint& hint, float32 delta_time) -> void override;

private:
    static constexpr float32 per_frame = 13.0f;

    uint64 frame_ = 0;
};

// Облёт по окружности: стриминг плюс хвосты p95 и p99.
class orbit_rig final : public camera_rig {
public:
    using camera_rig::camera_rig;

    [[nodiscard]] auto name() const -> std::string_view override {
        return "orbit";
    }

    auto drive(const camera_hint& hint, float32 delta_time) -> void override;

private:
    static constexpr float32 radius = 1500.0f;

    uint64 frame_ = 0;
};

// Камера человека: мышь и клавиши. Замерять этим нечего — путь у каждого прогона
// свой, — зато любую сцену можно облазить руками.
class free_rig final : public camera_rig {
public:
    using camera_rig::camera_rig;

    [[nodiscard]] auto name() const -> std::string_view override {
        return "free";
    }

    [[nodiscard]] auto needs_ground() const -> bool override {
        return false;
    }

    auto drive(const camera_hint& hint, float32 delta_time) -> void override;
};

[[nodiscard]] auto find_camera(std::string_view name) -> std::optional<camera_factory>;

// Для сообщения об ошибке: имя, не совпавшее ни с одним, — ошибка со списком, а
// не тихо другой путь.
[[nodiscard]] auto camera_names() -> std::vector<std::string_view>;

}  // namespace vw::testbed
