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
    baker_  = loader_ != nullptr ? std::make_unique<sky_light_baker>() : nullptr;
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

auto world_grid_system::get_loader_stats() const -> column_gen_stats {
    return loader_ ? loader_->get_gen_stats() : column_gen_stats{};
}

auto world_grid_system::get_light_stats() const -> sky_light_stats {
    return baker_ ? baker_->get_stats() : sky_light_stats{};
}

auto world_grid_system::get_stats() const -> const world_grid_system_stats& {
    return stats_;
}

void world_grid_system::shutdown() {
    grid_.reset();
    baker_.reset();
    loader_.reset();
}

void world_grid_system::update(float32 /*dt*/) {
    if (!grid_ || !loader_) {
        return;
    }

    auto& reg       = world_->registry();
    stats_.stage_ms = measure_ms([&] { stage_completed_columns_(); });
    stats_.light_apply_ms = measure_ms([&] {
        collect_lit_columns_();
        relight_dirty_columns_();
    });
    stats_.integrate_ms       = measure_ms([&] { integrate_completed_columns_(); });
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

namespace {
// All eight, diagonals included. Boundaries only ever needed the four, but
// sky light comes across a corner as readily as across a side, and a column
// lit without its diagonals has dark wedges where two seams meet.
constexpr vec2i column_neighbor_offsets[8] = {
    {1, 0},    //
    {-1, 0},   //
    {0, 1},    //
    {0, -1},   //
    {1, 1},    //
    {1, -1},   //
    {-1, 1},   //
    {-1, -1}
};
}  // namespace

auto world_grid_system::column_available_(
    vec2i coord
) const -> bool {
    return staged_columns_.contains(coord) || grid_->has_column(coord);
}

auto world_grid_system::within_draw_(
    vec2i coord
) const -> bool {
    const auto d = coord - camera_column_;
    return std::abs(d.x) <= draw_distance_ && std::abs(d.y) <= draw_distance_;
}

auto world_grid_system::column_ready_(
    vec2i coord
) const -> bool {
    return std::ranges::all_of(column_neighbor_offsets, [&](vec2i offset) -> bool {
        return column_available_(coord + offset);
    });
}

void world_grid_system::queue_if_ready_(
    vec2i coord
) {
    const auto it = staged_columns_.find(coord);
    if (it == staged_columns_.end()) {
        return;
    }

    const auto phase = it->second->get_phase();
    if (phase == column_phase::complete || phase == column_phase::lighting) {
        return;
    }
    if (!column_ready_(coord)) {
        return;
    }

    // A column that went out of drawing range and came back is a fresh
    // gen_column around the same models, so its phase says terrain however
    // long ago it was lit. The models are the ones that remember.
    if (already_lit_(*it->second)) {
        it->second->set_phase(column_phase::complete);
        ready_columns_.push_back(coord);
        return;
    }

    if (dispatch_light_(coord)) {
        it->second->set_phase(column_phase::lighting);
    }
}

auto world_grid_system::already_lit_(
    gen_column& col
) -> bool {
    const auto& chunks = col.get_all_chunk_data();
    return !chunks.empty() && std::ranges::all_of(chunks, [](const auto& entry) -> bool {
        return entry.second.chunk_model->has_sky_light();
    });
}

auto world_grid_system::column_bottom_(
    vec2i coord
) -> std::optional<int32> {
    if (const auto it = staged_columns_.find(coord); it != staged_columns_.end()) {
        const auto& chunks = it->second->get_all_chunk_data();
        if (chunks.empty()) {
            return std::nullopt;
        }
        // The map runs top down, so the last entry is the floor.
        return chunks.rbegin()->first;
    }

    const auto levels = grid_->column_levels(coord);
    if (levels.empty()) {
        return std::nullopt;
    }
    return levels.front();
}

auto world_grid_system::column_stack_(
    vec2i coord, int32 bottom
) -> std::vector<std::shared_ptr<asset::model>> {
    std::vector<std::shared_ptr<asset::model>> stack;

    const auto put = [&](int32 y, std::shared_ptr<asset::model> mdl) -> void {
        if (y < bottom) {
            return;
        }
        const auto slot = static_cast<std::size_t>(y - bottom);
        if (stack.size() <= slot) {
            stack.resize(slot + 1);
        }
        stack[slot] = std::move(mdl);
    };

    if (const auto it = staged_columns_.find(coord); it != staged_columns_.end()) {
        for (auto& [y, cd] : it->second->get_all_chunk_data()) {
            put(y, cd.chunk_model);
        }
        return stack;
    }

    for (int32 y : grid_->column_levels(coord)) {
        if (auto* placed = grid_->get_chunk(vec3i{coord.x, y, coord.y})) {
            put(y, placed->get_model());
        }
    }
    return stack;
}

auto world_grid_system::dispatch_light_(
    vec2i coord
) -> bool {
    if (baker_ == nullptr) {
        return false;
    }
    if (baker_->is_pending(coord)) {
        return true;
    }

    // Every column in this world stands on the same floor, so any one of the
    // nine gives it. Taking the lowest is the same answer with one less
    // assumption.
    int32 bottom = std::numeric_limits<int32>::max();
    for (auto offset : column_neighbor_offsets) {
        if (const auto at = column_bottom_(coord + offset)) {
            bottom = std::min(bottom, *at);
        }
    }
    const auto own = column_bottom_(coord);
    if (!own) {
        return false;
    }
    bottom = std::min(bottom, *own);

    sky_light_request job;
    job.coord    = coord;
    job.bottom_y = bottom;

    for (int32 dz = -1; dz <= 1; ++dz) {
        for (int32 dx = -1; dx <= 1; ++dx) {
            const auto slot  = static_cast<std::size_t>(((dz + 1) * 3) + (dx + 1));
            job.around[slot] = column_stack_(coord + vec2i{dx, dz}, bottom);
        }
    }

    return baker_->request(std::move(job));
}

void world_grid_system::collect_lit_columns_() {
    if (baker_ == nullptr) {
        return;
    }

    while (auto result = baker_->try_pop_completed()) {
        const auto it = staged_columns_.find(result->coord);
        if (it == staged_columns_.end()) {
            // Not staged, so this is a relight of a column already standing in
            // the world -- or of one that has since been unloaded, which
            // apply_relit_column_ finds nothing of and drops.
            apply_relit_column_(*result);
            continue;
        }

        for (auto& [y, cd] : it->second->get_all_chunk_data()) {
            const auto slot = static_cast<std::size_t>(y - result->bottom_y);
            if (slot < result->fields.size()) {
                cd.chunk_model->set_sky_light(std::move(result->fields[slot]));
            }
        }

        it->second->set_phase(column_phase::complete);
        ready_columns_.push_back(result->coord);
    }
}

// A relit column is already in the world with meshes built against its old
// light, so every chunk whose light has really moved has to be meshed again.
//
// Most of them have not. A relight bakes the whole column because sky light
// after an edit can change anywhere below it, but a spade underground changes
// nothing at all: rock is dark before and dark after, and comparing the fields
// is what keeps digging a mine from remeshing nine chunks a stroke.
void world_grid_system::apply_relit_column_(
    sky_light_result& result
) {
    for (int32 y : grid_->column_levels(result.coord)) {
        const auto slot = static_cast<std::size_t>(y - result.bottom_y);
        if (slot >= result.fields.size()) {
            continue;
        }

        const vec3i at{result.coord.x, y, result.coord.y};
        auto* placed = grid_->get_chunk(at);
        if (placed == nullptr) {
            continue;
        }

        auto& mdl         = *placed->get_model();
        const auto* stood = mdl.get_sky_light();
        if (stood != nullptr && *stood == result.fields[slot]) {
            continue;
        }

        mdl.set_sky_light(std::move(result.fields[slot]));
        grid_->remesh_drawn_chunk(at);
        ++stats_.relit_chunks;
    }
}

// Edits name the columns they spoiled; this sends them back through the same
// baker that lit them in the first place. Nothing incremental happens here --
// the column is flooded again from scratch, which is what makes the answer
// after an edit exactly the answer the column would have been generated with.
//
// A column being lit right now is left dirty rather than asked again: the job
// in flight was started from voxels that have since moved, so its result is
// already wrong and the next frame has to order another one anyway.
void world_grid_system::relight_dirty_columns_() {
    if (baker_ == nullptr) {
        return;
    }

    for (vec2i coord : grid_->take_light_dirty()) {
        light_dirty_.insert(coord);
    }

    if (light_dirty_.empty()) {
        return;
    }

    // Relights compete with streaming for the same four workers, and the
    // camera getting its ground is worth more than a shaft lighting up a frame
    // sooner.
    static constexpr int32 max_relights_per_frame = 2;
    int32 started                                 = 0;

    std::erase_if(light_dirty_, [&](vec2i coord) -> bool {
        if (!column_available_(coord)) {
            return true;
        }
        if (started >= max_relights_per_frame || baker_->is_pending(coord)) {
            return false;
        }
        if (!column_ready_(coord)) {
            return false;
        }

        if (!dispatch_light_(coord)) {
            return true;
        }

        ++started;
        ++stats_.relit_columns;
        return true;
    });

    stats_.relight_backlog = static_cast<uint32>(light_dirty_.size());
}

void world_grid_system::stage_completed_columns_() {
    while (true) {
        auto col = loader_->try_pop_completed();
        if (!col) {
            break;
        }

        const auto coord = col->get_coord();
        if (!active_columns_.contains(coord) && !pending_active_columns_.contains(coord)) {
            continue;
        }

        staged_columns_[coord] = std::move(col);

        // Staging this one can complete the neighbourhood of any column around
        // it, and of itself.
        queue_if_ready_(coord);
        for (auto offset : column_neighbor_offsets) {
            queue_if_ready_(coord + offset);
        }
    }
}

auto world_grid_system::model_at_(
    vec3i chunk_coord
) const -> asset::model* {
    if (auto* placed = grid_->get_chunk(chunk_coord)) {
        return placed->get_model().get();
    }

    const auto it = staged_columns_.find(vec2i{chunk_coord.x, chunk_coord.z});
    if (it == staged_columns_.end()) {
        return nullptr;
    }

    const auto* data = it->second->get_chunk_data(chunk_coord.y);
    return data != nullptr ? data->chunk_model.get() : nullptr;
}

void world_grid_system::integrate_completed_columns_() {
    static constexpr int32 max_columns_per_frame = 1;

    float32 boundary_from_total = 0.0f;
    float32 chunk_create_total  = 0.0f;
    int32 processed_columns     = 0;

    while (processed_columns < max_columns_per_frame && !ready_columns_.empty()) {
        const auto coord = ready_columns_.back();
        ready_columns_.pop_back();

        const auto it = staged_columns_.find(coord);
        if (it == staged_columns_.end()) {
            continue;
        }

        // The camera may have moved since it was queued and taken a neighbour
        // with it. Back to waiting rather than placed with a hole beside it.
        if (!column_ready_(coord)) {
            it->second->set_phase(column_phase::terrain);
            continue;
        }

        // Boundaries first, for the whole column, and only then the placing:
        // a chunk's neighbours above and below are its own column's, and taking
        // the column out of staging before reading them leaves those faces
        // thinking they face open air.
        boundary_from_total += measure_ms([&] -> auto {
            for (auto& [y, cd] : it->second->get_all_chunk_data()) {
                // Every neighbour that will ever exist exists now, staged or
                // placed, so this is the last word on the chunk's boundary. One
                // that is absent is absent for good -- open sky over a shorter
                // column -- and reads as air.
                for (int32 fd = 0; fd < 6; ++fd) {
                    if (auto* neighbor = model_at_(cd.coord + boundary_face_offsets[fd])) {
                        cd.chunk_model->set_boundary_slice(fd, *neighbor);
                    }
                }
            }
        });

        auto col = std::move(it->second);
        staged_columns_.erase(it);

        std::vector<int32> y_levels;

        for (auto& [y, cd] : col->get_all_chunk_data()) {
            chunk_create_total += measure_ms([&] -> auto {
                grid_->place_chunk(cd.coord, std::move(cd.chunk_model));
            });

            y_levels.push_back(y);
        }

        std::sort(y_levels.begin(), y_levels.end());
        grid_->register_column(coord, std::move(y_levels));
        ++processed_columns;
    }

    stats_.boundary_from_ms = boundary_from_total;
    stats_.chunk_create_ms  = chunk_create_total;
}

void world_grid_system::dispatch_column_requests_() {
    static constexpr int32 max_requests_per_frame = 8;

    // A ceiling on work in flight, not just on work started per frame. The
    // loader had neither: four workers kept generating whatever had been asked
    // for, and a column sitting generated-but-not-yet-placed holds nine models
    // and about 280 pages. Walking forward reached a queue peak of 482 columns
    // in a normal run, and one stall of the main thread -- a second is enough
    // -- took that past the addressable limit of the page pool and killed the
    // process. Backpressure instead: nothing new is asked for until what was
    // asked for has landed.
    static constexpr uint32 max_columns_in_flight = 96;

    // Light is a second stage with a queue of its own, and a column waiting
    // to be lit holds the same nine models as one waiting to be generated.
    // Counting only the loader would let generation run away from it.
    const uint32 in_flight =
        loader_->pending_count() + (baker_ != nullptr ? baker_->pending_count() : 0U);

    int32 requests = 0;
    while (!pending_requests_.empty() && requests < max_requests_per_frame &&
           in_flight < max_columns_in_flight) {
        auto coord = pending_requests_.back();
        pending_requests_.pop_back();
        if (loader_->request(coord)) {
            ++requests;
        }
    }
}

void world_grid_system::update_grid_stats_() {
    stats_.active_count      = static_cast<uint32>(active_columns_.size());
    stats_.pending_count     = loader_->pending_count();
    stats_.loaded_count      = grid_->chunk_count();
    stats_.drawn_count       = grid_->drawn_chunk_count();
    stats_.staged_count      = static_cast<uint32>(staged_columns_.size());
    stats_.lighting_count    = baker_ != nullptr ? baker_->pending_count() : 0U;
    stats_.rebuild_active_ms = 0.0f;
    stats_.unload_ms         = 0.0f;
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

        camera_column_ = camera_column;
        draw_distance_ = static_cast<int32>(wv.get_view_distance());

        // One column past what will be drawn: the ring is generated so the ring
        // inside it knows its boundaries and can be meshed once and for all.
        const auto dist = draw_distance_ + apron_columns;

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

void world_grid_system::demote_column_(
    vec2i coord
) {
    // Out of drawing range but still needed as a neighbour. Its meshes,
    // entities and instances go; the models stay, so coming back costs nothing
    // and the column is never generated twice.
    auto col = std::make_unique<gen_column>(coord.x, coord.y);

    for (int32 y : grid_->column_levels(coord)) {
        const vec3i chunk_coord{coord.x, y, coord.y};
        if (auto* placed = grid_->get_chunk(chunk_coord)) {
            col->create_chunk(y, chunk_data{chunk_coord, placed->get_model()});
        }
    }

    grid_->unload_column(coord);

    col->set_phase(column_phase::terrain);
    staged_columns_[coord] = std::move(col);
}

void world_grid_system::unload_inactive_columns_() {
    for (const auto& coord : active_columns_) {
        if (!pending_active_columns_.contains(coord)) {
            grid_->unload_column(coord);
        } else if (!within_draw_(coord) && grid_->has_column(coord)) {
            demote_column_(coord);
        }
    }

    std::erase_if(staged_columns_, [this](const auto& entry) -> bool {
        return !pending_active_columns_.contains(entry.first);
    });

    // A queued column whose neighbour has just gone is checked again when it
    // comes up for placement, so the queue only needs the vanished ones out.
    std::erase_if(ready_columns_, [this](vec2i coord) -> bool {
        return !staged_columns_.contains(coord);
    });

    // The camera moved, so columns that were only waiting on a neighbour may be
    // whole now, and demoted ones may be back in range. Marking one only sets a
    // phase, so walking the table while doing it is safe.
    for (const auto& [coord, col] : staged_columns_) {
        queue_if_ready_(coord);
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
        if (!column_available_(coord) && !loader_->is_pending(coord)) {
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
    staged_columns_.clear();
    ready_columns_.clear();
    light_dirty_.clear();
    active_columns_.clear();
    pending_active_columns_.clear();
}

void world_grid_system::clear_loader_transient_state_() {
    pending_requests_.clear();
}

}  // namespace vw::ecs
