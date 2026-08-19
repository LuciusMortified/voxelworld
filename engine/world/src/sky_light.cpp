module vw.world;

import std;

namespace vw::asset {

namespace {

constexpr int32 s     = sky_light_column::side;
constexpr int32 apron = sky_light_column::apron;
constexpr int32 span  = sky_light_column::span;

// Three words a row, and the middle column sits in the whole of the second one.
// That is what the offset buys: a neighbour's sixty-four bits land at bit
// 64 + dx * 64, so every one of the nine blits is word aligned and the skirt
// costs no shifting at all.
constexpr int32 words = 3;
constexpr int32 bit0  = s - apron;

static_assert(bit0 + span <= words * 64);
static_assert(bit0 + apron == 64);

// Everything outside the skirt is sealed, so those bits read as solid and as
// "already lit" -- the first stops light leaving, the second stops the frontier
// pass seeding along the outer edge for nothing.
constexpr uint64 outside_low  = (uint64{1} << bit0) - 1;
constexpr uint64 outside_high = ~((uint64{1} << (bit0 + span - 128)) - 1);

[[nodiscard]] auto row_base(int32 y, int32 z) -> std::size_t {
    return ((static_cast<std::size_t>(y) * span) + static_cast<std::size_t>(z)) * words;
}

void seal_outside(uint64* row) {
    row[0] |= outside_low;
    row[2] |= outside_high;
}

// Sky as the frontier test wants to read it: everything beyond the skirt counts
// as lit. It has to be done before the shift, not after -- a shift pulls the
// bit just past the edge into the first bit inside it, and reading that as dark
// seeds the whole outer wall of the skirt for nothing.
void pad_as_sky(const uint64* in, uint64* out) {
    out[0] = in[0] | outside_low;
    out[1] = in[1];
    out[2] = in[2] | outside_high;
}

// Bit x of the result is bit x - 1 of the input: what lies one step west.
void shift_west(const uint64* in, uint64* out) {
    out[2] = (in[2] << 1) | (in[1] >> 63);
    out[1] = (in[1] << 1) | (in[0] >> 63);
    out[0] = in[0] << 1;
    seal_outside(out);
}

void shift_east(const uint64* in, uint64* out) {
    out[0] = (in[0] >> 1) | (in[1] << 63);
    out[1] = (in[1] >> 1) | (in[2] << 63);
    out[2] = in[2] >> 1;
    seal_outside(out);
}

}  // namespace

sky_light_column::sky_light_column(std::span<const chunk_occupancy* const> chunks_bottom_up)
    : sky_light_column{neighbourhood{
          {{}, {}, {}, {}, chunks_bottom_up, {}, {}, {}, {}}
      }} {}

sky_light_column::sky_light_column(const neighbourhood& around) {
    int32 chunks = 0;
    for (const auto& column : around) {
        chunks = std::max(chunks, static_cast<int32>(column.size()));
    }

    height_ = chunks * s;
    if (height_ == 0) {
        return;
    }

    levels_.assign(static_cast<std::size_t>(height_) * span * span, 0);
    flood_(around);
}

void sky_light_column::flood_(const neighbourhood& around) {
    std::vector<uint64> solid(static_cast<std::size_t>(height_) * span * words, 0);

    for (int32 dz = -1; dz <= 1; ++dz) {
        for (int32 dx = -1; dx <= 1; ++dx) {
            const auto& column = around[static_cast<std::size_t>(((dz + 1) * 3) + (dx + 1))];
            const int32 word   = 1 + dx;

            for (int32 lz = 0; lz < s; ++lz) {
                const int32 z = apron + (dz * s) + lz;
                if (z < 0 || z >= span) {
                    continue;
                }

                // A column that is not there is rock to the top of the world.
                // One that is there but ends lower has open air above it.
                if (column.empty()) {
                    for (int32 y = 0; y < height_; ++y) {
                        solid[row_base(y, z) + static_cast<std::size_t>(word)] = ~uint64{0};
                    }
                    continue;
                }

                for (std::size_t i = 0; i < column.size(); ++i) {
                    const chunk_occupancy* occ = column[i];
                    if (occ == nullptr) {
                        continue;
                    }

                    const int32 base = static_cast<int32>(i) * s;
                    for (int32 ly = 0; ly < s; ++ly) {
                        solid[row_base(base + ly, z) + static_cast<std::size_t>(word)] =
                            occ->row(ly, lz);
                    }
                }
            }
        }
    }

    for (int32 y = 0; y < height_; ++y) {
        for (int32 z = 0; z < span; ++z) {
            seal_outside(&solid[row_base(y, z)]);
        }
    }

    // Rule one. A bit stays set while its column still sees the sky, and the
    // first opaque voxel clears it for good -- everything below has rock over
    // it whatever the shape of that rock.
    std::vector<uint64> sky(static_cast<std::size_t>(height_) * span * words, 0);
    for (int32 z = 0; z < span; ++z) {
        uint64 open[words] = {~uint64{0}, ~uint64{0}, ~uint64{0}};
        for (int32 y = height_ - 1; y >= 0; --y) {
            const auto at = row_base(y, z);

            uint64 any = 0;
            for (int32 w = 0; w < words; ++w) {
                open[w] &= ~solid[at + static_cast<std::size_t>(w)];
                sky[at + static_cast<std::size_t>(w)] = open[w];
                any |= open[w];
            }

            if (any == 0) {
                break;
            }
        }
    }

    std::vector<int32> current;
    std::vector<int32> next;

    // Rule two starts from the edge of rule one, not from all of it. A skylit
    // voxel has somewhere to give light only if one of its four lateral
    // neighbours is not itself skylit: the one below is either skylit too or
    // solid, by how the mask above is built, and the one above is skylit
    // whenever this one is. Seeding the interior of an open sky instead would
    // make the work proportional to the volume of the column.
    const uint64 all_sky[words] = {~uint64{0}, ~uint64{0}, ~uint64{0}};

    for (int32 y = 0; y < height_; ++y) {
        for (int32 z = 0; z < span; ++z) {
            const auto at        = row_base(y, z);
            const uint64* here   = &sky[at];
            const uint64* north  = (z > 0) ? &sky[row_base(y, z - 1)] : all_sky;
            const uint64* south  = (z + 1 < span) ? &sky[row_base(y, z + 1)] : all_sky;

            if ((here[0] | here[1] | here[2]) == 0) {
                continue;
            }

            uint64 padded[words]{};
            uint64 west[words]{};
            uint64 east[words]{};
            uint64 north_pad[words]{};
            uint64 south_pad[words]{};

            pad_as_sky(here, padded);
            pad_as_sky(north, north_pad);
            pad_as_sky(south, south_pad);
            shift_west(padded, west);
            shift_east(padded, east);

            for (int32 w = 0; w < words; ++w) {
                uint64 lit = here[w];
                while (lit != 0) {
                    const auto b = static_cast<int32>(std::countr_zero(lit));
                    lit &= lit - 1;
                    levels_[static_cast<std::size_t>(
                        index_((w * 64) + b - bit0, y, z)
                    )] = max_level;
                }

                uint64 edge =
                    here[w] & ~(west[w] & east[w] & north_pad[w] & south_pad[w]);
                while (edge != 0) {
                    const auto b = static_cast<int32>(std::countr_zero(edge));
                    edge &= edge - 1;
                    current.push_back(index_((w * 64) + b - bit0, y, z));
                }
            }
        }
    }

    for (uint8 level = max_level; level > 1 && !current.empty(); --level) {
        const auto child = static_cast<uint8>(level - 1);
        next.clear();

        for (const int32 at : current) {
            const int32 x = at % span;
            const int32 z = (at / span) % span;
            const int32 y = at / (span * span);

            const auto visit = [&](int32 nx, int32 ny, int32 nz) {
                if (nx < 0 || nx >= span || nz < 0 || nz >= span || ny < 0 || ny >= height_) {
                    return;
                }

                const int32 bit = nx + bit0;
                if (((solid[row_base(ny, nz) + static_cast<std::size_t>(bit / 64)] >>
                      (bit % 64)) &
                     1U) != 0) {
                    return;
                }

                const auto to = static_cast<std::size_t>(index_(nx, ny, nz));
                if (levels_[to] >= child) {
                    return;
                }

                levels_[to] = child;
                next.push_back(static_cast<int32>(to));
            };

            visit(x - 1, y, z);
            visit(x + 1, y, z);
            visit(x, y, z - 1);
            visit(x, y, z + 1);
            visit(x, y - 1, z);
            visit(x, y + 1, z);
        }

        current.swap(next);
    }
}

auto sky_light_column::bake(int32 y_base) const -> sky_light_field {
    constexpr int32 pages_side = sky_light_field::pages_side;
    constexpr int32 page_count = sky_light_field::page_count;
    constexpr int32 page       = sky_light_field::page;

    if (y_base < 0 || y_base + s > height_) {
        return sky_light_field{};
    }

    // Two passes, because which pages vary is worth knowing before anything is
    // written. The first walks the chunk the way the column stores it -- along
    // x, one row after another -- and tests eight voxels at a time: a run of
    // eight equal levels is one 64-bit compare against the first byte splatted.
    // The second visits only the pages that came out mixed, which on real
    // terrain is one page in forty.
    //
    // Doing it the obvious way instead, page by page through level_at, reads
    // every source byte off a different cache line and costs four times the
    // flood that produced it.
    std::array<uint8, page_count> level{};
    std::array<uint8, page_count> mixed{};

    for (int32 py = 0; py < pages_side; ++py) {
        for (int32 ly = 0; ly < page; ++ly) {
            const int32 y = y_base + (py * page) + ly;

            for (int32 z = 0; z < s; ++z) {
                const uint8* row = row_(y, z);
                const int32 pz   = z / page;

                for (int32 px = 0; px < pages_side; ++px) {
                    uint64 run = 0;
                    std::memcpy(&run, row + (px * page), sizeof(run));

                    const auto first = static_cast<uint8>(run & 0xFFU);
                    const auto slot  = static_cast<std::size_t>(sky_light_field::page_index(px, py, pz));

                    if (ly == 0 && (z % page) == 0) {
                        level[slot] = first;
                    }

                    const bool same =
                        run == (static_cast<uint64>(first) * 0x0101010101010101ULL) &&
                        first == level[slot];
                    mixed[slot] |= static_cast<uint8>(!same);
                }
            }
        }
    }

    const bool chunk_uniform = std::ranges::none_of(mixed, [](uint8 m) -> bool {
        return m != 0;
    }) && std::ranges::all_of(level, [&](uint8 l) -> bool { return l == level[0]; });

    if (chunk_uniform) {
        return sky_light_field{level[0]};
    }

    std::vector<uint16> table(page_count);
    std::vector<sky_light_field::page_type> pages;

    for (int32 pz = 0; pz < pages_side; ++pz) {
        for (int32 py = 0; py < pages_side; ++py) {
            for (int32 px = 0; px < pages_side; ++px) {
                const auto slot = static_cast<std::size_t>(sky_light_field::page_index(px, py, pz));

                if (mixed[slot] == 0) {
                    table[slot] = static_cast<uint16>(level[slot] << 1);
                    continue;
                }

                sky_light_field::page_type packed{};
                for (int32 lz = 0; lz < page; ++lz) {
                    for (int32 ly = 0; ly < page; ++ly) {
                        const uint8* row =
                            row_(y_base + (py * page) + ly, (pz * page) + lz) +
                            (px * page);

                        for (int32 lx = 0; lx < page; ++lx) {
                            const int32 at = lx + (ly * page) + (lz * page * page);
                            packed[static_cast<std::size_t>(at / 2)] |= static_cast<uint8>(
                                (at % 2) == 0 ? row[lx] : (row[lx] << 4)
                            );
                        }
                    }
                }

                table[slot] = static_cast<uint16>(1U | (pages.size() << 1));
                pages.push_back(packed);
            }
        }
    }

    return sky_light_field{std::move(table), std::move(pages)};
}

}  // namespace vw::asset
