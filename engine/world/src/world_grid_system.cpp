module vw.world;

import std;

namespace vw::ecs {

world_grid_system::world_grid_system(
    world& w
)
    : world_(&w) {}

world_grid_system::~world_grid_system() = default;

world_grid_system::world_grid_system(world_grid_system&&) noexcept = default;

auto world_grid_system::operator=(world_grid_system&&) noexcept -> world_grid_system& = default;

void world_grid_system::set_grid(
    std::unique_ptr<world_grid> grid
) {
    clear_grid_transient_state_();
    grid_ = std::move(grid);
}

void world_grid_system::set_loader(
    std::unique_ptr<chunk_loader> loader
) {
    clear_loader_transient_state_();
    loader_ = std::move(loader);
}

auto world_grid_system::grid() -> world_grid* {
    return grid_.get();
}

auto world_grid_system::grid() const -> const world_grid* {
    return grid_.get();
}

auto world_grid_system::loader() -> chunk_loader* {
    return loader_.get();
}

auto world_grid_system::loader() const -> const chunk_loader* {
    return loader_.get();
}

auto world_grid_system::has_grid() const -> bool {
    return grid_ != nullptr;
}

auto world_grid_system::has_loader() const -> bool {
    return loader_ != nullptr;
}

auto world_grid_system::get_stats() const -> const world_grid_system_stats& {
    return stats_;
}

void world_grid_system::shutdown() {
    grid_.reset();
    loader_.reset();
}

void world_grid_system::update(float32 /*dt*/) {
    if (!grid_ || !loader_) {
        return;
    }

    auto& reg                = world_->registry();
    stats_.integrate_ms      = measure_ms([&] { integrate_completed_columns_(); });
    stats_.deferred_remesh_ms = measure_ms([&] { process_deferred_remeshes_(); });
    stats_.request_columns_ms = measure_ms([&] { dispatch_column_requests_(); });
    update_grid_stats_();

    if (reg.requested<world_view_component>().empty()) {
        return;
    }

    if (process_dirty_entities_()) {
        vec2i camera_column{};
        stats_.rebuild_active_ms = measure_ms([&] { camera_column = rebuild_active_set_(); });
        stats_.unload_ms         = measure_ms([&] { unload_inactive_columns_(); });
        std::swap(active_columns_, pending_active_columns_);
        rebuild_pending_requests_(camera_column);
        stats_.active_count = static_cast<uint32>(active_columns_.size());
    }

    reg.clear_requested<world_view_component>();
}

void world_grid_system::integrate_completed_columns_() {
    static constexpr int32 max_columns_per_frame = 1;
    static constexpr vec3i neighbor_offsets[6]   = {
        {1, 0, 0},   //
        {-1, 0, 0},  //
        {0, 1, 0},   //
        {0, -1, 0},  //
        {0, 0, 1},   //
        {0, 0, -1}
    };

    float32 boundary_from_total = 0.0f;
    float32 chunk_create_total  = 0.0f;
    float32 boundary_to_total   = 0.0f;
    int32 processed_columns     = 0;

    while (processed_columns < max_columns_per_frame) {
        auto col = loader_->try_pop_completed();
        if (!col) {
            break;
        }

        auto col_coord = col->get_coord();
        if (!active_columns_.contains(col_coord) &&
            !pending_active_columns_.contains(col_coord)) {
            continue;
        }

        std::vector<int32> y_levels;

        for (auto& [y, cd] : col->get_all_chunk_data()) {
            boundary_from_total += measure_ms([&] -> auto {
                for (int fd = 0; fd < 6; ++fd) {
                    auto* neighbor = grid_->get_chunk(cd.coord + neighbor_offsets[fd]);
                    if (neighbor) {
                        cd.chunk_model->set_boundary_slice(fd, *neighbor->get_model());
                    }
                }
            });

            chunk* created = nullptr;
            chunk_create_total += measure_ms([&] -> auto {
                created = grid_->place_chunk(cd.coord, std::move(cd.chunk_model));
            });

            boundary_to_total += measure_ms([&] -> auto {
                for (int fd = 0; fd < 6; ++fd) {
                    auto* neighbor = grid_->get_chunk(cd.coord + neighbor_offsets[fd]);
                    if (neighbor && neighbor != created) {
                        int opposite_fd = fd ^ 1;
                        neighbor->get_model()->set_boundary_slice(
                            opposite_fd, *created->get_model()
                        );
                        deferred_remeshes_.push({cd.coord + neighbor_offsets[fd], opposite_fd});
                    }
                }
            });

            y_levels.push_back(y);
        }

        std::sort(y_levels.begin(), y_levels.end());
        grid_->register_column(col_coord, std::move(y_levels));
        ++processed_columns;
    }

    stats_.boundary_from_ms = boundary_from_total;
    stats_.chunk_create_ms  = chunk_create_total;
    stats_.boundary_to_ms   = boundary_to_total;
}

void world_grid_system::process_deferred_remeshes_() {
    static constexpr int32 max_remeshes_per_frame = 4;
    int32 processed                               = 0;

    while (processed < max_remeshes_per_frame && !deferred_remeshes_.empty()) {
        auto [coord, fd] = deferred_remeshes_.front();
        deferred_remeshes_.pop();

        auto* chunk_ptr = grid_->get_chunk(coord);
        if (!chunk_ptr) {
            continue;
        }

        auto mdl = chunk_ptr->get_model();
        mdl->invalidate();
        world_->system<model_system>().modify(chunk_ptr->get_entity()).set_model(mdl);
        ++processed;
    }
}

void world_grid_system::dispatch_column_requests_() {
    static constexpr int32 max_requests_per_frame = 8;
    int32 requests = 0;
    while (!pending_requests_.empty() && requests < max_requests_per_frame) {
        auto coord = pending_requests_.back();
        pending_requests_.pop_back();
        if (loader_->request(coord)) {
            ++requests;
        }
    }
}

void world_grid_system::update_grid_stats_() {
    stats_.active_count          = static_cast<uint32>(active_columns_.size());
    stats_.pending_count         = loader_->pending_count();
    stats_.loaded_count          = grid_->chunk_count();
    stats_.deferred_remesh_count = static_cast<uint32>(deferred_remeshes_.size());
    stats_.rebuild_active_ms     = 0.0f;
    stats_.unload_ms             = 0.0f;
}

auto world_grid_system::process_dirty_entities_() -> bool {
    auto& reg         = world_->registry();
    bool chunks_dirty = false;
    for (auto ent : reg.requested<world_view_component>()) {
        if (process_dirty_entity_(ent)) {
            chunks_dirty = true;
        }
        reg.notify_changed<world_view_component>(ent);
    }
    return chunks_dirty;
}

auto world_grid_system::rebuild_active_set_() -> vec2i {
    auto& reg = world_->registry();
    pending_active_columns_.clear();
    vec2i camera_column{};

    for (auto ent : reg.requested<world_view_component>()) {
        if (!reg.has<world_view_component>(ent) ||
            !reg.has<transform_component>(ent)) {
            continue;
        }

        const auto& wv   = reg.get<world_view_component>(ent);
        auto chunk_coord = wv.get_chunk_coord();
        camera_column    = {chunk_coord.x, chunk_coord.z};
        const auto dist  = static_cast<int32>(wv.get_view_distance());

        for (int32 dx = -dist; dx <= dist; ++dx) {
            for (int32 dz = -dist; dz <= dist; ++dz) {
                int32 cx = camera_column.x + dx;
                int32 cz = camera_column.y + dz;
                pending_active_columns_.insert({cx, cz});
            }
        }
    }

    return camera_column;
}

void world_grid_system::unload_inactive_columns_() {
    for (const auto& coord : active_columns_) {
        if (!pending_active_columns_.contains(coord)) {
            grid_->unload_column(coord);
        }
    }
}

auto world_grid_system::process_dirty_entity_(
    entity ent
) -> bool {
    auto& reg = world_->registry();
    if (!reg.has<world_view_component>(ent) ||
        !reg.has<transform_component>(ent)) {
        return false;
    }

    auto& wv       = reg.get<world_view_component>(ent);
    const auto& tc = reg.get<transform_component>(ent);
    auto pos       = tc.get_position();

    auto new_chunk_coord = grid_->world_to_chunk_coord({
        static_cast<int32>(pos.x),
        static_cast<int32>(pos.y),
        static_cast<int32>(pos.z)
    });

    bool changed     = wv.dirty_ || wv.chunk_coord_ != new_chunk_coord;
    wv.chunk_coord_  = new_chunk_coord;
    wv.dirty_        = false;
    return changed;
}

world_grid_system::view_modifier::view_modifier(
    world_grid_system* system, entity ent
)
    : system_(system), entity_(ent) {}

auto world_grid_system::modify_view(
    entity ent
) -> view_modifier {
    return view_modifier(this, ent);
}

auto world_grid_system::view_modifier::set_view_distance(
    uint32 distance
) -> view_modifier& {
    auto& reg = system_->world_->registry();
    if (!reg.has<world_view_component>(entity_)) {
        return *this;
    }

    auto& wv          = reg.get<world_view_component>(entity_);
    wv.view_distance_ = distance;
    reg.request_change<world_view_component>(entity_);

    return *this;
}

void world_grid_system::rebuild_pending_requests_(
    vec2i camera_column
) {
    pending_requests_.clear();

    for (const auto& coord : active_columns_) {
        if (!grid_->has_column(coord) && !loader_->is_pending(coord)) {
            pending_requests_.push_back(coord);
        }
    }

    std::sort(
        pending_requests_.begin(), pending_requests_.end(),
        [&camera_column](const vec2i& a, const vec2i& b) {
            auto da     = a - camera_column;
            auto db     = b - camera_column;
            auto dist_a = da.x * da.x + da.y * da.y;
            auto dist_b = db.x * db.x + db.y * db.y;
            return dist_a > dist_b;
        }
    );
}

void world_grid_system::clear_grid_transient_state_() {
    pending_requests_.clear();
    while (!deferred_remeshes_.empty()) {
        deferred_remeshes_.pop();
    }
    active_columns_.clear();
    pending_active_columns_.clear();
}

void world_grid_system::clear_loader_transient_state_() {
    pending_requests_.clear();
}

}  // namespace vw::ecs
