#pragma once

#ifndef VW_CORE_QUAT_H
#define VW_CORE_QUAT_H

#include "vw/core/types.h"

namespace vw {

struct quat {
    float32 x, y, z, w;

    quat() : x(0), y(0), z(0), w(1) {}
    quat(float32 x_, float32 y_, float32 z_, float32 w_) : x(x_), y(y_), z(z_), w(w_) {}

    [[nodiscard]] auto operator+(const quat& other) const -> quat {
        return {x + other.x, y + other.y, z + other.z, w + other.w};
    }

    [[nodiscard]] auto operator-(const quat& other) const -> quat {
        return {x - other.x, y - other.y, z - other.z, w - other.w};
    }

    [[nodiscard]] auto operator-() const -> quat { return {-x, -y, -z, -w}; }

    [[nodiscard]] auto operator*(float32 scalar) const -> quat {
        return {x * scalar, y * scalar, z * scalar, w * scalar};
    }

    [[nodiscard]] auto operator*(const quat& other) const -> quat {
        return {
            w * other.x + x * other.w + y * other.z - z * other.y,
            w * other.y - x * other.z + y * other.w + z * other.x,
            w * other.z + x * other.y - y * other.x + z * other.w,
            w * other.w - x * other.x - y * other.y - z * other.z
        };
    }

    [[nodiscard]] auto operator==(const quat& other) const -> bool {
        return x == other.x && y == other.y && z == other.z && w == other.w;
    }

    [[nodiscard]] auto operator!=(const quat& other) const -> bool {
        return !(*this == other);
    }
};

}  // namespace vw

#endif  // VW_CORE_QUAT_H
