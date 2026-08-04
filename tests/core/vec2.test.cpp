#include <catch2/catch_test_macros.hpp>


import vw.core;

using namespace vw;

TEST_CASE("vec2 constructors", "[vec2]") {
    SECTION("default") {
        vec2f v;
        REQUIRE(v.x == 0.0f);
        REQUIRE(v.y == 0.0f);
    }

    SECTION("scalar") {
        vec2f v{5.0f};
        REQUIRE(v.x == 5.0f);
        REQUIRE(v.y == 5.0f);
    }

    SECTION("component") {
        vec2f v{1.0f, 2.0f};
        REQUIRE(v.x == 1.0f);
        REQUIRE(v.y == 2.0f);
    }

    SECTION("type conversion int->float") {
        vec2i vi{10, 20};
        vec2f vf{vi};
        REQUIRE(vf.x == 10.0f);
        REQUIRE(vf.y == 20.0f);
    }

    SECTION("type conversion float->int truncation") {
        vec2f vf{1.7f, 2.3f};
        vec2i vi{vf};
        REQUIRE(vi.x == 1);
        REQUIRE(vi.y == 2);
    }
}

TEST_CASE("vec2 arithmetic operators", "[vec2]") {
    vec2f a{1.0f, 2.0f};
    vec2f b{3.0f, 4.0f};

    SECTION("addition") {
        auto r = a + b;
        REQUIRE(r.x == 4.0f);
        REQUIRE(r.y == 6.0f);
    }

    SECTION("subtraction") {
        auto r = b - a;
        REQUIRE(r.x == 2.0f);
        REQUIRE(r.y == 2.0f);
    }

    SECTION("component-wise multiplication") {
        auto r = a * b;
        REQUIRE(r.x == 3.0f);
        REQUIRE(r.y == 8.0f);
    }

    SECTION("component-wise division") {
        auto r = vec2f{10.0f, 20.0f} / vec2f{2.0f, 5.0f};
        REQUIRE(r.x == 5.0f);
        REQUIRE(r.y == 4.0f);
    }

    SECTION("scalar multiplication right") {
        auto r = a * 3.0f;
        REQUIRE(r.x == 3.0f);
        REQUIRE(r.y == 6.0f);
    }

    SECTION("scalar multiplication left") {
        auto r = 3.0f * a;
        REQUIRE(r.x == 3.0f);
        REQUIRE(r.y == 6.0f);
    }

    SECTION("scalar division") {
        auto r = vec2f{10.0f, 20.0f} / 2.0f;
        REQUIRE(r.x == 5.0f);
        REQUIRE(r.y == 10.0f);
    }

    SECTION("unary plus") {
        auto r = +a;
        REQUIRE(r.x == 1.0f);
        REQUIRE(r.y == 2.0f);
    }

    SECTION("unary minus") {
        auto r = -a;
        REQUIRE(r.x == -1.0f);
        REQUIRE(r.y == -2.0f);
    }
}

TEST_CASE("vec2 compound assignment", "[vec2]") {
    vec2f v{10.0f, 20.0f};

    SECTION("+=") {
        v += vec2f{1.0f, 2.0f};
        REQUIRE(v.x == 11.0f);
        REQUIRE(v.y == 22.0f);
    }

    SECTION("-=") {
        v -= vec2f{1.0f, 2.0f};
        REQUIRE(v.x == 9.0f);
        REQUIRE(v.y == 18.0f);
    }

    SECTION("*= vec") {
        v *= vec2f{2.0f, 3.0f};
        REQUIRE(v.x == 20.0f);
        REQUIRE(v.y == 60.0f);
    }

    SECTION("/= vec") {
        v /= vec2f{2.0f, 5.0f};
        REQUIRE(v.x == 5.0f);
        REQUIRE(v.y == 4.0f);
    }

    SECTION("*= scalar") {
        v *= 2.0f;
        REQUIRE(v.x == 20.0f);
        REQUIRE(v.y == 40.0f);
    }

    SECTION("/= scalar") {
        v /= 2.0f;
        REQUIRE(v.x == 5.0f);
        REQUIRE(v.y == 10.0f);
    }
}

TEST_CASE("vec2 comparison", "[vec2]") {
    REQUIRE(vec2f{1.0f, 2.0f} == vec2f{1.0f, 2.0f});
    REQUIRE(vec2f{1.0f, 2.0f} != vec2f{1.0f, 3.0f});
    REQUIRE(vec2i{5, 10} == vec2i{5, 10});
}

TEST_CASE("vec2 index access", "[vec2]") {
    vec2f v{1.0f, 2.0f};
    REQUIRE(v[0] == 1.0f);
    REQUIRE(v[1] == 2.0f);

    v[1] = 100.0f;
    REQUIRE(v[1] == 100.0f);
}

TEST_CASE("vec2 constexpr", "[vec2]") {
    constexpr vec2f a{1.0f, 2.0f};
    constexpr vec2f b{3.0f, 4.0f};
    constexpr auto sum = a + b;
    static_assert(sum.x == 4.0f && sum.y == 6.0f);

    constexpr auto diff = b - a;
    static_assert(diff.x == 2.0f && diff.y == 2.0f);

    constexpr auto scaled = a * 3.0f;
    static_assert(scaled.x == 3.0f && scaled.y == 6.0f);

    constexpr auto div = vec2f{10.0f, 20.0f} / 2.0f;
    static_assert(div.x == 5.0f && div.y == 10.0f);
}
