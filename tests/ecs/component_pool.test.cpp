#include <catch2/catch_test_macros.hpp>

#include <memory>
#include <string>
#include <utility>
#include <vector>

import vw.core;
import vw.ecs;

using namespace vw;
using namespace vw::ecs;

namespace {

struct test_component {
    int value = 0;
};

int live_instances = 0;

struct tracked_component {
    std::string label;

    explicit tracked_component(std::string text = {}) : label{std::move(text)} {
        ++live_instances;
    }
    tracked_component(const tracked_component& other) : label{other.label} {
        ++live_instances;
    }
    tracked_component(tracked_component&& other) noexcept : label{std::move(other.label)} {
        ++live_instances;
    }
    auto operator=(const tracked_component&) -> tracked_component& = delete;
    auto operator=(tracked_component&&) -> tracked_component&      = delete;
    ~tracked_component() {
        --live_instances;
    }
};

template <typename T, typename... Args>
auto put(component_pool& pool, entity e, Args&&... args) -> T& {
    return *std::construct_at(static_cast<T*>(pool.emplace(e)), std::forward<Args>(args)...);
}

template <typename T>
auto fetch(component_pool& pool, entity e) -> T& {
    return *static_cast<T*>(pool.get(e));
}

auto make_pool_of_test_component() -> component_pool {
    return component_pool{ops_of<test_component>()};
}

}  // namespace

TEST_CASE("component_pool emplace and get", "[component_pool]") {
    auto pool = make_pool_of_test_component();

    entity e{0, 0};
    put<test_component>(pool, e, 42);

    REQUIRE(pool.has(e));
    REQUIRE(fetch<test_component>(pool, e).value == 42);
    REQUIRE(pool.size() == 1);
}

TEST_CASE("component_pool get returns nullptr for absent entity", "[component_pool]") {
    auto pool = make_pool_of_test_component();

    REQUIRE(pool.get(entity{7, 0}) == nullptr);
}

TEST_CASE("component_pool distinguishes generations", "[component_pool]") {
    auto pool = make_pool_of_test_component();

    put<test_component>(pool, entity{3, 0}, 1);

    REQUIRE(pool.has(entity{3, 0}));
    REQUIRE_FALSE(pool.has(entity{3, 1}));
}

TEST_CASE("component_pool remove", "[component_pool]") {
    auto pool = make_pool_of_test_component();

    entity e{0, 0};
    put<test_component>(pool, e, 42);
    pool.remove(e);

    REQUIRE_FALSE(pool.has(e));
    REQUIRE(pool.size() == 0);
}

TEST_CASE("component_pool emplace on existing entity overwrites in place", "[component_pool]") {
    auto pool = make_pool_of_test_component();

    entity e{0, 0};
    put<test_component>(pool, e, 1);
    put<test_component>(pool, e, 42);

    REQUIRE(pool.size() == 1);
    REQUIRE(fetch<test_component>(pool, e).value == 42);
}

TEST_CASE("component_pool keeps a dense array", "[component_pool]") {
    auto pool = make_pool_of_test_component();

    const std::vector<entity> entities = {{0, 0}, {1, 0}, {2, 0}, {3, 0}, {4, 0}};
    for (auto e : entities) {
        put<test_component>(pool, e, static_cast<int>(e.index));
    }

    REQUIRE(pool.entities().size() == 5);
    for (uint32 i = 0; i < entities.size(); ++i) {
        REQUIRE(pool.entities()[i] == entities[i]);
    }

    const auto* raw = static_cast<const test_component*>(pool.get(entities.front()));
    for (uint32 i = 0; i < entities.size(); ++i) {
        REQUIRE(raw[i].value == static_cast<int>(i));
    }
}

TEST_CASE("component_pool remove swaps the last element into the hole", "[component_pool]") {
    auto pool = make_pool_of_test_component();

    put<test_component>(pool, entity{0, 0}, 10);
    put<test_component>(pool, entity{1, 0}, 20);
    put<test_component>(pool, entity{2, 0}, 30);

    pool.remove(entity{0, 0});

    REQUIRE(pool.size() == 2);
    REQUIRE(pool.entities()[0] == entity{2, 0});
    REQUIRE(fetch<test_component>(pool, entity{2, 0}).value == 30);
    REQUIRE(fetch<test_component>(pool, entity{1, 0}).value == 20);
}

TEST_CASE("component_pool batch_remove", "[component_pool]") {
    auto pool = make_pool_of_test_component();

    const std::vector<entity> entities = {{0, 0}, {1, 0}, {2, 0}};
    for (auto e : entities) {
        put<test_component>(pool, e, 10);
    }

    pool.batch_remove(entities);

    for (auto e : entities) {
        REQUIRE_FALSE(pool.has(e));
    }
    REQUIRE(pool.size() == 0);
}

TEST_CASE("component_pool grows the sparse table on demand", "[component_pool]") {
    auto pool = make_pool_of_test_component();

    put<test_component>(pool, entity{10, 0}, 5);
    put<test_component>(pool, entity{2000, 0}, 6);

    REQUIRE(fetch<test_component>(pool, entity{10, 0}).value == 5);
    REQUIRE(fetch<test_component>(pool, entity{2000, 0}).value == 6);
}

TEST_CASE("component_pool destroys non-trivial components", "[component_pool]") {
    live_instances = 0;

    {
        component_pool pool{ops_of<tracked_component>()};

        put<tracked_component>(pool, entity{0, 0}, "first");
        put<tracked_component>(pool, entity{1, 0}, "second");
        REQUIRE(live_instances == 2);

        pool.remove(entity{0, 0});
        REQUIRE(live_instances == 1);
        REQUIRE(fetch<tracked_component>(pool, entity{1, 0}).label == "second");
    }

    REQUIRE(live_instances == 0);
}

TEST_CASE("component_pool relocates non-trivial components on growth", "[component_pool]") {
    live_instances = 0;

    {
        component_pool pool{ops_of<tracked_component>()};

        constexpr uint32 count = 64;
        for (uint32 i = 0; i < count; ++i) {
            put<tracked_component>(pool, entity{i, 0}, "item-" + std::to_string(i));
        }

        REQUIRE(live_instances == static_cast<int>(count));
        for (uint32 i = 0; i < count; ++i) {
            REQUIRE(fetch<tracked_component>(pool, entity{i, 0}).label ==
                    "item-" + std::to_string(i));
        }
    }

    REQUIRE(live_instances == 0);
}

TEST_CASE("component_pool clear destroys everything", "[component_pool]") {
    live_instances = 0;

    component_pool pool{ops_of<tracked_component>()};
    put<tracked_component>(pool, entity{0, 0}, "a");
    put<tracked_component>(pool, entity{1, 0}, "b");

    pool.clear();

    REQUIRE(live_instances == 0);
    REQUIRE(pool.size() == 0);
    REQUIRE_FALSE(pool.has(entity{0, 0}));
}
