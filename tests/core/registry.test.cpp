#include <catch2/catch_test_macros.hpp>

#include <vw/gfx/world/registry.h>

using namespace vw;
using namespace vw::gfx;

struct position_component {
    float x = 0.f, y = 0.f, z = 0.f;
};

struct velocity_component {
    float dx = 0.f, dy = 0.f, dz = 0.f;
};

struct health_component {
    int hp = 100;
};

using test_registry = registry<position_component, velocity_component, health_component>;

TEST_CASE("registry create and destroy", "[registry]") {
    test_registry reg;

    auto e0 = reg.create();
    auto e1 = reg.create();
    REQUIRE(e0.index == 0);
    REQUIRE(e1.index == 1);

    reg.destroy(e0);
    auto e2 = reg.create();
    REQUIRE(e2.index == 0);
    REQUIRE(e2.generation == 1);
}

TEST_CASE("registry add and get components", "[registry]") {
    test_registry reg;
    auto e = reg.create();

    reg.add<position_component>(e, {1.f, 2.f, 3.f});
    REQUIRE(reg.has<position_component>(e));
    REQUIRE(reg.get<position_component>(e).x == 1.f);

    REQUIRE_FALSE(reg.has<velocity_component>(e));
}

TEST_CASE("registry remove components", "[registry]") {
    test_registry reg;
    auto e = reg.create();

    reg.add<position_component>(e);
    reg.remove<position_component>(e);
    REQUIRE_FALSE(reg.has<position_component>(e));
}

TEST_CASE("registry remove_all", "[registry]") {
    test_registry reg;
    auto e = reg.create();

    reg.add<position_component>(e);
    reg.add<velocity_component>(e);
    reg.add<health_component>(e);

    reg.remove_all(e);

    REQUIRE_FALSE(reg.has<position_component>(e));
    REQUIRE_FALSE(reg.has<velocity_component>(e));
    REQUIRE_FALSE(reg.has<health_component>(e));
}

TEST_CASE("registry batch_create", "[registry]") {
    test_registry reg;

    auto entities = reg.batch_create(10);
    REQUIRE(entities.size() == 10);
    for (uint32 i = 0; i < entities.size(); ++i) {
        REQUIRE(entities[i].index == i);
    }
}

TEST_CASE("registry batch_destroy", "[registry]") {
    test_registry reg;

    auto entities = reg.batch_create(5);
    reg.batch_destroy(entities);

    auto reused = reg.batch_create(5);
    for (auto& e : reused) {
        REQUIRE(e.generation == 1);
    }
}

TEST_CASE("registry batch_add", "[registry]") {
    test_registry reg;

    auto entities = reg.batch_create(5);
    reg.batch_add<position_component>(entities, {1.f, 2.f, 3.f});

    for (auto& e : entities) {
        REQUIRE(reg.has<position_component>(e));
        REQUIRE(reg.get<position_component>(e).x == 1.f);
    }
}

TEST_CASE("registry batch_remove", "[registry]") {
    test_registry reg;

    auto entities = reg.batch_create(5);
    reg.batch_add<position_component>(entities);
    reg.batch_remove<position_component>(entities);

    for (auto& e : entities) {
        REQUIRE_FALSE(reg.has<position_component>(e));
    }
}

TEST_CASE("registry component_view", "[registry]") {
    test_registry reg;

    auto e0 = reg.create();
    auto e1 = reg.create();
    auto e2 = reg.create();

    reg.add<position_component>(e0, {1.f, 0.f, 0.f});
    reg.add<velocity_component>(e0, {0.1f, 0.f, 0.f});

    reg.add<position_component>(e1, {2.f, 0.f, 0.f});

    reg.add<position_component>(e2, {3.f, 0.f, 0.f});
    reg.add<velocity_component>(e2, {0.3f, 0.f, 0.f});

    auto view = reg.view<position_component, velocity_component>();
    int count = 0;
    for (const auto& [ent, pos, vel] : view) {
        REQUIRE(pos.x > 0.f);
        REQUIRE(vel.dx > 0.f);
        ++count;
    }
    REQUIRE(count == 2);
}
