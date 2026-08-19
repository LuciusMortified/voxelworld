export module vw.world:terrain;

import std;

import vw.core;
import :model;

export namespace vw::ecs {

struct chunk_data {
    vec3i coord;
    std::shared_ptr<asset::model> chunk_model;
};

struct chunk_y_range {
    int32 min_y = 0;
    int32 max_y = 0;
};

struct terrain_context {
    int32 cx;
    int32 cz;
    std::function<auto(int32 y) -> chunk_data&> create_chunk;
};

class terrain_generator {
public:
    virtual ~terrain_generator()                = default;
    virtual void generate(terrain_context& ctx) = 0;
};

// A column is generated, then lit -- which needs its eight neighbours to
// exist -- and only then placed. Placing it before the light is in would
// mesh every chunk twice.
enum class column_phase : uint8 { empty, terrain, lighting, complete };

// One vertical stack of chunks as it is being generated, before the chunks
// reach the grid.
class gen_column {
public:
    explicit gen_column(int32 cx, int32 cz) : coord_{cx, cz} {}

    [[nodiscard]] auto get_coord() const -> vec2i {
        return coord_;
    }

    [[nodiscard]] auto get_phase() const -> column_phase {
        return phase_;
    }

    [[nodiscard]] auto has_chunk_data(int32 y) const -> bool {
        return chunks_.contains(y);
    }

    [[nodiscard]] auto get_chunk_data(int32 y) -> chunk_data* {
        const auto it = chunks_.find(y);
        return it != chunks_.end() ? &it->second : nullptr;
    }

    [[nodiscard]] auto get_chunk_data(int32 y) const -> const chunk_data* {
        const auto it = chunks_.find(y);
        return it != chunks_.end() ? &it->second : nullptr;
    }

    auto create_chunk(int32 y, chunk_data data) -> chunk_data& {
        auto [it, inserted] = chunks_.emplace(y, std::move(data));
        return it->second;
    }

    // Ordered by descending Y, so every walk over a column -- generation,
    // boundaries, placement -- goes from the sky down. Of nine chunks in a
    // column the viewer normally sees one, the surface, and it is the one that
    // now gets meshed first. Sky light will travel the same way, and the height
    // map it needs is already in column_profile::surface.
    using chunk_map = std::map<int32, chunk_data, std::greater<>>;

    [[nodiscard]] auto get_all_chunk_data() -> chunk_map& {
        return chunks_;
    }

    void set_phase(column_phase phase) {
        phase_ = phase;
    }

private:
    vec2i coord_;
    column_phase phase_ = column_phase::empty;
    chunk_map chunks_;
};

// Generates columns on worker threads and hands the finished ones back to the
// main thread through a queue.
// Column generation runs on its own workers, so it never shows up in a frame
// percentile. Counters live per worker and merge under the queue lock the
// worker already holds.
struct column_gen_worker_stats {
    uint64 columns = 0;
    uint64 chunks  = 0;
    uint64 nanos   = 0;
    std::vector<uint32> micros;
};

struct column_gen_stats {
    uint64 columns     = 0;
    uint64 chunks      = 0;
    float32 total_ms   = 0.0F;
    float32 mean_us    = 0.0F;
    float32 p50_us     = 0.0F;
    float32 p99_us     = 0.0F;
    float32 max_us     = 0.0F;
    uint32 queue_depth = 0;
    uint32 queue_peak  = 0;
};

class chunk_loader {
public:
    // Zero asks for the default, which is what the curve in
    // docs/frame-time-baseline.md settled on.
    explicit chunk_loader(std::unique_ptr<terrain_generator> generator, uint32 workers = 0);
    ~chunk_loader();

    chunk_loader(const chunk_loader&)                    = delete;
    auto operator=(const chunk_loader&) -> chunk_loader& = delete;
    chunk_loader(chunk_loader&&)                         = delete;
    auto operator=(chunk_loader&&) -> chunk_loader&      = delete;

    auto request(vec2i coord) -> bool;

    [[nodiscard]] auto try_pop_completed() -> std::unique_ptr<gen_column>;
    [[nodiscard]] auto is_pending(vec2i coord) const -> bool;
    [[nodiscard]] auto pending_count() const -> uint32;
    [[nodiscard]] auto get_gen_stats() const -> column_gen_stats;

private:
    void gen_thread_function_();
    void merge_worker_stats_(column_gen_worker_stats& worker);

    struct gen_task {
        vec2i coord;
    };

    std::unique_ptr<terrain_generator> generator_;
    std::vector<std::thread> gen_threads_;
    std::queue<gen_task> gen_queue_;
    std::queue<std::unique_ptr<gen_column>> completed_queue_;
    mutable std::mutex gen_mutex_;
    mutable std::mutex completed_mutex_;
    std::condition_variable gen_cv_;
    bool gen_running_ = true;
    std::unordered_set<vec2i> pending_columns_;

    column_gen_worker_stats gen_totals_;
    uint32 gen_queue_peak_ = 0;
};

class perlin_terrain_generator final : public terrain_generator {
public:
    struct params {
        uint32 seed       = 42;
        int32 voxel_scale = 8;

        // The world stands on a floor at a fixed height instead of a fixed
        // thickness under the terrain. Layers of rock -- and the dungeon
        // storeys that come later -- have to sit at the same height everywhere,
        // and a bottom that follows the hills would put them on a slope. The
        // value is a multiple of the chunk size, so no column ends in a partly
        // filled chunk.
        //
        // A thousand voxels was tried first and cost 1.2 GB and a 46 ms frame:
        // it is fifteen chunks of solid rock per column that nothing looks at,
        // and the engine has no way yet to leave them alone. Half of that is
        // the room caves need without paying for the rest.
        int32 world_bottom_y = -448;

        // Ground is built out of layers rather than painted on top of one
        // height field: rock has its own relief, soil lies on it, and a
        // mountain is simply where the rock rises out of the soil. Soil slides
        // off steep ground and thins out with altitude, so bare peaks need no
        // special case.
        int32 soil_depth_max      = 5;
        float32 soil_frequency    = 0.012F;
        float32 soil_slope_limit  = 1.4F;
        int32 soil_altitude_start = 55;
        int32 soil_altitude_end   = 78;
        int32 snow_line           = 82;

        // The few voxels of rock right under the ground are weathered, and
        // below that the rock changes with absolute depth -- a cave wall says
        // how deep it is.
        int32 rock_skin     = 4;
        int32 rock_deep_y   = -64;
        int32 rock_bottom_y = -256;

        // Caves come out of noise, and the noise is gated by a field: a low
        // frequency layer that says where the underground is hollow at all.
        // Inside a field there is a cave system of several storeys; between
        // fields the rock is solid. Dungeons will be the connected underground
        // -- caves are places, and a place has edges.
        //
        // Two earlier shapes are in docs/frame-time-baseline.md and both were
        // measured: a lattice of tunnels made one network of even calibre over
        // the whole world, and chains of hand-placed chambers read as rooms but
        // could never fill the depth. This keeps the clustering of the second
        // and gets its shapes from noise like the first.
        // One switch rather than a threshold no noise can reach: entrances
        // carry a field of their own, so silencing the blobs alone still left
        // holes in the ground.
        bool caves = true;

        // Three dimensional: a field is a body of hollow rock at some place
        // and some depth, not a column running the whole way down. Two
        // dimensional was measured first and is what made every chunk of the
        // world carry geometry -- 3 113 chunks drawn out of 3 594, against
        // 1 187 before -- and the generator forty times slower.
        float32 cave_field_frequency = 0.0090F;
        float32 cave_field_squash    = 0.55F;
        float32 cave_field_threshold = 0.26F;
        float32 cave_field_falloff   = 0.30F;

        // Fields are commoner in the rock just under the ground, which is both
        // how karst behaves and what gives the ways in something to lead to:
        // with a flat field two patches of the map out of four had a big system
        // and no opening anywhere.
        float32 cave_field_surface_bias = 0.10F;
        int32 cave_field_surface_reach  = 150;

        // The caverns: a 3D field thresholded near its zero surface, squashed
        // along Y so a chamber is wider than it is tall and a body can walk it.
        float32 cave_cheese_frequency = 0.0135F;
        float32 cave_cheese_squash    = 2.4F;
        float32 cave_cheese_width     = 0.090F;

        // The passages: where two independent fields both approach zero. That
        // intersection is a curve rather than a surface, which is what makes a
        // tunnel instead of a sheet.
        float32 cave_tunnel_frequency = 0.019F;
        float32 cave_tunnel_width     = 0.045F;

        // Storeys. Caverns widen in bands at fixed heights, so one field holds
        // levels that a passage then connects, and the levels line up with the
        // rock strata rather than with the hills above.
        int32 cave_level_spacing   = 46;
        float32 cave_level_contrast = 0.55F;

        // Daylight. A patch where the roof leaks is a field in its own right
        // for the depth of the fade: the cave noise gets carved there whether
        // or not a blob happens to reach up, so the hole exists and whatever is
        // under it is joined to the sky. Letting an entrance only lift the
        // penalty on an existing field was measured twice and left whole
        // patches of the map sealed.
        float32 cave_entrance_frequency = 0.0210F;
        float32 cave_entrance_threshold = 0.21F;
        float32 cave_entrance_falloff   = 0.12F;
        float32 cave_entrance_field     = 0.85F;

        // How far down the way in keeps carving. A hole that stops at the end
        // of the fade is a pit; one that reaches the depth fields live at is a
        // shaft, and joins what is under it to the sky.
        int32 cave_entrance_depth       = 110;
        float32 cave_entrance_lift      = 0.11F;

        // How the cave fades out as it approaches the surface, and how far
        // above the world floor it stops.
        int32 cave_surface_margin = 7;
        int32 cave_surface_fade   = 22;
        int32 bedrock_thickness   = 3;

        // The noise is sampled on a grid this many voxels apart and read back
        // with trilinear interpolation. Per voxel it would be four 3D samples
        // for every voxel of the world; at four it is one in sixty-four, and a
        // passage two voxels across still survives the interpolation.
        int32 cave_sample_stride = 4;

        float32 continent_frequency = 0.003F;
        float32 terrain_frequency   = 0.02F;
        int32 octaves               = 4;
        float32 lacunarity          = 2.0F;
        float32 persistence         = 0.5F;

        int32 plains_height    = 20;
        int32 hills_height     = 35;
        int32 mountains_height = 55;

        float32 ridge_frequency = 0.015F;
        float32 ridge_weight    = 0.6F;

        float32 warp_frequency = 0.01F;
        float32 warp_strength  = 30.0F;
    };

    perlin_terrain_generator(asset::model_identity_pool& identity_pool, asset::page_pool& pool);
    perlin_terrain_generator(asset::model_identity_pool& identity_pool, asset::page_pool& pool,
                             params p);

    void generate(terrain_context& ctx) override;

    [[nodiscard]] auto surface_height_at(int32 wx, int32 wz) const -> int32;

private:
    [[nodiscard]] auto noise2d(float64 x, float64 y) const -> float64;

    [[nodiscard]] auto noise3d(float64 x, float64 y, float64 z) const -> float64;

    // How much the cave field wants this rock hollow, 0 outside a field and 1
    // well inside one. Squashed along Y, so a field is wider than it is deep
    // and reads as a region of the map rather than a shaft.
    [[nodiscard]] auto cave_field_at(int32 wx, int32 wy, int32 wz, int32 depth) const -> float32;

    // Positive where rock becomes air. Caverns and passages are two terms of
    // the same value, so one interpolated number decides both.
    [[nodiscard]] auto cave_openness_at(
        int32 wx, int32 wy, int32 wz, float32 field, int32 surface, float32 leak
    ) const -> float32;

    // How leaky the roof is over this column, 0 for solid ground and 1 for a
    // way in.
    [[nodiscard]] auto cave_entrance_leak_at(int32 wx, int32 wz) const -> float32;

    [[nodiscard]] auto octave_noise(float64 x, float64 y) const -> float64;
    [[nodiscard]] auto ridged_noise(float64 x, float64 y) const -> float64;
    [[nodiscard]] auto continent_at(float64 nx, float64 nz) const -> float64;

    // The relief of the rock, without the soil on top of it.
    [[nodiscard]] auto stone_height_at(int32 wx, int32 wz) const -> int32;
    [[nodiscard]] auto soil_depth_at(int32 wx, int32 wz, int32 stone, float32 slope) const -> int32;

    [[nodiscard]] auto rock_block_at(int32 wy) const -> block_id;
    [[nodiscard]] auto block_at(int32 wy, int32 stone_top, int32 surface_top) const -> block_id;

    // stone_height_at is about twenty noise2d calls, and the same (x, z) used
    // to be re-evaluated once per chunk in the column. Sampled once here
    // instead. The exact extremes also replace the five-sample estimate that
    // decided the column's vertical range, which could clip terrain between its
    // samples.
    struct column_profile {
        static constexpr int32 size   = 64;
        static constexpr int32 apron  = 1;
        static constexpr int32 stride = size + (2 * apron);
        static constexpr int32 page   = 8;
        static constexpr int32 pages  = size / page;

        // Rock heights carry a one-voxel apron, so the slope that decides how
        // much soil settles is a difference over samples already taken rather
        // than four more noise evaluations per voxel column.
        std::array<int32, stride * stride> stone{};
        std::array<int32, size * size> surface{};

        // Per page footprint (8 x 8 voxels): the lowest rock and the highest
        // surface over it. A page below the first is solid rock and a page
        // above the second is air; neither needs to be visited voxel by voxel.
        std::array<int32, pages * pages> page_min_stone{};
        std::array<int32, pages * pages> page_max_surface{};

        int32 min_stone   = 0;
        int32 max_surface = 0;

        [[nodiscard]] static auto stone_index(int32 x, int32 z) -> int32 {
            return ((x + apron) * stride) + (z + apron);
        }
    };

    [[nodiscard]] auto sample_column_(int32 cx, int32 cz) const -> column_profile;

    void carve_caves_(asset::model& mdl, terrain_context& ctx, int32 chunk_y,
                      const column_profile& profile) const;

    void generate_chunk(terrain_context& ctx, int32 chunk_y, const column_profile& profile);

    static auto fade(float64 t) -> float64;
    static auto lerp(float64 t, float64 a, float64 b) -> float64;
    static auto grad(int32 hash, float64 x, float64 y) -> float64;

    asset::model_identity_pool* identity_pool_;
    asset::page_pool* page_pool_;
    params params_;
    std::array<int32, 512> perm_;
};

}  // namespace vw::ecs
