#include <catch2/catch_test_macros.hpp>

#include <vw/core/voxel.h>

using namespace vw;

TEST_CASE("voxel default constructor", "[voxel]") {
    voxel v;
    REQUIRE(v.is_empty());
    REQUIRE(v.id == 0);
}

TEST_CASE("voxel id constructor", "[voxel]") {
    voxel v{uint8{42}};
    REQUIRE_FALSE(v.is_empty());
    REQUIRE(v.id == 42);
}

TEST_CASE("voxel is_empty", "[voxel]") {
    REQUIRE(voxel{}.is_empty());
    REQUIRE(voxel{uint8{0}}.is_empty());
    REQUIRE_FALSE(voxel{uint8{1}}.is_empty());
    REQUIRE_FALSE(voxel{uint8{255}}.is_empty());
}

TEST_CASE("voxel empty_voxel constant", "[voxel]") {
    static_assert(empty_voxel.is_empty());
    REQUIRE(empty_voxel.is_empty());
    REQUIRE(empty_voxel.id == 0);
}

TEST_CASE("voxel equality", "[voxel]") {
    REQUIRE(voxel{uint8{1}} == voxel{uint8{1}});
    REQUIRE_FALSE(voxel{uint8{1}} == voxel{uint8{2}});
}
