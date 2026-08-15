#include <catch2/catch_test_macros.hpp>

import std;

import vw.core;
import vw.ecs;
import vw.world;

using namespace vw;
using namespace vw::ecs;

namespace {

// The world the generator produced, flattened into one array so the cave
// structure can be walked without going through chunk boundaries.
class sampled_region {
public:
    static constexpr int32 chunk = 64;

    sampled_region(
        int32 columns_x, int32 columns_z, perlin_terrain_generator& gen, int32 origin_x = 0,
        int32 origin_z = 0
    )
        : columns_x_{columns_x}, columns_z_{columns_z} {
        std::vector<std::pair<vec3i, std::shared_ptr<asset::model>>> chunks;

        for (int32 ix = 0; ix < columns_x_; ++ix) {
            for (int32 iz = 0; iz < columns_z_; ++iz) {
                const int32 cx = origin_x + ix;
                const int32 cz = origin_z + iz;

                ecs::gen_column column{cx, cz};
                terrain_context ctx{
                    .cx           = cx,
                    .cz           = cz,
                    .create_chunk = [&column](int32 y) -> chunk_data& {
                        return column.create_chunk(y, chunk_data{});
                    },
                };
                gen.generate(ctx);

                for (auto& [cy, data] : column.get_all_chunk_data()) {
                    min_y_ = std::min(min_y_, cy);
                    max_y_ = std::max(max_y_, cy);
                    chunks.emplace_back(vec3i{ix, cy, iz}, data.chunk_model);
                }
            }
        }

        columns_y_ = (max_y_ - min_y_) + 1;
        solid_.assign(
            static_cast<std::size_t>(size_x()) * size_y() * size_z(), false
        );

        for (const auto& [coord, model] : chunks) {
            for (int32 x = 0; x < chunk; ++x) {
                for (int32 y = 0; y < chunk; ++y) {
                    for (int32 z = 0; z < chunk; ++z) {
                        if (model->is_empty(x, y, z)) {
                            continue;
                        }
                        const int32 gx = (coord.x * chunk) + x;
                        const int32 gy = ((coord.y - min_y_) * chunk) + y;
                        const int32 gz = (coord.z * chunk) + z;
                        solid_[at(gx, gy, gz)] = true;
                    }
                }
            }
        }
    }

    [[nodiscard]] auto size_x() const -> int32 { return columns_x_ * chunk; }
    [[nodiscard]] auto size_y() const -> int32 { return columns_y_ * chunk; }
    [[nodiscard]] auto size_z() const -> int32 { return columns_z_ * chunk; }

    [[nodiscard]] auto is_solid(int32 x, int32 y, int32 z) const -> bool {
        if (x < 0 || y < 0 || z < 0 || x >= size_x() || y >= size_y() || z >= size_z()) {
            return true;  // outside the sampled region counts as rock
        }
        return solid_[at(x, y, z)];
    }

    // Rock above and rock below. Requiring only rock above counts the empty
    // space under the world floor as cave, which is most of what is down there
    // and swamps every other number.
    [[nodiscard]] auto is_underground(int32 x, int32 y, int32 z) const -> bool {
        bool above = false;
        for (int32 up = y + 1; up < size_y(); ++up) {
            if (is_solid(x, up, z)) {
                above = true;
                break;
            }
        }
        if (!above) {
            return false;
        }

        for (int32 down = y - 1; down >= 0; --down) {
            if (is_solid(x, down, z)) {
                return true;
            }
        }
        return false;
    }

    [[nodiscard]] auto at(int32 x, int32 y, int32 z) const -> std::size_t {
        return (static_cast<std::size_t>(y) * size_z() + z) * size_x() + x;
    }

private:
    int32 columns_x_;
    int32 columns_z_;
    int32 columns_y_ = 1;
    int32 min_y_     = std::numeric_limits<int32>::max();
    int32 max_y_     = std::numeric_limits<int32>::lowest();
    std::vector<bool> solid_;
};

struct cave_stats {
    std::size_t underground_total = 0;
    std::size_t underground_air = 0;

    // How thick the openings are. A tunnel a character walks down clears a
    // 3-cube around its middle; a sponge of small pockets does not.
    std::size_t radius1 = 0;

    // Air a character actually fits in. The arena body is 12x28x12 units at
    // voxel_scale 8, so it needs a 2x4x2 box of clear voxels.
    std::size_t walkable = 0;

    std::size_t components       = 0;
    std::size_t largest_component = 0;

    // The same question asked of the space a character can actually occupy: a
    // network of passages is only a network if the wide parts join up.
    std::size_t walkable_components = 0;
    std::size_t largest_walkable    = 0;

    // Faces between air and rock underground: what the mesher has to turn into
    // quads, and the reason a cave shape has a cost as well as a feel.
    std::size_t faces = 0;
};

auto fits_body(const sampled_region& r, int32 x, int32 y, int32 z) -> bool {
    for (int32 dx = 0; dx < 2; ++dx) {
        for (int32 dy = 0; dy < 4; ++dy) {
            for (int32 dz = 0; dz < 2; ++dz) {
                if (r.is_solid(x + dx, y + dy, z + dz)) {
                    return false;
                }
            }
        }
    }
    return true;
}

auto clear_cube(const sampled_region& r, int32 x, int32 y, int32 z, int32 radius) -> bool {
    for (int32 dx = -radius; dx <= radius; ++dx) {
        for (int32 dy = -radius; dy <= radius; ++dy) {
            for (int32 dz = -radius; dz <= radius; ++dz) {
                if (r.is_solid(x + dx, y + dy, z + dz)) {
                    return false;
                }
            }
        }
    }
    return true;
}

auto measure(const sampled_region& r) -> cave_stats {
    cave_stats stats;

    std::vector<bool> visited(
        static_cast<std::size_t>(r.size_x()) * r.size_y() * r.size_z(), false
    );
    std::vector<vec3i> stack;

    for (int32 y = 0; y < r.size_y(); ++y) {
        for (int32 z = 0; z < r.size_z(); ++z) {
            for (int32 x = 0; x < r.size_x(); ++x) {
                if (!r.is_underground(x, y, z)) {
                    continue;
                }

                ++stats.underground_total;
                if (r.is_solid(x, y, z)) {
                    continue;
                }

                ++stats.underground_air;
                if (fits_body(r, x, y, z)) {
                    ++stats.walkable;
                }

                constexpr std::array<vec3i, 6> sides{
                    vec3i{1, 0, 0},  vec3i{-1, 0, 0}, vec3i{0, 1, 0},
                    vec3i{0, -1, 0}, vec3i{0, 0, 1},  vec3i{0, 0, -1},
                };
                for (const auto& d : sides) {
                    if (r.is_solid(x + d.x, y + d.y, z + d.z)) {
                        ++stats.faces;
                    }
                }
                if (clear_cube(r, x, y, z, 1)) {
                    ++stats.radius1;
                }

                if (visited[r.at(x, y, z)]) {
                    continue;
                }

                // Flood fill this pocket to see how much of the underground is
                // one connected space rather than sealed bubbles.
                ++stats.components;
                std::size_t size = 0;
                stack.push_back({x, y, z});
                visited[r.at(x, y, z)] = true;

                while (!stack.empty()) {
                    const auto p = stack.back();
                    stack.pop_back();
                    ++size;

                    constexpr std::array<vec3i, 6> dirs{
                        vec3i{1, 0, 0},  vec3i{-1, 0, 0}, vec3i{0, 1, 0},
                        vec3i{0, -1, 0}, vec3i{0, 0, 1},  vec3i{0, 0, -1},
                    };
                    for (const auto& d : dirs) {
                        const vec3i n{p.x + d.x, p.y + d.y, p.z + d.z};
                        if (n.x < 0 || n.y < 0 || n.z < 0 || n.x >= r.size_x() ||
                            n.y >= r.size_y() || n.z >= r.size_z()) {
                            continue;
                        }
                        if (r.is_solid(n.x, n.y, n.z) || visited[r.at(n.x, n.y, n.z)]) {
                            continue;
                        }
                        if (!r.is_underground(n.x, n.y, n.z)) {
                            continue;
                        }
                        visited[r.at(n.x, n.y, n.z)] = true;
                        stack.push_back(n);
                    }
                }

                stats.largest_component = std::max(stats.largest_component, size);
            }
        }
    }

    // Second pass over the walkable subset only.
    std::vector<bool> seen(
        static_cast<std::size_t>(r.size_x()) * r.size_y() * r.size_z(), false
    );

    for (int32 y = 0; y < r.size_y(); ++y) {
        for (int32 z = 0; z < r.size_z(); ++z) {
            for (int32 x = 0; x < r.size_x(); ++x) {
                if (seen[r.at(x, y, z)] || !r.is_underground(x, y, z) ||
                    r.is_solid(x, y, z) || !fits_body(r, x, y, z)) {
                    continue;
                }

                ++stats.walkable_components;
                std::size_t size = 0;
                stack.push_back({x, y, z});
                seen[r.at(x, y, z)] = true;

                while (!stack.empty()) {
                    const auto p = stack.back();
                    stack.pop_back();
                    ++size;

                    constexpr std::array<vec3i, 6> dirs{
                        vec3i{1, 0, 0},  vec3i{-1, 0, 0}, vec3i{0, 1, 0},
                        vec3i{0, -1, 0}, vec3i{0, 0, 1},  vec3i{0, 0, -1},
                    };
                    for (const auto& d : dirs) {
                        const vec3i n{p.x + d.x, p.y + d.y, p.z + d.z};
                        if (n.x < 0 || n.y < 0 || n.z < 0 || n.x >= r.size_x() ||
                            n.y >= r.size_y() || n.z >= r.size_z()) {
                            continue;
                        }
                        if (seen[r.at(n.x, n.y, n.z)] || r.is_solid(n.x, n.y, n.z) ||
                            !r.is_underground(n.x, n.y, n.z) || !fits_body(r, n.x, n.y, n.z)) {
                            continue;
                        }
                        seen[r.at(n.x, n.y, n.z)] = true;
                        stack.push_back(n);
                    }
                }

                stats.largest_walkable = std::max(stats.largest_walkable, size);
            }
        }
    }

    return stats;
}

}  // namespace

// Prints the whole table at once, so tuning does not need a rebuild per value.
// Not part of the suite: tagged [.sweep], run it by name.
TEST_CASE("cave lattice sweep", "[world][caves][.sweep]") {
    // Every setting is sampled at more than one place in the world. The region
    // has to be several lattice cells across or its own edges decide the answer
    // -- the same settings measured 0.5 connected on three columns and 0.94 on
    // five.
    struct probe {
        int32 spacing;
        float32 radius;
        float32 chance;
        int32 origin_x;
        int32 origin_z;
    };

    // Surface area is what the mesher pays for, and it grows with the length of
    // the network times the radius while the air volume grows with the radius
    // squared. Fewer, wider passages are therefore cheaper than many thin ones
    // for the same amount of hollow -- so the sweep raises spacing and radius
    // together, and raises the edge chance to keep the sparser graph joined up.
    for (const auto [spacing, radius, chance, origin_x, origin_z] : {
             probe{64, 5.0F, 0.85F, 0, 0}, probe{64, 5.0F, 0.85F, 20, 12},
             probe{96, 6.0F, 0.85F, 0, 0}, probe{96, 6.0F, 0.85F, 20, 12},
         }) {
        asset::model_identity_pool identity_pool;
        asset::page_pool pages;

        auto p                 = perlin_terrain_generator::params{};
        p.cave_node_spacing_xz = spacing;
        p.cave_node_spacing_y  = spacing;
        p.cave_node_jitter     = spacing / 3;
        p.cave_edge_chance_xz  = chance;
        p.cave_edge_chance_y   = chance * 0.5F;
        p.cave_radius          = radius;

        perlin_terrain_generator gen{identity_pool, pages, p};
        const sampled_region region{5, 5, gen, origin_x, origin_z};
        const auto stats = measure(region);

        const auto total = static_cast<float64>(stats.underground_total);
        const auto air   = static_cast<float64>(stats.underground_air);

        std::println(
            "spacing {} r {:.1f} at {:>2},{:>2}: air {:.3f}  r1 {:.3f}  walkable {:.3f}  "
            "largest_air {:.3f}  walk_parts {}  largest_walk {:.3f}  faces/kvox {:.1f}",
            spacing,
            radius,
            origin_x,
            origin_z,
            total > 0 ? air / total : 0.0,
            air > 0 ? static_cast<float64>(stats.radius1) / air : 0.0,
            air > 0 ? static_cast<float64>(stats.walkable) / air : 0.0,
            air > 0 ? static_cast<float64>(stats.largest_component) / air : 0.0,
            stats.walkable_components,
            stats.walkable > 0
                ? static_cast<float64>(stats.largest_walkable) /
                      static_cast<float64>(stats.walkable)
                : 0.0,
            total > 0 ? static_cast<float64>(stats.faces) * 1000.0 / total : 0.0
        );
    }
}

TEST_CASE("underground caves are walkable and connected", "[world][caves]") {
    asset::model_identity_pool identity_pool;
    asset::page_pool pages;

    perlin_terrain_generator gen{identity_pool, pages, perlin_terrain_generator::params{}};

    // A small region says more about where it was cut than about the generator,
    // because the edge of the region severs passages: the same settings
    // measured 0.5 at three columns, 0.84 at four and 0.94 at five. Five
    // columns is 320 voxels, five lattice cells across.
    const sampled_region region{5, 5, gen};
    const auto stats = measure(region);

    const auto total    = static_cast<float64>(stats.underground_total);
    const auto air      = static_cast<float64>(stats.underground_air);
    const auto fraction = total > 0 ? air / total : 0.0;
    const auto walkable = air > 0 ? static_cast<float64>(stats.walkable) / air : 0.0;
    const auto connected = stats.walkable > 0
        ? static_cast<float64>(stats.largest_walkable) / static_cast<float64>(stats.walkable)
        : 0.0;

    INFO("underground volume: " << stats.underground_total);
    INFO("air fraction: " << fraction);
    INFO("radius1 share: " << (air > 0 ? static_cast<float64>(stats.radius1) / air : 0.0));
    INFO("walkable share of air: " << walkable);
    INFO("walkable pieces: " << stats.walkable_components);
    INFO("largest walkable share: " << connected);

    REQUIRE(stats.underground_air > 0);

    // Passages, not a sponge: a few per cent of the rock is hollow.
    REQUIRE(fraction > 0.02);
    REQUIRE(fraction < 0.08);

    // Most of the air is a passage wide enough to matter. The share never
    // approaches 1 even for a perfectly round tunnel, because this counts the
    // corners a 2 x 4 x 2 body can start from, and near the tunnel wall there
    // are none.
    REQUIRE(walkable > 0.3);

    // The part that matters for the game: what a character can walk is one
    // network rather than a set of sealed pockets. Measured across regions it
    // sits between 0.94 and 0.96, so the bar is set below the spread rather
    // than at the best sample.
    REQUIRE(connected > 0.85);
}
