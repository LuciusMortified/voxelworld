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
    : sky_light_column{
          neighbourhood{{{}, {}, {}, {}, chunks_bottom_up, {}, {}, {}, {}}}, {}
      } {}

sky_light_column::sky_light_column(
    const neighbourhood& around, sky_light_scratch scratch
)
    : buffers_{std::move(scratch)} {
    int32 chunks = 0;
    for (const auto& column : around) {
        chunks = std::max(chunks, static_cast<int32>(column.size()));
    }

    height_ = chunks * s;
    if (height_ == 0) {
        return;
    }

    buffers_.levels.assign(static_cast<std::size_t>(height_) * span * span, 0);
    flood_(around);
}

void sky_light_column::flood_(const neighbourhood& around) {
    auto& levels = buffers_.levels;
    auto& solid  = buffers_.solid;
    auto& sky    = buffers_.sky;

    const auto plane = static_cast<std::size_t>(height_) * span * words;
    solid.assign(plane, 0);

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
    sky.assign(plane, 0);
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

    auto& current = buffers_.frontier;
    auto& next    = buffers_.next;
    current.clear();

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
                    levels[static_cast<std::size_t>(
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
                if (levels[to] >= child) {
                    return;
                }

                levels[to] = child;
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

// The six planes one voxel outside the chunk, taken straight out of the skirt.
// The flood already reached fifteen voxels past the column, so a plane costs
// nothing to read and saves the mesher from having to ask a neighbour that may
// not be there yet.
//
// Two of them can fall off the column. Below its floor is bedrock and reads
// dark; above its top is open sky and reads 15 -- the same rule the flood
// itself uses for a column shorter than the one beside it.
//
// Each face is walked in the order the column stores its levels, x innermost,
// and the plane is written strided instead. Walking the plane in its own order
// steps the column by a row for the Y faces and by a whole layer -- nearly nine
// kilobytes -- for the Z ones, and that alone doubled what the bake cost.
auto gather_boundary(
    const sky_light_column& column, int32 y_base
) -> sky_light_field::boundary_light {
    constexpr int32 side = sky_light_field::side;

    sky_light_field::boundary_light out;

    const auto at = [&](int32 x, int32 y, int32 z) -> uint8 {
        if (y < 0) {
            return 0;
        }
        if (y >= column.height()) {
            return sky_light_column::max_level;
        }
        return column.level_at(x, y, z);
    };

    // Face order is the model's: +X, -X, +Y, -Y, +Z, -Z, and a plane is
    // addressed by the two axes that are not the normal, lower one first.
    for (int32 face = 0; face < sky_light_field::boundary_light::face_count; ++face) {
        std::vector<uint8> packed(static_cast<std::size_t>(side) * side / 2);

        uint8 first  = 0;
        bool uniform = true;

        const auto put = [&](int32 slot, uint8 level) {
            if (slot == 0) {
                first = level;
            }
            uniform = uniform && level == first;

            packed[static_cast<std::size_t>(slot / 2)] |=
                static_cast<uint8>((slot % 2) == 0 ? level : (level << 4));
        };

        switch (face) {
            case 0:
            case 1: {
                // Normal along x, so x is fixed and nothing is contiguous.
                const int32 x = face == 0 ? side : -1;
                for (int32 y = 0; y < side; ++y) {
                    for (int32 z = 0; z < side; ++z) {
                        put((y * side) + z, at(x, y_base + y, z));
                    }
                }
                break;
            }
            case 2:
            case 3: {
                const int32 y = face == 2 ? y_base + side : y_base - 1;
                for (int32 z = 0; z < side; ++z) {
                    for (int32 x = 0; x < side; ++x) {
                        put((x * side) + z, at(x, y, z));
                    }
                }
                break;
            }
            default: {
                const int32 z = face == 4 ? side : -1;
                for (int32 y = 0; y < side; ++y) {
                    for (int32 x = 0; x < side; ++x) {
                        put((x * side) + y, at(x, y_base + y, z));
                    }
                }
                break;
            }
        }

        out.uniform[static_cast<std::size_t>(face)] = first;
        if (!uniform) {
            out.packed[static_cast<std::size_t>(face)] = std::move(packed);
        }
    }

    return out;
}

auto sky_light_column::bake(int32 y_base) const -> sky_light_field {
    constexpr int32 pages_side = sky_light_field::pages_side;
    constexpr int32 page_count = sky_light_field::page_count;
    constexpr int32 page       = sky_light_field::page;

    if (y_base < 0 || y_base + s > height_) {
        return sky_light_field{};
    }

    auto around = gather_boundary(*this, y_base);

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
        return sky_light_field{level[0], std::move(around)};
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

    return sky_light_field{std::move(table), std::move(pages), std::move(around)};
}

}  // namespace vw::asset

namespace vw::ecs {

namespace {

constexpr int32 light_page     = asset::model::page_size;
constexpr int32 pages_per_side = asset::sky_light_column::side / light_page;
constexpr int32 skirt_pages = (asset::sky_light_column::apron + light_page - 1) / light_page;

static_assert(skirt_pages * light_page >= asset::sky_light_column::apron);

}  // namespace

sky_light_baker::sky_light_baker(
    uint32 workers
) {
    auto count = workers != 0 ? workers : std::min(std::thread::hardware_concurrency(), 4U);
    if (count == 0) {
        count = 1;
    }
    for (uint32 i = 0; i < count; ++i) {
        threads_.emplace_back(&sky_light_baker::worker_, this);
    }
}

sky_light_baker::~sky_light_baker() {
    {
        std::scoped_lock lock(mutex_);
        running_ = false;
    }
    cv_.notify_all();

    for (auto& t : threads_) {
        if (t.joinable()) {
            t.join();
        }
    }
}

auto sky_light_baker::request(
    sky_light_request job
) -> bool {
    if (pending_.contains(job.coord)) {
        return false;
    }
    pending_.insert(job.coord);

    {
        std::scoped_lock lock(mutex_);
        queue_.push(std::move(job));
        queue_peak_ = std::max(queue_peak_, static_cast<uint32>(queue_.size()));
    }
    cv_.notify_one();
    return true;
}

auto sky_light_baker::try_pop_completed() -> std::optional<sky_light_result> {
    sky_light_result result;
    {
        std::scoped_lock lock(completed_mutex_);
        if (completed_.empty()) {
            return std::nullopt;
        }
        result = std::move(completed_.front());
        completed_.pop();
    }
    pending_.erase(result.coord);
    return result;
}

auto sky_light_baker::is_pending(
    vec2i coord
) const -> bool {
    return pending_.contains(coord);
}

auto sky_light_baker::pending_count() const -> uint32 {
    return static_cast<uint32>(pending_.size());
}

void sky_light_baker::merge_worker_stats_(
    sky_light_worker_stats& worker
) {
    if (worker.columns == 0) {
        return;
    }

    totals_.columns += worker.columns;
    totals_.rows_nanos += worker.rows_nanos;
    totals_.flood_nanos += worker.flood_nanos;
    totals_.bake_nanos += worker.bake_nanos;
    totals_.micros.insert(totals_.micros.end(), worker.micros.begin(), worker.micros.end());

    worker = sky_light_worker_stats{};
}

auto sky_light_baker::get_stats() const -> sky_light_stats {
    std::scoped_lock lock(mutex_);

    sky_light_stats out{};
    out.columns     = totals_.columns;
    out.rows_ms     = static_cast<float32>(static_cast<float64>(totals_.rows_nanos) / 1.0e6);
    out.flood_ms    = static_cast<float32>(static_cast<float64>(totals_.flood_nanos) / 1.0e6);
    out.bake_ms     = static_cast<float32>(static_cast<float64>(totals_.bake_nanos) / 1.0e6);
    out.queue_depth = static_cast<uint32>(queue_.size());
    out.queue_peak  = queue_peak_;

    if (totals_.micros.empty()) {
        return out;
    }

    auto samples = totals_.micros;
    std::ranges::sort(samples);

    const auto at = [&samples](float32 quantile) -> float32 {
        const auto count = static_cast<float32>(samples.size());
        const auto rank  = static_cast<uint64>(std::ceil(quantile * count));
        const auto index = std::clamp<uint64>(rank, 1, samples.size()) - 1;
        return static_cast<float32>(samples[index]);
    };

    const auto total = totals_.rows_nanos + totals_.flood_nanos + totals_.bake_nanos;

    out.mean_us = static_cast<float32>(
        static_cast<float64>(total) / 1000.0 / static_cast<float64>(totals_.columns)
    );
    out.p50_us = at(0.50F);
    out.p99_us = at(0.99F);
    out.max_us = at(1.00F);

    return out;
}

void sky_light_baker::worker_() {
    sky_light_worker_stats local;

    // Nine columns of occupancy is 4.6 MB. It is scratch, not a result, so a
    // worker keeps it for as long as it lives. Rows outside the page range a
    // neighbour is read at are left as the last job wrote them, which is safe:
    // the flood skips exactly those rows on its own.
    std::vector<std::vector<asset::chunk_occupancy>> held(9);
    std::vector<std::vector<const asset::chunk_occupancy*>> pointers(9);

    // And the seven megabytes the flood itself runs in, for the same reason.
    asset::sky_light_scratch scratch;

    while (true) {
        sky_light_request job;

        {
            std::unique_lock lock(mutex_);
            merge_worker_stats_(local);
            cv_.wait(lock, [this] -> bool { return !queue_.empty() || !running_; });

            if (!running_ && queue_.empty()) {
                break;
            }
            if (queue_.empty()) {
                continue;
            }

            job = std::move(queue_.front());
            queue_.pop();
        }

        const auto started = std::chrono::steady_clock::now();

        asset::sky_light_column::neighbourhood around{};

        for (int32 dz = -1; dz <= 1; ++dz) {
            for (int32 dx = -1; dx <= 1; ++dx) {
                const auto slot  = static_cast<std::size_t>(((dz + 1) * 3) + (dx + 1));
                const auto& from = job.around[slot];

                pointers[slot].clear();

                // Only as deep as the skirt reaches. Fifteen voxels is two
                // pages of eight, so a side neighbour is read two page columns
                // wide and a corner two by two -- 2.25 columns of work for the
                // whole neighbourhood instead of nine.
                //
                // Rows outside that range are left as the last job wrote them,
                // which is safe: the flood skips exactly those rows on its own,
                // and everything west or east of the skirt is sealed solid
                // whatever it says.
                const int32 px0 = dx < 0 ? pages_per_side - skirt_pages : 0;
                const int32 px1 = dx > 0 ? skirt_pages : pages_per_side;
                const int32 pz0 = dz < 0 ? pages_per_side - skirt_pages : 0;
                const int32 pz1 = dz > 0 ? skirt_pages : pages_per_side;

                held[slot].resize(from.size());

                for (std::size_t i = 0; i < from.size(); ++i) {
                    if (from[i] == nullptr) {
                        pointers[slot].push_back(nullptr);
                        continue;
                    }

                    static_cast<void>(from[i]->build_x_rows(held[slot][i], px0, px1, pz0, pz1));
                    pointers[slot].push_back(&held[slot][i]);
                }

                around[slot] = pointers[slot];
            }
        }

        const auto rowed = std::chrono::steady_clock::now();

        asset::sky_light_column light{around, std::move(scratch)};

        const auto flooded = std::chrono::steady_clock::now();

        sky_light_result result;
        result.coord    = job.coord;
        result.bottom_y = job.bottom_y;
        result.fields.reserve(job.around[4].size());

        for (std::size_t i = 0; i < job.around[4].size(); ++i) {
            result.fields.push_back(
                light.bake(static_cast<int32>(i) * asset::sky_light_field::side)
            );
        }

        const auto baked = std::chrono::steady_clock::now();

        scratch = std::move(light).release();

        const auto span = [](auto from, auto to) -> uint64 {
            return static_cast<uint64>(
                std::chrono::duration_cast<std::chrono::nanoseconds>(to - from).count()
            );
        };

        ++local.columns;
        local.rows_nanos += span(started, rowed);
        local.flood_nanos += span(rowed, flooded);
        local.bake_nanos += span(flooded, baked);
        local.micros.push_back(static_cast<uint32>(span(started, baked) / 1000));

        {
            std::scoped_lock lock(completed_mutex_);
            completed_.push(std::move(result));
        }
    }
}

}  // namespace vw::ecs
