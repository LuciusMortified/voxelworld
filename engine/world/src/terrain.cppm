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

enum class column_phase : uint8 { empty, terrain, complete };

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

    [[nodiscard]] auto get_all_chunk_data() -> std::unordered_map<int32, chunk_data>& {
        return chunks_;
    }

    void set_phase(column_phase phase) {
        phase_ = phase;
    }

private:
    vec2i coord_;
    column_phase phase_ = column_phase::empty;
    std::unordered_map<int32, chunk_data> chunks_;
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
    explicit chunk_loader(std::unique_ptr<terrain_generator> generator);
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

        int32 shell_depth = 8;

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
    [[nodiscard]] auto octave_noise(float64 x, float64 y) const -> float64;
    [[nodiscard]] auto ridged_noise(float64 x, float64 y) const -> float64;
    [[nodiscard]] auto continent_at(float64 nx, float64 nz) const -> float64;
    [[nodiscard]] auto height_at(int32 wx, int32 wz) const -> int32;
    [[nodiscard]] auto block_at(int32 y, int32 surface_y, float64 continent) const -> block_id;

    // height_at is about twenty noise2d calls, and the same (x, z) used to be
    // re-evaluated once per chunk in the column. Sampled once here instead.
    // The exact extremes also replace the five-sample estimate that decided the
    // column's vertical range, which could clip terrain between its samples.
    struct column_profile {
        static constexpr int32 size = 64;

        std::array<int32, size * size> surface{};
        std::array<float64, size * size> continent{};
        int32 min_surface = 0;
        int32 max_surface = 0;
    };

    [[nodiscard]] auto sample_column_(int32 cx, int32 cz) const -> column_profile;

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
