module;

#include <memory>
#include <optional>
#include <unordered_map>
#include <vector>

export module vw.world:grid;

import vw.core;
import vw.ecs;
import :model;

export namespace vw::ecs {

class world;

// A cube of voxels backed by its own model, owning the entity that carries it
// in the scene.
class chunk {
public:
    static constexpr int32 size   = 64;
    static constexpr int32 volume = size * size * size;

    chunk(world& w, vec3i coord, std::shared_ptr<asset::model> mdl, int32 voxel_scale = 1);
    ~chunk();

    chunk(const chunk&)                    = delete;
    auto operator=(const chunk&) -> chunk& = delete;
    chunk(chunk&& other) noexcept;
    auto operator=(chunk&& other) noexcept -> chunk&;

    [[nodiscard]] auto get_voxel(int32 x, int32 y, int32 z) const -> voxel;
    [[nodiscard]] auto get_voxel(vec3i local) const -> voxel;
    void set_voxel(int32 x, int32 y, int32 z, const voxel& v) const;
    void set_voxel(vec3i local, const voxel& v) const;
    [[nodiscard]] auto is_empty(int32 x, int32 y, int32 z) const -> bool;

    [[nodiscard]] auto get_model() const -> std::shared_ptr<asset::model>;
    [[nodiscard]] auto get_entity() const -> entity;

    [[nodiscard]] static constexpr auto contains(int32 x, int32 y, int32 z) -> bool {
        return x >= 0 && x < size && y >= 0 && y < size && z >= 0 && z < size;
    }

private:
    world* world_;
    entity ent_;
    std::shared_ptr<asset::model> model_;
};

// The loaded part of the voxel world: chunks addressed by chunk coordinate,
// plus the column bookkeeping the loader fills in.
class world_grid {
public:
    explicit world_grid(world& w, int32 voxel_scale = 8);
    ~world_grid() = default;

    world_grid(const world_grid&)                    = delete;
    auto operator=(const world_grid&) -> world_grid& = delete;
    world_grid(world_grid&&)                         = delete;
    auto operator=(world_grid&&) -> world_grid&      = delete;

    [[nodiscard]] auto get_voxel(vec3i world_pos) const -> voxel;
    void set_voxel(vec3i world_pos, const voxel& v);

    [[nodiscard]] auto has_chunk(vec3i chunk_coord) const -> bool;
    [[nodiscard]] auto get_chunk(vec3i chunk_coord) -> chunk*;

    [[nodiscard]] auto get_surface_y(int32 wx, int32 wz) const -> std::optional<int32>;
    [[nodiscard]] auto has_column(vec2i coord) const -> bool;
    [[nodiscard]] auto column_count() const -> uint32;
    [[nodiscard]] auto chunk_count() const -> uint32;

    auto place_chunk(vec3i chunk_coord, std::shared_ptr<asset::model> mdl) -> chunk*;
    void register_column(vec2i coord, std::vector<int32> y_levels);
    void unload_column(vec2i coord);

    [[nodiscard]] auto voxel_scale() const -> int32;

    [[nodiscard]] auto world_to_chunk_coord(vec3i world_pos) const -> vec3i;
    [[nodiscard]] auto world_to_local_coord(vec3i world_pos) const -> vec3i;
    [[nodiscard]] auto chunk_to_world_coord(vec3i chunk_coord) const -> vec3i;

private:
    world* world_;
    int32 voxel_scale_{1};
    std::unordered_map<vec3i, std::unique_ptr<chunk>> chunks_;
    std::unordered_map<vec2i, std::vector<int32>> column_chunks_;
};

}  // namespace vw::ecs
