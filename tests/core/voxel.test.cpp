#include <catch2/catch_test_macros.hpp>


import vw.core;

using namespace vw;

TEST_CASE("voxel default constructor", "[voxel]") {
    voxel v;
    REQUIRE(v.is_empty());
    REQUIRE(v.id == blocks::air);
}

TEST_CASE("voxel id constructor", "[voxel]") {
    voxel v{block_id{42}};
    REQUIRE_FALSE(v.is_empty());
    REQUIRE(v.id == block_id{42});
}

TEST_CASE("voxel is_empty", "[voxel]") {
    REQUIRE(voxel{}.is_empty());
    REQUIRE(voxel{block_id{0}}.is_empty());
    REQUIRE_FALSE(voxel{block_id{1}}.is_empty());
    REQUIRE_FALSE(voxel{block_id{255}}.is_empty());
}

TEST_CASE("voxel empty_voxel constant", "[voxel]") {
    static_assert(empty_voxel.is_empty());
    REQUIRE(empty_voxel.is_empty());
    REQUIRE(empty_voxel.id == blocks::air);
}

TEST_CASE("voxel equality", "[voxel]") {
    REQUIRE(voxel{block_id{1}} == voxel{block_id{1}});
    REQUIRE_FALSE(voxel{block_id{1}} == voxel{block_id{2}});
}
