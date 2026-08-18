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
// Two things this does not do yet, both deliberate and both next:
//
// The four vertical sides of the column are sealed. Light that should arrive
// from the column next door does not, so a cave mouth straddling a column
// boundary is lit from one side only. The band this can reach is fifteen voxels
// deep from each side.
//
// Storage is dense, one byte a voxel, and meant to be built on a worker,
// sampled, and dropped. Where the field finally lives is an open question in
// docs/lighting.md, and count_pages() is here to answer it: it reports what an
// 8-cubed paged form would actually have to allocate.
class sky_light_column {
public:
    static constexpr int32 side      = chunk_occupancy::side;
    static constexpr int32 page      = 8;
    static constexpr uint8 max_level = 15;

    struct page_stats {
        int32 lit           = 0;  // whole page at 15
        int32 dark          = 0;  // whole page at 0
        int32 uniform_other = 0;  // whole page at one level in between
        int32 mixed         = 0;  // has to be stored voxel by voxel
    };

    // Chunks ordered top down, the way a column is walked everywhere else in
    // the engine. Nothing may stand above the first one, and a null entry is a
    // chunk of pure air. All of them are 64 cubed.
    explicit sky_light_column(std::span<const chunk_occupancy* const> chunks_top_down);

    [[nodiscard]] auto height() const -> int32 {
        return height_;
    }

    // y counts up from the bottom of the lowest chunk.
    [[nodiscard]] auto level_at(int32 x, int32 y, int32 z) const -> uint8 {
        return levels_[static_cast<std::size_t>(index_(x, y, z))];
    }

    [[nodiscard]] auto count_pages() const -> page_stats;

private:
    [[nodiscard]] auto index_(int32 x, int32 y, int32 z) const -> int32 {
        return (((y * side) + z) * side) + x;
    }

    int32 height_ = 0;
    std::vector<uint8> levels_;
};

}  // namespace vw::asset
