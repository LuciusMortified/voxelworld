#pragma once

#ifndef VW_CORE_BLOCK_REGISTRY_H
#define VW_CORE_BLOCK_REGISTRY_H

#include <array>
#include <span>
#include <vector>

#include "vw/core/color.h"
#include "vw/core/types.h"

namespace vw {

enum class block_category : uint8 {
    terrain,
    character,
    metal,
};

namespace block_flags {
constexpr uint8 none        = 0;
constexpr uint8 transparent = 1 << 0;
constexpr uint8 emissive    = 1 << 1;
constexpr uint8 liquid      = 1 << 2;
}  // namespace block_flags

struct block_type {
    color col;
    block_category category = block_category::terrain;
    uint8 flags = block_flags::none;
};

// clang-format off
enum class block_id : uint8 {
    air = 0,

    grass_1  = 1,  grass_2  = 2,  grass_3  = 3,
    dirt_1   = 4,  dirt_2   = 5,  dirt_3   = 6,
    stone_1  = 7,  stone_2  = 8,  stone_3  = 9,
    sand_1   = 10, sand_2   = 11, sand_3   = 12,
    snow_1   = 13, snow_2   = 14, snow_3   = 15,
    water_1  = 16, water_2  = 17, water_3  = 18,
    lava_1   = 19, lava_2   = 20, lava_3   = 21,
    log_1    = 22, log_2    = 23, log_3    = 24,
    leaves_1 = 25, leaves_2 = 26, leaves_3 = 27,
    bark_1   = 28, bark_2   = 29,
    moss_1   = 30, moss_2   = 31, moss_3   = 32,

    skin_light_1  = 33, skin_light_2  = 34, skin_light_3  = 35,
    skin_medium_1 = 36, skin_medium_2 = 37, skin_medium_3 = 38,
    skin_dark_1   = 39, skin_dark_2   = 40, skin_dark_3   = 41,
    hair_dark_1   = 42, hair_dark_2   = 43,
    hair_brown_1  = 44, hair_brown_2  = 45,
    hair_blonde   = 46, hair_red      = 47,
    eye_dark_1    = 48, eye_dark_2    = 49, eye_light = 50,
    cloth_red     = 51, cloth_blue    = 52,
    cloth_green   = 53, cloth_white   = 54,

    iron_1   = 55, iron_2   = 56, iron_3   = 57,
    gold_1   = 58, gold_2   = 59, gold_3   = 60,
    copper_1 = 61, copper_2 = 62, copper_3 = 63,
    steel_1  = 64, steel_2  = 65, steel_3  = 66,
};
// clang-format on

class block_registry {
public:
    block_registry();

    [[nodiscard]] auto get(uint8 id) const -> const block_type&;
    [[nodiscard]] auto get_color(uint8 id) const -> color;
    [[nodiscard]] auto count() const -> uint8;
    [[nodiscard]] auto blocks(block_category cat) const -> std::span<const uint8>;

private:
    static constexpr uint32 category_count = 3;

    void reg(block_id id, color c, block_category cat,
             uint8 flags = block_flags::none);

    std::array<block_type, 256> blocks_{};
    uint8 count_ = 0;
    std::array<std::vector<uint8>, category_count> category_blocks_;
};

}  // namespace vw

#include "vw/core/block_registry.inl.h"

#endif  // VW_CORE_BLOCK_REGISTRY_H