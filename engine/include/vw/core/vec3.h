#pragma once

#ifndef VW_CORE_VEC3_H
#define VW_CORE_VEC3_H

#include "vw/core/types.h"

namespace vw {
template <typename T>
struct vec3 {
    T x, y, z;

    vec3() : x(0), y(0), z(0) {}
    explicit vec3(T v) : x(v), y(v), z(v) {}
    vec3(T x_, T y_, T z_) : x(x_), y(y_), z(z_) {}

    [[nodiscard]]
    vec3 operator+(const vec3& other) const {
        return vec3{x + other.x, y + other.y, z + other.z};
    }

    [[nodiscard]]
    vec3 operator-(const vec3& other) const {
        return vec3{x - other.x, y - other.y, z - other.z};
    }

    [[nodiscard]]
    vec3 operator*(T scalar) const {
        return vec3(x * scalar, y * scalar, z * scalar);
    }

    [[nodiscard]]
    bool operator==(const vec3& other) const {
        return x == other.x && y == other.y && z == other.z;
    }

    [[nodiscard]]
    bool operator!=(const vec3& other) const {
        return !(*this == other);
    }
};

using vec3i = vec3<int32>;
using vec3f = vec3<float32>;
using vec3d = vec3<float64>;
}  // namespace vw

#endif  // VW_CORE_VEC3_H
