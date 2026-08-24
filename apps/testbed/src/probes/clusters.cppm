export module vw.testbed:probes.clusters;

import std;

import vw.core;
import vw.gfx;

export namespace vw::testbed {

// Вычитка сетки источников: сколько она раздаёт, насколько заполнена и сходится
// ли с эталоном на CPU. Не сцена, а прибор: включается ключом и снимается с
// любой сцены, какая идёт.
class cluster_probe {
public:
    // stats — дешёвая половина: только счётчики. verify — она же плюс списки и
    // эталон раз в N кадров, и это кадр заикания, поэтому по умолчанию выключено.
    cluster_probe(bool stats, uint32 verify_every)
        : stats_{stats}, verify_every_{verify_every} {}

    [[nodiscard]] auto wanted() const -> bool {
        return stats_ || verify_every_ > 0;
    }

    [[nodiscard]] auto readback_level() const -> gfx::cluster_readback_level {
        return verify_every_ > 0 ? gfx::cluster_readback_level::full
                                 : gfx::cluster_readback_level::counts;
    }

    // Кадр приходит на полное кольцо позже, поэтому это всегда кадр, который
    // точно закончился, и никогда тот, что записывается сейчас. Оба списка
    // порознь: среднее по источникам и телам вместе не описывает ни одного.
    auto collect(gfx::renderer& renderer, bool measuring) -> void;

    auto collect_report(gfx::report& out) const -> void;

private:
    // Итог по одному списку.
    struct tally {
        spatial::cluster_grid grid{};
        uint32 cap = 0;

        uint64 frames          = 0;
        uint64 assignments     = 0;
        uint64 lit             = 0;
        uint32 peak            = 0;
        uint64 overflow        = 0;
        uint64 overflow_frames = 0;

        uint64 verified   = 0;
        uint64 bad_frames = 0;
        uint64 clusters   = 0;
        uint64 bad_counts = 0;
        uint64 bad_sets   = 0;
        spatial::cluster_check worst{};
    };

    auto account_(gfx::cull_list kind, const gfx::cluster_readback& frame) -> void;

    // Эталон — тот же код, который прибит юнит-тестами, и кормят его теми же
    // сферами в пространстве вида, которые компьютный проход построил себе сам.
    auto verify_frame_(gfx::cull_list kind, const gfx::cluster_readback& frame) -> void;

    auto report_list_(gfx::report& out, gfx::cull_list kind, std::string_view what) const -> void;

    bool stats_           = false;
    uint32 verify_every_  = 0;

    std::array<tally, gfx::cull_list_count> tally_{};
    std::array<std::unique_ptr<spatial::cluster_lights>, gfx::cull_list_count> reference_;
};

}  // namespace vw::testbed
