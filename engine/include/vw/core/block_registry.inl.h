#pragma once

#ifndef VW_CORE_BLOCK_REGISTRY_INL_H
#define VW_CORE_BLOCK_REGISTRY_INL_H

namespace vw {

inline block_registry::block_registry() {
    blocks_[0] = {colors::empty, block_category::terrain};

    using enum block_id;
    using enum block_category;
    namespace f = block_flags;

    // clang-format off

    // Terrain — grass
    reg(grass_1,  {0x46, 0x82, 0x32}, terrain);
    reg(grass_2,  {0x75, 0xa7, 0x43}, terrain);
    reg(grass_3,  {0xa8, 0xca, 0x58}, terrain);

    // Terrain — dirt
    reg(dirt_1,   {0x4d, 0x2b, 0x32}, terrain);
    reg(dirt_2,   {0x7a, 0x48, 0x41}, terrain);
    reg(dirt_3,   {0xad, 0x77, 0x57}, terrain);

    // Terrain — stone
    reg(stone_1,  {0x39, 0x4a, 0x50}, terrain);
    reg(stone_2,  {0x57, 0x72, 0x77}, terrain);
    reg(stone_3,  {0x81, 0x97, 0x96}, terrain);

    // Terrain — sand
    reg(sand_1,   {0xd7, 0xb5, 0x94}, terrain);
    reg(sand_2,   {0xe7, 0xd5, 0xb3}, terrain);
    reg(sand_3,   {0xe8, 0xc1, 0x70}, terrain);

    // Terrain — snow
    reg(snow_1,   {0xc7, 0xcf, 0xcc}, terrain);
    reg(snow_2,   {0xeb, 0xed, 0xe9}, terrain);
    reg(snow_3,   {0xff, 0xff, 0xff}, terrain);

    // Terrain — water
    reg(water_1,  {0x3c, 0x5e, 0x8b}, terrain, f::transparent | f::liquid);
    reg(water_2,  {0x4f, 0x8f, 0xba}, terrain, f::transparent | f::liquid);
    reg(water_3,  {0x73, 0xbe, 0xd3}, terrain, f::transparent | f::liquid);

    // Terrain — lava
    reg(lava_1,   {0xa5, 0x30, 0x30}, terrain, f::emissive);
    reg(lava_2,   {0xcf, 0x57, 0x3c}, terrain, f::emissive);
    reg(lava_3,   {0xda, 0x86, 0x3e}, terrain, f::emissive);

    // Terrain — log
    reg(log_1,    {0x88, 0x4b, 0x2b}, terrain);
    reg(log_2,    {0xbe, 0x77, 0x2b}, terrain);
    reg(log_3,    {0xad, 0x77, 0x57}, terrain);

    // Terrain — leaves
    reg(leaves_1, {0x25, 0x56, 0x2e}, terrain);
    reg(leaves_2, {0x46, 0x82, 0x32}, terrain);
    reg(leaves_3, {0x75, 0xa7, 0x43}, terrain);

    // Terrain — bark
    reg(bark_1,   {0x34, 0x1c, 0x27}, terrain);
    reg(bark_2,   {0x60, 0x2c, 0x2c}, terrain);

    // Terrain — moss
    reg(moss_1,   {0x19, 0x33, 0x2d}, terrain);
    reg(moss_2,   {0x25, 0x56, 0x2e}, terrain);
    reg(moss_3,   {0xd0, 0xda, 0x91}, terrain);

    // Characters — skin
    reg(skin_light_1,  {0xc0, 0x94, 0x73}, character);
    reg(skin_light_2,  {0xd7, 0xb5, 0x94}, character);
    reg(skin_light_3,  {0xe7, 0xd5, 0xb3}, character);
    reg(skin_medium_1, {0x7a, 0x48, 0x41}, character);
    reg(skin_medium_2, {0xad, 0x77, 0x57}, character);
    reg(skin_medium_3, {0xc0, 0x94, 0x73}, character);
    reg(skin_dark_1,   {0x4d, 0x2b, 0x32}, character);
    reg(skin_dark_2,   {0x7a, 0x48, 0x41}, character);
    reg(skin_dark_3,   {0x60, 0x2c, 0x2c}, character);

    // Characters — hair
    reg(hair_dark_1,   {0x09, 0x0a, 0x14}, character);
    reg(hair_dark_2,   {0x00, 0x00, 0x00}, character);
    reg(hair_brown_1,  {0x7a, 0x48, 0x41}, character);
    reg(hair_brown_2,  {0x88, 0x4b, 0x2b}, character);
    reg(hair_blonde,   {0xe8, 0xc1, 0x70}, character);
    reg(hair_red,      {0xcf, 0x57, 0x3c}, character);

    // Characters — eyes
    reg(eye_dark_1,    {0x10, 0x14, 0x1f}, character);
    reg(eye_dark_2,    {0x25, 0x3a, 0x5e}, character);
    reg(eye_light,     {0x4f, 0x8f, 0xba}, character);

    // Characters — clothing
    reg(cloth_red,     {0xa5, 0x30, 0x30}, character);
    reg(cloth_blue,    {0x3c, 0x5e, 0x8b}, character);
    reg(cloth_green,   {0x46, 0x82, 0x32}, character);
    reg(cloth_white,   {0xff, 0xff, 0xff}, character);

    // Metals — iron
    reg(iron_1,   {0x20, 0x2e, 0x37}, metal);
    reg(iron_2,   {0x39, 0x4a, 0x50}, metal);
    reg(iron_3,   {0x57, 0x72, 0x77}, metal);

    // Metals — gold
    reg(gold_1,   {0xbe, 0x77, 0x2b}, metal);
    reg(gold_2,   {0xde, 0x9e, 0x41}, metal);
    reg(gold_3,   {0xe8, 0xc1, 0x70}, metal);

    // Metals — copper
    reg(copper_1, {0xcf, 0x57, 0x3c}, metal);
    reg(copper_2, {0xda, 0x86, 0x3e}, metal);
    reg(copper_3, {0xbe, 0x77, 0x2b}, metal);

    // Metals — steel
    reg(steel_1,  {0x81, 0x97, 0x96}, metal);
    reg(steel_2,  {0xa8, 0xb5, 0xb2}, metal);
    reg(steel_3,  {0xc7, 0xcf, 0xcc}, metal);

    // clang-format on
}

inline void block_registry::reg(
    block_id id, color c, block_category cat, uint8 flags
) {
    auto idx = static_cast<uint8>(id);
    blocks_[idx] = {c, cat, flags};
    category_blocks_[static_cast<uint8>(cat)].push_back(idx);
    if (idx >= count_) count_ = idx + 1;
}

inline auto block_registry::get(
    uint8 id
) const -> const block_type& {
    return blocks_[id];
}

inline auto block_registry::get_color(
    uint8 id
) const -> color {
    return blocks_[id].col;
}

inline auto block_registry::count() const -> uint8 {
    return count_;
}

inline auto block_registry::blocks(
    block_category cat
) const -> std::span<const uint8> {
    return category_blocks_[static_cast<uint8>(cat)];
}

}  // namespace vw

#endif  // VW_CORE_BLOCK_REGISTRY_INL_H