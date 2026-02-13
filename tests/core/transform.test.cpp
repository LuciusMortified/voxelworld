#include <catch2/catch_test_macros.hpp>

#include <vw/core/math.h>
#include <vw/core/transform.h>

using namespace vw;

TEST_CASE("transform default state", "[transform]") {
    transform t;
    REQUIRE(t.get_position() == vec3f{0.0f, 0.0f, 0.0f});
    REQUIRE(t.get_rotation() == vec3f{0.0f, 0.0f, 0.0f});
    REQUIRE(t.get_scale() == vec3f{1.0f, 1.0f, 1.0f});
    REQUIRE(t.get_origin() == vec3f{0.0f, 0.0f, 0.0f});
}

TEST_CASE("transform setters and getters", "[transform]") {
    transform t;

    t.set_position(vec3f{1.0f, 2.0f, 3.0f});
    REQUIRE(t.get_position() == vec3f{1.0f, 2.0f, 3.0f});

    t.set_rotation(vec3f{0.1f, 0.2f, 0.3f});
    REQUIRE(t.get_rotation() == vec3f{0.1f, 0.2f, 0.3f});

    t.set_scale(vec3f{2.0f, 3.0f, 4.0f});
    REQUIRE(t.get_scale() == vec3f{2.0f, 3.0f, 4.0f});

    t.set_origin(vec3f{5.0f, 5.0f, 5.0f});
    REQUIRE(t.get_origin() == vec3f{5.0f, 5.0f, 5.0f});
}

TEST_CASE("transform translate", "[transform]") {
    transform t;
    t.translate(vec3f{1.0f, 2.0f, 3.0f});
    REQUIRE(t.get_position() == vec3f{1.0f, 2.0f, 3.0f});

    t.translate(vec3f{-0.5f, 0.5f, 0.0f});
    REQUIRE(t.get_position() == vec3f{0.5f, 2.5f, 3.0f});
}

TEST_CASE("transform rotate", "[transform]") {
    transform t;
    t.rotate(vec3f{0.1f, 0.2f, 0.3f});
    REQUIRE(t.get_rotation() == vec3f{0.1f, 0.2f, 0.3f});

    t.rotate(vec3f{0.1f, 0.1f, 0.1f});
    REQUIRE(math::approx_equal(t.get_rotation(), vec3f{0.2f, 0.3f, 0.4f}));
}

TEST_CASE("transform scale", "[transform]") {
    transform t;
    REQUIRE(t.get_scale() == vec3f{1.0f, 1.0f, 1.0f});

    t.scale(vec3f{2.0f, 3.0f, 4.0f});
    REQUIRE(t.get_scale() == vec3f{2.0f, 3.0f, 4.0f});

    t.scale(vec3f{0.5f, 0.5f, 0.5f});
    REQUIRE(t.get_scale() == vec3f{1.0f, 1.5f, 2.0f});
}

TEST_CASE("transform calc_matrix", "[transform]") {
    SECTION("default transform produces identity") {
        transform t;
        auto m = t.calc_matrix();
        REQUIRE(math::approx_equal(m, math::identity_matrix()));
    }

    SECTION("translation only") {
        transform t;
        t.set_position(vec3f{1.0f, 2.0f, 3.0f});
        auto m = t.calc_matrix();
        auto expected = math::translation_matrix(vec3f{1.0f, 2.0f, 3.0f});
        REQUIRE(math::approx_equal(m, expected));
    }

    SECTION("scale only") {
        transform t;
        t.set_scale(vec3f{2.0f, 3.0f, 4.0f});
        auto m = t.calc_matrix();
        auto expected = math::scale_matrix(vec3f{2.0f, 3.0f, 4.0f});
        REQUIRE(math::approx_equal(m, expected));
    }

    SECTION("matches transform_matrix") {
        transform t;
        t.set_position(vec3f{1.0f, 2.0f, 3.0f});
        t.set_rotation(vec3f{0.1f, 0.2f, 0.3f});
        t.set_scale(vec3f{1.5f, 1.5f, 1.5f});
        t.set_origin(vec3f{0.5f, 0.5f, 0.5f});

        auto m = t.calc_matrix();
        auto expected = math::transform_matrix(
            vec3f{1.0f, 2.0f, 3.0f},
            vec3f{0.1f, 0.2f, 0.3f},
            vec3f{1.5f, 1.5f, 1.5f},
            vec3f{0.5f, 0.5f, 0.5f}
        );
        REQUIRE(math::approx_equal(m, expected));
    }
}
