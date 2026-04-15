#include <catch2/catch_test_macros.hpp>

#include "vw/gfx/world/world_def.h"

using namespace vw::gfx;

namespace {

struct comp_a {};
struct comp_b {};
struct comp_c {};
struct resource_x {};

template <typename WC>
class system_alpha {
public:
    void update(float) {}
};

template <typename WC>
class system_beta {
public:
    void update(float) {}
};

template <typename WC>
class system_gamma {
public:
    void update(float) {}
};

}  // namespace

template <>
struct vw::gfx::system_trait<system_alpha> {
    using components = std::tuple<comp_a>;
    using depends_on = system_list<>;
    using resources  = std::tuple<resource_x>;
};

template <>
struct vw::gfx::system_trait<system_beta> {
    using components = std::tuple<comp_b, comp_a>;
    using depends_on = system_list<system_alpha>;
    using resources  = std::tuple<>;
};

template <>
struct vw::gfx::system_trait<system_gamma> {
    using components = std::tuple<comp_c>;
    using depends_on = system_list<system_alpha, system_beta>;
    using resources  = std::tuple<resource_x>;
};

using test_def = world_def<system_alpha, system_beta, system_gamma>;

static_assert(std::is_same_v<test_def::components, std::tuple<comp_a, comp_b, comp_c>>);

static_assert(std::is_same_v<test_def::resources, std::tuple<resource_x>>);

static_assert(test_def::system_count == 3);

static_assert(test_def::system_index<system_alpha> == 0);
static_assert(test_def::system_index<system_beta> == 1);
static_assert(test_def::system_index<system_gamma> == 2);

static_assert(std::is_same_v<
    test_def::systems_tuple,
    std::tuple<
        system_alpha<test_def>,
        system_beta<test_def>,
        system_gamma<test_def>>>);

TEST_CASE("world_def collects components from system traits", "[world_def]") {
    STATIC_REQUIRE(std::is_same_v<test_def::components, std::tuple<comp_a, comp_b, comp_c>>);
}

TEST_CASE("world_def deduplicates components", "[world_def]") {
    STATIC_REQUIRE(tuple_contains_v<comp_a, test_def::components>);
    STATIC_REQUIRE(std::tuple_size_v<test_def::components> == 3);
}

TEST_CASE("world_def collects and deduplicates resources", "[world_def]") {
    STATIC_REQUIRE(std::is_same_v<test_def::resources, std::tuple<resource_x>>);
}

TEST_CASE("world_def system_index gives correct positions", "[world_def]") {
    STATIC_REQUIRE(test_def::system_index<system_alpha> == 0);
    STATIC_REQUIRE(test_def::system_index<system_beta> == 1);
    STATIC_REQUIRE(test_def::system_index<system_gamma> == 2);
}

TEST_CASE("world_def validates dependency ordering", "[world_def]") {
    STATIC_REQUIRE(test_def::deps_valid);
}

