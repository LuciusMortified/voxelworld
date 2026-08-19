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
// Storage is dense, one byte a voxel over the skirted volume, and meant to be
// built on a worker, sampled, and dropped. Where the field finally lives is
// settled in docs/lighting.md: pages beside the model, which count_pages()
// measured at about 3 KB a chunk.
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

    struct page_stats {
        int32 lit           = 0;  // whole page at 15
        int32 dark          = 0;  // whole page at 0
        int32 uniform_other = 0;  // whole page at one level in between
        int32 mixed         = 0;  // has to be stored voxel by voxel
    };

    [[nodiscard]] auto height() const -> int32 {
        return height_;
    }

    // x and z address the middle column, 0 to 63. y counts up from the bottom.
    [[nodiscard]] auto level_at(int32 x, int32 y, int32 z) const -> uint8 {
        return levels_[static_cast<std::size_t>(index_(x + apron, y, z + apron))];
    }

    [[nodiscard]] auto count_pages() const -> page_stats;

private:
    [[nodiscard]] static auto index_(int32 x, int32 y, int32 z) -> int32 {
        return (((y * span) + z) * span) + x;
    }

    void flood_(const neighbourhood& around);

    int32 height_ = 0;
    std::vector<uint8> levels_;
};

}  // namespace vw::asset
