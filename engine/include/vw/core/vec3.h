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

    [[nodiscard]] constexpr auto operator[](size_t index) -> T& { return (&x)[index]; }
    [[nodiscard]] constexpr auto operator[](size_t index) const -> const T& { return (&x)[index]; }

    [[nodiscard]] constexpr auto operator+() const -> vec3 { return *this; }
    [[nodiscard]] constexpr auto operator-() const -> vec3 { return vec3(-x, -y, -z); }

    [[nodiscard]] constexpr auto operator+(const vec3& o) const -> vec3 { return {x + o.x, y + o.y, z + o.z}; }
    [[nodiscard]] constexpr auto operator-(const vec3& o) const -> vec3 { return {x - o.x, y - o.y, z - o.z}; }
    [[nodiscard]] constexpr auto operator*(const vec3& o) const -> vec3 { return {x * o.x, y * o.y, z * o.z}; }
    [[nodiscard]] constexpr auto operator/(const vec3& o) const -> vec3 { return {x / o.x, y / o.y, z / o.z}; }

    [[nodiscard]] constexpr auto operator*(T s) const -> vec3 { return {x * s, y * s, z * s}; }
    [[nodiscard]] constexpr auto operator/(T s) const -> vec3 { return {x / s, y / s, z / s}; }

    [[nodiscard]] constexpr auto operator==(const vec3& o) const -> bool { return x == o.x && y == o.y && z == o.z; }
    [[nodiscard]] constexpr auto operator!=(const vec3& o) const -> bool { return !(*this == o); }

    constexpr auto operator+=(const vec3& o) -> vec3& { x += o.x; y += o.y; z += o.z; return *this; }
    constexpr auto operator-=(const vec3& o) -> vec3& { x -= o.x; y -= o.y; z -= o.z; return *this; }
    constexpr auto operator*=(const vec3& o) -> vec3& { x *= o.x; y *= o.y; z *= o.z; return *this; }
    constexpr auto operator/=(const vec3& o) -> vec3& { x /= o.x; y /= o.y; z /= o.z; return *this; }
    constexpr auto operator*=(T s) -> vec3& { x *= s; y *= s; z *= s; return *this; }
    constexpr auto operator/=(T s) -> vec3& { x /= s; y /= s; z /= s; return *this; }
};

template <typename T>
[[nodiscard]] constexpr auto operator*(T scalar, const vec3<T>& v) -> vec3<T> { return v * scalar; }

using vec3i = vec3<int32>;
using vec3f = vec3<float32>;
using vec3d = vec3<float64>;

}  // namespace vw

#endif  // VW_CORE_VEC3_H
