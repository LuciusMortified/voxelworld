#pragma once

#ifndef VW_CORE_VEC2_H
#define VW_CORE_VEC2_H

#include "vw/core/types.h"

namespace vw {

template <typename T>
struct vec2 {
    T x, y;

    constexpr vec2() : x(0), y(0) {}
    constexpr explicit vec2(T v) : x(v), y(v) {}
    constexpr vec2(T x_, T y_) : x(x_), y(y_) {}

    template <typename U>
    constexpr explicit vec2(const vec2<U>& other)
        : x(static_cast<T>(other.x))
        , y(static_cast<T>(other.y)) {}

    [[nodiscard]] constexpr T& operator[](size_t index) { return (&x)[index]; }
    [[nodiscard]] constexpr const T& operator[](size_t index) const { return (&x)[index]; }

    [[nodiscard]] constexpr vec2 operator+() const { return *this; }
    [[nodiscard]] constexpr vec2 operator-() const { return vec2(-x, -y); }

    [[nodiscard]] constexpr vec2 operator+(const vec2& o) const { return {x + o.x, y + o.y}; }
    [[nodiscard]] constexpr vec2 operator-(const vec2& o) const { return {x - o.x, y - o.y}; }
    [[nodiscard]] constexpr vec2 operator*(const vec2& o) const { return {x * o.x, y * o.y}; }
    [[nodiscard]] constexpr vec2 operator/(const vec2& o) const { return {x / o.x, y / o.y}; }

    [[nodiscard]] constexpr vec2 operator*(T s) const { return {x * s, y * s}; }
    [[nodiscard]] constexpr vec2 operator/(T s) const { return {x / s, y / s}; }

    [[nodiscard]] constexpr bool operator==(const vec2& o) const { return x == o.x && y == o.y; }
    [[nodiscard]] constexpr bool operator!=(const vec2& o) const { return !(*this == o); }

    constexpr vec2& operator+=(const vec2& o) { x += o.x; y += o.y; return *this; }
    constexpr vec2& operator-=(const vec2& o) { x -= o.x; y -= o.y; return *this; }
    constexpr vec2& operator*=(const vec2& o) { x *= o.x; y *= o.y; return *this; }
    constexpr vec2& operator/=(const vec2& o) { x /= o.x; y /= o.y; return *this; }
    constexpr vec2& operator*=(T s) { x *= s; y *= s; return *this; }
    constexpr vec2& operator/=(T s) { x /= s; y /= s; return *this; }
};

template <typename T>
[[nodiscard]] constexpr vec2<T> operator*(T scalar, const vec2<T>& v) { return v * scalar; }

using vec2i = vec2<int32>;
using vec2f = vec2<float32>;
using vec2d = vec2<float64>;

}  // namespace vw

#endif  // VW_CORE_VEC2_H
