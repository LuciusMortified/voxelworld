#pragma once

using uint8  = unsigned char;
using uint16 = unsigned short;
using uint32 = unsigned int;
using uint64 = unsigned long long;

using int8  = signed char;
using int16 = signed short;
using int32 = signed int;
using int64 = signed long long;

namespace voxel {
    template<typename T>
    struct vec3 {
        T x, y, z;

        vec3() : x(0), y(0), z(0) {}
        vec3(T x_, T y_, T z_) : x(x_), y(y_), z(z_) {}

        vec3 operator+(const vec3& other) const {
            return vec3(x + other.x, y + other.y, z + other.z);
        }
        vec3 operator-(const vec3& other) const {
            return vec3(x - other.x, y - other.y, z - other.z);
        }
        vec3 operator*(T scalar) const {
            return vec3(x * scalar, y * scalar, z * scalar);
        }
        bool operator==(const vec3& other) const {
            return x == other.x && y == other.y && z == other.z;
        }
        bool operator!=(const vec3& other) const {
            return !(*this == other);
        }
    };

    using ivec3 = vec3<int>;
    using vec3f = vec3<float>;
    using dvec3 = vec3<double>;
} 