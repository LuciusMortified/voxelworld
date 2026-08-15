module vw.world;

import std;

namespace vw::asset {

model_identity_pool::model_identity_pool(std::size_t capacity) {
    generations_.reserve(capacity);
}

auto model_identity_pool::create() -> model_identity {
    std::scoped_lock lock(mutex_);
    if (!free_indices_.empty()) [[unlikely]] {
        const uint32 index = free_indices_.back();
        free_indices_.pop_back();
        return {.index = index, .generation = generations_[index]};
    }

    const auto index = static_cast<uint32>(generations_.size());
    generations_.push_back(0);
    return {.index = index, .generation = 0};
}

auto model_identity_pool::next_generation(model_identity id) -> model_identity {
    std::scoped_lock lock(mutex_);
    if (has_unlocked_(id)) [[likely]] {
        return {.index = id.index, .generation = ++generations_[id.index]};
    }
    return invalid_model_identity;
}

auto model_identity_pool::has(model_identity id) const -> bool {
    std::scoped_lock lock(mutex_);
    return has_unlocked_(id);
}

void model_identity_pool::destroy(model_identity id) {
    std::scoped_lock lock(mutex_);
    if (has_unlocked_(id)) [[likely]] {
        ++generations_[id.index];
        free_indices_.push_back(id.index);
    }
}

page_pool::page_pool() {
    blocks_.reserve(max_blocks);
}

auto page_pool::alloc() -> uint32 {
    std::scoped_lock lock(mutex_);
    if (!free_indices_.empty()) {
        const uint32 idx = free_indices_.back();
        free_indices_.pop_back();
        return idx;
    }
    const uint32 idx = next_index_++;
    ensure_capacity_(idx);
    return idx;
}

void page_pool::free(uint32 index) {
    std::scoped_lock lock(mutex_);
    free_indices_.push_back(index);
}

auto page_pool::alloc_batch(uint32 count) -> std::vector<uint32> {
    std::scoped_lock lock(mutex_);
    std::vector<uint32> result;
    result.reserve(count);

    const uint32 consecutive =
        count - static_cast<uint32>(std::min(static_cast<std::size_t>(count), free_indices_.size()));

    const uint32 bump_start = next_index_;
    for (uint32 i = 0; i < consecutive; ++i) {
        result.push_back(bump_start + i);
    }
    next_index_ = bump_start + consecutive;
    if (consecutive > 0) {
        ensure_capacity_(next_index_ - 1);
    }

    const uint32 remaining = count - consecutive;
    for (uint32 i = 0; i < remaining; ++i) {
        const uint32 idx = free_indices_.back();
        free_indices_.pop_back();
        result.push_back(idx);
    }

    return result;
}

void page_pool::free_batch(std::span<const uint32> indices) {
    std::scoped_lock lock(mutex_);
    free_indices_.reserve(free_indices_.size() + indices.size());
    for (uint32 idx : indices) {
        free_indices_.push_back(idx);
    }
}

auto page_pool::allocated_count() const -> uint32 {
    std::scoped_lock lock(mutex_);
    return next_index_ - static_cast<uint32>(free_indices_.size());
}

auto page_pool::free_count() const -> uint32 {
    std::scoped_lock lock(mutex_);
    return static_cast<uint32>(free_indices_.size());
}

void page_pool::ensure_capacity_(uint32 index) {
    const uint32 block_idx = index / block_size;
    while (blocks_.size() <= block_idx) {
        blocks_.push_back(std::make_unique<std::array<page_type, block_size>>());
    }
}

model::model(model_identity_pool& identity_pool, page_pool& pool, int32 width, int32 height,
             int32 depth, int32 voxel_scale)
    : identity_pool_(&identity_pool)
    , pool_ptr_(&pool)
    , width_(width)
    , height_(height)
    , depth_(depth)
    , voxel_scale_(voxel_scale)
    , pages_x_((width + page_size - 1) / page_size)
    , pages_y_((height + page_size - 1) / page_size)
    , pages_z_((depth + page_size - 1) / page_size) {
    pages_.resize(static_cast<std::size_t>(pages_x_) * static_cast<std::size_t>(pages_y_) *
                  static_cast<std::size_t>(pages_z_));
    identity_ = identity_pool_->create();
}

model::~model() {
    if (pool_ptr_ != nullptr && !owned_pages_.empty()) {
        pool_ptr_->free_batch(owned_pages_);
    }
    if (identity_pool_ != nullptr) {
        identity_pool_->destroy(identity_);
    }
}

model::model(model&& other) noexcept
    : identity_pool_(other.identity_pool_)
    , pool_ptr_(other.pool_ptr_)
    , width_(other.width_)
    , height_(other.height_)
    , depth_(other.depth_)
    , voxel_scale_(other.voxel_scale_)
    , pages_x_(other.pages_x_)
    , pages_y_(other.pages_y_)
    , pages_z_(other.pages_z_)
    , pages_(std::move(other.pages_))
    , owned_pages_(std::move(other.owned_pages_))
    , identity_(other.identity_) {
    other.identity_pool_ = nullptr;
    other.pool_ptr_      = nullptr;
}

auto model::operator=(model&& other) noexcept -> model& {
    if (this != &other) {
        if (pool_ptr_ != nullptr && !owned_pages_.empty()) {
            pool_ptr_->free_batch(owned_pages_);
        }
        if (identity_pool_ != nullptr) {
            identity_pool_->destroy(identity_);
        }
        identity_pool_       = other.identity_pool_;
        pool_ptr_            = other.pool_ptr_;
        width_               = other.width_;
        height_              = other.height_;
        depth_               = other.depth_;
        voxel_scale_         = other.voxel_scale_;
        pages_x_             = other.pages_x_;
        pages_y_             = other.pages_y_;
        pages_z_             = other.pages_z_;
        pages_               = std::move(other.pages_);
        owned_pages_         = std::move(other.owned_pages_);
        identity_            = other.identity_;
        other.identity_pool_ = nullptr;
        other.pool_ptr_      = nullptr;
    }
    return *this;
}

void model::set_voxel(int32 x, int32 y, int32 z, const voxel& v) {
    set_voxel_raw(x, y, z, v);
    increment_generation_();
}

void model::set_voxel_raw(int32 x, int32 y, int32 z, const voxel& v) {
    const int32 px = x / page_size;
    const int32 py = y / page_size;
    const int32 pz = z / page_size;
    const int32 li = local_index(x % page_size, y % page_size, z % page_size);
    auto& entry    = pages_[page_index(px, py, pz)];

    switch (entry.mode()) {
        case page_mode::empty:
            if (v.is_empty()) {
                return;
            }
            promote_to_sparse(px, py, pz)[li] = v;
            break;
        case page_mode::uniform:
            if (v.id == entry.fill_id()) {
                return;
            }
            promote_to_sparse(px, py, pz)[li] = v;
            break;
        case page_mode::sparse:
            pool_ptr_->get(entry.pool_index())[li] = v;
            break;
    }
}

void model::compute_own_boundaries() {
    constexpr int32 ps = page_size;

    own_faces_valid_ = 0;

    // The bit planes are a fixed 64x64, which is exactly a world chunk. Larger
    // models (Sculptor's, mostly) have no neighbours to hand them to anyway.
    if (width_ > face_occupancy::side || height_ > face_occupancy::side ||
        depth_ > face_occupancy::side) {
        return;
    }

    for (auto& face : own_faces_) {
        face.clear();
    }

    const auto solid_at = [this](int32 x, int32 y, int32 z) -> bool {
        return !is_empty(x, y, z);
    };

    // Page modes still short-circuit the scan: an empty page contributes
    // nothing, a uniform one contributes everything.
    const auto scan_face = [&](face_occupancy& face, int32 axis, int32 layer) {
        for (int32 b = 0; b < face_occupancy::side; ++b) {
            for (int32 a = 0; a < face_occupancy::side; ++a) {
                int32 x = 0;
                int32 y = 0;
                int32 z = 0;
                switch (axis) {
                    case 0: x = layer; y = a; z = b; break;
                    case 1: x = a; y = layer; z = b; break;
                    default: x = a; y = b; z = layer; break;
                }

                if (x >= width_ || y >= height_ || z >= depth_) {
                    continue;
                }
                if (solid_at(x, y, z)) {
                    face.set(a, b);
                }
            }
        }
    };

    scan_face(own_faces_[0], 0, 0);
    scan_face(own_faces_[1], 0, width_ - 1);
    scan_face(own_faces_[2], 1, 0);
    scan_face(own_faces_[3], 1, height_ - 1);
    scan_face(own_faces_[4], 2, 0);
    scan_face(own_faces_[5], 2, depth_ - 1);

    own_faces_valid_ = 0x3F;

    static_cast<void>(ps);
}

void model::set_boundary_slice(int32 face_direction, const model& neighbor) {
    if ((neighbor.own_faces_valid_ & (1U << face_direction)) == 0) {
        return;
    }

    neighbor_faces_[face_direction] = neighbor.own_faces_[face_direction];
    neighbor_faces_valid_ |= static_cast<uint8>(1U << face_direction);
}

auto model::is_boundary_solid(int32 face_direction, int32 x, int32 y, int32 z) const -> bool {
    const auto& face = neighbor_faces_[face_direction];
    switch (face_direction / 2) {
        case 0:
            return face.test(y, z);
        case 1:
            return face.test(x, z);
        default:
            return face.test(x, y);
    }
}

void model::invalidate() {
    increment_generation_();
}

void model::fill(const voxel& v) {
    if (!owned_pages_.empty()) {
        pool_ptr_->free_batch(owned_pages_);
        owned_pages_.clear();
    }

    if (v.is_empty()) {
        std::ranges::fill(pages_, page_entry::make_empty());
    } else {
        std::ranges::fill(pages_, page_entry::make_uniform(v.id));
    }
    increment_generation_();
}

void model::clone_pages_from(const model& source) {
    if (!owned_pages_.empty()) {
        pool_ptr_->free_batch(owned_pages_);
        owned_pages_.clear();
    }

    pages_ = source.pages_;

    if (!source.owned_pages_.empty()) {
        auto new_pages = pool_ptr_->alloc_batch(static_cast<uint32>(source.owned_pages_.size()));

        std::unordered_map<uint32, uint32> remap;
        remap.reserve(source.owned_pages_.size());
        for (std::size_t i = 0; i < source.owned_pages_.size(); ++i) {
            remap[source.owned_pages_[i]] = new_pages[i];
            pool_ptr_->get(new_pages[i])  = pool_ptr_->get(source.owned_pages_[i]);
        }

        for (auto& entry : pages_) {
            if (entry.mode() == page_mode::sparse) {
                entry = page_entry::make_sparse(remap[entry.pool_index()]);
            }
        }

        owned_pages_ = std::move(new_pages);
    }

    increment_generation_();
}

auto model::alloc_sparse_page() -> uint32 {
    const uint32 idx = pool_ptr_->alloc();
    owned_pages_.push_back(idx);
    return idx;
}

void model::free_sparse_page(uint32 index) {
    pool_ptr_->free(index);
    const auto it = std::ranges::find(owned_pages_, index);
    if (it != owned_pages_.end()) {
        std::iter_swap(it, owned_pages_.end() - 1);
        owned_pages_.pop_back();
    }
}

auto model::promote_to_sparse(int32 px, int32 py, int32 pz) -> page_type& {
    auto& entry      = pages_[page_index(px, py, pz)];
    const uint32 idx = alloc_sparse_page();
    auto& page       = pool_ptr_->get(idx);
    page.fill(entry.mode() == page_mode::uniform ? voxel{entry.fill_id()} : voxel{});
    entry = page_entry::make_sparse(idx);
    return page;
}

void model::increment_generation_() {
    identity_ = identity_pool_->next_generation(identity_);
}

auto model_registry::has(std::string_view name) const -> bool {
    return models_.contains(std::string(name));
}

auto model_registry::get(std::string_view name) const -> std::shared_ptr<model> {
    const auto iter = models_.find(std::string(name));
    return iter != models_.end() ? iter->second : nullptr;
}

auto model_registry::create(std::string_view name, int32 width, int32 height, int32 depth)
    -> std::shared_ptr<model> {
    auto new_model = std::make_shared<model>(identity_pool_, page_pool_, width, height, depth);
    models_[std::string(name)] = new_model;
    return new_model;
}

auto model_registry::create(std::string_view name, vec3i size) -> std::shared_ptr<model> {
    return create(name, size.x, size.y, size.z);
}

auto model_registry::create_unnamed(int32 width, int32 height, int32 depth)
    -> std::shared_ptr<model> {
    return std::make_shared<model>(identity_pool_, page_pool_, width, height, depth);
}

auto model_registry::create_unnamed(vec3i size) -> std::shared_ptr<model> {
    return create_unnamed(size.x, size.y, size.z);
}

auto model_registry::create_clone(std::string_view name) -> std::shared_ptr<model> {
    const auto original = get(name);
    if (!original) {
        return nullptr;
    }

    auto cloned_model = std::make_shared<model>(
        identity_pool_, page_pool_, original->width(), original->height(), original->depth());
    cloned_model->clone_pages_from(*original);

    return cloned_model;
}

void model_registry::erase(std::string_view name) {
    models_.erase(std::string(name));
}

}  // namespace vw::asset
