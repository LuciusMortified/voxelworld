#include <catch2/catch_test_macros.hpp>

import vw.core.spike;

TEST_CASE("spike: length of exported vec3f", "[spike]") {
    REQUIRE(vw::length(vw::vec3f{3.0F, 4.0F, 0.0F}) == 5.0F);
}

TEST_CASE("spike: describe formats via implementation unit", "[spike]") {
    REQUIRE(vw::describe(vw::vec3f{1.0F, 2.0F, 3.0F}) == "(1, 2, 3)");
}
