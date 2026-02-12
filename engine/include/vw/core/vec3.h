#pragma once

#ifndef VW_CORE_VEC3_H
#define VW_CORE_VEC3_H

#include "vw/core/types.h"

namespace vw {

template <typename T>
struct vec3 {
    T x, y, z;

    constexpr vec3() : x(0), y(0), z(0) {}
    constexpr explicit vec3(T v) : x(v), y(v), z(v) {}
    constexpr vec3(T x_, T y_, T z_) : x(x_), y(y_), z(z_) {}

    template <typename U>
    constexpr explicit vec3(const vec3<U>& other)
        : x(static_cast<T>(other.x))
        , y(static_cast<T>(other.y))
        , z(static_cast<T>(other.z)) {}

    [[nodiscard]] constexpr T& operator[](size_t index) { return (&x)[index]; }
    [[nodiscard]] constexpr const T& operator[](size_t index) const { return (&x)[index]; }

    [[nodiscard]] constexpr vec3 operator+() const { return *this; }
    [[nodiscard]] constexpr vec3 operator-() const { return vec3(-x, -y, -z); }

    [[nodiscard]] constexpr vec3 operator+(const vec3& o) const { return {x + o.x, y + o.y, z + o.z}; }
    [[nodiscard]] constexpr vec3 operator-(const vec3& o) const { return {x - o.x, y - o.y, z - o.z}; }
    [[nodiscard]] constexpr vec3 operator*(const vec3& o) const { return {x * o.x, y * o.y, z * o.z}; }
    [[nodiscard]] constexpr vec3 operator/(const vec3& o) const { return {x / o.x, y / o.y, z / o.z}; }

    [[nodiscard]] constexpr vec3 operator*(T s) const { return {x * s, y * s, z * s}; }
    [[nodiscard]] constexpr vec3 operator/(T s) const { return {x / s, y / s, z / s}; }

    [[nodiscard]] constexpr bool operator==(const vec3& o) const { return x == o.x && y == o.y && z == o.z; }
    [[nodiscard]] constexpr bool operator!=(const vec3& o) const { return !(*this == o); }

    constexpr vec3& operator+=(const vec3& o) { x += o.x; y += o.y; z += o.z; return *this; }
    constexpr vec3& operator-=(const vec3& o) { x -= o.x; y -= o.y; z -= o.z; return *this; }
    constexpr vec3& operator*=(const vec3& o) { x *= o.x; y *= o.y; z *= o.z; return *this; }
    constexpr vec3& operator/=(const vec3& o) { x /= o.x; y /= o.y; z /= o.z; return *this; }
    constexpr vec3& operator*=(T s) { x *= s; y *= s; z *= s; return *this; }
    constexpr vec3& operator/=(T s) { x /= s; y /= s; z /= s; return *this; }
};

template <typename T>
[[nodiscard]] constexpr vec3<T> operator*(T scalar, const vec3<T>& v) { return v * scalar; }

using vec3i = vec3<int32>;
using vec3f = vec3<float32>;
using vec3d = vec3<float64>;

}  // namespace vw

#endif  // VW_CORE_VEC3_H
