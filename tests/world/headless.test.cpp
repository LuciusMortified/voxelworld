#include <catch2/catch_test_macros.hpp>

import vw.core;
import vw.ecs;
import vw.world;

using namespace vw;
using namespace vw::ecs;

namespace {

auto make_cube(world& w, asset::model_registry& models, const char* name) -> entity {
    auto model = models.create(name, 4, 4, 4);
    model->fill(voxel{blocks::green_2});

    const auto ent = w.create().with<transform_component>().with<model_component>().get_entity();
    w.system<model_system>().modify(ent).set_model(std::move(model));
    w.modify(ent).with<spatial_component>();
    return ent;
}

}  // namespace

TEST_CASE("world runs without a graphics device", "[world]") {
    world w;
    auto& models = w.resource<asset::model_registry>();

    const auto ent = make_cube(w, models, "cube");
    w.update(0.016F);

    REQUIRE(w.has<spatial_component>(ent));
    REQUIRE(w.get<spatial_component>(ent).get_bounds().size().x > 0.0F);
}

TEST_CASE("moving a transform refreshes the spatial bounds", "[world]") {
    world w;
    auto& models = w.resource<asset::model_registry>();

    const auto ent = make_cube(w, models, "cube");
    w.update(0.016F);

    const auto before = w.get<spatial_component>(ent).get_bounds();

    w.system<transform_system>().modify(ent).set_position(vec3f{100.0F, 0.0F, 0.0F});
    w.update(0.016F);

    const auto after = w.get<spatial_component>(ent).get_bounds();
    REQUIRE(after.min.x > before.min.x + 50.0F);
}

TEST_CASE("a moved viewer is reported to the world grid", "[world]") {
    world w;

    const auto ent =
        w.create().with<transform_component>().with<world_view_component>().get_entity();
    w.update(0.016F);
    w.registry().clear_requested<world_view_component>();

    w.system<transform_system>().modify(ent).set_position(vec3f{0.0F, 0.0F, 64.0F});
    w.update(0.016F);

    REQUIRE(w.registry().requested<world_view_component>().contains(ent));
}

TEST_CASE("voxels survive a round trip through the model registry", "[world]") {
    asset::model_registry models;

    auto model = models.create("scratch", 16, 16, 16);
    model->set_voxel(1, 2, 3, voxel{blocks::brown_0});

    REQUIRE(models.get("scratch")->get_voxel(1, 2, 3).id == blocks::brown_0);
    REQUIRE(models.get("scratch")->is_empty(4, 5, 6));
}

TEST_CASE("chunk occupancy matches the voxel volume bit for bit", "[world][occupancy]") {
    asset::model_identity_pool identity_pool;
    asset::page_pool pages;

    constexpr int32 side = asset::chunk_occupancy::side;
    asset::model model{identity_pool, pages, side, side, side};

    asset::model_writer writer{model};

    // One uniform page, one sparse page and a lot of empty ones, so all three
    // page modes take part.
    for (int32 x = 0; x < 8; ++x) {
        for (int32 y = 0; y < 8; ++y) {
            for (int32 z = 0; z < 8; ++z) {
                writer.set(x, y, z, voxel{blocks::gray_3});
            }
        }
    }

    uint32 state = 4242;
    for (int32 i = 0; i < 4000; ++i) {
        state = (state * 1664525U) + 1013904223U;
        const int32 x = static_cast<int32>((state >> 8) % side);
        const int32 y = static_cast<int32>((state >> 14) % side);
        const int32 z = static_cast<int32>((state >> 20) % side);
        writer.set(x, y, z, voxel{blocks::red_2});
    }

    asset::chunk_occupancy occupancy;
    REQUIRE(model.build_occupancy(occupancy));

    std::size_t mismatches = 0;
    for (int32 x = 0; x < side; ++x) {
        for (int32 y = 0; y < side; ++y) {
            for (int32 z = 0; z < side; ++z) {
                if (occupancy.test(x, y, z) == model.is_empty(x, y, z)) {
                    ++mismatches;
                }
            }
        }
    }
    REQUIRE(mismatches == 0);
}

TEST_CASE("chunk occupancy declines models that are not 64 cubes", "[world][occupancy]") {
    asset::model_identity_pool identity_pool;
    asset::page_pool pages;
    asset::model model{identity_pool, pages, 32, 32, 32};

    asset::chunk_occupancy occupancy;
    REQUIRE_FALSE(model.build_occupancy(occupancy));
}
