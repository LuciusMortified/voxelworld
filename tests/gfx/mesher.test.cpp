#include <catch2/catch_test_macros.hpp>

import std;

import vw.core;
import vw.ecs;
import vw.world;
import vw.gfx;

using namespace vw;

namespace {

// One unit face, so a greedy result can be compared against a per-voxel one:
// the two agree on which faces exist, never on how many quads they took to say
// it.
struct face_cell {
    int32 x = 0;
    int32 y = 0;
    int32 z = 0;
    uint8 normal = 0;
    uint8 block  = 0;

    auto operator<=>(const face_cell&) const = default;
};

auto unpack_min(const gfx::quad& q) -> vec3i {
    return {
        static_cast<int32>(q.data0 & 0x7FU),
        static_cast<int32>((q.data0 >> 7) & 0x7FU),
        static_cast<int32>((q.data0 >> 14) & 0x7FU),
    };
}

auto unpack_max(const gfx::quad& q) -> vec3i {
    return {
        static_cast<int32>(q.data1 & 0x7FU),
        static_cast<int32>((q.data1 >> 7) & 0x7FU),
        static_cast<int32>((q.data1 >> 14) & 0x7FU),
    };
}

auto unpack_normal(const gfx::quad& q) -> uint8 {
    return static_cast<uint8>((q.data0 >> 21) & 0x7U);
}

auto unpack_block(const gfx::quad& q) -> uint8 {
    return static_cast<uint8>((q.data1 >> 21) & 0xFFU);
}

// Two bits a corner, in winding order: 0 open, 3 shut in by two faces.
auto unpack_ao(const gfx::quad& q) -> std::array<uint8, 4> {
    const uint32 packed = (q.data0 >> 24) & 0xFFU;
    return {
        static_cast<uint8>(packed & 0x3U),
        static_cast<uint8>((packed >> 2) & 0x3U),
        static_cast<uint8>((packed >> 4) & 0x3U),
        static_cast<uint8>((packed >> 6) & 0x3U),
    };
}

constexpr vec3i face_normal[6] = {
    {1, 0, 0}, {-1, 0, 0}, {0, 1, 0}, {0, -1, 0}, {0, 0, 1}, {0, 0, -1},
};

// Which end of the box each winding-order corner takes its components from.
// Copied from voxel.vert: the shader is what turns a rectangle into vertices,
// so this is the table that decides where a corner's occlusion lands on screen.
constexpr int32 face_verts[6][4][3] = {
    {{1, 0, 0}, {1, 0, 1}, {1, 1, 1}, {1, 1, 0}},
    {{0, 0, 0}, {0, 1, 0}, {0, 1, 1}, {0, 0, 1}},
    {{0, 1, 0}, {1, 1, 0}, {1, 1, 1}, {0, 1, 1}},
    {{0, 0, 0}, {0, 0, 1}, {1, 0, 1}, {1, 0, 0}},
    {{0, 0, 1}, {0, 1, 1}, {1, 1, 1}, {1, 0, 1}},
    {{1, 0, 0}, {1, 1, 0}, {0, 1, 0}, {0, 0, 0}},
};

// Worked out from the model and the face normal alone, never from the mesher's
// own idea of which way is u and which is v. That is the point of it: the rule
// below is the same one the mesher applies, but where it lands on the face is
// derived here from world coordinates, so a tangent table that is turned or
// mirrored for one face out of six shows up as a mismatch.
auto expected_corner_ao(
    const asset::model& mdl, vec3i cell, int32 face, vec3i corner
) -> uint8 {
    const vec3i n     = face_normal[face];
    const vec3i front = cell + n;

    const int32 axis = (n.x != 0) ? 0 : ((n.y != 0) ? 1 : 2);
    const int32 a    = (axis + 1) % 3;
    const int32 b    = (axis + 2) % 3;

    const auto solid = [&mdl](vec3i p) {
        if (p.x < 0 || p.y < 0 || p.z < 0) {
            return false;
        }
        if (p.x >= mdl.width() || p.y >= mdl.height() || p.z >= mdl.depth()) {
            return false;
        }
        return !mdl.is_empty(p.x, p.y, p.z);
    };

    // Cell (i, j) of the plane in front of the face, counted from the corner:
    // cell 0 is the one whose near edge starts at the corner, cell -1 the one
    // ending there.
    const auto at = [&](int32 i, int32 j) {
        vec3i p = front;
        p[a]    = corner[a] + i;
        p[b]    = corner[b] + j;
        return p;
    };

    // Where the face's own cell sits relative to the corner. The three cells
    // that matter are the two across an edge from it and the one diagonally
    // opposite; the fourth is the face's own, and it is empty whenever the face
    // is drawn at all.
    const int32 self_i  = front[a] - corner[a];
    const int32 self_j  = front[b] - corner[b];
    const int32 other_i = (self_i == 0) ? -1 : 0;
    const int32 other_j = (self_j == 0) ? -1 : 0;

    const bool edge_a   = solid(at(other_i, self_j));
    const bool edge_b   = solid(at(self_i, other_j));
    const bool diagonal = solid(at(other_i, other_j));

    if (edge_a && edge_b) {
        return 3;
    }
    return static_cast<uint8>(edge_a) + static_cast<uint8>(edge_b) +
           static_cast<uint8>(diagonal);
}

auto ao_levels(const gfx::mesh& m, uint8 normal) -> std::set<uint8> {
    std::set<uint8> levels;
    for (const auto& q : m.quads) {
        if (unpack_normal(q) != normal) {
            continue;
        }
        for (const uint8 value : unpack_ao(q)) {
            levels.insert(value);
        }
    }
    return levels;
}

// Every quad covers a rectangle of unit faces on one plane. Splitting it back
// into those units is what makes greedy and simple comparable.
auto to_face_cells(const gfx::mesh& m) -> std::set<face_cell> {
    std::set<face_cell> cells;

    for (const auto& q : m.quads) {
        const auto lo = unpack_min(q);
        const auto hi = unpack_max(q);

        const auto normal = unpack_normal(q);
        const auto block  = unpack_block(q);

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

    for (const auto& q : m.quads) {
        mix(q.data0);
        mix(q.data1);
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
        REQUIRE(fixture.greedy().quads.empty());
    }

    SECTION("single voxel") {
        model_fixture fixture{16};
        fixture.get()->set_voxel(4, 5, 6, voxel{blocks::red_3});

        const auto greedy = fixture.greedy();
        REQUIRE(greedy.quads.size() == 6);
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
        REQUIRE(greedy.quads.size() == 6);
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
        REQUIRE(greedy.quads.size() == 6);
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

// The sampler grades occlusion from nothing to shut in, and the packing has to
// carry that grading through. It did not: the last step compared the value
// against zero, so a corner brushed by one diagonal block came out identical to
// the inside of a right angle, and the shading read as an outline rather than
// as depth. A digest would not have caught it -- it was stable and wrong.
TEST_CASE("ambient occlusion keeps all four levels", "[mesh]") {
    model_fixture fixture{16};

    for (int32 x = 0; x < 8; ++x) {
        for (int32 z = 0; z < 8; ++z) {
            fixture.get()->set_voxel(x, 4, z, voxel{blocks::gray_5});
        }
    }

    SECTION("open floor is not occluded anywhere") {
        REQUIRE(ao_levels(fixture.simple(), 2) == std::set<uint8>{0});
    }

    SECTION("three blocks are enough to produce every level") {
        // Against the floor cell at (1, 4, 2) these are, in turn, a diagonal on
        // its own, an edge, and a second edge at right angles to the first --
        // which is one corner of each kind, plus the untouched ones.
        fixture.get()->set_voxel(2, 5, 1, voxel{blocks::red_3});
        fixture.get()->set_voxel(2, 5, 2, voxel{blocks::red_3});
        fixture.get()->set_voxel(1, 5, 3, voxel{blocks::red_3});

        REQUIRE(ao_levels(fixture.simple(), 2) == std::set<uint8>{0, 1, 2, 3});
        REQUIRE(ao_levels(fixture.greedy(), 2) == std::set<uint8>{0, 1, 2, 3});
    }
}

// A trench that turns a corner, checked for the one thing that reads as a bug
// rather than as shading: a point lighter than everything around it.
//
// A bend is where an occlusion term is most likely to produce one. A wider
// kernel did, at every turn, because its far diagonals were the only samples in
// range there and had been given no weight. Three samples cannot make that
// mistake -- a corner is dark exactly when something touches it -- but the
// invariant is worth keeping whatever the kernel becomes.
TEST_CASE("a bent trench has no point lighter than its surroundings", "[mesh]") {
    model_fixture fixture{32};

    for (int32 x = 0; x < 32; ++x) {
        for (int32 z = 0; z < 32; ++z) {
            for (int32 y = 0; y < 10; ++y) {
                fixture.get()->set_voxel(x, y, z, voxel{blocks::gray_5});
            }
        }
    }

    const auto in_corridor = [](int32 x, int32 z) {
        const bool leg_a = x >= 10 && x < 13 && z >= 8 && z < 18;
        const bool leg_b = x >= 10 && x < 20 && z >= 15 && z < 18;
        return leg_a || leg_b;
    };

    for (int32 x = 0; x < 32; ++x) {
        for (int32 z = 0; z < 32; ++z) {
            if (!in_corridor(x, z)) {
                continue;
            }
            for (int32 y = 6; y < 10; ++y) {
                fixture.get()->set_voxel(x, y, z, voxel{});
            }
        }
    }

    std::map<std::pair<int32, int32>, uint8> lattice;
    for (const auto& q : fixture.simple().quads) {
        if (unpack_normal(q) != 2) {
            continue;
        }
        const auto lo = unpack_min(q);
        const auto hi = unpack_max(q);
        if (lo.y != 5) {
            continue;
        }
        const auto ao = unpack_ao(q);
        for (int32 slot = 0; slot < 4; ++slot) {
            const int32 cx = face_verts[2][slot][0] != 0 ? hi.x : lo.x;
            const int32 cz = face_verts[2][slot][2] != 0 ? hi.z : lo.z;
            lattice[{cx, cz}] = ao[slot];
        }
    }

    REQUIRE(lattice.size() > 60);

    for (const auto& [at, value] : lattice) {
        static constexpr std::array<std::pair<int32, int32>, 4> steps{
            std::pair{1, 0}, std::pair{-1, 0}, std::pair{0, 1}, std::pair{0, -1}
        };

        bool lighter_than_all = true;
        int32 neighbours      = 0;

        for (const auto& [dx, dz] : steps) {
            const auto it = lattice.find({at.first + dx, at.second + dz});
            if (it == lattice.end()) {
                continue;
            }
            ++neighbours;
            if (it->second <= value) {
                lighter_than_all = false;
            }
        }

        if (neighbours == 4 && lighter_than_all) {
            INFO("lattice " << at.first << "," << at.second << " reads " << int32{value}
                            << " against darker neighbours on all four sides");
            FAIL();
        }
    }
}

// How far a wall reaches, written down as a number rather than left to be
// rediscovered from a screenshot. One cell, and then nothing: the column beside
// the wall is shaded, the next one is open ground. That is why the middle of a
// trench three voxels wide reads exactly as bright as a field, and it is the
// known price of three samples -- docs/lighting.md records what a two-cell
// kernel bought instead, and what it charged.
TEST_CASE("occlusion reaches exactly one cell from a wall", "[mesh]") {
    model_fixture fixture{16};

    for (int32 x = 0; x < 16; ++x) {
        for (int32 z = 0; z < 16; ++z) {
            fixture.get()->set_voxel(x, 4, z, voxel{blocks::gray_5});
        }
    }

    // One voxel tall, running the length of the floor: everything the up-faces
    // sample lies in the plane just above them.
    for (int32 z = 0; z < 16; ++z) {
        fixture.get()->set_voxel(4, 5, z, voxel{blocks::red_3});
    }

    // Every up-face corner, gathered by how far along x its lattice point sits.
    // Away from the ends in z, where the wall stops and the picture changes.
    std::map<int32, std::set<uint8>> by_column;
    for (const auto& q : fixture.simple().quads) {
        const auto lo = unpack_min(q);
        const auto hi = unpack_max(q);
        const auto ao = unpack_ao(q);

        // The floor's own up-faces. The wall has one too, a level higher, and
        // nothing stands over it -- counting it in reads as the wall failing to
        // occlude itself.
        if (unpack_normal(q) != 2 || lo.y != 4) {
            continue;
        }

        for (int32 slot = 0; slot < 4; ++slot) {
            const int32 cx = face_verts[2][slot][0] != 0 ? hi.x : lo.x;
            const int32 cz = face_verts[2][slot][2] != 0 ? hi.z : lo.z;
            if (cz > 4 && cz < 12) {
                by_column[cx].insert(ao[slot]);
            }
        }
    }

    REQUIRE(by_column[5] == std::set<uint8>{2});
    REQUIRE(by_column[6] == std::set<uint8>{0});
    REQUIRE(by_column[7] == std::set<uint8>{0});
}

// Every corner of every rectangle, against occlusion worked out from the model
// directly. This is the check that a corner's value lands on the corner it was
// computed for: the mesher indexes corners by its own two tangents, the shader
// by winding order, and a table between them that is turned or mirrored for one
// face out of six produces shading on the wrong side of that face -- which
// reads as a bright seam where two faces meet and both went light.
TEST_CASE("packed occlusion matches the model at every corner", "[mesh]") {
    model_fixture fixture{16};

    // Away from the model's own walls, so nothing is decided by the absence of
    // a boundary slice, and dense enough to put every corner case somewhere.
    uint32 state = 7771;
    for (int32 x = 2; x < 14; ++x) {
        for (int32 y = 2; y < 14; ++y) {
            for (int32 z = 2; z < 14; ++z) {
                state = (state * 1664525U) + 1013904223U;
                if (((state >> 28) & 7U) < 4U) {
                    fixture.get()->set_voxel(x, y, z, voxel{blocks::gray_5});
                }
            }
        }
    }

    const auto check = [&fixture](const gfx::mesh& m, std::string_view what) {
        std::size_t checked = 0;

        for (const auto& q : m.quads) {
            const int32 face = unpack_normal(q);
            const auto lo    = unpack_min(q);
            const auto hi    = unpack_max(q);
            const auto ao    = unpack_ao(q);

            for (int32 slot = 0; slot < 4; ++slot) {
                vec3i corner{};
                vec3i cell{};
                for (std::size_t i = 0; i < 3; ++i) {
                    const bool high = face_verts[face][slot][i] != 0;
                    corner[i]       = high ? hi[i] : lo[i];
                    cell[i]         = high ? hi[i] - 1 : lo[i];
                }

                const uint8 want = expected_corner_ao(*fixture.get(), cell, face, corner);
                if (ao[slot] != want) {
                    INFO(
                        what << ": face " << face << " cell " << cell.x << "," << cell.y << ","
                             << cell.z << " slot " << slot << " packed " << int32{ao[slot]}
                             << " expected " << int32{want}
                    );
                    FAIL();
                }
                ++checked;
            }
        }

        return checked;
    };

    REQUIRE(check(fixture.simple(), "simple") > 1000);
    REQUIRE(check(fixture.greedy(), "greedy") > 1000);
}

// Occlusion samples reach one cell past the face, so a face on the edge of a
// chunk asks its neighbour. If it did not, every chunk boundary would draw
// itself as a line of unoccluded ground across an otherwise shaded corner.
TEST_CASE("ambient occlusion reads across the chunk seam", "[mesh]") {
    // Full chunk size on both sides: a boundary slice is only taken between
    // models of exactly that side, and a smaller one is dropped in silence.
    model_fixture left{64};
    model_fixture right{64};

    for (int32 x = 0; x < 64; ++x) {
        for (int32 z = 0; z < 64; ++z) {
            left.get()->set_voxel(x, 4, z, voxel{blocks::gray_5});
        }
    }

    // Just over the seam and one above the floor: from the last floor cell of
    // `left` this is the neighbour along +x, in the plane its up-face samples.
    right.get()->set_voxel(0, 5, 8, voxel{blocks::red_3});

    // The up-face of the last floor cell. Found rather than assumed: a lookup
    // that silently misses would return four zeroes, which is exactly the value
    // the unwired case is supposed to have.
    const auto seam_corners = [](const gfx::mesh& m) -> std::optional<std::array<uint8, 4>> {
        for (const auto& q : m.quads) {
            const auto lo = unpack_min(q);
            const auto hi = unpack_max(q);
            if (unpack_normal(q) != 2) {
                continue;
            }
            if (lo.x <= 63 && hi.x > 63 && lo.y == 4 && lo.z <= 8 && hi.z > 8) {
                return unpack_ao(q);
            }
        }
        return std::nullopt;
    };

    static constexpr std::array<uint8, 4> open{0, 0, 0, 0};

    const auto before = seam_corners(left.simple());
    REQUIRE(before.has_value());
    REQUIRE(*before == open);

    left.get()->set_boundary_slice(0, *right.get());

    const auto after = seam_corners(left.simple());
    REQUIRE(after.has_value());
    REQUIRE(*after != open);

    const auto after_greedy = seam_corners(left.greedy());
    REQUIRE(after_greedy.has_value());
    REQUIRE(*after_greedy != open);
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
    REQUIRE(mesh.quads.size() > 0);

    const auto digest = hash_mesh(mesh);
    INFO("mesh digest: " << digest << ", quads: " << mesh.quads.size());
    REQUIRE(mesh.quads.size() == 5452);
    REQUIRE(digest == 16222259620568564683ULL);
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
    INFO("full-size digest: " << digest << ", quads: " << mesh.quads.size());
    REQUIRE(mesh.quads.size() == 29218);
    REQUIRE(digest == 8033663954171315545ULL);
}
