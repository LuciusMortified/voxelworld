export module vw.world:grid;

import std;

import vw.core;
import vw.ecs;
import :model;
import :sky_light;

export namespace vw::ecs {

class world;

// Directions in the order chunk_links uses: -X, +X, -Y, +Y, -Z, +Z. The
// opposite of a direction is its partner in the pair, so d ^ 1.
inline constexpr std::array<vec3i, 6> chunk_face_offsets{
    vec3i{-1, 0, 0}, vec3i{1, 0, 0}, vec3i{0, -1, 0},
    vec3i{0, 1, 0},  vec3i{0, 0, -1}, vec3i{0, 0, 1},
};

// The order the mesher and set_boundary_slice use, which is the other one:
// +X, -X, +Y, -Y, +Z, -Z. Opposite is still d ^ 1.
inline constexpr std::array<vec3i, 6> boundary_face_offsets{
    vec3i{1, 0, 0},  vec3i{-1, 0, 0}, vec3i{0, 1, 0},
    vec3i{0, -1, 0}, vec3i{0, 0, 1},  vec3i{0, 0, -1},
};

// Breadth-first walk from the chunk holding the viewer, following the pockets
// of air that join up across chunk faces. Sight enters a chunk through one
// pocket and can only leave through the openings of that same pocket, so solid
// rock -- or a hillside with tunnels behind it -- ends the walk. That is what
// takes a whole cave system out of the frame when it is seen from above.
//
// Conservative where it can afford to be: a loaded chunk whose connectivity is
// not known yet counts as fully open, so the walk reports more than is truly
// visible rather than less.
//
// An empty cell is either sky or nothing. Sky is open -- the camera stands in
// it. A gap inside the volume of the world is not entered at all: it holds no
// geometry to see, and treating it as open lets a tunnel that runs off the edge
// of the loaded world carry the walk underneath everything and back up the far
// side.
//
// Bounds are inclusive.
//
// Coordinates here are connectivity cells, not chunks: a chunk holds
// chunk_links::cells_per_side of them along each axis. Whole chunks are too
// coarse to walk -- see the note on chunk_links.
//
// links_at(vec3i) -> const asset::cell_links* for loaded cells and null for the
// rest, is_sky(vec3i) -> bool, starts_in(const asset::chunk_pocket&) -> bool
// picks the pocket the viewer occupies, visit(vec3i) -> void.
template <typename LinksAt, typename IsSky, typename StartsIn, typename Visit>
void walk_visible_chunks(
    vec3i origin, vec3i lo, vec3i hi, LinksAt&& links_at, IsSky&& is_sky, StartsIn&& starts_in,
    Visit&& visit
) {
    constexpr int32 face_count = asset::chunk_pocket::face_count;

    // A cell with no connectivity of its own -- sky, or a chunk that has not
    // been meshed yet -- is one pocket open on every face.
    static const asset::chunk_pocket open_pocket = asset::chunk_pocket::wide_open();

    // Which pockets of which chunk have already been queued, plus a top bit for
    // "already reported". A chunk is walked once per pocket, not once per entry
    // face: two pockets of the same chunk are two different places.
    constexpr uint64 seen_bit = uint64{1} << 63;
    std::unordered_map<vec3i, uint64> queued;
    std::vector<std::pair<vec3i, int32>> pending;

    const auto pockets_of = [&](vec3i coord) -> std::span<const asset::chunk_pocket> {
        const auto* links = links_at(coord);
        if (links == nullptr) {
            return {&open_pocket, 1};
        }
        return links->pockets;
    };

    // The viewer sees out of the pocket it stands in, and only that one. A
    // camera in open air over a hillside shares its cell with the tunnels
    // beneath it, and starting from every pocket would hand it the whole
    // network -- which is exactly what it did.
    {
        const auto pockets = pockets_of(origin);
        uint64 mask        = seen_bit;

        bool found = false;
        for (std::size_t i = 0; i < pockets.size(); ++i) {
            if (!starts_in(pockets[i])) {
                continue;
            }
            found = true;
            mask |= uint64{1} << i;
            pending.emplace_back(origin, static_cast<int32>(i));
        }

        // Inside rock, or nothing known about this cell: fall back to every
        // pocket, which over-reports rather than hiding something.
        if (!found) {
            for (std::size_t i = 0; i < pockets.size(); ++i) {
                mask |= uint64{1} << i;
                pending.emplace_back(origin, static_cast<int32>(i));
            }
        }

        queued[origin] = mask;
        visit(origin);
    }

    while (!pending.empty()) {
        const auto [coord, pocket_index] = pending.back();
        pending.pop_back();

        const auto pockets = pockets_of(coord);
        if (static_cast<std::size_t>(pocket_index) >= pockets.size()) {
            continue;
        }
        const auto& pocket = pockets[static_cast<std::size_t>(pocket_index)];

        for (int32 face = 0; face < face_count; ++face) {
            if (!pocket.touches(face)) {
                continue;
            }

            const vec3i next = coord + chunk_face_offsets[face];
            if (next.x < lo.x || next.y < lo.y || next.z < lo.z || next.x > hi.x ||
                next.y > hi.y || next.z > hi.z) {
                continue;
            }

            const auto* next_links = links_at(next);
            if (next_links == nullptr && !is_sky(next)) {
                continue;
            }

            const auto next_pockets = next_links == nullptr
                ? std::span<const asset::chunk_pocket>{&open_pocket, 1}
                : std::span<const asset::chunk_pocket>{next_links->pockets};

            // Seen, whether or not sight goes any further: the pocket reaches
            // this face, so the near side of the neighbour is in view. A wall
            // is exactly the chunk the walk stops at, and it has to be drawn.
            auto& mask = queued[next];
            if ((mask & seen_bit) == 0) {
                mask |= seen_bit;
                visit(next);
            }

            // Further in only through a pocket that lines up with this one.
            for (std::size_t i = 0; i < next_pockets.size(); ++i) {
                if (!pocket.meets(next_pockets[i], face)) {
                    continue;
                }
                if ((mask & (uint64{1} << i)) != 0) {
                    continue;
                }
                mask |= uint64{1} << i;
                pending.emplace_back(next, static_cast<int32>(i));
            }
        }
    }
}

// Everything empty counts as sky. For tests and for callers with no world
// underneath the walk.
template <typename LinksAt, typename Visit>
void walk_visible_chunks(vec3i origin, int32 radius, LinksAt&& links_at, Visit&& visit) {
    const vec3i extent{radius, radius, radius};
    walk_visible_chunks(
        origin, origin - extent, origin + extent, std::forward<LinksAt>(links_at),
        [](vec3i) { return true; }, [](const asset::chunk_pocket&) { return true; },
        std::forward<Visit>(visit)
    );
}

// A cube of voxels backed by its own model, owning the entity that carries it
// in the scene.
//
// A chunk with nothing to show gets no entity at all. Buried rock is the common
// case -- seven of nine chunks in a column of a world 512 voxels deep -- and an
// entity is what buys a mesh, a GPU instance and a place in every per-frame
// walk. The voxels are still here: the world can be dug into, and digging is
// what hands the chunk its entity.
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
    void set_voxel(int32 x, int32 y, int32 z, const voxel& v);
    void set_voxel(vec3i local, const voxel& v);
    [[nodiscard]] auto is_empty(int32 x, int32 y, int32 z) const -> bool;

    [[nodiscard]] auto get_model() const -> std::shared_ptr<asset::model>;

    // Invalid for a chunk that is not in the scene. Callers that look an entity
    // up in a table get nothing back, which is the right answer, but the
    // visibility walk has to ask is_solid instead: an unmeshed chunk counts as
    // wide open there, and solid rock is the opposite of that.
    [[nodiscard]] auto get_entity() const -> entity;
    [[nodiscard]] auto is_drawn() const -> bool;
    [[nodiscard]] auto is_solid() const -> bool;

    // Which of the six neighbours the chunk was told about when it was placed.
    // The planes themselves are dropped as soon as the mesher is done with them
    // -- or straight away for a chunk that will never be meshed -- so the model
    // stops answering; whether they were there is what says the chunk was
    // placed into a complete neighbourhood.
    [[nodiscard]] auto known_neighbors() const -> uint8;

    // Brings a skipped chunk into the scene. True when it was not there before.
    auto ensure_entity() -> bool;

    [[nodiscard]] static constexpr auto contains(int32 x, int32 y, int32 z) -> bool {
        return x >= 0 && x < size && y >= 0 && y < size && z >= 0 && z < size;
    }

    void set_known_neighbors(uint8 mask);

private:
    void create_entity_();

    world* world_;
    vec3i coord_{};
    int32 voxel_scale_{1};
    entity ent_;
    std::shared_ptr<asset::model> model_;
    asset::model_fill fill_ = asset::model_fill::mixed;
    uint8 known_neighbors_  = 0;
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

    // The topmost solid voxel over a spot, in voxels rather than world units --
    // both the argument and the answer. Everything else on this class takes
    // world units, so multiply by voxel_scale() to get back to them.
    [[nodiscard]] auto get_surface_y(int32 vx, int32 vz) const -> std::optional<int32>;
    [[nodiscard]] auto has_column(vec2i coord) const -> bool;

    // Which chunk levels a loaded column has. Empty for a column that is not
    // loaded; the levels are sorted.
    [[nodiscard]] auto column_levels(vec2i coord) const -> std::span<const int32>;
    [[nodiscard]] auto column_count() const -> uint32;
    [[nodiscard]] auto chunk_count() const -> uint32;

    // Of those, the ones that made it into the scene. The rest are solid rock
    // or open air: loaded and walkable, but with nothing to draw.
    [[nodiscard]] auto drawn_chunk_count() const -> uint32;

    auto place_chunk(vec3i chunk_coord, std::shared_ptr<asset::model> mdl) -> chunk*;
    void register_column(vec2i coord, std::vector<int32> y_levels);
    void unload_column(vec2i coord);

    // Hands a chunk its six neighbour planes again and puts it up for meshing.
    // The planes are a cache over the neighbours' voxels and are dropped once
    // the mesher is done, so an edit re-derives them -- which is what an edit
    // needs anyway, since a copy kept from placement would now be the stale
    // side of the seam.
    void refresh_chunk(vec3i chunk_coord);

    // The same, for a chunk that is already in the scene, and only for one:
    // relighting touches every chunk of a column, and buried rock has no faces
    // for light to land on. refresh_chunk would hand it an entity and a mesh
    // for nothing.
    void remesh_drawn_chunk(vec3i chunk_coord);

    // Columns whose sky light no longer matches their voxels, taken and
    // cleared. The grid does not know what light is -- it knows which voxels
    // moved and how far a change can carry -- so it names the columns and
    // leaves the relighting to whoever owns the baker.
    [[nodiscard]] auto take_light_dirty() -> std::vector<vec2i>;

    [[nodiscard]] auto voxel_scale() const -> int32;

    // f(vec3i coord, const chunk&). Used by the renderer, which has to say
    // something about every loaded chunk each frame, visible or not.
    template <typename F>
    void for_each_chunk(F&& f) const {
        for (const auto& [coord, ptr] : chunks_) {
            f(coord, *ptr);
        }
    }

    [[nodiscard]] auto world_to_chunk_coord(vec3i world_pos) const -> vec3i;
    [[nodiscard]] auto world_to_local_coord(vec3i world_pos) const -> vec3i;
    [[nodiscard]] auto chunk_to_world_coord(vec3i chunk_coord) const -> vec3i;

private:
    void mark_light_dirty_(vec3i chunk_coord, vec3i local);

    world* world_;
    int32 voxel_scale_{1};
    std::unordered_map<vec3i, std::unique_ptr<chunk>> chunks_;
    std::unordered_map<vec2i, std::vector<int32>> column_chunks_;
    std::unordered_set<vec2i> light_dirty_;
    uint32 drawn_chunks_ = 0;
};

}  // namespace vw::ecs
