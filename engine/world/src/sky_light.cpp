module vw.world;

import std;

namespace vw::asset {

namespace {

constexpr int32 s = sky_light_column::side;

[[nodiscard]] auto row_of(int32 y, int32 z) -> std::size_t {
    return (static_cast<std::size_t>(y) * s) + static_cast<std::size_t>(z);
}

}  // namespace

sky_light_column::sky_light_column(std::span<const chunk_occupancy* const> chunks_top_down)
    : height_{static_cast<int32>(chunks_top_down.size()) * s} {
    if (height_ == 0) {
        return;
    }

    levels_.assign(static_cast<std::size_t>(height_) * s * s, 0);

    // One flat bit field for the whole column, so the flood never repeats the
    // top-down-to-bottom-up arithmetic once per neighbour test.
    std::vector<uint64> solid(static_cast<std::size_t>(height_) * s, 0);

    const auto chunks = static_cast<int32>(chunks_top_down.size());
    for (int32 i = 0; i < chunks; ++i) {
        const chunk_occupancy* occ = chunks_top_down[i];
        if (occ == nullptr) {
            continue;
        }

        const int32 base = (chunks - 1 - i) * s;
        for (int32 ly = 0; ly < s; ++ly) {
            for (int32 z = 0; z < s; ++z) {
                solid[row_of(base + ly, z)] = occ->row(ly, z);
            }
        }
    }

    // Rule one. A bit stays set while its column still sees the sky, and the
    // first opaque voxel clears it for good -- everything below has rock over
    // it whatever the shape of that rock.
    std::vector<uint64> sky(static_cast<std::size_t>(height_) * s, 0);
    for (int32 z = 0; z < s; ++z) {
        uint64 open = ~uint64{0};
        for (int32 y = height_ - 1; y >= 0; --y) {
            open &= ~solid[row_of(y, z)];
            if (open == 0) {
                break;
            }
            sky[row_of(y, z)] = open;
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
    for (int32 y = 0; y < height_; ++y) {
        for (int32 z = 0; z < s; ++z) {
            const uint64 here = sky[row_of(y, z)];
            if (here == 0) {
                continue;
            }

            const uint64 west  = (here << 1) | 1U;
            const uint64 east  = (here >> 1) | (uint64{1} << (s - 1));
            const uint64 north = (z > 0) ? sky[row_of(y, z - 1)] : ~uint64{0};
            const uint64 south = (z + 1 < s) ? sky[row_of(y, z + 1)] : ~uint64{0};

            uint64 bits = here;
            while (bits != 0) {
                const auto x = static_cast<int32>(std::countr_zero(bits));
                bits &= bits - 1;
                levels_[static_cast<std::size_t>(index_(x, y, z))] = max_level;
            }

            uint64 edge = here & ~(west & east & north & south);
            while (edge != 0) {
                const auto x = static_cast<int32>(std::countr_zero(edge));
                edge &= edge - 1;
                current.push_back(index_(x, y, z));
            }
        }
    }

    for (uint8 level = max_level; level > 1 && !current.empty(); --level) {
        const auto child = static_cast<uint8>(level - 1);
        next.clear();

        for (const int32 at : current) {
            const int32 x = at % s;
            const int32 z = (at / s) % s;
            const int32 y = at / (s * s);

            const auto visit = [&](int32 nx, int32 ny, int32 nz) {
                if (nx < 0 || nx >= s || nz < 0 || nz >= s || ny < 0 || ny >= height_) {
                    return;
                }
                if (((solid[row_of(ny, nz)] >> nx) & 1U) != 0) {
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

auto sky_light_column::count_pages() const -> page_stats {
    page_stats stats;

    for (int32 py = 0; py < height_ / page; ++py) {
        for (int32 pz = 0; pz < s / page; ++pz) {
            for (int32 px = 0; px < s / page; ++px) {
                const uint8 first = level_at(px * page, py * page, pz * page);

                bool uniform = true;
                for (int32 y = 0; y < page && uniform; ++y) {
                    for (int32 z = 0; z < page && uniform; ++z) {
                        for (int32 x = 0; x < page; ++x) {
                            if (level_at((px * page) + x, (py * page) + y, (pz * page) + z)
                                != first) {
                                uniform = false;
                                break;
                            }
                        }
                    }
                }

                if (!uniform) {
                    ++stats.mixed;
                } else if (first == max_level) {
                    ++stats.lit;
                } else if (first == 0) {
                    ++stats.dark;
                } else {
                    ++stats.uniform_other;
                }
            }
        }
    }

    return stats;
}

}  // namespace vw::asset
