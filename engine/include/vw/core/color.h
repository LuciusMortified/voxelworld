#pragma once

#ifndef VW_CORE_COLOR_H
#define VW_CORE_COLOR_H

#include "vw/core.h"

namespace vw {
struct color {
    constexpr color() : value(0) {}
    constexpr explicit color(uint32 value_) : value(value_) {}

    constexpr color(const color& c)                   = default;
    constexpr color& operator=(const color& c)        = default;
    constexpr bool operator==(const color& rhs) const = default;

    constexpr color(color&& c)            = default;
    constexpr color& operator=(color&& c) = default;

    [[nodiscard]]
    constexpr bool is_empty() const {
        return value == 0;
    }

    uint32 value;
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
constexpr auto maroon    = color(0x800000FF);

// Оттенки зеленого
constexpr auto dark_green   = color(0x008000FF);
constexpr auto light_green  = color(0x90EE90FF);
constexpr auto lime         = color(0x00FF00FF);
constexpr auto forest_green = color(0x228B22FF);
constexpr auto olive        = color(0x808000FF);

// Оттенки синего
constexpr auto dark_blue  = color(0x000080FF);
constexpr auto light_blue = color(0xADD8E6FF);
constexpr auto navy       = color(0x000080FF);
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
constexpr auto silver = color(0xC0C0C0FF);
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
constexpr auto leaves = color(0x228B22FF);

}  // namespace colors
}  // namespace vw

#endif  // VW_CORE_COLOR_H
