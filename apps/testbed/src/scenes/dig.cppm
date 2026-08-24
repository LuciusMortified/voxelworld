export module vw.testbed:scenes.dig;

import std;

import vw.core;
import :app;
import :args;
import :scene;

export namespace vw::testbed {

// Стоит и разбирает мир. Стриминг закончился до того, как ушёл первый воксель,
// поэтому каждый меш, построенный дальше, оплачен правкой — а это то число, по
// которому разрушаемый мир живёт или не живёт, и до этой сцены его не мерил
// никто.
//
// Коробка земли снимается по вокселю, растровым порядком, верхним слоем вперёд,
// и шагает по номеру кадра, как всякая замерная сцена: копка по стенным часам
// уносит на каждой машине разное число вокселей, и числа на правку перестают
// что-либо значить.
//
// Коробка стоит на начале координат, где сходятся четыре колонки, поэтому швы
// чанков пересекаются постоянно, а не по случайности. Воздух перешагивается, а
// не копается: model::set_voxel поднимает поколение, что бы ни записал, поэтому
// запись воздуха по воздуху заказала бы перестроение меша впустую и приукрасила
// бы среднее на правку.
class dig_scene final : public scene {
public:
    dig_scene(testbed_app& stand, const arg_reader& args);

    [[nodiscard]] auto name() const -> std::string_view override {
        return "dig";
    }

    auto drive_camera() -> void override;
    auto tick(float32 delta_time) -> void override;
    auto collect_report(gfx::report& out) const -> void override;
    auto ui() -> void override;

private:
    // Тридцать два вокселя в стороне — это два чанка поперёк в худшем случае и
    // один в лучшем, то есть тот размах, где шов пересекается достаточно часто,
    // чтобы себя показать.
    static constexpr int32 side  = 32;
    static constexpr int32 cells = side * side * side;

    auto start_() -> void;

    // Вокселей за кадр: --bench-dig.
    int32 per_frame_ = 1;

    int32 cursor_    = 0;
    int32 top_voxel_ = 0;
    uint64 edits_    = 0;
    bool started_    = false;

    uint64 mesh_base_        = 0;
    uint64 relight_base_     = 0;
    uint64 relit_chunk_base_ = 0;
    uint64 light_base_       = 0;
};

}  // namespace vw::testbed
