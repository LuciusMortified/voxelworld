module;

#include <algorithm>
#include <array>
#include <cstddef>
#include <memory>
#include <span>
#include <tuple>
#include <type_traits>
#include <unordered_set>
#include <utility>
#include <vector>

export module vw.ecs:registry;

import vw.core;
import :entity;
import :pool;

namespace vw::ecs::detail {
auto next_component_id() -> uint32;
}

export namespace vw::ecs {

// Component ids are handed out lazily on first use, so a type never mentioned
// at compile time (a script-registered component) can join through the untyped
// surface of the registry with an id obtained the same way.
template <typename T>
auto component_id_of() -> uint32 {
    static const uint32 id = detail::next_component_id();
    return id;
}

template <typename... Cs>
class component_view;

class registry final {
public:
    [[nodiscard]] auto create() -> entity;
    [[nodiscard]] auto batch_create(uint32 count) -> std::vector<entity>;

    void destroy(entity e);
    void batch_destroy(const std::vector<entity>& entities);

    [[nodiscard]] auto alive(entity e) const -> bool;
    [[nodiscard]] auto alive_entities() const -> std::vector<entity>;
    [[nodiscard]] auto destroyed() const -> const std::vector<entity>&;

    auto ensure_pool(uint32 component_id, component_ops ops) -> component_pool&;
    [[nodiscard]] auto try_pool(uint32 component_id) -> component_pool*;
    [[nodiscard]] auto try_pool(uint32 component_id) const -> const component_pool*;
    [[nodiscard]] auto pool_count() const -> uint32;

    void remove_all(entity e);
    void collect_components(entity e, std::vector<uint32>& ids_out) const;

    void add_change_dep(uint32 component_id, uint32 dependent_id);
    void notify_changed(uint32 component_id, entity e);
    void request_change(uint32 component_id, entity e);
    [[nodiscard]] auto changed_set(uint32 component_id) -> std::unordered_set<entity>&;
    [[nodiscard]] auto requested_set(uint32 component_id) -> std::unordered_set<entity>&;
    void clear_requested(uint32 component_id);
    void clear_changed();

    template <typename T>
    auto pool_of() -> component_pool& {
        return ensure_pool(component_id_of<T>(), ops_of<T>());
    }

    template <typename T>
    [[nodiscard]] auto try_pool_of() -> component_pool* {
        return try_pool(component_id_of<T>());
    }

    template <typename T>
    [[nodiscard]] auto try_pool_of() const -> const component_pool* {
        return try_pool(component_id_of<T>());
    }

    template <typename T, typename... Args>
    auto emplace(entity e, Args&&... args) -> T& {
        void* slot = pool_of<T>().emplace(e);
        return *std::construct_at(static_cast<T*>(slot), std::forward<Args>(args)...);
    }

    template <typename T>
    void add(entity e, T&& value = {}) {
        using C = std::remove_cvref_t<T>;
        void* slot = pool_of<C>().emplace(e);
        std::construct_at(static_cast<C*>(slot), std::forward<T>(value));
    }

    template <typename T>
    void batch_add(const std::vector<entity>& entities, const T& value = {}) {
        auto& pool = pool_of<T>();
        for (auto e : entities) {
            std::construct_at(static_cast<T*>(pool.emplace(e)), value);
        }
    }

    template <typename T>
    [[nodiscard]] auto has(entity e) const -> bool {
        if (!alive(e)) {
            return false;
        }
        const auto* pool = try_pool_of<T>();
        return pool != nullptr && pool->has(e);
    }

    template <typename T>
    [[nodiscard]] auto get(entity e) -> T& {
        return *static_cast<T*>(try_pool_of<T>()->get(e));
    }

    template <typename T>
    [[nodiscard]] auto get(entity e) const -> const T& {
        return *static_cast<const T*>(try_pool_of<T>()->get(e));
    }

    template <typename T>
    void remove(entity e) {
        if (auto* pool = try_pool_of<T>()) {
            pool->remove(e);
        }
    }

    template <typename T>
    void batch_remove(const std::vector<entity>& entities) {
        if (auto* pool = try_pool_of<T>()) {
            pool->batch_remove(entities);
        }
    }

    template <typename... Cs>
    [[nodiscard]] auto view() -> component_view<Cs...> {
        return component_view<Cs...>(*this);
    }

    template <typename T>
    void request_change(entity e) {
        request_change(component_id_of<T>(), e);
    }

    template <typename T>
    [[nodiscard]] auto requested() -> std::unordered_set<entity>& {
        return requested_set(component_id_of<T>());
    }

    template <typename T>
    void clear_requested() {
        clear_requested(component_id_of<T>());
    }

    template <typename T>
    void notify_changed(entity e) {
        notify_changed(component_id_of<T>(), e);
    }

    template <typename T>
    [[nodiscard]] auto changed() -> std::unordered_set<entity>& {
        return changed_set(component_id_of<T>());
    }

private:
    void ensure_id_slot_(uint32 component_id);

    entity_pool entity_pool_;
    std::vector<std::unique_ptr<component_pool>> pools_;
    std::vector<std::vector<uint32>> change_deps_;
    std::vector<std::unordered_set<entity>> request_sets_;
    std::vector<std::unordered_set<entity>> changed_sets_;
    std::vector<entity> destroyed_set_;
};

// Iterates the smallest of the requested pools and skips entities missing any
// of the others. Pool pointers are resolved once per view, not per element.
template <typename... Cs>
class component_view {
public:
    explicit component_view(registry& reg) : registry_{&reg} {
        pick_entities_();
    }

    struct iterator {
        const component_view* view = nullptr;
        std::size_t index          = 0;

        [[nodiscard]] auto operator==(const iterator& rhs) const -> bool {
            return index == rhs.index;
        }

        [[nodiscard]] auto operator!=(const iterator& rhs) const -> bool {
            return !(*this == rhs);
        }

        void operator++() {
            ++index;
            skip_missing();
        }

        [[nodiscard]] auto operator*() const -> std::tuple<entity, Cs&...> {
            const entity ent = (*view->entities_)[index];
            return std::tuple<entity, Cs&...>{ent, view->template component_of<Cs>(ent)...};
        }

        void skip_missing() {
            while (index < view->entities_->size() &&
                   !view->present_in_all((*view->entities_)[index])) {
                ++index;
            }
        }
    };

    [[nodiscard]] auto begin() -> iterator {
        iterator it{this, 0};
        it.skip_missing();
        return it;
    }

    [[nodiscard]] auto end() -> iterator {
        return iterator{this, entities_->size()};
    }

private:
    static inline const std::vector<entity> no_entities{};

    template <typename C>
    [[nodiscard]] auto component_of(entity e) const -> C& {
        return *static_cast<C*>(pools_[index_of_<C>()]->get(e));
    }

    template <typename C>
    static consteval auto index_of_() -> std::size_t {
        constexpr std::array<bool, sizeof...(Cs)> matches{std::is_same_v<C, Cs>...};
        for (std::size_t i = 0; i < matches.size(); ++i) {
            if (matches[i]) {
                return i;
            }
        }
        return matches.size();
    }

    void pick_entities_() {
        entities_ = &no_entities;
        if constexpr (sizeof...(Cs) == 0) {
            return;
        } else {
            pools_ = {registry_->template try_pool_of<Cs>()...};
            if (std::ranges::find(pools_, nullptr) != pools_.end()) {
                return;
            }

            const auto* smallest = *std::ranges::min_element(
                pools_,
                [](const component_pool* a, const component_pool* b) -> bool {
                    return a->size() < b->size();
                });
            entities_ = &smallest->entities();
        }
    }

    [[nodiscard]] auto present_in_all(entity e) const -> bool {
        return std::ranges::all_of(pools_, [e](const component_pool* p) -> bool {
            return p->has(e);
        });
    }

    registry* registry_ = nullptr;
    std::array<component_pool*, sizeof...(Cs)> pools_{};
    const std::vector<entity>* entities_ = nullptr;
};

}  // namespace vw::ecs
