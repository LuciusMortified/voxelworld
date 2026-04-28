#include <catch2/catch_test_macros.hpp>

#include <vw/ecs/component_pool.h>

using namespace vw;
using namespace vw::ecs;

struct test_component {
    int value = 0;
};

TEST_CASE("component_pool add and get", "[component_pool]") {
    component_pool<test_component> pool;

    entity e{0, 0};
    pool.add(e, test_component{42});

    REQUIRE(pool.has(e));
    REQUIRE(pool.get(e).value == 42);
}

TEST_CASE("component_pool remove", "[component_pool]") {
    component_pool<test_component> pool;

    entity e{0, 0};
    pool.add(e, test_component{42});
    pool.remove(e);

    REQUIRE_FALSE(pool.has(e));
}

TEST_CASE("component_pool batch_add", "[component_pool]") {
    component_pool<test_component> pool;

    std::vector<entity> entities = {{0, 0}, {1, 0}, {2, 0}, {3, 0}, {4, 0}};
    pool.batch_add(entities, test_component{99});

    SECTION("all entities have components") {
        for (auto& e : entities) {
            REQUIRE(pool.has(e));
            REQUIRE(pool.get(e).value == 99);
        }
    }

    SECTION("contiguous in dense array") {
        auto& dense = pool.entities();
        REQUIRE(dense.size() == 5);
        for (uint32 i = 0; i < entities.size(); ++i) {
            REQUIRE(dense[i] == entities[i]);
        }
    }
}

TEST_CASE("component_pool batch_add overwrites existing", "[component_pool]") {
    component_pool<test_component> pool;

    entity e{0, 0};
    pool.add(e, test_component{1});

    std::vector<entity> entities = {{0, 0}, {1, 0}};
    pool.batch_add(entities, test_component{42});

    REQUIRE(pool.get(e).value == 42);
    REQUIRE(pool.has(entity{1, 0}));
}

TEST_CASE("component_pool batch_remove", "[component_pool]") {
    component_pool<test_component> pool;

    std::vector<entity> entities = {{0, 0}, {1, 0}, {2, 0}};
    pool.batch_add(entities, test_component{10});

    pool.batch_remove(entities);

    for (auto& e : entities) {
        REQUIRE_FALSE(pool.has(e));
    }
}

TEST_CASE("component_pool batch_add sparse resize", "[component_pool]") {
    component_pool<test_component> pool(4);

    std::vector<entity> entities = {{10, 0}, {20, 0}};
    pool.batch_add(entities, test_component{5});

    REQUIRE(pool.has(entity{10, 0}));
    REQUIRE(pool.has(entity{20, 0}));
    REQUIRE(pool.get(entity{10, 0}).value == 5);
}
