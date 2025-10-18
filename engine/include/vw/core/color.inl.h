#pragma once

#ifndef VW_CORE_COLOR_INL_H
#define VW_CORE_COLOR_INL_H

#include "vw/core/color.h"

namespace vw {

constexpr auto color::is_empty() const -> bool {
    return value == 0;
}

}  // namespace vw

#endif  // VW_CORE_COLOR_INL_H