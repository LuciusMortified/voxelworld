#include <catch2/catch_test_macros.hpp>

#include <vw/core/mat4.h>
#include <vw/core/math.h>

using namespace vw;

TEST_CASE("mat4 default constructor", "[mat4]") {
    mat4f m;
    for (int i = 0; i < 16; ++i) {
        REQUIRE(m[i] == 0.0f);
    }
}

TEST_CASE("mat4 from pointer", "[mat4]") {
    float data[16] = {1, 0, 0, 0, 0, 2, 0, 0, 0, 0, 3, 0, 0, 0, 0, 4};
    mat4f m{data};

    REQUIRE(m[0] == 1.0f);
    REQUIRE(m[5] == 2.0f);
    REQUIRE(m[10] == 3.0f);
    REQUIRE(m[15] == 4.0f);
}

TEST_CASE("mat4 indexing", "[mat4]") {
    auto m = math::identity_matrix();

    SECTION("2D indexing") {
        REQUIRE(m[0, 0] == 1.0f);
        REQUIRE(m[1, 1] == 1.0f);
        REQUIRE(m[2, 2] == 1.0f);
        REQUIRE(m[3, 3] == 1.0f);
        REQUIRE(m[0, 1] == 0.0f);
        REQUIRE(m[1, 0] == 0.0f);
    }

    SECTION("2D write") {
        m[0, 3] = 5.0f;
        REQUIRE(m[0, 3] == 5.0f);
    }

    SECTION("cptr") {
        auto* ptr = m.cptr();
        REQUIRE(ptr != nullptr);
    }
}

TEST_CASE("mat4 arithmetic", "[mat4]") {
    auto m1 = math::identity_matrix();
    auto m2 = math::identity_matrix();

    SECTION("addition") {
        auto r = m1 + m2;
        REQUIRE(r[0, 0] == 2.0f);
        REQUIRE(r[1, 1] == 2.0f);
        REQUIRE(r[0, 1] == 0.0f);
    }

    SECTION("subtraction") {
        auto r = m1 - m2;
        REQUIRE(r[0, 0] == 0.0f);
        REQUIRE(r[1, 1] == 0.0f);
    }

    SECTION("scalar multiplication") {
        auto r = m1 * 3.0f;
        REQUIRE(r[0, 0] == 3.0f);
        REQUIRE(r[1, 1] == 3.0f);
        REQUIRE(r[0, 1] == 0.0f);
    }

    SECTION("scalar multiplication left") {
        auto r = 2.0f * m1;
        REQUIRE(r[0, 0] == 2.0f);
        REQUIRE(r[1, 1] == 2.0f);
    }

    SECTION("scalar division") {
        auto m3 = m1 * 3.0f;
        auto r = m3 / 3.0f;
        REQUIRE(math::approx_equal(r[0, 0], 1.0f));
        REQUIRE(math::approx_equal(r[1, 1], 1.0f));
    }

    SECTION("unary minus") {
        auto r = -m1;
        REQUIRE(r[0, 0] == -1.0f);
        REQUIRE(r[1, 1] == -1.0f);
    }
}

TEST_CASE("mat4 compound assignment", "[mat4]") {
    auto m = math::identity_matrix();

    SECTION("+=") {
        m += math::identity_matrix();
        REQUIRE(m[0, 0] == 2.0f);
    }

    SECTION("-=") {
        m -= math::identity_matrix();
        REQUIRE(m[0, 0] == 0.0f);
    }

    SECTION("*= scalar") {
        m *= 2.0f;
        REQUIRE(m[0, 0] == 2.0f);
    }

    SECTION("/= scalar") {
        m *= 4.0f;
        m /= 2.0f;
        REQUIRE(m[0, 0] == 2.0f);
    }
}

TEST_CASE("mat4 matrix multiplication", "[mat4]") {
    auto id = math::identity_matrix();

    SECTION("identity * identity == identity") {
        auto r = id * id;
        REQUIRE(math::approx_equal(r, id));
    }

    SECTION("identity * M == M") {
        auto t = math::translation_matrix(vec3f{1.0f, 2.0f, 3.0f});
        auto r = id * t;
        REQUIRE(math::approx_equal(r, t));
    }
}

TEST_CASE("mat4 vector multiplication", "[mat4]") {
    SECTION("identity * vec4") {
        auto id = math::identity_matrix();
        vec4f v{1.0f, 2.0f, 3.0f, 1.0f};
        auto r = id * v;
        REQUIRE(r == v);
    }

    SECTION("identity * vec3") {
        auto id = math::identity_matrix();
        vec3f v{1.0f, 2.0f, 3.0f};
        auto r = id * v;
        REQUIRE(r == v);
    }

    SECTION("translation * vec3") {
        auto t = math::translation_matrix(vec3f{10.0f, 20.0f, 30.0f});
        vec3f v{1.0f, 2.0f, 3.0f};
        auto r = t * v;
        REQUIRE(math::approx_equal(r, vec3f{11.0f, 22.0f, 33.0f}));
    }
}

TEST_CASE("mat4 comparison", "[mat4]") {
    auto a = math::identity_matrix();
    auto b = math::identity_matrix();
    REQUIRE(a == b);

    b[0, 0] = 2.0f;
    REQUIRE(a != b);
}
