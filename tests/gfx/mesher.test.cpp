#include <catch2/catch_test_macros.hpp>

import std;

import vw.core;
import vw.ecs;
import vw.world;
import vw.gfx;

using namespace vw;

namespace {

// A quad decoded back out of the packed vertices, so a greedy result can be
// compared against a per-voxel one: the two agree on which faces exist, never
// on how many quads they took to say it.
struct face_cell {
    int32 x = 0;
    int32 y = 0;
    int32 z = 0;
    uint8 normal = 0;
    uint8 block  = 0;

    auto operator<=>(const face_cell&) const = default;
};

auto unpack_position(const gfx::vertex& v) -> vec3i {
    return {
        static_cast<int32>(v.data0 & 0x7FU),
        static_cast<int32>((v.data0 >> 7) & 0x7FU),
        static_cast<int32>((v.data0 >> 14) & 0x7FU),
    };
}

auto unpack_normal(const gfx::vertex& v) -> uint8 {
    return static_cast<uint8>((v.data0 >> 21) & 0x7U);
}

auto unpack_block(const gfx::vertex& v) -> uint8 {
    return static_cast<uint8>(v.data1 & 0xFFU);
}

// Every quad covers a rectangle of unit faces on one plane. Splitting it back
// into those units is what makes greedy and simple comparable.
auto to_face_cells(const gfx::mesh& m) -> std::set<face_cell> {
    std::set<face_cell> cells;

    for (size_t base = 0; base + 3 < m.vertices.size(); base += 4) {
        auto lo = unpack_position(m.vertices[base]);
        auto hi = lo;
        for (size_t i = base + 1; i < base + 4; ++i) {
            const auto p = unpack_position(m.vertices[i]);
            lo = {std::min(lo.x, p.x), std::min(lo.y, p.y), std::min(lo.z, p.z)};
            hi = {std::max(hi.x, p.x), std::max(hi.y, p.y), std::max(hi.z, p.z)};
        }

        const auto normal = unpack_normal(m.vertices[base]);
        const auto block  = unpack_block(m.vertices[base]);

        const int32 sx = std::max(1, hi.x - lo.x);
        const int32 sy = std::max(1, hi.y - lo.y);
        const int32 sz = std::max(1, hi.z - lo.z);

        for (int32 dx = 0; dx < sx; ++dx) {
            for (int32 dy = 0; dy < sy; ++dy) {
                for (int32 dz = 0; dz < sz; ++dz) {
                    cells.insert(
                        face_cell{lo.x + dx, lo.y + dy, lo.z + dz, normal, block}
                    );
                }
            }
        }
    }

    return cells;
}

auto hash_mesh(const gfx::mesh& m) -> uint64 {
    uint64 hash = 1469598103934665603ULL;
    const auto mix = [&hash](uint32 value) {
        for (uint32 byte = 0; byte < 4; ++byte) {
            hash ^= (value >> (byte * 8)) & 0xFFU;
            hash *= 1099511628211ULL;
        }
    };

    for (const auto& v : m.vertices) {
        mix(v.data0);
        mix(v.data1);
    }
    for (const auto index : m.indices) {
        mix(index);
    }

    return hash;
}

class model_fixture {
public:
    explicit model_fixture(int32 size) : size_{size} {
        model_ = std::make_shared<asset::model>(identity_pool_, pages_, size, size, size);
    }

    [[nodiscard]] auto get() const -> const std::shared_ptr<asset::model>& {
        return model_;
    }

    [[nodiscard]] auto size() const -> int32 {
        return size_;
    }

    [[nodiscard]] auto greedy() -> gfx::mesh {
        gfx::mesh_generation_storage storage;
        return gfx::greedy_mesh_generator::generate_mesh_data(storage, *model_, registry_);
    }

    [[nodiscard]] auto simple() const -> gfx::mesh {
        return gfx::simple_mesh_generator::generate_mesh_data(model_, registry_);
    }

private:
    int32 size_;
    asset::model_identity_pool identity_pool_;
    asset::page_pool pages_;
    block_registry registry_;
    std::shared_ptr<asset::model> model_;
};

}  // namespace

TEST_CASE("greedy meshing agrees with per-voxel meshing", "[mesh]") {
    SECTION("empty model") {
        model_fixture fixture{16};
        REQUIRE(fixture.greedy().vertices.empty());
    }

    SECTION("single voxel") {
        model_fixture fixture{16};
        fixture.get()->set_voxel(4, 5, 6, voxel{blocks::red_3});

        const auto greedy = fixture.greedy();
        REQUIRE(greedy.vertices.size() == 24);
        REQUIRE(to_face_cells(greedy) == to_face_cells(fixture.simple()));
    }

    SECTION("voxel in the corner") {
        model_fixture fixture{16};
        fixture.get()->set_voxel(0, 0, 0, voxel{blocks::gray_5});
        REQUIRE(to_face_cells(fixture.greedy()) == to_face_cells(fixture.simple()));
    }

    SECTION("solid block merges into six quads") {
        model_fixture fixture{16};
        fixture.get()->fill(voxel{blocks::green_2});

        const auto greedy = fixture.greedy();
        REQUIRE(greedy.vertices.size() == 24);
        REQUIRE(to_face_cells(greedy) == to_face_cells(fixture.simple()));
    }

    SECTION("checkerboard defeats merging but not correctness") {
        model_fixture fixture{16};
        for (int32 x = 0; x < fixture.size(); ++x) {
            for (int32 y = 0; y < fixture.size(); ++y) {
                for (int32 z = 0; z < fixture.size(); ++z) {
                    if (((x + y + z) % 2) == 0) {
                        fixture.get()->set_voxel(x, y, z, voxel{blocks::blue_4});
                    }
                }
            }
        }
        REQUIRE(to_face_cells(fixture.greedy()) == to_face_cells(fixture.simple()));
    }

    SECTION("two materials never merge across the seam") {
        model_fixture fixture{16};
        for (int32 x = 0; x < fixture.size(); ++x) {
            for (int32 y = 0; y < 4; ++y) {
                for (int32 z = 0; z < fixture.size(); ++z) {
                    const auto block = (x < 8) ? blocks::brown_2 : blocks::gray_2;
                    fixture.get()->set_voxel(x, y, z, voxel{block});
                }
            }
        }
        REQUIRE(to_face_cells(fixture.greedy()) == to_face_cells(fixture.simple()));
    }

    // A 64-cube is the only size that takes the bit-occupancy path, so the
    // cases below are the ones that actually exercise it.
    SECTION("full-size chunk, terrain-like") {
        model_fixture fixture{64};
        uint32 state = 777;
        for (int32 x = 0; x < fixture.size(); ++x) {
            for (int32 z = 0; z < fixture.size(); ++z) {
                state = (state * 1664525U) + 1013904223U;
                const int32 height = 8 + static_cast<int32>((state >> 26) % 24);
                for (int32 y = 0; y < height; ++y) {
                    fixture.get()->set_voxel(x, y, z, voxel{blocks::brown_3});
                }
            }
        }
        REQUIRE(to_face_cells(fixture.greedy()) == to_face_cells(fixture.simple()));
    }

    SECTION("full-size chunk, sparse noise") {
        model_fixture fixture{64};
        uint32 state = 31337;
        for (int32 x = 0; x < fixture.size(); ++x) {
            for (int32 y = 0; y < fixture.size(); ++y) {
                for (int32 z = 0; z < fixture.size(); ++z) {
                    state = (state * 1664525U) + 1013904223U;
                    if ((state >> 29) == 0) {
                        fixture.get()->set_voxel(
                            x, y, z, voxel{block_id{static_cast<uint8>(1 + (state % 40))}}
                        );
                    }
                }
            }
        }
        REQUIRE(to_face_cells(fixture.greedy()) == to_face_cells(fixture.simple()));
    }

    SECTION("full-size chunk, solid") {
        model_fixture fixture{64};
        fixture.get()->fill(voxel{blocks::gray_6});

        const auto greedy = fixture.greedy();
        REQUIRE(greedy.vertices.size() == 24);
        REQUIRE(to_face_cells(greedy) == to_face_cells(fixture.simple()));
    }

    SECTION("pseudo-random fill") {
        model_fixture fixture{32};
        uint32 state = 12345;
        for (int32 x = 0; x < fixture.size(); ++x) {
            for (int32 y = 0; y < fixture.size(); ++y) {
                for (int32 z = 0; z < fixture.size(); ++z) {
                    state = (state * 1664525U) + 1013904223U;
                    if ((state >> 28) < 6) {
                        fixture.get()->set_voxel(x, y, z, voxel{block_id{static_cast<uint8>(1 + (state % 40))}});
                    }
                }
            }
        }
        REQUIRE(to_face_cells(fixture.greedy()) == to_face_cells(fixture.simple()));
    }
}

TEST_CASE("boundary faces close the seam between chunks", "[mesh]") {
    asset::model_identity_pool identity_pool;
    asset::page_pool pages;
    block_registry registry;

    // 64 so the bit path is the one under test: the +-X faces gather the
    // neighbour plane bit by bit, which the other four directions never do.
    constexpr int32 size = 64;
    auto left  = std::make_shared<asset::model>(identity_pool, pages, size, size, size);
    auto right = std::make_shared<asset::model>(identity_pool, pages, size, size, size);

    left->fill(voxel{blocks::gray_4});
    right->fill(voxel{blocks::gray_4});
    right->compute_own_boundaries();

    const auto count_faces = [&registry](const asset::model& m, uint8 normal) {
        gfx::mesh_generation_storage storage;
        const auto mesh = gfx::greedy_mesh_generator::generate_mesh_data(storage, m, registry);

        std::size_t count = 0;
        for (const auto& cell : to_face_cells(mesh)) {
            if (cell.normal == normal) {
                ++count;
            }
        }
        return count;
    };

    // Face 0 is +X: without a neighbour the whole side is drawn, with one it
    // disappears, and the other five sides are untouched either way.
    REQUIRE(count_faces(*left, 0) == size * size);
    const auto before_minus_x = count_faces(*left, 1);

    left->set_boundary_slice(0, *right);

    REQUIRE(left->has_boundary_slice(0));
    REQUIRE(count_faces(*left, 0) == 0);
    REQUIRE(count_faces(*left, 1) == before_minus_x);
}

// Guards the shape of the output across refactors that are meant to preserve
// it: the value only changes when the mesher deliberately changes.
TEST_CASE("greedy meshing output is stable", "[mesh]") {
    model_fixture fixture{32};

    uint32 state = 999;
    for (int32 x = 0; x < fixture.size(); ++x) {
        for (int32 z = 0; z < fixture.size(); ++z) {
            state = (state * 1664525U) + 1013904223U;
            const int32 height = 4 + static_cast<int32>((state >> 27) % 8);
            for (int32 y = 0; y < height; ++y) {
                fixture.get()->set_voxel(x, y, z, voxel{blocks::green_3});
            }
        }
    }

    const auto mesh = fixture.greedy();
    REQUIRE(mesh.vertices.size() > 0);
    REQUIRE(mesh.indices.size() == (mesh.vertices.size() / 4) * 6);

    const auto digest = hash_mesh(mesh);
    INFO("mesh digest: " << digest << ", quads: " << mesh.vertices.size() / 4);
    REQUIRE(mesh.vertices.size() / 4 == 5452);
    REQUIRE(digest == 16201069180391058227ULL);
}

// The 32-cube above never reaches the bit path, so ambient occlusion out of
// occupancy bits needs its own anchor: this digest covers the packed corner
// values, which the greedy-against-simple comparison does not look at.
TEST_CASE("full-size greedy meshing output is stable", "[mesh]") {
    model_fixture fixture{64};

    uint32 state = 20250815;
    for (int32 x = 0; x < fixture.size(); ++x) {
        for (int32 z = 0; z < fixture.size(); ++z) {
            state = (state * 1664525U) + 1013904223U;
            const int32 height = 6 + static_cast<int32>((state >> 26) % 20);
            for (int32 y = 0; y < height; ++y) {
                fixture.get()->set_voxel(x, y, z, voxel{blocks::green_3});
            }
        }
    }

    const auto mesh = fixture.greedy();
    const auto digest = hash_mesh(mesh);
    INFO("full-size digest: " << digest << ", quads: " << mesh.vertices.size() / 4);
    REQUIRE(mesh.vertices.size() / 4 == 29218);
    REQUIRE(digest == 419103052605503707ULL);
}
