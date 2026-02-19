#pragma once

#ifndef VW_CORE_VEC4_H
#define VW_CORE_VEC4_H

#include "vw/core/types.h"

namespace vw {

template <typename T>
struct vec3;

template <typename T>
struct vec4 {
    T x, y, z, w;

    constexpr vec4() : x(0), y(0), z(0), w(0) {}
    constexpr explicit vec4(T v) : x(v), y(v), z(v), w(v) {}
    constexpr vec4(T x_, T y_, T z_, T w_) : x(x_), y(y_), z(z_), w(w_) {}

    template <typename U>
    constexpr explicit vec4(const vec4<U>& other)
        : x(static_cast<T>(other.x)), y(static_cast<T>(other.y))
        , z(static_cast<T>(other.z)), w(static_cast<T>(other.w)) {}

    template <typename U>
    constexpr vec4(const vec3<U>& v, T w_)
        : x(static_cast<T>(v.x)), y(static_cast<T>(v.y))
        , z(static_cast<T>(v.z)), w(w_) {}

    [[nodiscard]] constexpr auto operator[](size_t index) -> T& { return (&x)[index]; }
    [[nodiscard]] constexpr auto operator[](size_t index) const -> const T& { return (&x)[index]; }

    [[nodiscard]] constexpr auto operator+() const -> vec4 { return *this; }
    [[nodiscard]] constexpr auto operator-() const -> vec4 { return vec4(-x, -y, -z, -w); }

    [[nodiscard]] constexpr auto operator+(const vec4& o) const -> vec4 { return {x+o.x, y+o.y, z+o.z, w+o.w}; }
    [[nodiscard]] constexpr auto operator-(const vec4& o) const -> vec4 { return {x-o.x, y-o.y, z-o.z, w-o.w}; }
    [[nodiscard]] constexpr auto operator*(const vec4& o) const -> vec4 { return {x*o.x, y*o.y, z*o.z, w*o.w}; }
    [[nodiscard]] constexpr auto operator/(const vec4& o) const -> vec4 { return {x/o.x, y/o.y, z/o.z, w/o.w}; }

    [[nodiscard]] constexpr auto operator*(T s) const -> vec4 { return {x*s, y*s, z*s, w*s}; }
    [[nodiscard]] constexpr auto operator/(T s) const -> vec4 { return {x/s, y/s, z/s, w/s}; }

    [[nodiscard]] constexpr auto operator==(const vec4& o) const -> bool { return x==o.x && y==o.y && z==o.z && w==o.w; }
    [[nodiscard]] constexpr auto operator!=(const vec4& o) const -> bool { return !(*this == o); }

    constexpr auto operator+=(const vec4& o) -> vec4& { x+=o.x; y+=o.y; z+=o.z; w+=o.w; return *this; }
    constexpr auto operator-=(const vec4& o) -> vec4& { x-=o.x; y-=o.y; z-=o.z; w-=o.w; return *this; }
    constexpr auto operator*=(const vec4& o) -> vec4& { x*=o.x; y*=o.y; z*=o.z; w*=o.w; return *this; }
    constexpr auto operator/=(const vec4& o) -> vec4& { x/=o.x; y/=o.y; z/=o.z; w/=o.w; return *this; }
    constexpr auto operator*=(T s) -> vec4& { x*=s; y*=s; z*=s; w*=s; return *this; }
    constexpr auto operator/=(T s) -> vec4& { x/=s; y/=s; z/=s; w/=s; return *this; }
};

template <typename T>
[[nodiscard]] constexpr auto operator*(T scalar, const vec4<T>& v) -> vec4<T> { return v * scalar; }

using vec4i = vec4<int32>;
using vec4f = vec4<float32>;
using vec4d = vec4<float64>;

}  // namespace vw

#endif  // VW_CORE_VEC4_H
