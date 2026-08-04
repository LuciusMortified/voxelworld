module;

#include <array>

export module vw.core:color;

import :types;

export namespace vw {
struct color {
    uint32 value;

    constexpr color() : value(0U) {}
    constexpr explicit color(
        uint32 value_
    )
        : value(value_) {}
    constexpr color(
        uint8 r, uint8 g, uint8 b, uint8 a = 255
    )
        : value((static_cast<uint32>(r) << 24) |
                (static_cast<uint32>(g) << 16) |
                (static_cast<uint32>(b) << 8) |
                static_cast<uint32>(a)) {}

    constexpr color(const color&)                    = default;
    constexpr auto operator=(const color&) -> color& = default;
    constexpr color(color&&)                         = default;
    constexpr auto operator=(color&&) -> color&      = default;

    constexpr auto operator==(const color&) const -> bool = default;
    constexpr auto operator!=(const color&) const -> bool = default;

    [[nodiscard]] constexpr auto is_empty() const -> bool;

    [[nodiscard]] constexpr auto r() const -> uint8;
    [[nodiscard]] constexpr auto g() const -> uint8;
    [[nodiscard]] constexpr auto b() const -> uint8;
    [[nodiscard]] constexpr auto a() const -> uint8;
};

// 255-цветная палитра - основные цвета
namespace colors {

// Прозрачный (пустой)
constexpr auto empty = color(0x00000000);

// Основные цвета
constexpr auto black   = color(0x000000FF);
constexpr auto white   = color(0xFFFFFFFF);
constexpr auto red     = color(0xFF0000FF);
constexpr auto green   = color(0x00FF00FF);
constexpr auto blue    = color(0x0000FFFF);
constexpr auto yellow  = color(0xFFFF00FF);
constexpr auto cyan    = color(0x00FFFFFF);
constexpr auto magenta = color(0xFF00FFFF);

// Оттенки серого
constexpr auto gray       = color(0x808080FF);
constexpr auto light_gray = color(0xC0C0C0FF);
constexpr auto dark_gray  = color(0x404040FF);

// Оттенки красного
constexpr auto dark_red  = color(0x800000FF);
constexpr auto light_red = color(0xFF8080FF);
constexpr auto pink      = color(0xFFC0CBFF);
constexpr auto crimson   = color(0xDC143CFF);

// Оттенки зеленого
constexpr auto dark_green   = color(0x008000FF);
constexpr auto light_green  = color(0x90EE90FF);
constexpr auto forest_green = color(0x228B22FF);

// Оттенки синего
constexpr auto dark_blue  = color(0x000080FF);
constexpr auto light_blue = color(0xADD8E6FF);
constexpr auto sky_blue   = color(0x87CEEBFF);
constexpr auto royal_blue = color(0x4169E1FF);

// Оттенки желтого/оранжевого
constexpr auto dark_yellow  = color(0x808000FF);
constexpr auto light_yellow = color(0xFFFFE0FF);
constexpr auto orange       = color(0xFFA500FF);
constexpr auto dark_orange  = color(0xFF8C00FF);

// Оттенки коричневого
constexpr auto brown       = color(0xA52A2AFF);
constexpr auto light_brown = color(0xD2B48CFF);
constexpr auto dark_brown  = color(0x654321FF);
constexpr auto chocolate   = color(0xD2691EFF);

// Оттенки фиолетового
constexpr auto violet       = color(0xEE82EEFF);
constexpr auto purple       = color(0x800080FF);
constexpr auto light_purple = color(0xE6E6FAFF);
constexpr auto dark_purple  = color(0x483D8BFF);

// Металлические цвета
constexpr auto silver = color(0xC5C5C5FF);
constexpr auto gold   = color(0xFFD700FF);
constexpr auto bronze = color(0xCD7F32FF);
constexpr auto copper = color(0xB87333FF);
constexpr auto iron   = color(0x696969FF);

// Природные цвета
constexpr auto grass  = color(0x7CFC00FF);
constexpr auto dirt   = color(0x8B4513FF);
constexpr auto stone  = color(0xA9A9A9FF);
constexpr auto sand   = color(0xFFF5B4FF);
constexpr auto water  = color(0x1E90FFFF);
constexpr auto lava   = color(0xFF4500FF);
constexpr auto wood   = color(0xDEB887FF);
constexpr auto leaves = color(0x208B20FF);

// clang-format off
constexpr std::array all_old = {
    // Основные цвета
    black, white, red, green, blue, yellow, cyan, magenta,
    // Оттенки серого
    gray, light_gray, dark_gray,
    // Оттенки красного
    dark_red, light_red, pink, crimson,
    // Оттенки зеленого
    dark_green, light_green, forest_green,
    // Оттенки синего
    dark_blue, light_blue, sky_blue, royal_blue,
    // Оттенки желтого/оранжевого
    dark_yellow, light_yellow, orange, dark_orange,
    // Оттенки коричневого
    brown, light_brown, dark_brown, chocolate,
    // Оттенки фиолетового
    violet, purple, light_purple, dark_purple,
    // Металлические цвета
    silver, gold, bronze, copper, iron,
    // Природные цвета
    grass, dirt, stone, sand, water, lava, wood, leaves
};
// clang-format on

constexpr std::array resurrect_all = {
    color{0x2E222FFF}, color{0x3e3546ff}, color{0x625565ff}, color{0x966c6cff}, color{0xab947aff},
    color{0x694f62ff}, color{0x7f708aff}, color{0x9babb2ff}, color{0xc7dcd0ff}, color{0xffffffff},
    color{0x6e2727ff}, color{0xb33831ff}, color{0xea4f36ff}, color{0xf57d4aff}, color{0xae2334ff},
    color{0xe83b3bff}, color{0xfb6b1dff}, color{0xf79617ff}, color{0xf9c22bff}, color{0x7a3045ff},
    color{0x9e4539ff}, color{0xcd683dff}, color{0xe6904eff}, color{0xfbb954ff}, color{0x4c3e24ff},
    color{0x676633ff}, color{0xa2a947ff}, color{0xd5e04bff}, color{0xfbff86ff}, color{0x165a4cff},
    color{0x239063ff}, color{0x1ebc73ff}, color{0x91db69ff}, color{0xcddf6cff}, color{0x313638ff},
    color{0x374e4aff}, color{0x547e64ff}, color{0x92a984ff}, color{0xb2ba90ff}, color{0x0b5e65ff},
    color{0x0b8a8fff}, color{0x0eaf9bff}, color{0x30e1b9ff}, color{0x8ff8e2ff}, color{0x323353ff},
    color{0x484a77ff}, color{0x4d65b4ff}, color{0x4d9be6ff}, color{0x8fd3ffff}, color{0x45293fff},
    color{0x6b3e75ff}, color{0x905ea9ff}, color{0xa884f3ff}, color{0xeaadedff}, color{0x753c54ff},
    color{0xa24b6fff}, color{0xcf657fff}, color{0xed8099ff}, color{0x831c5dff}, color{0xc32454ff},
    color{0xf04f78ff}, color{0xf68181ff}, color{0xfca790ff}, color{0xfdcbb0ff},
};

// Apollo Color Palette https://lospec.com/palette-list/apollo
constexpr std::array all = {
    color{0x172038ff},
    color{0x253a5eff},
    color{0x3c5e8bff},
    color{0x4f8fbaff},
    color{0x73bed3ff},
    color{0xa4dddbff},
    color{0x19332dff},
    color{0x25562eff},
    color{0x468232ff},
    color{0x75a743ff},
    color{0xa8ca58ff},
    color{0xd0da91ff},
    color{0x4d2b32ff},
    color{0x7a4841ff},
    color{0xad7757ff},
    color{0xc09473ff},
    color{0xd7b594ff},
    color{0xe7d5b3ff},
    color{0x341c27ff},
    color{0x602c2cff},
    color{0x884b2bff},
    color{0xbe772bff},
    color{0xde9e41ff},
    color{0xe8c170ff},
    color{0x241527ff},
    color{0x411d31ff},
    color{0x752438ff},
    color{0xa53030ff},
    color{0xcf573cff},
    color{0xda863eff},
    color{0x1e1d39ff},
    color{0x402751ff},
    color{0x7a367bff},
    color{0xa23e8cff},
    color{0xc65197ff},
    color{0xdf84a5ff},
    color{0x090a14ff},
    color{0x10141fff},
    color{0x151d28ff},
    color{0x202e37ff},
    color{0x394a50ff},
    color{0x577277ff},
    color{0x819796ff},
    color{0xa8b5b2ff},
    color{0xc7cfccff},
    color{0xebede9ff},
    color{0xffffffff},
    color{0x000000ff},
};

}  // namespace colors
}  // namespace vw

namespace vw {

constexpr auto color::is_empty() const -> bool {
    return value == 0;
}

constexpr auto color::r() const -> uint8 {
    return static_cast<uint8>((value >> 24) & 0xFF);
}

constexpr auto color::g() const -> uint8 {
    return static_cast<uint8>((value >> 16) & 0xFF);
}

constexpr auto color::b() const -> uint8 {
    return static_cast<uint8>((value >> 8) & 0xFF);
}

constexpr auto color::a() const -> uint8 {
    return static_cast<uint8>(value & 0xFF);
}
}  // namespace vw
