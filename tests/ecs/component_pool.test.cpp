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

}  // namespace

TEST_CASE("component_pool emplace and get", "[component_pool]") {
    component_pool<test_component> pool;

    entity e{0, 0};
    pool.emplace(e, 42);

    REQUIRE(pool.has(e));
    REQUIRE(pool.get(e)->value == 42);
    REQUIRE(pool.size() == 1);
}

TEST_CASE("component_pool get returns nullptr for absent entity", "[component_pool]") {
    component_pool<test_component> pool;

    REQUIRE(pool.get(entity{7, 0}) == nullptr);
}

TEST_CASE("component_pool distinguishes generations", "[component_pool]") {
    component_pool<test_component> pool;

    pool.emplace(entity{3, 0}, 1);

    REQUIRE(pool.has(entity{3, 0}));
    REQUIRE_FALSE(pool.has(entity{3, 1}));
}

TEST_CASE("component_pool remove", "[component_pool]") {
    component_pool<test_component> pool;

    entity e{0, 0};
    pool.emplace(e, 42);
    pool.remove(e);

    REQUIRE_FALSE(pool.has(e));
    REQUIRE(pool.size() == 0);
}

TEST_CASE("component_pool emplace on existing entity overwrites in place", "[component_pool]") {
    component_pool<test_component> pool;

    entity e{0, 0};
    pool.emplace(e, 1);
    pool.emplace(e, 42);

    REQUIRE(pool.size() == 1);
    REQUIRE(pool.get(e)->value == 42);
}

TEST_CASE("component_pool keeps a dense array", "[component_pool]") {
    component_pool<test_component> pool;

    const std::vector<entity> entities = {{0, 0}, {1, 0}, {2, 0}, {3, 0}, {4, 0}};
    for (auto e : entities) {
        pool.emplace(e, static_cast<int>(e.index));
    }

    REQUIRE(pool.entities().size() == 5);
    for (uint32 i = 0; i < entities.size(); ++i) {
        REQUIRE(pool.entities()[i] == entities[i]);
    }

    const auto* raw = pool.data();
    for (uint32 i = 0; i < entities.size(); ++i) {
        REQUIRE(raw[i].value == static_cast<int>(i));
    }
}

TEST_CASE("component_pool remove swaps the last element into the hole", "[component_pool]") {
    component_pool<test_component> pool;

    pool.emplace(entity{0, 0}, 10);
    pool.emplace(entity{1, 0}, 20);
    pool.emplace(entity{2, 0}, 30);

    pool.remove(entity{0, 0});

    REQUIRE(pool.size() == 2);
    REQUIRE(pool.entities()[0] == entity{2, 0});
    REQUIRE(pool.get(entity{2, 0})->value == 30);
    REQUIRE(pool.get(entity{1, 0})->value == 20);
}

TEST_CASE("component_pool batch_remove", "[component_pool]") {
    component_pool<test_component> pool;

    const std::vector<entity> entities = {{0, 0}, {1, 0}, {2, 0}};
    for (auto e : entities) {
        pool.emplace(e, 10);
    }

    pool.batch_remove(entities);

    for (auto e : entities) {
        REQUIRE_FALSE(pool.has(e));
    }
    REQUIRE(pool.size() == 0);
}

TEST_CASE("component_pool grows the sparse table on demand", "[component_pool]") {
    component_pool<test_component> pool;

    pool.emplace(entity{10, 0}, 5);
    pool.emplace(entity{2000, 0}, 6);

    REQUIRE(pool.get(entity{10, 0})->value == 5);
    REQUIRE(pool.get(entity{2000, 0})->value == 6);
}

TEST_CASE("component_pool destroys non-trivial components", "[component_pool]") {
    live_instances = 0;

    {
        component_pool<tracked_component> pool;

        pool.emplace(entity{0, 0}, "first");
        pool.emplace(entity{1, 0}, "second");
        REQUIRE(live_instances == 2);

        pool.remove(entity{0, 0});
        REQUIRE(live_instances == 1);
        REQUIRE(pool.get(entity{1, 0})->label == "second");
    }

    REQUIRE(live_instances == 0);
}

TEST_CASE("component_pool relocates non-trivial components on growth", "[component_pool]") {
    live_instances = 0;

    {
        component_pool<tracked_component> pool;

        constexpr uint32 count = 64;
        for (uint32 i = 0; i < count; ++i) {
            pool.emplace(entity{i, 0}, "item-" + std::to_string(i));
        }

        REQUIRE(live_instances == static_cast<int>(count));
        for (uint32 i = 0; i < count; ++i) {
            REQUIRE(pool.get(entity{i, 0})->label == "item-" + std::to_string(i));
        }
    }

    REQUIRE(live_instances == 0);
}

TEST_CASE("component_pool clear destroys everything", "[component_pool]") {
    live_instances = 0;

    component_pool<tracked_component> pool;
    pool.emplace(entity{0, 0}, "a");
    pool.emplace(entity{1, 0}, "b");

    pool.clear();

    REQUIRE(live_instances == 0);
    REQUIRE(pool.size() == 0);
    REQUIRE_FALSE(pool.has(entity{0, 0}));
}

TEST_CASE("dynamic_pool stores a component known only at runtime", "[component_pool]") {
    dynamic_pool pool{component_layout{.size = sizeof(int), .align = alignof(int)}};

    entity first{0, 0};
    entity second{1, 0};

    *static_cast<int*>(pool.emplace(first))  = 7;
    *static_cast<int*>(pool.emplace(second)) = 9;

    REQUIRE(pool.size() == 2);
    REQUIRE(*static_cast<const int*>(pool.get(first)) == 7);

    pool.remove(first);

    REQUIRE(pool.size() == 1);
    REQUIRE_FALSE(pool.has(first));
    REQUIRE(*static_cast<const int*>(pool.get(second)) == 9);
}
