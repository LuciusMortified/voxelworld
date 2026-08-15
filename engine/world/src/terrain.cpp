module vw.world;


import std;


namespace vw::ecs {

chunk_loader::chunk_loader(
    std::unique_ptr<terrain_generator> generator
)
    : generator_(std::move(generator)) {
    auto count = std::min(std::thread::hardware_concurrency(), 4u);
    if (count == 0) {
        count = 1;
    }
    for (uint32 i = 0; i < count; ++i) {
        gen_threads_.emplace_back(&chunk_loader::gen_thread_function_, this);
    }
}

chunk_loader::~chunk_loader() {
    {
        std::scoped_lock lock(gen_mutex_);
        gen_running_ = false;
    }
    gen_cv_.notify_all();

    for (auto& t : gen_threads_) {
        if (t.joinable()) {
            t.join();
        }
    }
}

auto chunk_loader::request(
    vec2i coord
) -> bool {
    if (pending_columns_.contains(coord)) {
        return false;
    }
    pending_columns_.insert(coord);

    {
        std::scoped_lock lock(gen_mutex_);
        gen_queue_.push({coord});
        gen_queue_peak_ = std::max(gen_queue_peak_, static_cast<uint32>(gen_queue_.size()));
    }
    gen_cv_.notify_one();
    return true;
}

auto chunk_loader::try_pop_completed() -> std::unique_ptr<gen_column> {
    std::unique_ptr<gen_column> col;
    {
        std::scoped_lock lock(completed_mutex_);
        if (completed_queue_.empty()) {
            return nullptr;
        }
        col = std::move(completed_queue_.front());
        completed_queue_.pop();
    }
    pending_columns_.erase(col->get_coord());
    return col;
}

auto chunk_loader::is_pending(
    vec2i coord
) const -> bool {
    return pending_columns_.contains(coord);
}

auto chunk_loader::pending_count() const -> uint32 {
    return static_cast<uint32>(pending_columns_.size());
}

void chunk_loader::merge_worker_stats_(
    column_gen_worker_stats& worker
) {
    if (worker.columns == 0) {
        return;
    }

    gen_totals_.columns += worker.columns;
    gen_totals_.chunks += worker.chunks;
    gen_totals_.nanos += worker.nanos;
    gen_totals_.micros.insert(
        gen_totals_.micros.end(), worker.micros.begin(), worker.micros.end()
    );

    worker.columns = 0;
    worker.chunks  = 0;
    worker.nanos   = 0;
    worker.micros.clear();
}

auto chunk_loader::get_gen_stats() const -> column_gen_stats {
    std::scoped_lock lock(gen_mutex_);

    column_gen_stats out{};
    out.columns     = gen_totals_.columns;
    out.chunks      = gen_totals_.chunks;
    out.total_ms    = static_cast<float32>(static_cast<float64>(gen_totals_.nanos) / 1.0e6);
    out.queue_depth = static_cast<uint32>(gen_queue_.size());
    out.queue_peak  = gen_queue_peak_;

    if (gen_totals_.micros.empty()) {
        return out;
    }

    auto samples = gen_totals_.micros;
    std::ranges::sort(samples);

    const auto at = [&samples](float32 quantile) -> float32 {
        const auto count = static_cast<float32>(samples.size());
        const auto rank  = static_cast<uint64>(std::ceil(quantile * count));
        const auto index = std::clamp<uint64>(rank, 1, samples.size()) - 1;
        return static_cast<float32>(samples[index]);
    };

    out.mean_us = static_cast<float32>(
        static_cast<float64>(gen_totals_.nanos) / 1000.0 / static_cast<float64>(gen_totals_.columns)
    );
    out.p50_us = at(0.50F);
    out.p99_us = at(0.99F);
    out.max_us = at(1.00F);

    return out;
}

void chunk_loader::gen_thread_function_() {
    column_gen_worker_stats local;

    while (true) {
        gen_task task{};

        {
            std::unique_lock lock(gen_mutex_);
            merge_worker_stats_(local);
            gen_cv_.wait(lock, [this] -> bool { return !gen_queue_.empty() || !gen_running_; });

            if (!gen_running_ && gen_queue_.empty()) {
                break;
            }

            if (!gen_queue_.empty()) {
                task = gen_queue_.front();
                gen_queue_.pop();
            } else {
                continue;
            }
        }

        const auto started = std::chrono::steady_clock::now();

        auto col = std::make_unique<gen_column>(task.coord.x, task.coord.y);

        terrain_context ctx{
            .cx           = task.coord.x,
            .cz           = task.coord.y,
            .create_chunk = [&col](int32 y) -> chunk_data& {
                return col->create_chunk(y, chunk_data{});
            }
        };

        generator_->generate(ctx);
        col->set_phase(column_phase::terrain);

        for (auto& [y, cd] : col->get_all_chunk_data()) {
            cd.chunk_model->compute_own_boundaries();
        }

        const auto elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::steady_clock::now() - started
        );
        ++local.columns;
        local.chunks += col->get_all_chunk_data().size();
        local.nanos += static_cast<uint64>(elapsed.count());
        local.micros.push_back(static_cast<uint32>(elapsed.count() / 1000));

        {
            std::scoped_lock lock(completed_mutex_);
            completed_queue_.push(std::move(col));
        }
    }
}

}  // namespace vw::ecs




namespace vw::ecs {


perlin_terrain_generator::perlin_terrain_generator(
    vw::asset::model_identity_pool& identity_pool, vw::asset::page_pool& pool
)
    : perlin_terrain_generator(identity_pool, pool, params{}) {}

perlin_terrain_generator::perlin_terrain_generator(
    vw::asset::model_identity_pool& identity_pool, vw::asset::page_pool& pool, params p
)
    : identity_pool_(&identity_pool), page_pool_(&pool), params_(p) {
    for (int32 i = 0; i < 256; ++i) {
        perm_[i] = i;
    }

    uint32 state = params_.seed;
    for (int32 i = 255; i > 0; --i) {
        state   = state * 1664525u + 1013904223u;
        int32 j = static_cast<int32>(state % static_cast<uint32>(i + 1));
        std::swap(perm_[i], perm_[j]);
    }

    for (int32 i = 0; i < 256; ++i) {
        perm_[i + 256] = perm_[i];
    }
}

auto perlin_terrain_generator::fade(
    float64 t
) -> float64 {
    return t * t * t * (t * (t * 6.0 - 15.0) + 10.0);
}

auto perlin_terrain_generator::lerp(
    float64 t, float64 a, float64 b
) -> float64 {
    return a + t * (b - a);
}

auto perlin_terrain_generator::grad(
    int32 hash, float64 x, float64 y
) -> float64 {
    int32 h   = hash & 3;
    float64 u = h < 2 ? x : y;
    float64 v = h < 2 ? y : x;
    return ((h & 1) ? -u : u) + ((h & 2) ? -v : v);
}

auto perlin_terrain_generator::noise2d(
    float64 x, float64 y
) const -> float64 {
    int32 xi = static_cast<int32>(std::floor(x)) & 255;
    int32 yi = static_cast<int32>(std::floor(y)) & 255;

    float64 xf = x - std::floor(x);
    float64 yf = y - std::floor(y);

    float64 u = fade(xf);
    float64 v = fade(yf);

    int32 aa = perm_[perm_[xi] + yi];
    int32 ab = perm_[perm_[xi] + yi + 1];
    int32 ba = perm_[perm_[xi + 1] + yi];
    int32 bb = perm_[perm_[xi + 1] + yi + 1];

    float64 x1 = lerp(u, grad(aa, xf, yf), grad(ba, xf - 1.0, yf));
    float64 x2 = lerp(u, grad(ab, xf, yf - 1.0), grad(bb, xf - 1.0, yf - 1.0));

    return lerp(v, x1, x2);
}

auto perlin_terrain_generator::hash_cell(
    int32 i, int32 j, int32 k, uint32 salt
) -> uint32 {
    uint32 h = 0x9E3779B9u ^ salt;
    h        = (h ^ static_cast<uint32>(i)) * 0x85EBCA6Bu;
    h ^= h >> 13;
    h = (h ^ static_cast<uint32>(j)) * 0xC2B2AE35u;
    h ^= h >> 16;
    h = (h ^ static_cast<uint32>(k)) * 0x27D4EB2Fu;
    h ^= h >> 15;
    return h;
}

auto perlin_terrain_generator::cave_node(
    int32 i, int32 j, int32 k
) const -> vec3i {
    const int32 sxz = params_.cave_node_spacing_xz;
    const int32 sy  = params_.cave_node_spacing_y;
    const int32 jit = params_.cave_node_jitter;

    const uint32 h = hash_cell(i, j, k, params_.seed);

    const auto offset = [jit](uint32 bits) -> int32 {
        const auto span = static_cast<uint32>((jit * 2) + 1);
        return static_cast<int32>(bits % span) - jit;
    };

    return vec3i{
        (i * sxz) + (sxz / 2) + offset(h & 0x3FFu),
        (j * sy) + (sy / 2) + offset((h >> 10) & 0x3FFu),
        (k * sxz) + (sxz / 2) + offset((h >> 20) & 0x3FFu),
    };
}

auto perlin_terrain_generator::cave_edge_open(
    int32 i, int32 j, int32 k, int32 axis
) const -> bool {
    const uint32 h = hash_cell(i, j, k, params_.seed + 0x51ED2701u + static_cast<uint32>(axis));

    const float32 chance =
        (axis == 1) ? params_.cave_edge_chance_y : params_.cave_edge_chance_xz;

    constexpr float32 scale = 1.0F / 65536.0F;
    return static_cast<float32>(h & 0xFFFFu) * scale < chance;
}

void perlin_terrain_generator::carve_tunnels_(
    vw::asset::model& mdl, terrain_context& ctx, int32 chunk_y, const column_profile& profile
) const {
    constexpr int32 s = 64;

    const float32 radius = params_.cave_radius;
    const auto reach     = static_cast<int32>(std::ceil(radius)) + 1;

    const int32 x0 = ctx.cx * s;
    const int32 y0 = chunk_y * s;
    const int32 z0 = ctx.cz * s;

    const int32 sxz = params_.cave_node_spacing_xz;
    const int32 sy  = params_.cave_node_spacing_y;

    // A node sits anywhere in its cell and an edge reaches into the next one,
    // so the cells that can touch this chunk extend a cell past it either way.
    const auto floor_div = [](int32 a, int32 b) -> int32 {
        return a >= 0 ? a / b : (a - b + 1) / b;
    };

    const int32 i0 = floor_div(x0 - reach, sxz) - 1;
    const int32 i1 = floor_div(x0 + s + reach, sxz) + 1;
    const int32 j0 = floor_div(y0 - reach, sy) - 1;
    const int32 j1 = floor_div(y0 + s + reach, sy) + 1;
    const int32 k0 = floor_div(z0 - reach, sxz) - 1;
    const int32 k1 = floor_div(z0 + s + reach, sxz) + 1;

    const float32 radius2 = radius * radius;

    const auto carve_segment = [&](vec3i from, vec3i to) {
        const int32 lo_x = std::max(std::min(from.x, to.x) - reach, x0) - x0;
        const int32 hi_x = std::min(std::max(from.x, to.x) + reach, x0 + s - 1) - x0;
        const int32 lo_y = std::max(std::min(from.y, to.y) - reach, y0) - y0;
        const int32 hi_y = std::min(std::max(from.y, to.y) + reach, y0 + s - 1) - y0;
        const int32 lo_z = std::max(std::min(from.z, to.z) - reach, z0) - z0;
        const int32 hi_z = std::min(std::max(from.z, to.z) + reach, z0 + s - 1) - z0;

        if (lo_x > hi_x || lo_y > hi_y || lo_z > hi_z) {
            return;
        }

        const auto ax = static_cast<float32>(from.x);
        const auto ay = static_cast<float32>(from.y);
        const auto az = static_cast<float32>(from.z);
        const auto dx = static_cast<float32>(to.x - from.x);
        const auto dy = static_cast<float32>(to.y - from.y);
        const auto dz = static_cast<float32>(to.z - from.z);

        const float32 length2 = (dx * dx) + (dy * dy) + (dz * dz);
        if (length2 <= 0.0F) {
            return;
        }
        const float32 inv_length2 = 1.0F / length2;

        for (int32 z = lo_z; z <= hi_z; ++z) {
            for (int32 x = lo_x; x <= hi_x; ++x) {
                const int32 index        = (x * s) + z;
                const int32 surface      = profile.surface[index];
                const int32 world_bottom = surface - params_.depth_below_surface;

                for (int32 y = lo_y; y <= hi_y; ++y) {
                    const int32 wy = y0 + y;

                    // The world keeps a floor and a closed lid: no passage
                    // breaks the surface open and none cuts into the bedrock.
                    if ((surface - wy) < params_.cave_surface_margin ||
                        (wy - world_bottom) < params_.bedrock_thickness) {
                        continue;
                    }

                    const float32 px = static_cast<float32>(x0 + x) - ax;
                    const float32 py = static_cast<float32>(wy) - ay;
                    const float32 pz = static_cast<float32>(z0 + z) - az;

                    const float32 t = std::clamp(
                        ((px * dx) + (py * dy) + (pz * dz)) * inv_length2, 0.0F, 1.0F
                    );

                    const float32 ox = px - (t * dx);
                    const float32 oy = py - (t * dy);
                    const float32 oz = pz - (t * dz);

                    if ((ox * ox) + (oy * oy) + (oz * oz) <= radius2) {
                        mdl.set_voxel_raw(x, y, z, voxel{blocks::air});
                    }
                }
            }
        }
    };

    const int32 bend = params_.cave_node_jitter;

    for (int32 j = j0; j <= j1; ++j) {
        for (int32 k = k0; k <= k1; ++k) {
            for (int32 i = i0; i <= i1; ++i) {
                const vec3i from = cave_node(i, j, k);

                for (int32 axis = 0; axis < 3; ++axis) {
                    if (!cave_edge_open(i, j, k, axis)) {
                        continue;
                    }

                    const vec3i to = cave_node(
                        i + (axis == 0 ? 1 : 0), j + (axis == 1 ? 1 : 0), k + (axis == 2 ? 1 : 0)
                    );

                    // The midpoint is pushed aside so a passage bends instead of
                    // running dead straight for the whole span between nodes.
                    const uint32 h = hash_cell(i, j, k, params_.seed + 0x1B873593u +
                                                            static_cast<uint32>(axis));

                    const auto offset = [bend](uint32 bits) -> int32 {
                        const auto span = static_cast<uint32>((bend * 2) + 1);
                        return static_cast<int32>(bits % span) - bend;
                    };

                    const vec3i mid{
                        ((from.x + to.x) / 2) + offset(h & 0x3FFu),
                        ((from.y + to.y) / 2) + (offset((h >> 10) & 0x3FFu) / 2),
                        ((from.z + to.z) / 2) + offset((h >> 20) & 0x3FFu),
                    };

                    carve_segment(from, mid);
                    carve_segment(mid, to);
                }
            }
        }
    }
}

auto perlin_terrain_generator::octave_noise(
    float64 x, float64 y
) const -> float64 {
    float64 total   = 0.0;
    float64 freq    = 1.0;
    float64 amp     = 1.0;
    float64 max_amp = 0.0;

    for (int32 i = 0; i < params_.octaves; ++i) {
        total += noise2d(x * freq, y * freq) * amp;
        max_amp += amp;
        freq *= params_.lacunarity;
        amp *= params_.persistence;
    }

    return total / max_amp;
}

auto perlin_terrain_generator::ridged_noise(
    float64 x, float64 y
) const -> float64 {
    float64 total   = 0.0;
    float64 freq    = 1.0;
    float64 amp     = 1.0;
    float64 max_amp = 0.0;

    for (int32 i = 0; i < params_.octaves; ++i) {
        float64 n = noise2d(x * freq, y * freq);
        n         = 1.0 - std::abs(n);
        n         = n * n;
        total += n * amp;
        max_amp += amp;
        freq *= params_.lacunarity;
        amp *= params_.persistence;
    }

    return total / max_amp;
}

auto perlin_terrain_generator::continent_at(
    float64 nx, float64 nz
) const -> float64 {
    float64 c = octave_noise(nx * params_.continent_frequency, nz * params_.continent_frequency);
    return (c + 1.0) * 0.5;
}

auto perlin_terrain_generator::height_at(
    int32 wx, int32 wz
) const -> int32 {
    auto nx = static_cast<float64>(wx);
    auto nz = static_cast<float64>(wz);

    float64 warp_x =
        octave_noise(nx * params_.warp_frequency + 5.2, nz * params_.warp_frequency + 1.3) *
        params_.warp_strength;
    float64 warp_z =
        octave_noise(nx * params_.warp_frequency + 9.7, nz * params_.warp_frequency + 6.1) *
        params_.warp_strength;

    float64 warped_x = nx + warp_x;
    float64 warped_z = nz + warp_z;

    float64 continent = continent_at(nx, nz);

    float64 terrain =
        octave_noise(warped_x * params_.terrain_frequency, warped_z * params_.terrain_frequency);
    terrain = (terrain + 1.0) * 0.5;

    float64 ridge =
        ridged_noise(warped_x * params_.ridge_frequency, warped_z * params_.ridge_frequency);

    float64 mixed = terrain * (1.0 - params_.ridge_weight) + ridge * params_.ridge_weight;

    int32 base_h;
    float64 amplitude;
    if (continent < 0.35) {
        base_h    = params_.plains_height;
        amplitude = 5.0;
    } else if (continent < 0.55) {
        float64 t = (continent - 0.35) / 0.2;
        base_h    = params_.plains_height +
            static_cast<int32>(t * (params_.hills_height - params_.plains_height));
        amplitude = 5.0 + t * 15.0;
    } else if (continent < 0.75) {
        float64 t = (continent - 0.55) / 0.2;
        base_h    = params_.hills_height +
            static_cast<int32>(t * (params_.mountains_height - params_.hills_height));
        amplitude = 20.0 + t * 20.0;
    } else {
        base_h    = params_.mountains_height;
        amplitude = 40.0;
    }

    return base_h + static_cast<int32>(mixed * amplitude);
}

auto perlin_terrain_generator::block_at(
    int32 y, int32 surface_y, float64 continent
) const -> block_id {
    int32 depth = surface_y - y;

    if (continent >= 0.7) {
        if (depth == 0) {
            if (surface_y > params_.mountains_height + 20)
                return blocks::gray_9;
            return blocks::gray_5;
        }
        return blocks::gray_4;
    }

    if (depth == 0) {
        if (surface_y > params_.mountains_height + 10)
            return blocks::gray_9;
        if (surface_y > params_.hills_height + 10)
            return blocks::gray_4;
        return blocks::green_2;
    }
    if (depth < 3)
        return blocks::brown_0;
    return blocks::gray_3;
}

auto perlin_terrain_generator::surface_height_at(
    int32 wx, int32 wz
) const -> int32 {
    return height_at(wx, wz);
}

void perlin_terrain_generator::generate(
    terrain_context& ctx
) {
    constexpr int32 s = 64;

    const auto profile = sample_column_(ctx.cx, ctx.cz);

    const int32 bottom = profile.min_surface - params_.depth_below_surface;

    auto floor_div = [](int32 a, int32 b) -> int32 { return a >= 0 ? a / b : (a - b + 1) / b; };

    int32 min_cy = floor_div(bottom, s);
    int32 max_cy = floor_div(profile.max_surface, s);

    for (int32 cy = min_cy; cy <= max_cy; ++cy) {
        generate_chunk(ctx, cy, profile);
    }
}

auto perlin_terrain_generator::sample_column_(
    int32 cx, int32 cz
) const -> column_profile {
    constexpr int32 s = column_profile::size;

    column_profile profile{};
    profile.min_surface = std::numeric_limits<int32>::max();
    profile.max_surface = std::numeric_limits<int32>::lowest();

    for (int32 x = 0; x < s; ++x) {
        for (int32 z = 0; z < s; ++z) {
            const int32 wx = (cx * s) + x;
            const int32 wz = (cz * s) + z;
            const int32 surface = height_at(wx, wz);

            const auto index = (x * s) + z;
            profile.surface[index] = surface;
            profile.continent[index] =
                continent_at(static_cast<float64>(wx), static_cast<float64>(wz));

            profile.min_surface = std::min(profile.min_surface, surface);
            profile.max_surface = std::max(profile.max_surface, surface);
        }
    }

    return profile;
}

void perlin_terrain_generator::generate_chunk(
    terrain_context& ctx, int32 chunk_y, const column_profile& profile
) {
    constexpr int32 s = 64;

    auto mdl = std::make_shared<vw::asset::model>(*identity_pool_, *page_pool_, s, s, s, params_.voxel_scale);

    for (int32 x = 0; x < s; ++x) {
        for (int32 z = 0; z < s; ++z) {
            const auto index    = (x * s) + z;
            const int32 surface = profile.surface[index];
            const float64 continent = profile.continent[index];

            const int32 world_bottom = surface - params_.depth_below_surface;
            const int32 top      = std::min(surface, (chunk_y * s) + s - 1);
            const int32 bottom_y = std::max(chunk_y * s, world_bottom);

            for (int32 wy = bottom_y; wy <= top; ++wy) {
                const int32 y = wy - (chunk_y * s);
                if (y < 0 || y >= s) {
                    continue;
                }

                mdl->set_voxel_raw(x, y, z, voxel{block_at(wy, surface, continent)});
            }
        }
    }

    carve_tunnels_(*mdl, ctx, chunk_y, profile);

    // Solid rock and hollowed-out caverns both collapse back to a single page
    // entry here; without this a deep world runs the page pool dry.
    mdl->compact_pages();

    ctx.create_chunk(chunk_y) = {vec3i{ctx.cx, chunk_y, ctx.cz}, std::move(mdl)};
}

}  // namespace vw::ecs

