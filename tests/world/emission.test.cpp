#include <catch2/catch_test_macros.hpp>

import std;

import vw.core;
import vw.world;

using namespace vw;

// The table is what the flood asks "does this byte emit" without dragging the
// registry into itself. It has to agree with the registry on all 256 entries,
// because a block whose emission is written down in one place and read from the
// other would light the world differently from how it was authored.
TEST_CASE("the emission table mirrors the registry", "[emission]") {
    const block_registry registry;
    const asset::emission_table table = asset::build_emission_table(registry);

    REQUIRE(table[blocks::air.value] == 0);
    REQUIRE(table[blocks::green_5.value] == 0);
    REQUIRE(table[blocks::lamp.value] == 14);
    REQUIRE(table[blocks::lava.value] == 15);

    for (std::size_t i = 0; i < table.size(); ++i) {
        REQUIRE(table[i] == registry.get(block_id{static_cast<uint8>(i)}).light);
    }
}

// A default-built table lights nothing, and the flood leans on that: the
// convenience constructors hand one over so a sky-only column needs no registry
// at all.
TEST_CASE("a default table emits nothing", "[emission]") {
    const asset::emission_table table{};

    REQUIRE(std::ranges::all_of(table, [](uint8 level) -> bool { return level == 0; }));
}
