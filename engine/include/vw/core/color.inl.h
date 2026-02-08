#pragma once

#ifndef VW_CORE_COLOR_INL_H
#define VW_CORE_COLOR_INL_H

#include "vw/core/color.h"

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

#endif  // VW_CORE_COLOR_INL_H