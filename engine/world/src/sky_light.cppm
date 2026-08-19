export module vw.world:sky_light;

import std;

import vw.core;
import :model;

export namespace vw::asset {

// Sky light over one column of chunks, copied from Minecraft exactly.
//
// Two rules, and the second is the one that matters. A voxel is at 15 when
// nothing opaque stands above it in its own column -- that is the whole of
// "downwards is free", and it holds only under open sky. Everything else is a
// flood from those seeds: six neighbours, one level a step, in every direction
// including down. Once light has had to go around anything it falls no faster
// than it spreads sideways, and that is what turns a cave mouth into a gradient
// instead of a shaft of light.
//
// Opacity is binary here, so a step always costs exactly one and Minecraft's
// max(1, opacity) degenerates. Light inside solid voxels is never read: every
// sample is taken at a corner of a visible face, which is always on the air
// side of it.
//
// The flood runs over the column plus a skirt fifteen voxels wide taken from
// its eight neighbours, and only the middle is kept. That makes the answer
// exact rather than nearly right: a source outside the skirt is at least
// sixteen steps from any voxel of the middle column, so it contributes
// max(0, 15 - 16) = 0 and sealing the skirt's outer edge can only drop sources
// that were already out of range. Fourteen is the tight bound; fifteen is a
// voxel of slack against off-by-one in the corners.
//
// Storage here is dense, one byte a voxel over the skirted volume, and meant to
// be built on a worker, read once and dropped. What survives it is
// sky_light_field, one paged chunk at a time.
class sky_light_column {
public:
    static constexpr int32 side      = chunk_occupancy::side;
    static constexpr int32 apron     = 15;
    static constexpr int32 span      = side + (2 * apron);
    static constexpr int32 page      = 8;
    static constexpr uint8 max_level = 15;

    // Nine columns, indexed (dz + 1) * 3 + (dx + 1), so index 4 is the middle
    // one. Each holds that column's chunk occupancy bottom up -- every column
    // in this world starts at the same world_bottom_y, so a common floor is
    // safe to assume and a common ceiling is not.
    //
    // An empty span is a column that is not there at all, and reads as solid
    // rock: it neither gives light nor lets any through. Above the top of a
    // column that *is* there is open air, and rule one seeds it -- that is what
    // keeps a plain beside a mountain from being walled off in the dark.
    using neighbourhood = std::array<std::span<const chunk_occupancy* const>, 9>;

    explicit sky_light_column(const neighbourhood& around);

    // One column with its four sides sealed. Exact only where nothing outside
    // it matters, which is why the skirted form exists.
    explicit sky_light_column(std::span<const chunk_occupancy* const> chunks_bottom_up);

    [[nodiscard]] auto height() const -> int32 {
        return height_;
    }

    // x and z address the middle column, 0 to 63. y counts up from the bottom.
    [[nodiscard]] auto level_at(int32 x, int32 y, int32 z) const -> uint8 {
        return levels_[static_cast<std::size_t>(index_(x + apron, y, z + apron))];
    }

    // The middle column's sixty-four levels along x, contiguous. Baking a chunk
    // reads the whole column this way; going voxel by voxel through level_at
    // instead lands every read on a different cache line and costs more than
    // the flood that produced it.
    [[nodiscard]] auto row(int32 y, int32 z) const -> const uint8* {
        return &levels_[static_cast<std::size_t>(index_(apron, y, z + apron))];
    }

private:
    [[nodiscard]] static auto index_(int32 x, int32 y, int32 z) -> int32 {
        return (((y * span) + z) * span) + x;
    }

    void flood_(const neighbourhood& around);

    int32 height_ = 0;
    std::vector<uint8> levels_;
};

// One chunk of sky light in the form it is actually kept: pages of 8x8x8, four
// bits a voxel. The nibble is not a saving to be clever about -- a level is
// 0 to 15 and nothing else, so a byte would be half padding.
//
// Two degenerate cases carry almost everything. Rock below the caves is dark
// all through and air above the surface is 15 all through, and either costs one
// byte and no table at all. What is left keeps a table of 512 entries, one a
// page, and a packed page only for the pages that really vary. On real terrain
// 2.4% of pages vary; the rest of the chunk is the table.
class sky_light_field {
public:
    static constexpr int32 side        = chunk_occupancy::side;
    static constexpr int32 page        = 8;
    static constexpr int32 pages_side  = side / page;
    static constexpr int32 page_count  = pages_side * pages_side * pages_side;
    static constexpr int32 page_voxels = page * page * page;
    static constexpr int32 page_bytes  = page_voxels / 2;

    using page_type = std::array<uint8, page_bytes>;

    sky_light_field() = default;

    explicit sky_light_field(uint8 level) : uniform_{level} {}

    // The chunk whose floor stands y_base voxels above the column's own floor.
    // Anything the column does not cover comes back dark rather than read past
    // its end -- the pipeline cannot ask for that, and a wrong answer is better
    // than a wild read if it ever does.
    sky_light_field(const sky_light_column& column, int32 y_base);

    [[nodiscard]] auto level_at(int32 x, int32 y, int32 z) const -> uint8 {
        if (table_.empty()) {
            return uniform_;
        }

        const uint16 entry =
            table_[static_cast<std::size_t>(page_index_(x / page, y / page, z / page))];
        if ((entry & 1U) == 0) {
            return static_cast<uint8>(entry >> 1);
        }

        const page_type& packed = pages_[entry >> 1];
        const int32 at          = (x % page) + ((y % page) * page) + ((z % page) * page * page);
        const uint8 pair        = packed[static_cast<std::size_t>(at / 2)];

        return static_cast<uint8>((at % 2) == 0 ? (pair & 0xFU) : (pair >> 4));
    }

    [[nodiscard]] auto level_at(vec3i pos) const -> uint8 {
        return level_at(pos.x, pos.y, pos.z);
    }

    // No table: the whole chunk is uniform_level().
    [[nodiscard]] auto is_uniform() const -> bool {
        return table_.empty();
    }

    [[nodiscard]] auto uniform_level() const -> uint8 {
        return uniform_;
    }

    [[nodiscard]] auto mixed_pages() const -> int32 {
        return static_cast<int32>(pages_.size());
    }

    [[nodiscard]] auto bytes() const -> std::size_t {
        return (table_.size() * sizeof(uint16)) + (pages_.size() * sizeof(page_type));
    }

private:
    [[nodiscard]] static auto page_index_(int32 px, int32 py, int32 pz) -> int32 {
        return px + (py * pages_side) + (pz * pages_side * pages_side);
    }

    uint8 uniform_ = 0;
    std::vector<uint16> table_;
    std::vector<page_type> pages_;
};

}  // namespace vw::asset
