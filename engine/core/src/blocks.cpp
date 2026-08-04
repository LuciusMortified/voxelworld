module;

#include <array>

module vw.core;

namespace vw {

block_registry::block_registry() {
    using namespace blocks;
    namespace f = block_flags;

    reg(air, colors::empty);

    // clang-format off
    reg(blue_0,   colors::all[ 0]);
    reg(blue_1,   colors::all[ 1]);
    reg(blue_2,   colors::all[ 2]);
    reg(blue_3,   colors::all[ 3]);
    reg(blue_4,   colors::all[ 4]);
    reg(blue_5,   colors::all[ 5]);

    reg(green_0,  colors::all[ 6]);
    reg(green_1,  colors::all[ 7]);
    reg(green_2,  colors::all[ 8]);
    reg(green_3,  colors::all[ 9]);
    reg(green_4,  colors::all[10]);
    reg(green_5,  colors::all[11]);

    reg(brown_0,  colors::all[12]);
    reg(brown_1,  colors::all[13]);
    reg(brown_2,  colors::all[14]);
    reg(brown_3,  colors::all[15]);
    reg(brown_4,  colors::all[16]);
    reg(brown_5,  colors::all[17]);

    reg(orange_0, colors::all[18]);
    reg(orange_1, colors::all[19]);
    reg(orange_2, colors::all[20]);
    reg(orange_3, colors::all[21]);
    reg(orange_4, colors::all[22]);
    reg(orange_5, colors::all[23]);

    reg(red_0,    colors::all[24]);
    reg(red_1,    colors::all[25]);
    reg(red_2,    colors::all[26]);
    reg(red_3,    colors::all[27]);
    reg(red_4,    colors::all[28]);
    reg(red_5,    colors::all[29]);

    reg(purple_0, colors::all[30]);
    reg(purple_1, colors::all[31]);
    reg(purple_2, colors::all[32]);
    reg(purple_3, colors::all[33]);
    reg(purple_4, colors::all[34]);
    reg(purple_5, colors::all[35]);

    reg(gray_0,   colors::all[36]);
    reg(gray_1,   colors::all[37]);
    reg(gray_2,   colors::all[38]);
    reg(gray_3,   colors::all[39]);
    reg(gray_4,   colors::all[40]);
    reg(gray_5,   colors::all[41]);
    reg(gray_6,   colors::all[42]);
    reg(gray_7,   colors::all[43]);
    reg(gray_8,   colors::all[44]);
    reg(gray_9,   colors::all[45]);

    reg(white,    colors::all[46]);
    reg(black,    colors::all[47]);
    // clang-format on
}

void block_registry::reg(
    block_id id, color c, uint8 flags
) {
    blocks_[id.value] = {id, c, flags};
}

auto block_registry::get(
    block_id id
) const -> const block_type& {
    return blocks_[id.value];
}

auto block_registry::get_color(
    block_id id
) const -> color {
    return blocks_[id.value].clr;
}

auto block_registry::find_by_color(
    color c
) const -> block_id {
    for (const auto& block : blocks_) {
        if (block.clr == c && block.id != blocks::air) {
            return block.id;
        }
    }
    return blocks::air;
}

auto block_registry::blocks() const -> const std::array<block_type, 256>& {
    return blocks_;
}

}  // namespace vw
