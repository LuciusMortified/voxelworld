#include <catch2/catch_test_macros.hpp>

import std;

import vw.core;
import vw.ecs;
import vw.world;

using namespace vw;
using namespace vw::ecs;

namespace {

constexpr int32 side = asset::chunk_occupancy::side;

// A column built by hand out of occupancy bits. Coordinates are the ones
// sky_light_column uses: y counts up from the bottom of the lowest chunk. The
// chunks are handed over top down, which the fixture does for the caller.
class column_fixture {
public:
    explicit column_fixture(int32 chunks) : occupancy_(static_cast<std::size_t>(chunks)) {}

    void set_solid(int32 x, int32 y, int32 z) {
        occupancy_[static_cast<std::size_t>(y / side)].set_row(y % side, z, uint64{1} << x);
    }

    void fill_solid(int32 x0, int32 y0, int32 z0, int32 x1, int32 y1, int32 z1) {
        for (int32 y = y0; y <= y1; ++y) {
            for (int32 z = z0; z <= z1; ++z) {
                for (int32 x = x0; x <= x1; ++x) {
                    set_solid(x, y, z);
                }
            }
        }
    }

    // Rock from the bottom of the column up to and including y.
    void floor_at(int32 y) {
        fill_solid(0, 0, 0, side - 1, y, side - 1);
    }

    [[nodiscard]] auto light() const -> asset::sky_light_column {
        std::vector<const asset::chunk_occupancy*> top_down;
        for (auto it = occupancy_.rbegin(); it != occupancy_.rend(); ++it) {
            top_down.push_back(&*it);
        }
        return asset::sky_light_column{std::span<const asset::chunk_occupancy* const>{top_down}};
    }

private:
    std::vector<asset::chunk_occupancy> occupancy_;
};

}  // namespace

TEST_CASE("open sky is fully lit and rock is dark", "[sky_light]") {
    column_fixture fixture{2};
    fixture.floor_at(40);

    const auto light = fixture.light();

    REQUIRE(light.height() == 128);
    REQUIRE(light.level_at(32, 41, 32) == 15);
    REQUIRE(light.level_at(32, 127, 32) == 15);
    REQUIRE(light.level_at(32, 40, 32) == 0);
    REQUIRE(light.level_at(32, 0, 32) == 0);
}

// Rule one on its own: a column that sees the sky is at 15 however deep it
// goes, with no falloff and no flood involved. This is the shaft of light
// through a hole in a ceiling.
TEST_CASE("a shaft open to the sky is lit to the bottom", "[sky_light]") {
    column_fixture fixture{2};

    for (int32 y = 0; y <= 100; ++y) {
        for (int32 z = 0; z < side; ++z) {
            for (int32 x = 0; x < side; ++x) {
                if (x == 32 && z == 32) {
                    continue;
                }
                fixture.set_solid(x, y, z);
            }
        }
    }

    const auto light = fixture.light();

    REQUIRE(light.level_at(32, 100, 32) == 15);
    REQUIRE(light.level_at(32, 50, 32) == 15);
    REQUIRE(light.level_at(32, 0, 32) == 15);
    REQUIRE(light.level_at(33, 50, 32) == 0);
}

// Rule two, and the one that matters. Under an overhang light is no longer
// under open sky, so a step costs one whichever way it goes -- down exactly as
// much as sideways. Getting this wrong gives shafts instead of a gradient.
//
// The wall is what makes the test say anything: without it the open side is
// skylit all the way down, every voxel of the pocket touches a 15, and going
// down would look free when it is not.
TEST_CASE("under an overhang light falls one level a step", "[sky_light]") {
    column_fixture fixture{2};
    fixture.floor_at(40);

    fixture.fill_solid(0, 60, 0, 31, 60, side - 1);
    fixture.fill_solid(32, 41, 0, 32, 58, side - 1);

    const auto light = fixture.light();

    SECTION("the open half is untouched") {
        REQUIRE(light.level_at(40, 59, 32) == 15);
        REQUIRE(light.level_at(40, 41, 32) == 15);
    }

    SECTION("stepping in under the roof costs one a voxel") {
        REQUIRE(light.level_at(31, 59, 32) == 14);
        REQUIRE(light.level_at(30, 59, 32) == 13);
        REQUIRE(light.level_at(29, 59, 32) == 12);
    }

    SECTION("going down under the roof costs the same as going in") {
        REQUIRE(light.level_at(31, 58, 32) == 13);
        REQUIRE(light.level_at(31, 57, 32) == 12);
    }

    SECTION("fifteen steps from the mouth is dark") {
        REQUIRE(light.level_at(18, 59, 32) == 1);
        REQUIRE(light.level_at(17, 59, 32) == 0);
    }
}

// What a height map cannot do. Every column under the lid is closed to the sky,
// so a height map would call the whole chamber dark. Light gets in the only way
// it can, sideways from the mouth, and runs out before the middle.
TEST_CASE("light reaches into a chamber a height map would call sealed", "[sky_light]") {
    column_fixture fixture{2};
    fixture.floor_at(40);

    fixture.fill_solid(10, 50, 10, 50, 50, 50);

    const auto light = fixture.light();

    SECTION("the lid itself carries nothing") {
        REQUIRE(light.level_at(30, 50, 30) == 0);
    }

    SECTION("light comes in under the lip and fades inward") {
        REQUIRE(light.level_at(9, 49, 30) == 15);
        REQUIRE(light.level_at(10, 49, 30) == 14);
        REQUIRE(light.level_at(11, 49, 30) == 13);
    }

    SECTION("the middle of the chamber is out of reach") {
        REQUIRE(light.level_at(30, 49, 30) == 0);
    }
}

// The number that decides where this field ends up living. A flat world has to
// come out nearly free: rock below, sky above, and one layer of pages where the
// two meet.
TEST_CASE("page counts say what a paged form would cost", "[sky_light]") {
    column_fixture fixture{2};
    fixture.floor_at(40);

    const auto stats = fixture.light().count_pages();

    REQUIRE(stats.lit + stats.dark + stats.uniform_other + stats.mixed == 1024);
    REQUIRE(stats.dark == 320);
    REQUIRE(stats.lit == 640);
    REQUIRE(stats.uniform_other == 0);
    REQUIRE(stats.mixed == 64);
}

// Not run by default: it generates real terrain and reports what a paged form
// would have to allocate, which is the number that decides where this field
// lives. Run with `world_tests "[.sky_light_measure]"`.
TEST_CASE("sky light page cost on real terrain", "[.sky_light_measure]") {
    asset::model_identity_pool identity_pool;
    asset::page_pool pages;
    perlin_terrain_generator gen{identity_pool, pages, perlin_terrain_generator::params{}};

    asset::sky_light_column::page_stats total;
    int32 columns  = 0;
    int32 chunks   = 0;
    uint64 flood_ns = 0;

    for (int32 cx = 0; cx < 4; ++cx) {
        for (int32 cz = 0; cz < 4; ++cz) {
            gen_column column{cx, cz};
            terrain_context ctx{
                .cx           = cx,
                .cz           = cz,
                .create_chunk = [&column](int32 y) -> chunk_data& {
                    return column.create_chunk(y, chunk_data{});
                },
            };
            gen.generate(ctx);

            std::vector<asset::chunk_occupancy> occupancy;
            std::vector<const asset::chunk_occupancy*> top_down;
            occupancy.reserve(column.get_all_chunk_data().size());

            for (auto& [cy, data] : column.get_all_chunk_data()) {
                auto& occ = occupancy.emplace_back();
                static_cast<void>(data.chunk_model->build_occupancy(occ));
            }
            for (auto& occ : occupancy) {
                top_down.push_back(&occ);
            }

            const auto started = std::chrono::steady_clock::now();
            const asset::sky_light_column light{
                std::span<const asset::chunk_occupancy* const>{top_down}
            };
            flood_ns += static_cast<uint64>(
                std::chrono::duration_cast<std::chrono::nanoseconds>(
                    std::chrono::steady_clock::now() - started
                )
                    .count()
            );

            const auto stats = light.count_pages();

            total.lit           += stats.lit;
            total.dark          += stats.dark;
            total.uniform_other += stats.uniform_other;
            total.mixed         += stats.mixed;

            ++columns;
            chunks += static_cast<int32>(top_down.size());
        }
    }

    const int32 all = total.lit + total.dark + total.uniform_other + total.mixed;

    WARN(
        "columns " << columns << ", chunks " << chunks << ", pages " << all
        << ": lit " << total.lit << ", dark " << total.dark
        << ", uniform_other " << total.uniform_other << ", mixed " << total.mixed
        << " (" << (100.0 * total.mixed / all) << "%), mixed bytes "
        << (total.mixed * 256) << ", flood " << (flood_ns / 1000 / columns) << " us a column"
    );

    REQUIRE(all > 0);
}
