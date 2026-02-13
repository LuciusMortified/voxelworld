#include <catch2/catch_test_macros.hpp>

#include <vw/core/voxel.h>

using namespace vw;

TEST_CASE("voxel default constructor", "[voxel]") {
    voxel v;
    REQUIRE(v.is_empty());
    REQUIRE(v.value == colors::empty);
}

TEST_CASE("voxel color constructor", "[voxel]") {
    voxel v{colors::red};
    REQUIRE_FALSE(v.is_empty());
    REQUIRE(v.value.r() == 255);
    REQUIRE(v.value.g() == 0);
    REQUIRE(v.value.b() == 0);
    REQUIRE(v.value.a() == 255);
}

TEST_CASE("voxel is_empty", "[voxel]") {
    REQUIRE(voxel{}.is_empty());
    REQUIRE(voxel{colors::empty}.is_empty());
    REQUIRE_FALSE(voxel{colors::black}.is_empty());
    REQUIRE_FALSE(voxel{colors::white}.is_empty());
}

TEST_CASE("voxel empty_voxel constant", "[voxel]") {
    static_assert(empty_voxel.is_empty());
    REQUIRE(empty_voxel.is_empty());
    REQUIRE(empty_voxel.value == colors::empty);
}
