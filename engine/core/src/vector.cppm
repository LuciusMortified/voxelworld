module;

#include <cstddef>
#include <functional>

export module vw.core:vector;

import :types;

export namespace vw {

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

    [[nodiscard]] constexpr auto operator[](std::size_t index) -> T& { return (&x)[index]; }
    [[nodiscard]] constexpr auto operator[](std::size_t index) const -> const T& { return (&x)[index]; }

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

    [[nodiscard]] constexpr auto operator[](std::size_t index) -> T& { return (&x)[index]; }
    [[nodiscard]] constexpr auto operator[](std::size_t index) const -> const T& { return (&x)[index]; }

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

template <typename T>
struct vec4 {
    T x, y, z, w;

    constexpr vec4() : x(0), y(0), z(0), w(0) {}
    constexpr explicit vec4(T v) : x(v), y(v), z(v), w(v) {}
    constexpr vec4(T x_, T y_, T z_, T w_) : x(x_), y(y_), z(z_), w(w_) {}

    template <typename U>
    constexpr explicit vec4(const vec4<U>& other)
        : x(static_cast<T>(other.x))
        , y(static_cast<T>(other.y))
        , z(static_cast<T>(other.z))
        , w(static_cast<T>(other.w)) {}

    template <typename U>
    constexpr vec4(const vec3<U>& v, T w_)
        : x(static_cast<T>(v.x))
        , y(static_cast<T>(v.y))
        , z(static_cast<T>(v.z))
        , w(w_) {}

    [[nodiscard]] constexpr auto operator[](std::size_t index) -> T& { return (&x)[index]; }
    [[nodiscard]] constexpr auto operator[](std::size_t index) const -> const T& { return (&x)[index]; }

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

struct quat {
    float32 x, y, z, w;

    constexpr quat() : x(0), y(0), z(0), w(1) {}
    constexpr quat(float32 x_, float32 y_, float32 z_, float32 w_) : x(x_), y(y_), z(z_), w(w_) {}

    [[nodiscard]] constexpr auto operator[](std::size_t index) -> float32& { return (&x)[index]; }
    [[nodiscard]] constexpr auto operator[](std::size_t index) const -> const float32& { return (&x)[index]; }

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

[[nodiscard]] constexpr auto operator*(float32 scalar, const quat& q) -> quat { return q * scalar; }

}  // namespace vw

export template <>
struct std::hash<vw::vec2<vw::int32>> {
    auto operator()(const vw::vec2<vw::int32>& v) const noexcept -> std::size_t {
        std::size_t seed = std::hash<vw::int32>{}(v.x);
        seed ^= std::hash<vw::int32>{}(v.y) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        return seed;
    }
};

export template <>
struct std::hash<vw::vec3<vw::int32>> {
    auto operator()(const vw::vec3<vw::int32>& v) const noexcept -> std::size_t {
        std::size_t seed = std::hash<vw::int32>{}(v.x);
        seed ^= std::hash<vw::int32>{}(v.y) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        seed ^= std::hash<vw::int32>{}(v.z) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        return seed;
    }
};
