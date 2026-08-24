module vw.ecs;

import std;
import vw.core;

namespace vw::ecs::detail {

auto next_component_id() -> uint32 {
    static std::atomic<uint32> counter{0};
    return counter.fetch_add(1, std::memory_order_relaxed);
}

}  // namespace vw::ecs::detail

namespace vw::ecs {

auto registry::create() -> entity {
    return entity_pool_.create();
}

auto registry::batch_create(uint32 count) -> std::vector<entity> {
    return entity_pool_.batch_create(count);
}

void registry::destroy(entity e) {
    remove_all(e);
    destroyed_set_.push_back(e);
    entity_pool_.destroy(e);
}

void registry::batch_destroy(const std::vector<entity>& entities) {
    for (auto e : entities) {
        remove_all(e);
    }
    destroyed_set_.insert(destroyed_set_.end(), entities.begin(), entities.end());
    entity_pool_.batch_destroy(entities);
}

auto registry::alive(entity e) const -> bool {
    return entity_pool_.has(e);
}

auto registry::alive_entities() const -> std::vector<entity> {
    return entity_pool_.alive_entities();
}

auto registry::destroyed() const -> const std::vector<entity>& {
    return destroyed_set_;
}

auto registry::ensure_pool(uint32 component_id, component_ops ops) -> component_pool& {
    ensure_id_slot_(component_id);
    if (pools_[component_id] == nullptr) {
        pools_[component_id] = std::make_unique<component_pool>(ops);
    }
    return *pools_[component_id];
}

auto registry::try_pool(uint32 component_id) -> component_pool* {
    return component_id < pools_.size() ? pools_[component_id].get() : nullptr;
}

auto registry::try_pool(uint32 component_id) const -> const component_pool* {
    return component_id < pools_.size() ? pools_[component_id].get() : nullptr;
}

auto registry::pool_count() const -> uint32 {
    return static_cast<uint32>(pools_.size());
}

void registry::remove_all(entity e) {
    for (auto& pool : pools_) {
        if (pool != nullptr) {
            pool->remove(e);
        }
    }
}

void registry::collect_components(entity e, std::vector<uint32>& ids_out) const {
    for (uint32 id = 0; id < pools_.size(); ++id) {
        if (pools_[id] != nullptr && pools_[id]->has(e)) {
            ids_out.push_back(id);
        }
    }
}

void registry::add_change_dep(uint32 component_id, uint32 dependent_id) {
    ensure_id_slot_(component_id);
    ensure_id_slot_(dependent_id);
    auto& deps = change_deps_[component_id];
    if (std::ranges::find(deps, dependent_id) == deps.end()) {
        deps.push_back(dependent_id);
    }
}

void registry::notify_changed(uint32 component_id, entity e) {
    ensure_id_slot_(component_id);
    changed_sets_[component_id].insert(e);

    for (uint32 dep : change_deps_[component_id]) {
        const auto* pool = try_pool(dep);
        if (pool != nullptr && pool->has(e)) {
            request_sets_[dep].insert(e);
        }
    }
}

void registry::request_change(uint32 component_id, entity e) {
    ensure_id_slot_(component_id);
    request_sets_[component_id].insert(e);
}

auto registry::changed_set(uint32 component_id) -> std::unordered_set<entity>& {
    ensure_id_slot_(component_id);
    return changed_sets_[component_id];
}

auto registry::requested_set(uint32 component_id) -> std::unordered_set<entity>& {
    ensure_id_slot_(component_id);
    return request_sets_[component_id];
}

void registry::clear_requested(uint32 component_id) {
    ensure_id_slot_(component_id);
    request_sets_[component_id].clear();
}

void registry::clear_changed() {
    for (auto& set : changed_sets_) {
        set.clear();
    }
    destroyed_set_.clear();
}

void registry::ensure_id_slot_(uint32 component_id) {
    const std::size_t needed = static_cast<std::size_t>(component_id) + 1;
    if (needed > pools_.size()) {
        pools_.resize(needed);
        change_deps_.resize(needed);
        request_sets_.resize(needed);
        changed_sets_.resize(needed);
    }
}

}  // namespace vw::ecs
