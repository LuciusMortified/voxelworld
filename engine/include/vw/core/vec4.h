#pragma once

#ifndef VW_CORE_VEC4_H
#define VW_CORE_VEC4_H

#include "vw/core/types.h"

namespace vw {
template <typename T>
struct vec4 {
    T x, y, z, w;

    vec4() : x(0), y(0), z(0), w(0) {}
    vec4(T x_, T y_, T z_, T w_) : x(x_), y(y_), z(z_), w(w_) {}

    [[nodiscard]]
    vec4 operator+(const vec4& other) const {
        return vec4{x + other.x, y + other.y, z + other.z, w + other.w};
    }

    [[nodiscard]]
    vec4 operator-(const vec4& other) const {
        return vec4{x - other.x, y - other.y, z - other.z, w - other.w};
    }

    [[nodiscard]]
    vec4 operator*(T scalar) const {
        return vec4{x * scalar, y * scalar, z * scalar, w * scalar};
    }

    [[nodiscard]]
    bool operator==(const vec4& other) const {
        return x == other.x && y == other.y && z == other.z && w == other.w;
    }

    [[nodiscard]]
    bool operator!=(const vec4& other) const {
        return !(*this == other);
    }
};

using vec4i = vec4<int32>;
using vec4f = vec4<float32>;
using vec4d = vec4<float64>;
}  // namespace vw

#endif  // VW_CORE_VEC4_H
