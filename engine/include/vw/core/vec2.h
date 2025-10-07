#pragma once

#ifndef VW_CORE_VEC2_H
#define VW_CORE_VEC2_H

#include "vw/core/types.h"

namespace vw {
template <typename T>
struct vec2 {
    T x, y;

    vec2() : x(0), y(0) {}
    explicit vec2(T v) : x(v), y(v) {}
    vec2(T x_, T y_) : x(x_), y(y_) {}

    [[nodiscard]]
    vec2 operator+(const vec2& other) const {
        return vec2{x + other.x, y + other.y};
    }

    [[nodiscard]]
    vec2 operator-(const vec2& other) const {
        return vec2{x - other.x, y - other.y};
    }

    [[nodiscard]]
    vec2 operator*(T scalar) const {
        return vec2(x * scalar, y * scalar);
    }

    [[nodiscard]]
    bool operator==(const vec2& other) const {
        return x == other.x && y == other.y;
    }

    [[nodiscard]]
    bool operator!=(const vec2& other) const {
        return !(*this == other);
    }
};

using vec2i = vec2<int32>;
using vec2f = vec2<float32>;
using vec2d = vec2<float64>;
}

#endif  // VW_CORE_VEC2_H
