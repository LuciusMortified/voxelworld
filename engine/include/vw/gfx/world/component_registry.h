#pragma once

#ifndef VW_GFX_COMPONENT_REGISTRY_H
#define VW_GFX_COMPONENT_REGISTRY_H

#include <algorithm>
#include <memory>
#include <ranges>
#include <typeindex>

#include "vw/gfx/world/component_pool.h"

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
class component_registry {
public:
    template <typename T>
    component_pool<T>& get_pool() {
        constexpr size_t index = type_index_in<T, Ts...>();
        static_assert(index < sizeof...(Ts), "type not in component registry");

        return std::get<index>(pools_);
    }

    template <typename T>
    void add(entity e, T&& value = {}) {
        get_pool<T>().add(e, std::forward<T>(value));
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
        (get_pool<Ts>().remove(e), ...);
    }

    template <typename... Cs>
    auto view() {
        return component_view<component_registry, Cs...>(*this);
    }

private:
    std::tuple<component_pool<Ts>...> pools_;
};

template <typename T, typename... Cs>
class component_view {
public:
    explicit component_view(T& registry) : registry_{registry} {
        pick_entities_();
    }

    struct iterator {
        const component_view<Cs...>* view = nullptr;

        size_t index = 0;

        [[nodiscard]]
        bool operator==(const iterator& rhs) const {
            return index == rhs.index;
        }

        [[nodiscard]]
        bool operator!=(const iterator& rhs) const {
            return !(*this == rhs);
        }

        void operator++() {
            advance_();
        }

        [[nodiscard]]
        auto operator*() const {
            const entity e = (*view->entities_)[index];
            return view->tuple_for(e);
        }

    private:
        void advance_() {
            bool not_at_end = true, not_in_all = true;
            for (; not_at_end && not_in_all; index++) {
                not_at_end = index < view->entities_->size();
                not_in_all = not_at_end && !view->present_in_all((*view->entities_)[index]);
            }
        }
    };

    iterator begin() const {
        iterator it{this, 0};
        if (entities_->empty()) {
            return it;
        }
        if (!present_in_all(entities_->front())) {
            ++it;
        }
        return it;
    }

    iterator end() const {
        return {this, entities_->size()};
    }

private:
    T* registry_;
    std::vector<entity>* entities_ = nullptr;

    void pick_entities_() {
        if constexpr (sizeof...(Cs) == 0) {
            static std::vector<entity> empty{};
            entities_ = &empty;
            return;
        }

        std::array<std::pair<size_t, std::vector<entity>*>, sizeof...(Cs)> candidates = {
            {registry_->template get_pool<Cs>().entities().size(),
             &registry_->template get_pool<Cs>().entities()}...
        };

        auto it = std::min_element(
            std::begin(candidates),
            std::end(candidates),
            [](const auto& a, const auto& b) { return a.first < b.first; }
        );
        entities_ = it->second;
    }

    bool present_in_all(entity e) const {
        return (registry_->template get_pool<Cs>().has(e) && ...);
    }

    auto tuple_for(entity e) const {
        return std::forward_as_tuple(e, registry_->template get<Cs>(e)...);
    }
};

}  // namespace vw::gfx

#endif  // VW_GFX_COMPONENT_REGISTRY_H
