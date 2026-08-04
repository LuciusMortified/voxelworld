#include <catch2/catch_test_macros.hpp>


import vw.core;

using namespace vw;

TEST_CASE("vec3 constructors", "[vec3]") {
    SECTION("default") {
        vec3f v;
        REQUIRE(v.x == 0.0f);
        REQUIRE(v.y == 0.0f);
        REQUIRE(v.z == 0.0f);
    }

    SECTION("scalar") {
        vec3f v{5.0f};
        REQUIRE(v.x == 5.0f);
        REQUIRE(v.y == 5.0f);
        REQUIRE(v.z == 5.0f);
    }

    SECTION("component") {
        vec3f v{1.0f, 2.0f, 3.0f};
        REQUIRE(v.x == 1.0f);
        REQUIRE(v.y == 2.0f);
        REQUIRE(v.z == 3.0f);
    }

    SECTION("type conversion int->float") {
        vec3i vi{10, 20, 30};
        vec3f vf{vi};
        REQUIRE(vf.x == 10.0f);
        REQUIRE(vf.y == 20.0f);
        REQUIRE(vf.z == 30.0f);
    }

    SECTION("type conversion float->int truncation") {
        vec3f vf{1.7f, 2.3f, 3.9f};
        vec3i vi{vf};
        REQUIRE(vi.x == 1);
        REQUIRE(vi.y == 2);
        REQUIRE(vi.z == 3);
    }
}

TEST_CASE("vec3 arithmetic operators", "[vec3]") {
    vec3f a{1.0f, 2.0f, 3.0f};
    vec3f b{4.0f, 5.0f, 6.0f};

    SECTION("addition") {
        auto r = a + b;
        REQUIRE(r == vec3f{5.0f, 7.0f, 9.0f});
    }

    SECTION("subtraction") {
        auto r = b - a;
        REQUIRE(r == vec3f{3.0f, 3.0f, 3.0f});
    }

    SECTION("component-wise multiplication") {
        vec3f v1{10.0f, 20.0f, 30.0f};
        vec3f v2{2.0f, 3.0f, 4.0f};
        auto r = v1 * v2;
        REQUIRE(r == vec3f{20.0f, 60.0f, 120.0f});
    }

    SECTION("component-wise division") {
        vec3f v1{10.0f, 20.0f, 30.0f};
        vec3f v2{2.0f, 4.0f, 5.0f};
        auto r = v1 / v2;
        REQUIRE(r == vec3f{5.0f, 5.0f, 6.0f});
    }

    SECTION("scalar multiplication right") {
        auto r = a * 2.0f;
        REQUIRE(r == vec3f{2.0f, 4.0f, 6.0f});
    }

    SECTION("scalar multiplication left") {
        auto r = 2.0f * a;
        REQUIRE(r == vec3f{2.0f, 4.0f, 6.0f});
    }

    SECTION("scalar division") {
        auto r = vec3f{10.0f, 20.0f, 30.0f} / 2.0f;
        REQUIRE(r == vec3f{5.0f, 10.0f, 15.0f});
    }

    SECTION("unary plus") {
        REQUIRE(+a == a);
    }

    SECTION("unary minus") {
        auto r = -a;
        REQUIRE(r == vec3f{-1.0f, -2.0f, -3.0f});
    }
}

TEST_CASE("vec3 compound assignment", "[vec3]") {
    SECTION("-=") {
        vec3f v{1.0f, 2.0f, 3.0f};
        v -= vec3f{0.5f, 0.5f, 0.5f};
        REQUIRE(v == vec3f{0.5f, 1.5f, 2.5f});
    }

    SECTION("*= scalar") {
        vec3f v{0.5f, 1.5f, 2.5f};
        v *= 2.0f;
        REQUIRE(v == vec3f{1.0f, 3.0f, 5.0f});
    }

    SECTION("/= scalar") {
        vec3f v{1.0f, 3.0f, 5.0f};
        v /= 2.0f;
        REQUIRE(v == vec3f{0.5f, 1.5f, 2.5f});
    }

    SECTION("+= and *=/= vec") {
        vec3f v{10.0f, 20.0f, 30.0f};
        v += vec3f{1.0f, 2.0f, 3.0f};
        REQUIRE(v == vec3f{11.0f, 22.0f, 33.0f});
    }
}

TEST_CASE("vec3 comparison and indexing", "[vec3]") {
    REQUIRE(vec3f{1.0f, 2.0f, 3.0f} == vec3f{1.0f, 2.0f, 3.0f});
    REQUIRE(vec3f{1.0f, 2.0f, 3.0f} != vec3f{1.0f, 2.0f, 4.0f});

    vec3f v{1.0f, 2.0f, 3.0f};
    REQUIRE(v[0] == 1.0f);
    REQUIRE(v[1] == 2.0f);
    REQUIRE(v[2] == 3.0f);

    v[1] = 100.0f;
    REQUIRE(v[1] == 100.0f);
}

TEST_CASE("vec3 constexpr", "[vec3]") {
    constexpr vec3f a{1.0f, 2.0f, 3.0f};
    constexpr vec3f b{4.0f, 5.0f, 6.0f};

    constexpr auto sum = a + b;
    static_assert(sum.x == 5.0f && sum.y == 7.0f && sum.z == 9.0f);

    constexpr auto diff = b - a;
    static_assert(diff.x == 3.0f && diff.y == 3.0f && diff.z == 3.0f);

    constexpr auto scale = a * 2.0f;
    static_assert(scale.x == 2.0f && scale.y == 4.0f && scale.z == 6.0f);

    constexpr auto div = b / 2.0f;
    static_assert(div.x == 2.0f && div.y == 2.5f && div.z == 3.0f);
}
