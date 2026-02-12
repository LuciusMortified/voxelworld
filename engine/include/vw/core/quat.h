#pragma once

#ifndef VW_CORE_QUAT_H
#define VW_CORE_QUAT_H

#include "vw/core/types.h"

namespace vw {

struct quat {
    float32 x, y, z, w;

    constexpr quat() : x(0), y(0), z(0), w(1) {}
    constexpr quat(float32 x_, float32 y_, float32 z_, float32 w_) : x(x_), y(y_), z(z_), w(w_) {}

    [[nodiscard]] constexpr float32& operator[](size_t index) { return (&x)[index]; }
    [[nodiscard]] constexpr const float32& operator[](size_t index) const { return (&x)[index]; }

    [[nodiscard]] constexpr quat operator+() const { return *this; }
    [[nodiscard]] constexpr quat operator-() const { return {-x, -y, -z, -w}; }

    [[nodiscard]] constexpr quat operator+(const quat& o) const { return {x+o.x, y+o.y, z+o.z, w+o.w}; }
    [[nodiscard]] constexpr quat operator-(const quat& o) const { return {x-o.x, y-o.y, z-o.z, w-o.w}; }

    [[nodiscard]] constexpr quat operator*(float32 s) const { return {x*s, y*s, z*s, w*s}; }
    [[nodiscard]] constexpr quat operator/(float32 s) const { return {x/s, y/s, z/s, w/s}; }

    [[nodiscard]] constexpr quat operator*(const quat& o) const {
        return {
            w*o.x + x*o.w + y*o.z - z*o.y,
            w*o.y - x*o.z + y*o.w + z*o.x,
            w*o.z + x*o.y - y*o.x + z*o.w,
            w*o.w - x*o.x - y*o.y - z*o.z
        };
    }

    constexpr quat& operator+=(const quat& o) { x+=o.x; y+=o.y; z+=o.z; w+=o.w; return *this; }
    constexpr quat& operator-=(const quat& o) { x-=o.x; y-=o.y; z-=o.z; w-=o.w; return *this; }
    constexpr quat& operator*=(float32 s) { x*=s; y*=s; z*=s; w*=s; return *this; }
    constexpr quat& operator/=(float32 s) { x/=s; y/=s; z/=s; w/=s; return *this; }
    constexpr quat& operator*=(const quat& o) { *this = *this * o; return *this; }

    [[nodiscard]] constexpr bool operator==(const quat& o) const { return x==o.x && y==o.y && z==o.z && w==o.w; }
    [[nodiscard]] constexpr bool operator!=(const quat& o) const { return !(*this == o); }
};

[[nodiscard]] constexpr inline quat operator*(float32 scalar, const quat& q) { return q * scalar; }

}  // namespace vw

#endif  // VW_CORE_QUAT_H
