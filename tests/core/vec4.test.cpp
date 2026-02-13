#include <catch2/catch_test_macros.hpp>

#include <vw/core/vec3.h>
#include <vw/core/vec4.h>

using namespace vw;

TEST_CASE("vec4 constructors", "[vec4]") {
    SECTION("default") {
        vec4f v;
        REQUIRE(v.x == 0.0f);
        REQUIRE(v.y == 0.0f);
        REQUIRE(v.z == 0.0f);
        REQUIRE(v.w == 0.0f);
    }

    SECTION("scalar") {
        vec4f v{5.0f};
        REQUIRE(v.x == 5.0f);
        REQUIRE(v.y == 5.0f);
        REQUIRE(v.z == 5.0f);
        REQUIRE(v.w == 5.0f);
    }

    SECTION("component") {
        vec4f v{1.0f, 2.0f, 3.0f, 4.0f};
        REQUIRE(v.x == 1.0f);
        REQUIRE(v.y == 2.0f);
        REQUIRE(v.z == 3.0f);
        REQUIRE(v.w == 4.0f);
    }

    SECTION("type conversion int->float") {
        vec4i vi{1, 2, 3, 4};
        vec4f vf{vi};
        REQUIRE(vf.x == 1.0f);
        REQUIRE(vf.y == 2.0f);
        REQUIRE(vf.z == 3.0f);
        REQUIRE(vf.w == 4.0f);
    }

    SECTION("from vec3 + w") {
        vec3f v3{1.0f, 2.0f, 3.0f};
        vec4f v4{v3, 4.0f};
        REQUIRE(v4.x == 1.0f);
        REQUIRE(v4.y == 2.0f);
        REQUIRE(v4.z == 3.0f);
        REQUIRE(v4.w == 4.0f);
    }
}

TEST_CASE("vec4 arithmetic operators", "[vec4]") {
    vec4f a{1.0f, 2.0f, 3.0f, 4.0f};
    vec4f b{5.0f, 6.0f, 7.0f, 8.0f};

    SECTION("addition") {
        auto r = a + b;
        REQUIRE(r == vec4f{6.0f, 8.0f, 10.0f, 12.0f});
    }

    SECTION("subtraction") {
        auto r = b - a;
        REQUIRE(r == vec4f{4.0f, 4.0f, 4.0f, 4.0f});
    }

    SECTION("scalar multiplication") {
        auto r = a * 2.0f;
        REQUIRE(r == vec4f{2.0f, 4.0f, 6.0f, 8.0f});
    }

    SECTION("scalar multiplication left") {
        auto r = 2.0f * a;
        REQUIRE(r == vec4f{2.0f, 4.0f, 6.0f, 8.0f});
    }

    SECTION("scalar division") {
        auto r = vec4f{10.0f, 20.0f, 30.0f, 40.0f} / 2.0f;
        REQUIRE(r == vec4f{5.0f, 10.0f, 15.0f, 20.0f});
    }

    SECTION("unary minus") {
        auto r = -a;
        REQUIRE(r == vec4f{-1.0f, -2.0f, -3.0f, -4.0f});
    }
}

TEST_CASE("vec4 compound assignment", "[vec4]") {
    vec4f v{10.0f, 20.0f, 30.0f, 40.0f};

    SECTION("+=") {
        v += vec4f{1.0f, 2.0f, 3.0f, 4.0f};
        REQUIRE(v == vec4f{11.0f, 22.0f, 33.0f, 44.0f});
    }

    SECTION("*= scalar") {
        v *= 2.0f;
        REQUIRE(v == vec4f{20.0f, 40.0f, 60.0f, 80.0f});
    }

    SECTION("/= scalar") {
        v /= 2.0f;
        REQUIRE(v == vec4f{5.0f, 10.0f, 15.0f, 20.0f});
    }
}

TEST_CASE("vec4 comparison and indexing", "[vec4]") {
    REQUIRE(vec4f{1.0f, 2.0f, 3.0f, 4.0f} == vec4f{1.0f, 2.0f, 3.0f, 4.0f});
    REQUIRE(vec4f{1.0f, 2.0f, 3.0f, 4.0f} != vec4f{1.0f, 2.0f, 3.0f, 5.0f});

    vec4f v{1.0f, 2.0f, 3.0f, 4.0f};
    REQUIRE(v[0] == 1.0f);
    REQUIRE(v[1] == 2.0f);
    REQUIRE(v[2] == 3.0f);
    REQUIRE(v[3] == 4.0f);

    v[2] = 100.0f;
    REQUIRE(v[2] == 100.0f);
}
