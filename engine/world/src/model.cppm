export module vw.world:model;

import std;

import vw.core;

export namespace vw::asset {

struct model_identity {
    static constexpr uint32 invalid_index = std::numeric_limits<uint32>::max();

    uint32 index      = invalid_index;
    uint32 generation = 0;

    [[nodiscard]] auto operator==(const model_identity& other) const -> bool {
        return index == other.index && generation == other.generation;
    }

    [[nodiscard]] auto operator!=(const model_identity& other) const -> bool {
        return !(*this == other);
    }

    [[nodiscard]] auto is_valid() const -> bool {
        return index != invalid_index;
    }
};

inline constexpr auto invalid_model_identity = model_identity{};

// Hands out generation-stamped model handles. Shared between the loader
// threads and the main thread, hence the lock.
class model_identity_pool final {
public:
    static constexpr std::size_t default_capacity = 1024;

    explicit model_identity_pool(std::size_t capacity = default_capacity);

    [[nodiscard]] auto create() -> model_identity;
    [[nodiscard]] auto next_generation(model_identity id) -> model_identity;
    [[nodiscard]] auto has(model_identity id) const -> bool;

    void destroy(model_identity id);

private:
    [[nodiscard]] auto has_unlocked_(model_identity id) const -> bool {
        return id.index != model_identity::invalid_index && id.index < generations_.size() &&
               generations_[id.index] == id.generation;
    }

    mutable std::mutex mutex_;
    std::vector<uint32> generations_;
    std::vector<uint32> free_indices_;
};

// Slab allocator for sparse voxel pages: models hold indices, the pages
// themselves stay in blocks that never move.
class page_pool final {
public:
    using page_type                    = std::array<voxel, 512>;
    static constexpr uint32 block_size = 4096;
    static constexpr uint32 max_blocks = 256;

    page_pool();

    [[nodiscard]] auto alloc() -> uint32;
    void free(uint32 index);

    [[nodiscard]] auto alloc_batch(uint32 count) -> std::vector<uint32>;
    void free_batch(std::span<const uint32> indices);

    [[nodiscard]] auto get(uint32 index) -> page_type& {
        return (*blocks_[index / block_size])[index % block_size];
    }

    [[nodiscard]] auto get(uint32 index) const -> const page_type& {
        return (*blocks_[index / block_size])[index % block_size];
    }

    [[nodiscard]] auto allocated_count() const -> uint32;
    [[nodiscard]] auto free_count() const -> uint32;

private:
    void ensure_capacity_(uint32 index);

    mutable std::mutex mutex_;
    std::vector<std::unique_ptr<std::array<page_type, block_size>>> blocks_;
    std::vector<uint32> free_indices_;
    uint32 next_index_ = 0;
};

enum class page_mode : uint8 { empty = 0, uniform = 1, sparse = 2 };

struct page_entry {
    uint32 data = 0;

    [[nodiscard]] auto mode() const -> page_mode {
        return static_cast<page_mode>(data & 0x3U);
    }

    [[nodiscard]] auto fill_id() const -> block_id {
        return block_id{static_cast<uint8>((data >> 2) & 0xFFU)};
    }

    [[nodiscard]] auto pool_index() const -> uint32 {
        return (data >> 10) & 0xFFFFFU;
    }

    [[nodiscard]] static auto make_empty() -> page_entry {
        return {0U};
    }

    [[nodiscard]] static auto make_uniform(block_id id) -> page_entry {
        return {1U | (static_cast<uint32>(id.value) << 2)};
    }

    [[nodiscard]] static auto make_sparse(uint32 index) -> page_entry {
        return {2U | (index << 10)};
    }
};

}  // namespace vw::asset

export namespace vw::asset {

// One face of a chunk as occupancy bits: 64x64 in 64 words. Handing it to a
// neighbour is a copy of 512 bytes with no allocation, and the mesher can take
// a whole row of neighbour bits in one read instead of asking per voxel.
struct face_occupancy {
    static constexpr int32 side = 64;

    std::array<uint64, side> rows{};

    [[nodiscard]] auto test(int32 a, int32 b) const -> bool {
        return ((rows[b] >> a) & 1U) != 0;
    }

    auto set(int32 a, int32 b) -> void {
        rows[b] |= uint64{1} << a;
    }

    auto clear() -> void {
        rows.fill(0);
    }
};


// A whole chunk as occupancy bits: one word per row along X, indexed by
// (y, z). 32 KB, built once per mesh and read a row at a time -- the scans the
// mesher does per cell are single instructions against this.
struct chunk_occupancy {
    static constexpr int32 side = 64;

    // Two orientations of the same volume. Rows along X serve the +-Y and +-Z
    // faces; rows along Z serve +-X. Both are filled in one pass, which is
    // cheaper than transposing 64x64 bit planes afterwards.
    std::array<uint64, side * side> rows{};   // rows[y * side + z], bit x
    std::array<uint64, side * side> zrows{};  // zrows[y * side + x], bit z

    auto clear() -> void {
        rows.fill(0);
        zrows.fill(0);
    }

    [[nodiscard]] auto row(int32 y, int32 z) const -> uint64 {
        return rows[(y * side) + z];
    }

    [[nodiscard]] auto zrow(int32 y, int32 x) const -> uint64 {
        return zrows[(y * side) + x];
    }

    [[nodiscard]] auto test(int32 x, int32 y, int32 z) const -> bool {
        return ((row(y, z) >> x) & 1U) != 0;
    }

    auto set_row(int32 y, int32 z, uint64 bits) -> void {
        rows[(y * side) + z] |= bits;
    }

    auto set_zrow(int32 y, int32 x, uint64 bits) -> void {
        zrows[(y * side) + x] |= bits;
    }
};

// Paged voxel volume. Pages are empty, uniform or sparse, so a mostly solid or
// mostly empty model costs one entry per page instead of a voxel array.
class model {
public:
    static constexpr int32 page_size   = 8;
    static constexpr int32 page_volume = page_size * page_size * page_size;
    using page_type                    = std::array<voxel, page_volume>;

    model(model_identity_pool& identity_pool, page_pool& pool, int32 width, int32 height,
          int32 depth, int32 voxel_scale = 1);
    ~model();

    model(const model&)                    = delete;
    auto operator=(const model&) -> model& = delete;

    model(model&& other) noexcept;
    auto operator=(model&& other) noexcept -> model&;

    void set_voxel(int32 x, int32 y, int32 z, const voxel& v);

    // The same write without bumping the generation. set_voxel takes the
    // identity pool mutex on every call, which is fine for an edit and ruinous
    // for generation: a chunk is tens of thousands of writes across four
    // workers. Filling a fresh model needs no invalidation at all -- nobody has
    // meshed it yet; changing a live one calls invalidate() once at the end.
    void set_voxel_raw(int32 x, int32 y, int32 z, const voxel& v);

    void set_voxel(vec3i pos, const voxel& v) {
        set_voxel(pos.x, pos.y, pos.z, v);
    }

    [[nodiscard]] auto get_voxel(int32 x, int32 y, int32 z) const -> voxel {
        const auto& entry = pages_[page_index(x / page_size, y / page_size, z / page_size)];

        switch (entry.mode()) {
            case page_mode::empty:
                return voxel{};
            case page_mode::uniform:
                return voxel{entry.fill_id()};
            case page_mode::sparse:
                return pool_ptr_->get(entry.pool_index())
                    [local_index(x % page_size, y % page_size, z % page_size)];
        }
        return voxel{};
    }

    [[nodiscard]] auto get_voxel(vec3i pos) const -> voxel {
        return get_voxel(pos.x, pos.y, pos.z);
    }

    [[nodiscard]] auto is_empty(int32 x, int32 y, int32 z) const -> bool {
        const auto& entry = pages_[page_index(x / page_size, y / page_size, z / page_size)];

        switch (entry.mode()) {
            case page_mode::empty:
                return true;
            case page_mode::uniform:
                return false;
            case page_mode::sparse:
                return pool_ptr_->get(entry.pool_index())
                    [local_index(x % page_size, y % page_size, z % page_size)]
                        .is_empty();
        }
        return true;
    }

    [[nodiscard]] auto is_empty(vec3i pos) const -> bool {
        return is_empty(pos.x, pos.y, pos.z);
    }

    [[nodiscard]] auto width() const -> int32 {
        return width_;
    }

    [[nodiscard]] auto height() const -> int32 {
        return height_;
    }

    [[nodiscard]] auto depth() const -> int32 {
        return depth_;
    }

    [[nodiscard]] auto size() const -> vec3i {
        return vec3i{width_, height_, depth_};
    }

    [[nodiscard]] auto voxel_scale() const -> int32 {
        return voxel_scale_;
    }

    void compute_own_boundaries();

    // Fills the bit volume from the page table rather than voxel by voxel: an
    // empty page contributes nothing, a uniform one contributes eight set bits
    // per row at once. False when the model is not a 64-cube.
    [[nodiscard]] auto build_occupancy(chunk_occupancy& out) const -> bool;
    void set_boundary_slice(int32 face_direction, const model& neighbor);

    [[nodiscard]] auto get_boundary_face(int32 face_direction) const -> const face_occupancy& {
        return neighbor_faces_[face_direction];
    }

    [[nodiscard]] auto has_boundary_slice(int32 face_direction) const -> bool {
        return (neighbor_faces_valid_ & (1U << face_direction)) != 0;
    }

    [[nodiscard]] auto is_boundary_solid(int32 face_direction, int32 x, int32 y, int32 z) const
        -> bool;

    void invalidate();

    void fill(const voxel& v);

    [[nodiscard]] auto get_identity() const -> model_identity {
        return identity_;
    }

    void clone_pages_from(const model& source);

    [[nodiscard]] auto get_page_mode(int32 px, int32 py, int32 pz) const -> page_mode {
        return pages_[page_index(px, py, pz)].mode();
    }

    [[nodiscard]] auto get_page_fill_id(int32 px, int32 py, int32 pz) const -> block_id {
        return pages_[page_index(px, py, pz)].fill_id();
    }

    [[nodiscard]] auto get_page(int32 px, int32 py, int32 pz) const -> const page_type* {
        const auto& entry = pages_[page_index(px, py, pz)];
        return entry.mode() == page_mode::sparse ? &pool_ptr_->get(entry.pool_index()) : nullptr;
    }

    [[nodiscard]] auto pages_x() const -> int32 {
        return pages_x_;
    }

    [[nodiscard]] auto pages_y() const -> int32 {
        return pages_y_;
    }

    [[nodiscard]] auto pages_z() const -> int32 {
        return pages_z_;
    }

private:
    [[nodiscard]] auto page_index(int32 px, int32 py, int32 pz) const -> int32 {
        return px + (py * pages_x_) + (pz * pages_x_ * pages_y_);
    }

    [[nodiscard]] static auto local_index(int32 lx, int32 ly, int32 lz) -> int32 {
        return lx + (ly * page_size) + (lz * page_size * page_size);
    }

    auto alloc_sparse_page() -> uint32;
    void free_sparse_page(uint32 index);
    auto promote_to_sparse(int32 px, int32 py, int32 pz) -> page_type&;

    void increment_generation_();

    model_identity_pool* identity_pool_;
    page_pool* pool_ptr_;
    int32 width_{0}, height_{0}, depth_{0};
    int32 voxel_scale_{1};
    int32 pages_x_{0}, pages_y_{0}, pages_z_{0};
    std::vector<page_entry> pages_;
    std::vector<uint32> owned_pages_;
    model_identity identity_;
    std::array<face_occupancy, 6> neighbor_faces_{};
    std::array<face_occupancy, 6> own_faces_{};
    uint8 neighbor_faces_valid_ = 0;
    uint8 own_faces_valid_      = 0;
};

// Owns the named models of a world together with the pools their pages and
// identities come from.
class model_registry {
public:
    [[nodiscard]] auto has(std::string_view name) const -> bool;
    [[nodiscard]] auto get(std::string_view name) const -> std::shared_ptr<model>;

    [[nodiscard]] auto create(std::string_view name, int32 width, int32 height, int32 depth)
        -> std::shared_ptr<model>;
    [[nodiscard]] auto create(std::string_view name, vec3i size) -> std::shared_ptr<model>;
    [[nodiscard]] auto create_unnamed(int32 width, int32 height, int32 depth)
        -> std::shared_ptr<model>;
    [[nodiscard]] auto create_unnamed(vec3i size) -> std::shared_ptr<model>;
    [[nodiscard]] auto create_clone(std::string_view name) -> std::shared_ptr<model>;

    void erase(std::string_view name);

    [[nodiscard]] auto get_identity_pool() -> model_identity_pool& {
        return identity_pool_;
    }

    [[nodiscard]] auto get_page_pool() -> page_pool& {
        return page_pool_;
    }

private:
    model_identity_pool identity_pool_;
    page_pool page_pool_;
    std::unordered_map<std::string, std::shared_ptr<model>> models_;
};

}  // namespace vw::asset

export template <>
struct std::hash<vw::asset::model_identity> {
    auto operator()(const vw::asset::model_identity& id) const noexcept -> std::size_t {
        std::size_t x = (std::size_t{id.generation} << 32) | std::size_t{id.index};

        x += 0x9e3779b97f4a7c15ULL;
        x = (x ^ (x >> 30)) * 0xbf58476d1ce4e5b9ULL;
        x = (x ^ (x >> 27)) * 0x94d049bb133111ebULL;
        x = x ^ (x >> 31);

        return x;
    }
};
