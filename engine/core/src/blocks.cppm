export module vw.core:blocks;

import std;

import :types;
import :color;

export namespace vw {

namespace block_flags {
constexpr uint8 none        = 0;
constexpr uint8 transparent = 1 << 0;
// Bit 1 was emissive, and block_type::glow says the same thing with a number
// instead of a yes. Two ways to ask whether a block glows is one too many; the
// bit is left as a hole rather than renumbering what sits above it.
constexpr uint8 liquid      = 1 << 2;
}  // namespace block_flags

struct block_id {
    uint8 value;

    constexpr block_id() : value(0) {}
    constexpr explicit block_id(
        uint8 value_
    )
        : value(value_) {}

    constexpr auto operator==(const block_id&) const -> bool = default;
};

struct block_type {
    block_id id = block_id{0};
    color clr = colors::empty;
    uint8 flags = block_flags::none;

    // Zero to fifteen, and it is both numbers at once: how bright the source is
    // and how far it carries. A flood step costs exactly one, so a block that
    // was bright but short-ranged would need a second value on every quad
    // corner, and there is one nibble there, not two. Minecraft's scale for the
    // same reason -- torch 14, glowstone 15.
    uint8 light = 0;

    // How brightly the block draws itself, nothing to do with what it gives its
    // neighbours. 255 means it renders at exactly the colour it was drawn even
    // where no light reaches it at all.
    //
    // Two properties and not one because they come apart: lava has both, a
    // crystal that glows without lighting the room has only this, and a lamp
    // sunk into a wall could have only the other. Nothing in the shader ties
    // them together either -- this term takes no occluder, that one takes AO.
    uint8 glow = 0;
};

namespace blocks {
constexpr auto air = block_id{0};

// clang-format off
constexpr auto blue_0   = block_id{1};
constexpr auto blue_1   = block_id{2};
constexpr auto blue_2   = block_id{3};
constexpr auto blue_3   = block_id{4};
constexpr auto blue_4   = block_id{5};
constexpr auto blue_5   = block_id{6};

constexpr auto green_0  = block_id{7};
constexpr auto green_1  = block_id{8};
constexpr auto green_2  = block_id{9};
constexpr auto green_3  = block_id{10};
constexpr auto green_4  = block_id{11};
constexpr auto green_5  = block_id{12};

constexpr auto brown_0  = block_id{13};
constexpr auto brown_1  = block_id{14};
constexpr auto brown_2  = block_id{15};
constexpr auto brown_3  = block_id{16};
constexpr auto brown_4  = block_id{17};
constexpr auto brown_5  = block_id{18};

constexpr auto orange_0 = block_id{19};
constexpr auto orange_1 = block_id{20};
constexpr auto orange_2 = block_id{21};
constexpr auto orange_3 = block_id{22};
constexpr auto orange_4 = block_id{23};
constexpr auto orange_5 = block_id{24};

constexpr auto red_0    = block_id{25};
constexpr auto red_1    = block_id{26};
constexpr auto red_2    = block_id{27};
constexpr auto red_3    = block_id{28};
constexpr auto red_4    = block_id{29};
constexpr auto red_5    = block_id{30};

constexpr auto purple_0 = block_id{31};
constexpr auto purple_1 = block_id{32};
constexpr auto purple_2 = block_id{33};
constexpr auto purple_3 = block_id{34};
constexpr auto purple_4 = block_id{35};
constexpr auto purple_5 = block_id{36};

constexpr auto gray_0   = block_id{37};
constexpr auto gray_1   = block_id{38};
constexpr auto gray_2   = block_id{39};
constexpr auto gray_3   = block_id{40};
constexpr auto gray_4   = block_id{41};
constexpr auto gray_5   = block_id{42};
constexpr auto gray_6   = block_id{43};
constexpr auto gray_7   = block_id{44};
constexpr auto gray_8   = block_id{45};
constexpr auto gray_9   = block_id{46};

constexpr auto white    = block_id{47};
constexpr auto black    = block_id{48};

// The two that emit. Their colours are deliberately not Apollo's: find_by_color
// returns the first block wearing a colour, and that is the function .vox
// import maps a palette through, so a duplicate would silently reassign it.
constexpr auto lamp     = block_id{49};
constexpr auto lava     = block_id{50};
// clang-format on

}  // namespace blocks

class block_registry {
public:
    block_registry();

    [[nodiscard]] auto get(block_id id) const -> const block_type&;
    [[nodiscard]] auto get_color(block_id id) const -> color;
    [[nodiscard]] auto find_by_color(color c) const -> block_id;
    [[nodiscard]] auto blocks() const -> const std::array<block_type, 256>&;

private:
    void reg(block_id id, color c, uint8 flags = block_flags::none, uint8 light = 0,
             uint8 glow = 0);

    std::array<block_type, 256> blocks_{};
};

struct voxel {
    block_id id = blocks::air;

    constexpr voxel() = default;
    constexpr explicit voxel(block_id block_id) : id(block_id) {}

    [[nodiscard]] constexpr auto is_empty() const -> bool { return id == blocks::air; }

    constexpr auto operator==(const voxel&) const -> bool = default;
};

constexpr auto empty_voxel = voxel{};

}  // namespace vw
