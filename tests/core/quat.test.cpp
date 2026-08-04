#include <catch2/catch_test_macros.hpp>


import vw.core;

using namespace vw;

TEST_CASE("quat default constructor", "[quat]") {
    quat q;
    REQUIRE(q.x == 0.0f);
    REQUIRE(q.y == 0.0f);
    REQUIRE(q.z == 0.0f);
    REQUIRE(q.w == 1.0f);
}

TEST_CASE("quat component constructor", "[quat]") {
    quat q{0.1f, 0.2f, 0.3f, 0.4f};
    REQUIRE(q.x == 0.1f);
    REQUIRE(q.y == 0.2f);
    REQUIRE(q.z == 0.3f);
    REQUIRE(q.w == 0.4f);
}

TEST_CASE("quat arithmetic operators", "[quat]") {
    quat a{1.0f, 2.0f, 3.0f, 4.0f};
    quat b{5.0f, 6.0f, 7.0f, 8.0f};

    SECTION("addition") {
        auto r = a + b;
        REQUIRE(r.x == 6.0f);
        REQUIRE(r.y == 8.0f);
        REQUIRE(r.z == 10.0f);
        REQUIRE(r.w == 12.0f);
    }

    SECTION("subtraction") {
        auto r = b - a;
        REQUIRE(r.x == 4.0f);
        REQUIRE(r.y == 4.0f);
        REQUIRE(r.z == 4.0f);
        REQUIRE(r.w == 4.0f);
    }

    SECTION("scalar multiplication") {
        auto r = a * 2.0f;
        REQUIRE(r.x == 2.0f);
        REQUIRE(r.y == 4.0f);
        REQUIRE(r.z == 6.0f);
        REQUIRE(r.w == 8.0f);
    }

    SECTION("scalar multiplication left") {
        auto r = 2.0f * a;
        REQUIRE(r.x == 2.0f);
        REQUIRE(r.y == 4.0f);
    }

    SECTION("scalar division") {
        auto r = a / 2.0f;
        REQUIRE(r.x == 0.5f);
        REQUIRE(r.y == 1.0f);
        REQUIRE(r.z == 1.5f);
        REQUIRE(r.w == 2.0f);
    }

    SECTION("unary minus") {
        auto r = -a;
        REQUIRE(r.x == -1.0f);
        REQUIRE(r.y == -2.0f);
        REQUIRE(r.z == -3.0f);
        REQUIRE(r.w == -4.0f);
    }

    SECTION("unary plus") {
        auto r = +a;
        REQUIRE(r == a);
    }
}

TEST_CASE("quat Hamilton product", "[quat]") {
    quat identity;
    quat q{0.1f, 0.2f, 0.3f, 0.9f};

    SECTION("identity multiplication") {
        auto r1 = q * identity;
        REQUIRE(r1.x == q.x);
        REQUIRE(r1.y == q.y);
        REQUIRE(r1.z == q.z);
        REQUIRE(r1.w == q.w);

        auto r2 = identity * q;
        REQUIRE(r2.x == q.x);
        REQUIRE(r2.y == q.y);
        REQUIRE(r2.z == q.z);
        REQUIRE(r2.w == q.w);
    }
}

TEST_CASE("quat compound assignment", "[quat]") {
    quat q{1.0f, 2.0f, 3.0f, 4.0f};

    SECTION("+=") {
        q += quat{0.5f, 0.5f, 0.5f, 0.5f};
        REQUIRE(q.x == 1.5f);
        REQUIRE(q.w == 4.5f);
    }

    SECTION("*= scalar") {
        q *= 2.0f;
        REQUIRE(q.x == 2.0f);
        REQUIRE(q.w == 8.0f);
    }

    SECTION("/= scalar") {
        q /= 2.0f;
        REQUIRE(q.x == 0.5f);
        REQUIRE(q.w == 2.0f);
    }
}

TEST_CASE("quat comparison", "[quat]") {
    REQUIRE(quat{1.0f, 2.0f, 3.0f, 4.0f} == quat{1.0f, 2.0f, 3.0f, 4.0f});
    REQUIRE(quat{1.0f, 2.0f, 3.0f, 4.0f} != quat{1.0f, 2.0f, 3.0f, 5.0f});
}

TEST_CASE("quat index access", "[quat]") {
    quat q{0.1f, 0.2f, 0.3f, 0.9f};
    REQUIRE(q[0] == 0.1f);
    REQUIRE(q[1] == 0.2f);
    REQUIRE(q[2] == 0.3f);
    REQUIRE(q[3] == 0.9f);

    q[3] = 1.0f;
    REQUIRE(q[3] == 1.0f);
}
