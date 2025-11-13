#pragma once

#ifndef VW_GFX_COMPONENT_REGISTRY_H
#define VW_GFX_COMPONENT_REGISTRY_H

#include <algorithm>
#include <memory>
#include <ranges>
#include <typeindex>

#include "vw/gfx/world/component_pool.h"
#include "vw/gfx/world/entity_pool.h"

namespace vw::gfx {

template <typename T, typename... Cs>
class component_view;

template <typename T, typename... Ts>
consteval size_t type_index_in() {
    static_assert((std::same_as<T, Ts> || ...), "type not in parameter pack");
    static_assert(
        ((std::same_as<T, Ts> ? 1 : 0) + ...) == 1,
        "type must appear exactly once in parameter pack"
    );

    constexpr bool matches[] = {std::same_as<T, Ts>...};
    for (std::size_t i = 0; i < sizeof...(Ts); ++i) {
        if (matches[i]) {
            return i;
        }
    }
    return std::numeric_limits<size_t>::max();
}

template <typename... Ts>
class registry {
public:
    template <typename T>
    component_pool<T>& get_pool() {
        constexpr size_t index = type_index_in<T, Ts...>();
        static_assert(index < sizeof...(Ts), "type not in component registry");

        return std::get<index>(component_pools_);
    }

    template <typename T>
    const component_pool<T>& get_pool() const {
        constexpr size_t index = type_index_in<T, Ts...>();
        static_assert(index < sizeof...(Ts), "type not in component registry");

        return std::get<index>(component_pools_);
    }

    entity create() {
        return entity_pool_.create();
    }

    template <typename T>
    void add(entity e, T&& value = {}) {
        get_pool<T>().add(e, std::forward<T>(value));
    }

    template <typename T>
    [[nodiscard]] auto has(entity ent) const -> bool {
        return get_pool<T>().has(ent);
    }

    template <typename T>
    T& get(entity e) {
        return get_pool<T>().get(e);
    }

    template <typename T>
    const T& get(entity e) const {
        return get_pool<T>().get(e);
    }

    template <typename T>
    void remove(entity e) {
        get_pool<T>().remove(e);
    }

    void remove_all(entity e) {
        (remove<Ts>(e), ...);
    }

    void destroy(entity e) {
        remove_all(e);
        entity_pool_.destroy(e);
    }

    template <typename... Cs>
    auto view() {
        return component_view<registry, Cs...>(*this);
    }

private:
    entity_pool entity_pool_;
    std::tuple<component_pool<Ts>...> component_pools_;
};

template <typename T, typename... Cs>
class component_view {
public:
    explicit component_view(T& registry) : registry_{&registry} {
        pick_entities_();
    }

    struct iterator {
        component_view* view = nullptr;
        size_t index         = 0;

        [[nodiscard]]
        auto operator==(const iterator& rhs) const -> bool {
            return index == rhs.index;
        }

        [[nodiscard]]
        auto operator!=(const iterator& rhs) const -> bool {
            return !(*this == rhs);
        }

        void operator++() {
            advance_();
        }

        [[nodiscard]] auto operator*() const -> std::tuple<const entity&, const Cs&...> {
            const entity& ent = (*view->entities_)[index];
            return std::tuple<const entity&, const Cs&...>{
                ent,
                view->registry_->template get<Cs>(ent)...
            };
        }

    private:
        void advance_() {
            bool not_at_end = true;
            bool not_in_all = true;
            for (; not_at_end && not_in_all; index++) {
                not_at_end = index < view->entities_->size();
                not_in_all = not_at_end && !view->present_in_all((*view->entities_)[index]);
            }
        }
    };

    [[nodiscard]] auto begin() -> iterator {
        iterator it{this, 0};
        if (entities_->empty()) [[unlikely]] {
            return it;
        }
        if (!present_in_all(entities_->front())) {
            ++it;
        }
        return it;
    }

    [[nodiscard]] auto end() -> iterator {
        iterator it{this, entities_->size()};
        return it;
    }

private:
    T* registry_;
    const std::vector<entity>* entities_ = nullptr;

    void pick_entities_() {
        if constexpr (sizeof...(Cs) == 0) {
            static std::vector<entity> empty{};
            entities_ = &empty;
            return;
        }

        std::array<std::pair<size_t, const std::vector<entity>*>, sizeof...(Cs)> candidates = {
            std::pair<size_t, const std::vector<entity>*>{
                registry_->template get_pool<Cs>().entities().size(),
                &registry_->template get_pool<Cs>().entities()
            }...
        };

        auto it = std::min_element(
            std::begin(candidates),
            std::end(candidates),
            [](const auto& a, const auto& b) { return a.first < b.first; }
        );
        entities_ = it->second;
    }

    [[nodiscard]] auto present_in_all(entity e) const -> bool {
        return (registry_->template get_pool<Cs>().has(e) && ...);
    }
};

template <typename... Ts>
struct registry_from_tuple;

template <typename... Ts>
struct registry_from_tuple<std::tuple<Ts...>> {
    using type = registry<Ts...>;
};

}  // namespace vw::gfx

#endif  // VW_GFX_COMPONENT_REGISTRY_H
