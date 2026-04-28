#include <catch2/catch_test_macros.hpp>

#include <vw/ecs/entity_pool.h>

using namespace vw;
using namespace vw::ecs;

TEST_CASE("entity_pool create", "[entity_pool]") {
    entity_pool pool;

    SECTION("sequential indices") {
        auto e0 = pool.create();
        auto e1 = pool.create();
        auto e2 = pool.create();

        REQUIRE(e0.index == 0);
        REQUIRE(e1.index == 1);
        REQUIRE(e2.index == 2);
        REQUIRE(e0.generation == 0);
        REQUIRE(e1.generation == 0);
        REQUIRE(e2.generation == 0);
    }

    SECTION("has returns true for alive entities") {
        auto e = pool.create();
        REQUIRE(pool.has(e));
    }

    SECTION("has returns false for destroyed entities") {
        auto e = pool.create();
        pool.destroy(e);
        REQUIRE_FALSE(pool.has(e));
    }

    SECTION("index reuse after destroy") {
        auto e0 = pool.create();
        pool.destroy(e0);
        auto e1 = pool.create();

        REQUIRE(e1.index == e0.index);
        REQUIRE(e1.generation == e0.generation + 1);
    }
}

TEST_CASE("entity_pool batch_create", "[entity_pool]") {
    entity_pool pool;

    SECTION("creates requested count") {
        auto entities = pool.batch_create(5);
        REQUIRE(entities.size() == 5);
        for (auto& e : entities) {
            REQUIRE(pool.has(e));
        }
    }

    SECTION("unique indices") {
        auto entities = pool.batch_create(10);
        for (uint32 i = 0; i < entities.size(); ++i) {
            for (uint32 j = i + 1; j < entities.size(); ++j) {
                REQUIRE(entities[i].index != entities[j].index);
            }
        }
    }

    SECTION("batch_create zero") {
        auto entities = pool.batch_create(0);
        REQUIRE(entities.empty());
    }
}

TEST_CASE("entity_pool batch_destroy", "[entity_pool]") {
    entity_pool pool;

    auto entities = pool.batch_create(5);
    pool.batch_destroy(entities);

    for (auto& e : entities) {
        REQUIRE_FALSE(pool.has(e));
    }
}
