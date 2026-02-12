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

    [[nodiscard]] constexpr T& operator[](size_t index) { return (&x)[index]; }
    [[nodiscard]] constexpr const T& operator[](size_t index) const { return (&x)[index]; }

    [[nodiscard]] constexpr vec4 operator+() const { return *this; }
    [[nodiscard]] constexpr vec4 operator-() const { return vec4(-x, -y, -z, -w); }

    [[nodiscard]] constexpr vec4 operator+(const vec4& o) const { return {x+o.x, y+o.y, z+o.z, w+o.w}; }
    [[nodiscard]] constexpr vec4 operator-(const vec4& o) const { return {x-o.x, y-o.y, z-o.z, w-o.w}; }
    [[nodiscard]] constexpr vec4 operator*(const vec4& o) const { return {x*o.x, y*o.y, z*o.z, w*o.w}; }
    [[nodiscard]] constexpr vec4 operator/(const vec4& o) const { return {x/o.x, y/o.y, z/o.z, w/o.w}; }

    [[nodiscard]] constexpr vec4 operator*(T s) const { return {x*s, y*s, z*s, w*s}; }
    [[nodiscard]] constexpr vec4 operator/(T s) const { return {x/s, y/s, z/s, w/s}; }

    [[nodiscard]] constexpr bool operator==(const vec4& o) const { return x==o.x && y==o.y && z==o.z && w==o.w; }
    [[nodiscard]] constexpr bool operator!=(const vec4& o) const { return !(*this == o); }

    constexpr vec4& operator+=(const vec4& o) { x+=o.x; y+=o.y; z+=o.z; w+=o.w; return *this; }
    constexpr vec4& operator-=(const vec4& o) { x-=o.x; y-=o.y; z-=o.z; w-=o.w; return *this; }
    constexpr vec4& operator*=(const vec4& o) { x*=o.x; y*=o.y; z*=o.z; w*=o.w; return *this; }
    constexpr vec4& operator/=(const vec4& o) { x/=o.x; y/=o.y; z/=o.z; w/=o.w; return *this; }
    constexpr vec4& operator*=(T s) { x*=s; y*=s; z*=s; w*=s; return *this; }
    constexpr vec4& operator/=(T s) { x/=s; y/=s; z/=s; w/=s; return *this; }
};

template <typename T>
[[nodiscard]] constexpr vec4<T> operator*(T scalar, const vec4<T>& v) { return v * scalar; }

using vec4i = vec4<int32>;
using vec4f = vec4<float32>;
using vec4d = vec4<float64>;

}  // namespace vw

#endif  // VW_CORE_VEC4_H
