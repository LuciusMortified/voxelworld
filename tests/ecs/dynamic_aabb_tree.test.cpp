#include <catch2/catch_test_macros.hpp>

#include <vw/gfx/spatial/dynamic_aabb_tree.h>

using namespace vw;
using namespace vw::gfx;

static auto make_entity(uint32 idx) -> entity {
    return entity{idx, 0};
}

static auto make_aabb(float32 x, float32 y, float32 z, float32 size) -> aabb {
    return aabb{
        vec3f{x, y, z},
        vec3f{x + size, y + size, z + size}
    };
}

TEST_CASE("dynamic_aabb_tree insert and query", "[aabb_tree]") {
    dynamic_aabb_tree tree;

    SECTION("empty tree returns no results") {
        std::vector<entity> results;
        tree.query_all(make_aabb(0, 0, 0, 10), results);
        REQUIRE(results.empty());
    }

    SECTION("single entity insert and query") {
        tree.insert(make_entity(0), make_aabb(0, 0, 0, 1));
        REQUIRE(tree.size() == 1);

        std::vector<entity> results;
        tree.query_all(make_aabb(-1, -1, -1, 3), results);
        REQUIRE(results.size() == 1);
        REQUIRE(results[0] == make_entity(0));
    }

    SECTION("query miss returns empty") {
        tree.insert(make_entity(0), make_aabb(0, 0, 0, 1));

        std::vector<entity> results;
        tree.query_all(make_aabb(100, 100, 100, 1), results);
        REQUIRE(results.empty());
    }

    SECTION("two entities overlapping query") {
        tree.insert(make_entity(0), make_aabb(0, 0, 0, 2));
        tree.insert(make_entity(1), make_aabb(1, 1, 1, 2));

        std::vector<entity> results;
        tree.query_all(make_aabb(0.5f, 0.5f, 0.5f, 1), results);
        REQUIRE(results.size() == 2);
    }

    SECTION("three entities, query hits only two") {
        tree.insert(make_entity(0), make_aabb(0, 0, 0, 1));
        tree.insert(make_entity(1), make_aabb(2, 0, 0, 1));
        tree.insert(make_entity(2), make_aabb(100, 0, 0, 1));

        std::vector<entity> results;
        tree.query_all(make_aabb(-1, -1, -1, 5), results);
        REQUIRE(results.size() == 2);
    }
}

TEST_CASE("dynamic_aabb_tree remove", "[aabb_tree]") {
    dynamic_aabb_tree tree;

    SECTION("remove single entity") {
        tree.insert(make_entity(0), make_aabb(0, 0, 0, 1));
        tree.remove(make_entity(0));
        REQUIRE(tree.size() == 0);

        std::vector<entity> results;
        tree.query_all(make_aabb(-1, -1, -1, 3), results);
        REQUIRE(results.empty());
    }

    SECTION("remove one of two entities") {
        tree.insert(make_entity(0), make_aabb(0, 0, 0, 1));
        tree.insert(make_entity(1), make_aabb(5, 5, 5, 1));
        tree.remove(make_entity(0));
        REQUIRE(tree.size() == 1);

        std::vector<entity> results;
        tree.query_all(make_aabb(4, 4, 4, 3), results);
        REQUIRE(results.size() == 1);
        REQUIRE(results[0] == make_entity(1));
    }

    SECTION("remove middle entity from three") {
        tree.insert(make_entity(0), make_aabb(0, 0, 0, 1));
        tree.insert(make_entity(1), make_aabb(5, 0, 0, 1));
        tree.insert(make_entity(2), make_aabb(10, 0, 0, 1));
        tree.remove(make_entity(1));
        REQUIRE(tree.size() == 2);

        std::vector<entity> results;
        tree.query_all(make_aabb(-1, -1, -1, 100), results);
        REQUIRE(results.size() == 2);
    }
}

TEST_CASE("dynamic_aabb_tree update", "[aabb_tree]") {
    dynamic_aabb_tree tree;

    SECTION("update moves entity") {
        tree.insert(make_entity(0), make_aabb(0, 0, 0, 1));

        tree.update(make_entity(0), make_aabb(50, 50, 50, 1));

        std::vector<entity> results;
        tree.query_all(make_aabb(-1, -1, -1, 3), results);
        REQUIRE(results.empty());

        tree.query_all(make_aabb(49, 49, 49, 3), results);
        REQUIRE(results.size() == 1);
    }
}

TEST_CASE("dynamic_aabb_tree layer filtering", "[aabb_tree]") {
    dynamic_aabb_tree tree;

    SECTION("query filters by layer") {
        tree.insert(make_entity(0), make_aabb(0, 0, 0, 1), spatial_layer::terrain);
        tree.insert(make_entity(1), make_aabb(0, 0, 0, 1), spatial_layer::character);

        std::vector<entity> results;
        tree.query_all(make_aabb(-1, -1, -1, 3), results, spatial_layer::character);
        REQUIRE(results.size() == 1);
        REQUIRE(results[0] == make_entity(1));
    }

    SECTION("query with all layers returns everything") {
        tree.insert(make_entity(0), make_aabb(0, 0, 0, 1), spatial_layer::terrain);
        tree.insert(make_entity(1), make_aabb(0, 0, 0, 1), spatial_layer::character);

        std::vector<entity> results;
        tree.query_all(make_aabb(-1, -1, -1, 3), results, spatial_layer::all);
        REQUIRE(results.size() == 2);
    }

    SECTION("many terrain entities, query only characters") {
        for (uint32 i = 0; i < 50; ++i) {
            tree.insert(
                make_entity(i),
                make_aabb(static_cast<float32>(i) * 10.0f, 0, 0, 8),
                spatial_layer::terrain
            );
        }
        tree.insert(make_entity(100), make_aabb(25, 0, 0, 1), spatial_layer::character);

        std::vector<entity> results;
        tree.query_all(make_aabb(0, -1, -1, 500), results, spatial_layer::character);
        REQUIRE(results.size() == 1);
        REQUIRE(results[0] == make_entity(100));
    }
}

TEST_CASE("dynamic_aabb_tree sequential inserts stay balanced", "[aabb_tree]") {
    dynamic_aabb_tree tree;

    SECTION("linear sequential inserts do not hang") {
        for (uint32 i = 0; i < 200; ++i) {
            tree.insert(
                make_entity(i),
                make_aabb(static_cast<float32>(i) * 2.0f, 0, 0, 1)
            );
        }
        REQUIRE(tree.size() == 200);

        std::vector<entity> results;
        tree.query_all(make_aabb(50, -1, -1, 10), results);
        REQUIRE_FALSE(results.empty());
    }

    SECTION("insert and remove cycle") {
        for (uint32 i = 0; i < 100; ++i) {
            tree.insert(
                make_entity(i),
                make_aabb(static_cast<float32>(i), 0, 0, 1)
            );
        }
        for (uint32 i = 0; i < 50; ++i) {
            tree.remove(make_entity(i));
        }
        REQUIRE(tree.size() == 50);

        for (uint32 i = 200; i < 250; ++i) {
            tree.insert(
                make_entity(i),
                make_aabb(static_cast<float32>(i), 0, 0, 1)
            );
        }
        REQUIRE(tree.size() == 100);

        std::vector<entity> results;
        tree.query_all(make_aabb(-1, -1, -1, 500), results);
        REQUIRE(results.size() == 100);
    }
}

TEST_CASE("dynamic_aabb_tree mixed layers stress", "[aabb_tree]") {
    dynamic_aabb_tree tree;

    for (uint32 i = 0; i < 100; ++i) {
        tree.insert(
            make_entity(i),
            make_aabb(static_cast<float32>(i % 10) * 64.0f, 0, static_cast<float32>(i / 10) * 64.0f, 60),
            spatial_layer::terrain
        );
    }

    for (uint32 i = 100; i < 110; ++i) {
        tree.insert(
            make_entity(i),
            make_aabb(static_cast<float32>(i - 100) * 20.0f, 0, 0, 2),
            spatial_layer::character
        );
    }

    REQUIRE(tree.size() == 110);

    std::vector<entity> results;
    tree.query_all(make_aabb(0, -1, -1, 30), results, spatial_layer::character);
    REQUIRE_FALSE(results.empty());
    for (const auto& e : results) {
        REQUIRE(e.index >= 100);
    }
}