#include <catch2/catch_test_macros.hpp>

import std;
import vw.core;

using namespace vw;

TEST_CASE("blocks default to unlit and unglowing", "[blocks]") {
    const block_registry registry;

    REQUIRE(registry.get(blocks::air).light == 0);
    REQUIRE(registry.get(blocks::air).glow == 0);
    REQUIRE(registry.get(blocks::green_5).light == 0);
    REQUIRE(registry.get(blocks::green_5).glow == 0);
}

// Two properties, not one. What a block gives its neighbours is a level the
// flood carries; what it draws itself with is a brightness the shader adds
// outside every occluder. Lava has both, and nothing in the engine makes one
// follow from the other -- a crystal that glows without lighting the room is a
// glow with no light, and a lamp buried in a wall is a light with no glow.
TEST_CASE("an emitter carries a flood level and a glow apart", "[blocks]") {
    const block_registry registry;

    const block_type& lamp = registry.get(blocks::lamp);
    REQUIRE(lamp.light == 14);
    REQUIRE(lamp.glow == 200);

    const block_type& lava = registry.get(blocks::lava);
    REQUIRE(lava.light == 15);
    REQUIRE(lava.glow == 255);
}

// A level over fifteen cannot be baked: the quad keeps four bits a corner and
// the flood steps down by one, so the two numbers have to agree on a ceiling.
TEST_CASE("no block emits past the nibble", "[blocks]") {
    const block_registry registry;

    for (const block_type& block : registry.blocks()) {
        REQUIRE(block.light <= 15);
    }
}

// find_by_color returns the first block wearing a colour, and .vox import maps
// its palette through it. Two blocks sharing one colour would make that pick
// arbitrary -- which is exactly the trap a new emitting block walks into if it
// borrows an Apollo entry.
TEST_CASE("registered colours are unique", "[blocks]") {
    const block_registry registry;

    std::vector<color> seen;
    for (const block_type& block : registry.blocks()) {
        if (block.id == blocks::air) {
            continue;
        }

        REQUIRE(registry.find_by_color(block.clr) == block.id);
        REQUIRE(std::ranges::find(seen, block.clr) == seen.end());
        seen.push_back(block.clr);
    }
}
