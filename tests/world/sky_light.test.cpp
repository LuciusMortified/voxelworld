#include <catch2/catch_test_macros.hpp>

import std;

import vw.core;
import vw.ecs;
import vw.world;

using namespace vw;
using namespace vw::ecs;

namespace {

constexpr int32 side = asset::chunk_occupancy::side;

// A column built by hand out of occupancy bits, with nothing around it. The
// four sides come out sealed, which is what the tests below are about.
// Coordinates are the ones sky_light_column uses: y counts up from the bottom.
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
        std::vector<const asset::chunk_occupancy*> bottom_up;
        for (const auto& occ : occupancy_) {
            bottom_up.push_back(&occ);
        }
        return asset::sky_light_column{
            std::span<const asset::chunk_occupancy* const>{bottom_up}
        };
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

namespace {

// Nine columns in one continuous voxel space, so a cave can be dug across a
// seam without thinking about which column it lands in. x and z run 0..191, and
// the middle column -- the one under examination -- is 64..127 in both.
class world_fixture {
public:
    static constexpr int32 wide = side * 3;

    explicit world_fixture(int32 chunks)
        : chunks_{chunks}, occupancy_(static_cast<std::size_t>(9 * chunks)) {
        heights_.fill(chunks);
    }

    void set_solid(int32 x, int32 y, int32 z) {
        occupancy_[slot(x / side, z / side, y / side)].set_row(
            y % side, z % side, uint64{1} << (x % side)
        );
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

    void floor_at(int32 y) {
        fill_solid(0, 0, 0, wide - 1, y, wide - 1);
    }

    // A lid over the whole neighbourhood with one square hole left in it. Built
    // as four rectangles because occupancy bits only ever go on.
    void lid_with_hole(int32 y, int32 hx0, int32 hz0, int32 hx1, int32 hz1) {
        fill_solid(0, y, 0, hx0 - 1, y, wide - 1);
        fill_solid(hx1 + 1, y, 0, wide - 1, y, wide - 1);
        fill_solid(hx0, y, 0, hx1, y, hz0 - 1);
        fill_solid(hx0, y, hz1 + 1, hx1, y, wide - 1);
    }

    // How many chunks of a column the flood is allowed to see. Anything above
    // that is open air, which is what a plain beside a mountain looks like.
    void set_column_height(int32 cx, int32 cz, int32 chunks) {
        heights_[static_cast<std::size_t>((cz * 3) + cx)] = chunks;
    }

    [[nodiscard]] auto height() const -> int32 {
        return chunks_ * side;
    }

    [[nodiscard]] auto solid_at(int32 x, int32 y, int32 z) const -> bool {
        if (x < 0 || x >= wide || z < 0 || z >= wide || y < 0 || y >= height()) {
            return true;
        }
        const int32 cx = x / side;
        const int32 cz = z / side;
        if (y / side >= heights_[static_cast<std::size_t>((cz * 3) + cx)]) {
            return false;
        }
        return occupancy_[slot(cx, cz, y / side)].test(x % side, y % side, z % side);
    }

    // The nine columns as sky_light_column wants them. drop_diagonals turns the
    // four corner columns into rock, which is how the tests below show the
    // corners are being read at all.
    [[nodiscard]] auto light(bool drop_diagonals = false) const -> asset::sky_light_column {
        std::vector<std::vector<const asset::chunk_occupancy*>> held(9);
        asset::sky_light_column::neighbourhood around{};

        for (int32 i = 0; i < 9; ++i) {
            const bool corner = (i % 3) != 1 && (i / 3) != 1;
            if (drop_diagonals && corner) {
                continue;
            }
            for (int32 y = 0; y < heights_[static_cast<std::size_t>(i)]; ++y) {
                held[static_cast<std::size_t>(i)].push_back(&occupancy_[slot(i % 3, i / 3, y)]);
            }
            around[static_cast<std::size_t>(i)] = held[static_cast<std::size_t>(i)];
        }

        return asset::sky_light_column{around};
    }

    [[nodiscard]] auto sealed() const -> asset::sky_light_column {
        std::vector<const asset::chunk_occupancy*> middle;
        for (int32 y = 0; y < heights_[4]; ++y) {
            middle.push_back(&occupancy_[slot(1, 1, y)]);
        }
        return asset::sky_light_column{std::span<const asset::chunk_occupancy* const>{middle}};
    }

    // A second implementation, on purpose the dumbest one that can be right: a
    // dense array over all nine columns, rule one by walking each voxel column
    // down from the top, then a breadth-first walk. No bit tricks, no skirt, and
    // no code shared with the thing it checks. The middle column sits sixty-four
    // voxels from this array's own edge, further than light travels, so what it
    // says there is the true answer.
    [[nodiscard]] auto reference() const -> std::vector<uint8> {
        const int32 h = height();
        std::vector<uint8> level(
            static_cast<std::size_t>(wide) * wide * static_cast<std::size_t>(h), 0
        );

        std::vector<std::size_t> current;
        for (int32 x = 0; x < wide; ++x) {
            for (int32 z = 0; z < wide; ++z) {
                for (int32 y = h - 1; y >= 0; --y) {
                    if (solid_at(x, y, z)) {
                        break;
                    }
                    level[flat(x, y, z)] = asset::sky_light_column::max_level;
                    current.push_back(flat(x, y, z));
                }
            }
        }

        static constexpr int32 steps[6][3] = {
            {1, 0, 0}, {-1, 0, 0}, {0, 1, 0}, {0, -1, 0}, {0, 0, 1}, {0, 0, -1},
        };

        std::vector<std::size_t> next;
        for (uint8 value = asset::sky_light_column::max_level; value > 1 && !current.empty();
             --value) {
            const auto child = static_cast<uint8>(value - 1);
            next.clear();

            for (const std::size_t from : current) {
                const auto x = static_cast<int32>(from % wide);
                const auto z = static_cast<int32>((from / wide) % wide);
                const auto y = static_cast<int32>(from / (static_cast<std::size_t>(wide) * wide));

                for (const auto& step : steps) {
                    const int32 nx = x + step[0];
                    const int32 ny = y + step[1];
                    const int32 nz = z + step[2];
                    if (nx < 0 || nx >= wide || nz < 0 || nz >= wide || ny < 0 || ny >= h) {
                        continue;
                    }
                    if (solid_at(nx, ny, nz)) {
                        continue;
                    }

                    const auto to = flat(nx, ny, nz);
                    if (level[to] >= child) {
                        continue;
                    }
                    level[to] = child;
                    next.push_back(to);
                }
            }

            current.swap(next);
        }

        return level;
    }

    [[nodiscard]] static auto flat(int32 x, int32 y, int32 z) -> std::size_t {
        return (((static_cast<std::size_t>(y) * wide) + static_cast<std::size_t>(z)) * wide) +
            static_cast<std::size_t>(x);
    }

private:
    [[nodiscard]] auto slot(int32 cx, int32 cz, int32 y) const -> std::size_t {
        return static_cast<std::size_t>(((((cz * 3) + cx) * chunks_) + y));
    }

    int32 chunks_;
    std::array<int32, 9> heights_{};
    std::vector<asset::chunk_occupancy> occupancy_;
};

// Every voxel of the middle column against the reference. Returns how many
// disagreed, and names the first one on the way past.
auto middle_mismatches(
    const asset::sky_light_column& light, const world_fixture& fixture,
    const std::vector<uint8>& reference
) -> int32 {
    int32 mismatches = 0;

    for (int32 y = 0; y < fixture.height(); ++y) {
        for (int32 z = 0; z < side; ++z) {
            for (int32 x = 0; x < side; ++x) {
                const uint8 want = reference[world_fixture::flat(x + side, y, z + side)];
                const uint8 got  = light.level_at(x, y, z);
                if (got == want) {
                    continue;
                }
                if (mismatches == 0) {
                    UNSCOPED_INFO(
                        "first mismatch at " << x << "," << y << "," << z << ": got "
                                             << int32{got} << ", want " << int32{want}
                    );
                }
                ++mismatches;
            }
        }
    }

    return mismatches;
}

}  // namespace

// The whole point of the skirt, put as strongly as it can be: the middle column
// lit with fifteen voxels of its neighbours has to come out identical to the
// middle of a flood over all nine columns at once. Not close. Identical.
TEST_CASE("the skirt reproduces a flood over the whole neighbourhood", "[sky_light]") {
    world_fixture fixture{2};
    fixture.floor_at(40);

    // A lid whose mouth is nine voxels outside the middle column. Nothing under
    // it is open to the sky, so every level inside the middle column arrived
    // from a neighbour -- which is exactly what a sealed flood cannot know.
    fixture.fill_solid(56, 50, 20, 170, 50, 170);

    const auto reference = fixture.reference();

    SECTION("the skirt agrees everywhere") {
        REQUIRE(middle_mismatches(fixture.light(), fixture, reference) == 0);
    }

    SECTION("light really did come in from outside") {
        const auto light = fixture.light();
        REQUIRE(light.level_at(0, 49, 32) == 6);
        REQUIRE(light.level_at(5, 49, 32) == 1);
        REQUIRE(light.level_at(6, 49, 32) == 0);
    }

    // Without this the section above proves nothing. A check that also passes
    // on the broken version is not a check.
    SECTION("a sealed column gets it wrong") {
        REQUIRE(middle_mismatches(fixture.sealed(), fixture, reference) > 0);
    }
}

// The corners of the skirt come from the four diagonal columns, and a path can
// turn a corner. Leave them out and nothing notices until a cave mouth happens
// to sit on a diagonal.
TEST_CASE("the skirt corners come from the diagonal neighbours", "[sky_light]") {
    world_fixture fixture{2};
    fixture.floor_at(40);

    // Sealed over everything but one hole, and the hole is in the corner column
    // so the only way into the middle one is across a diagonal.
    fixture.lid_with_hole(50, 58, 58, 61, 61);

    const auto reference = fixture.reference();

    SECTION("with the diagonals it agrees") {
        REQUIRE(middle_mismatches(fixture.light(), fixture, reference) == 0);
    }

    SECTION("the corner is carrying real light") {
        REQUIRE(fixture.light().level_at(0, 49, 0) == 9);
    }

    SECTION("without the diagonals it does not agree") {
        REQUIRE(middle_mismatches(fixture.light(true), fixture, reference) > 0);
    }
}

// A short column beside tall ones. Above the top of a column that exists there
// is open sky, not rock: get that backwards and the seam between a plain and a
// mountain draws itself as a dark wall.
TEST_CASE("a column shorter than its neighbours is not walled off", "[sky_light]") {
    world_fixture fixture{3};
    fixture.floor_at(40);
    fixture.fill_solid(0, 41, 0, 63, 170, world_fixture::wide - 1);
    fixture.set_column_height(1, 1, 2);

    const auto reference = fixture.reference();

    REQUIRE(middle_mismatches(fixture.light(), fixture, reference) == 0);
    REQUIRE(fixture.light().level_at(32, 130, 32) == 15);
}

// Not run by default: it generates real terrain and reports what the skirt
// costs and what a paged form would have to allocate. Those two numbers decide
// where this field lives and how it is scheduled, so they get measured rather
// than guessed. Run with `world_tests "[.sky_light_measure]"`.
TEST_CASE("sky light cost on real terrain", "[.sky_light_measure]") {
    static constexpr int32 grid = 4;

    asset::model_identity_pool identity_pool;
    asset::page_pool pages;
    perlin_terrain_generator gen{identity_pool, pages, perlin_terrain_generator::params{}};

    std::unordered_map<vec2i, std::unique_ptr<gen_column>> columns;
    int32 bottom = std::numeric_limits<int32>::max();

    for (int32 cx = 0; cx < grid; ++cx) {
        for (int32 cz = 0; cz < grid; ++cz) {
            auto column = std::make_unique<gen_column>(cx, cz);
            terrain_context ctx{
                .cx           = cx,
                .cz           = cz,
                .create_chunk = [&column](int32 y) -> chunk_data& {
                    return column->create_chunk(y, chunk_data{});
                },
            };
            gen.generate(ctx);

            for (const auto& [cy, data] : column->get_all_chunk_data()) {
                bottom = std::min(bottom, cy);
            }
            columns.emplace(vec2i{cx, cz}, std::move(column));
        }
    }

    // Occupancy once per chunk, bottom up from the floor every column shares.
    std::unordered_map<vec2i, std::vector<asset::chunk_occupancy>> occupancy;
    int32 chunk_count = 0;

    for (auto& [coord, column] : columns) {
        auto& stack = occupancy[coord];
        for (const auto& [cy, data] : column->get_all_chunk_data()) {
            const auto slot = static_cast<std::size_t>(cy - bottom);
            if (stack.size() <= slot) {
                stack.resize(slot + 1);
            }
            static_cast<void>(data.chunk_model->build_occupancy(stack[slot]));
            ++chunk_count;
        }
    }

    asset::sky_light_column::page_stats total;
    uint64 flood_ns = 0;
    int32 lit       = 0;

    // Only the columns that have all eight neighbours, which is the only case
    // the engine ever lights.
    for (int32 cx = 1; cx < grid - 1; ++cx) {
        for (int32 cz = 1; cz < grid - 1; ++cz) {
            std::vector<std::vector<const asset::chunk_occupancy*>> held(9);
            asset::sky_light_column::neighbourhood around{};

            for (int32 dz = -1; dz <= 1; ++dz) {
                for (int32 dx = -1; dx <= 1; ++dx) {
                    const auto slot = static_cast<std::size_t>(((dz + 1) * 3) + (dx + 1));
                    for (const auto& occ : occupancy.at(vec2i{cx + dx, cz + dz})) {
                        held[slot].push_back(&occ);
                    }
                    around[slot] = held[slot];
                }
            }

            const auto started = std::chrono::steady_clock::now();
            const asset::sky_light_column light{around};
            flood_ns += static_cast<uint64>(
                std::chrono::duration_cast<std::chrono::nanoseconds>(
                    std::chrono::steady_clock::now() - started
                )
                    .count()
            );

            const auto stats = light.count_pages();
            total.lit += stats.lit;
            total.dark += stats.dark;
            total.uniform_other += stats.uniform_other;
            total.mixed += stats.mixed;
            ++lit;
        }
    }

    const int32 all = total.lit + total.dark + total.uniform_other + total.mixed;

    WARN(
        "generated " << columns.size() << " columns / " << chunk_count << " chunks, lit " << lit
                     << " of them with a skirt\n  pages " << all << ": lit " << total.lit
                     << ", dark " << total.dark << ", uniform_other " << total.uniform_other
                     << ", mixed " << total.mixed << " (" << (100.0 * total.mixed / all)
                     << "%)\n  mixed bytes " << (total.mixed * 256) << " over " << (all / 512)
                     << " chunks, " << (total.mixed * 256 / (all / 512))
                     << " a chunk\n  flood " << (flood_ns / 1000 / static_cast<uint64>(lit))
                     << " us a column"
    );

    REQUIRE(all > 0);
}
