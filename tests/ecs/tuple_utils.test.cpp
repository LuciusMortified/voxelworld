#include <catch2/catch_test_macros.hpp>

#include "vw/ecs/tuple_utils.h"

using namespace vw::ecs;

namespace {

struct A {};
struct B {};
struct C {};
struct D {};

// tuple_cat_t
static_assert(std::is_same_v<
    tuple_cat_t<std::tuple<A, B>, std::tuple<C, D>>,
    std::tuple<A, B, C, D>>);

static_assert(std::is_same_v<
    tuple_cat_t<std::tuple<>, std::tuple<A>>,
    std::tuple<A>>);

static_assert(std::is_same_v<
    tuple_cat_t<std::tuple<A>, std::tuple<>, std::tuple<B>>,
    std::tuple<A, B>>);

static_assert(std::is_same_v<
    tuple_cat_t<std::tuple<A>>,
    std::tuple<A>>);

// tuple_unique_t
static_assert(std::is_same_v<
    tuple_unique_t<std::tuple<A, B, A, C, B>>,
    std::tuple<A, B, C>>);

static_assert(std::is_same_v<
    tuple_unique_t<std::tuple<A, B, C>>,
    std::tuple<A, B, C>>);

static_assert(std::is_same_v<
    tuple_unique_t<std::tuple<A, A, A>>,
    std::tuple<A>>);

static_assert(std::is_same_v<
    tuple_unique_t<std::tuple<>>,
    std::tuple<>>);

// tuple_contains_v
static_assert(tuple_contains_v<A, std::tuple<A, B, C>>);
static_assert(tuple_contains_v<C, std::tuple<A, B, C>>);
static_assert(!tuple_contains_v<D, std::tuple<A, B, C>>);
static_assert(!tuple_contains_v<A, std::tuple<>>);

// tuple_index_of_v
static_assert(tuple_index_of_v<A, std::tuple<A, B, C>> == 0);
static_assert(tuple_index_of_v<B, std::tuple<A, B, C>> == 1);
static_assert(tuple_index_of_v<C, std::tuple<A, B, C>> == 2);

}  // namespace

TEST_CASE("tuple_cat_t concatenates tuples", "[tuple_utils]") {
    using result = tuple_cat_t<std::tuple<A, B>, std::tuple<C>, std::tuple<D>>;
    STATIC_REQUIRE(std::is_same_v<result, std::tuple<A, B, C, D>>);
}

TEST_CASE("tuple_unique_t removes duplicates preserving order", "[tuple_utils]") {
    using result = tuple_unique_t<std::tuple<A, B, A, C, B, D, A>>;
    STATIC_REQUIRE(std::is_same_v<result, std::tuple<A, B, C, D>>);
}

TEST_CASE("tuple_contains_v checks type membership", "[tuple_utils]") {
    using tup = std::tuple<A, B, C>;
    STATIC_REQUIRE(tuple_contains_v<A, tup>);
    STATIC_REQUIRE(tuple_contains_v<B, tup>);
    STATIC_REQUIRE(tuple_contains_v<C, tup>);
    STATIC_REQUIRE(!tuple_contains_v<D, tup>);
}

TEST_CASE("tuple_index_of_v returns compile-time index", "[tuple_utils]") {
    using tup = std::tuple<A, B, C, D>;
    STATIC_REQUIRE(tuple_index_of_v<A, tup> == 0);
    STATIC_REQUIRE(tuple_index_of_v<B, tup> == 1);
    STATIC_REQUIRE(tuple_index_of_v<C, tup> == 2);
    STATIC_REQUIRE(tuple_index_of_v<D, tup> == 3);
}
