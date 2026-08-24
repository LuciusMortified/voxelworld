export module vw.testbed:scenes.crowd;

import std;

import vw.core;
import vw.ecs;
import vw.world;
import :app;
import :args;
import :scene;

export namespace vw::testbed {

// Толпа анимированных физических тел: сцена, о которой продукт и говорит, и
// единственная, где работа CPU на сущность вообще видна.
//
// Тела сбрасываются в воздухе и должны приземлиться до того, как их начнут
// мерить: пойманные в падении, они кладут в каждый прогон разное количество
// работы, и разброс топит то, ради чего сцена существует.
class crowd_scene final : public scene {
public:
    crowd_scene(testbed_app& stand, const arg_reader& args);

    [[nodiscard]] auto name() const -> std::string_view override {
        return "crowd";
    }

    auto drive_camera() -> void override;
    auto tick(float32 delta_time) -> void override;
    auto on_world_ready() -> void override;

    [[nodiscard]] auto is_ready() const -> bool override {
        return spawned_ && settle_frames_ >= settle_target;
    }

    auto ui() -> void override;

private:
    static constexpr uint32 settle_target = 400;

    static constexpr std::array<std::string_view, 4> target_names{
        "body", "head", "hand_left", "hand_right",
    };

    [[nodiscard]] static auto make_clip_(ecs::world& world)
        -> std::shared_ptr<asset::animation_clip>;

    auto spawn_(float32 ground_y) -> void;

    // Тел: --bench-crowd. Полсотни по умолчанию — столько эта сцена и мерила,
    // когда размер задавался снаружи.
    uint32 size_ = 50;

    std::vector<ecs::entity> bodies_;
    uint32 settle_frames_ = 0;
    bool spawned_         = false;
};

}  // namespace vw::testbed
