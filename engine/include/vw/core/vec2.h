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

    [[nodiscard]] constexpr auto operator[](size_t index) -> T& { return (&x)[index]; }
    [[nodiscard]] constexpr auto operator[](size_t index) const -> const T& { return (&x)[index]; }

    [[nodiscard]] constexpr auto operator+() const -> vec2 { return *this; }
    [[nodiscard]] constexpr auto operator-() const -> vec2 { return vec2(-x, -y); }

    [[nodiscard]] constexpr auto operator+(const vec2& o) const -> vec2 { return {x + o.x, y + o.y}; }
    [[nodiscard]] constexpr auto operator-(const vec2& o) const -> vec2 { return {x - o.x, y - o.y}; }
    [[nodiscard]] constexpr auto operator*(const vec2& o) const -> vec2 { return {x * o.x, y * o.y}; }
    [[nodiscard]] constexpr auto operator/(const vec2& o) const -> vec2 { return {x / o.x, y / o.y}; }

    [[nodiscard]] constexpr auto operator*(T s) const -> vec2 { return {x * s, y * s}; }
    [[nodiscard]] constexpr auto operator/(T s) const -> vec2 { return {x / s, y / s}; }

    [[nodiscard]] constexpr auto operator==(const vec2& o) const -> bool { return x == o.x && y == o.y; }
    [[nodiscard]] constexpr auto operator!=(const vec2& o) const -> bool { return !(*this == o); }

    constexpr auto operator+=(const vec2& o) -> vec2& { x += o.x; y += o.y; return *this; }
    constexpr auto operator-=(const vec2& o) -> vec2& { x -= o.x; y -= o.y; return *this; }
    constexpr auto operator*=(const vec2& o) -> vec2& { x *= o.x; y *= o.y; return *this; }
    constexpr auto operator/=(const vec2& o) -> vec2& { x /= o.x; y /= o.y; return *this; }
    constexpr auto operator*=(T s) -> vec2& { x *= s; y *= s; return *this; }
    constexpr auto operator/=(T s) -> vec2& { x /= s; y /= s; return *this; }
};

template <typename T>
[[nodiscard]] constexpr auto operator*(T scalar, const vec2<T>& v) -> vec2<T> { return v * scalar; }

using vec2i = vec2<int32>;
using vec2f = vec2<float32>;
using vec2d = vec2<float64>;

}  // namespace vw

template <>
struct std::hash<vw::vec2<vw::int32>> {
    auto operator()(const vw::vec2<vw::int32>& v) const noexcept -> size_t {
        size_t seed = std::hash<vw::int32>{}(v.x);
        seed ^= std::hash<vw::int32>{}(v.y) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        return seed;
    }
};

#endif  // VW_CORE_VEC2_H
