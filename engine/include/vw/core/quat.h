#pragma once

#ifndef VW_CORE_QUAT_H
#define VW_CORE_QUAT_H

#include "vw/core/types.h"

namespace vw {

struct quat {
    float32 x, y, z, w;

    constexpr quat() : x(0), y(0), z(0), w(1) {}
    constexpr quat(float32 x_, float32 y_, float32 z_, float32 w_) : x(x_), y(y_), z(z_), w(w_) {}

    [[nodiscard]] constexpr auto operator[](size_t index) -> float32& { return (&x)[index]; }
    [[nodiscard]] constexpr auto operator[](size_t index) const -> const float32& { return (&x)[index]; }

    [[nodiscard]] constexpr auto operator+() const -> quat { return *this; }
    [[nodiscard]] constexpr auto operator-() const -> quat { return {-x, -y, -z, -w}; }

    [[nodiscard]] constexpr auto operator+(const quat& o) const -> quat { return {x+o.x, y+o.y, z+o.z, w+o.w}; }
    [[nodiscard]] constexpr auto operator-(const quat& o) const -> quat { return {x-o.x, y-o.y, z-o.z, w-o.w}; }

    [[nodiscard]] constexpr auto operator*(float32 s) const -> quat { return {x*s, y*s, z*s, w*s}; }
    [[nodiscard]] constexpr auto operator/(float32 s) const -> quat { return {x/s, y/s, z/s, w/s}; }

    [[nodiscard]] constexpr auto operator*(const quat& o) const -> quat {
        return {
            (w*o.x) + (x*o.w) + (y*o.z) - (z*o.y),
            (w*o.y) - (x*o.z) + (y*o.w) + (z*o.x),
            (w*o.z) + (x*o.y) - (y*o.x) + (z*o.w),
            (w*o.w) - (x*o.x) - (y*o.y) - (z*o.z)
        };
    }

    constexpr auto operator+=(const quat& o) -> quat& { x+=o.x; y+=o.y; z+=o.z; w+=o.w; return *this; }
    constexpr auto operator-=(const quat& o) -> quat& { x-=o.x; y-=o.y; z-=o.z; w-=o.w; return *this; }
    constexpr auto operator*=(float32 s) -> quat& { x*=s; y*=s; z*=s; w*=s; return *this; }
    constexpr auto operator/=(float32 s) -> quat& { x/=s; y/=s; z/=s; w/=s; return *this; }
    constexpr auto operator*=(const quat& o) -> quat& { *this = *this * o; return *this; }

    [[nodiscard]] constexpr auto operator==(const quat& o) const -> bool { return x==o.x && y==o.y && z==o.z && w==o.w; }
    [[nodiscard]] constexpr auto operator!=(const quat& o) const -> bool { return !(*this == o); }
};

[[nodiscard]] constexpr inline auto operator*(float32 scalar, const quat& q) -> quat { return q * scalar; }

}  // namespace vw

#endif  // VW_CORE_QUAT_H
